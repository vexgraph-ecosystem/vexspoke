#include "net/tls.h"

#include "annotation/definition.h"
#include "annotation/overview.h"
#include "annotation/intention.h"
#include "exception/throw.h"

#import <Foundation/Foundation.h>
#import <Network/Network.h>

#include <stdlib.h>
#include <string.h>

;;DEFINITION
/**
 * ============================================================================
 * DEFINITION: AppleTlsBackend (net/tls_apple.m)
 * ============================================================================
 * The Apple implementation of the net/tls.h seam, on Network.framework. It is
 * the translation unit the seam names for this platform, and the only one that
 * is linked here: `tls_curl.c` is the non-Apple alternative, never a companion.
 * Vexspoke owns it because Vexspoke owns the network protocols AND their socket
 * adapters; the engines above borrow the seam and never see Network.framework.
 *
 * Network.framework is asynchronous and this seam is synchronous, so each
 * connection owns a serial dispatch queue and every operation waits on a
 * semaphore with a DEADLINE. That is the whole reason this file exists rather
 * than a direct socket call: a bounded wait is a property the seam can promise
 * only if the adapter enforces it. A wait that runs out cancels the connection
 * so the pending handler cannot fire into a caller that has already moved on,
 * and the connection is torn down before it is reported as failed.
 *
 * Verification is not configurable. The secure-TCP parameters carry the default
 * TLS options, so the peer's chain must validate against the system trust
 * anchors; an invalid or self-signed certificate is refused by the framework
 * before this code sees a Byte. There is deliberately no trust override, no
 * "accept any" mode and no pinning API here: a switch that disables verification
 * is the one thing a TLS adapter must never ship, and a test that needs a
 * self-signed peer has to say so as an explicit gap instead of weakening it.
 *
 * Scope: connect, send, receive and close. No session resumption, no ALPN
 * selection, no client certificates, no proxy, no revocation policy beyond the
 * platform default, and no Windows/Linux path (that is tls_curl.c's job).
 * ============================================================================
 */

;;OVERVIEW
/**
 * ============================================================================
 * CLASS: AppleTlsBackend (net/tls_apple.m)
 * ============================================================================
 * Network.framework TLS behind the platform-neutral TlsConn seam.
 *
 * STRUCT FIELDS (Mirroring net/tls.h):
 * ----------------------------------------------------------------------------
 *   TlsConn {
 *     int backend;   // TLS_BACKEND_APPLE for every connection made here
 *     void *opaque;  // __bridge_retained VexTlsSession (released by Tls_close)
 *   }
 *
 * PRIVATE HELPER RECORD (file-local, Objective-C; never exposed):
 *   VexTlsSession {
 *     nw_connection_t connection; // the secure TCP connection (strong)
 *     dispatch_queue_t queue;     // this session's serial queue (strong)
 *   }
 *
 * PRIVATE HELPERS (file-local):
 *   vexWaitForReady(connection, timeoutSeconds)  // bounded state wait
 *   vexWaitForSend(...) / vexWaitForReceive(...) // bounded IO with deadlines
 *   vexDeadlineSeconds(milliseconds)
 *
 * FUNCTION REGISTRY:
 * ----------------------------------------------------------------------------
 * Seam operations:  Tls_connect / Tls_send / Tls_recv / Tls_close / Tls_backend
 * CONTRACT: Tls_connect fills *connOut only on a completed handshake; every
 * failure path cancels the connection and leaves *connOut null, so a refused
 * peer never yields a half-open session. Every wait has a deadline, and a wait
 * that expires is a refusal, never an unbounded park (the Bounded Wait Law).
 * Tls_send/Tls_recv report Bytes moved; a closed or failed connection reports
 * false with the count left at zero. Tls_close is null-safe, releases the
 * session, and never waits. Certificate verification is the framework's and is
 * not configurable from here.
 * ============================================================================
 */

// The seam's own budgets. A connect or an IO burst never waits longer than its
// constant here; the seam carries no per-call budget, so these are the contract.
// A refused or unresolvable peer is reported when this budget runs out rather
// than immediately, because Network.framework holds such a connection in
// `waiting` instead of `failed` -- so the budget IS the worst-case connect cost,
// and it is deliberately kept small for that reason.
#define VEX_TLS_CONNECT_TIMEOUT_MS 5000u
#define VEX_TLS_IO_TIMEOUT_MS 10000u

static TlsBackendKind s_lastBackend = TLS_BACKEND_NONE;

/** One connection's retained state; owned by TlsConn.opaque via a bridge cast. */
@interface VexTlsSession : NSObject
@property (nonatomic, strong) nw_connection_t connection;
@property (nonatomic, strong) dispatch_queue_t queue;
@end

@implementation VexTlsSession
@end

;;INTENTION("the seam is synchronous and Network.framework is not, so this adapter waits on a semaphore with a deadline and cancels the connection on expiry -- a bounded wait the seam could not otherwise promise (the Bounded Wait Law)")

/** Convert a millisecond budget into a dispatch deadline. */
static dispatch_time_t vexDeadline(uint32_t milliseconds) {
    return dispatch_time(DISPATCH_TIME_NOW, (int64_t) milliseconds * NSEC_PER_MSEC);
}

/**
 * Start the connection and wait, bounded, until it is ready. Returns false on
 * failure, cancellation or deadline; the caller cancels on any false.
 */
static bool vexWaitForReady(nw_connection_t connection, dispatch_queue_t queue,
                            uint32_t timeoutMs) {
    dispatch_semaphore_t settled = dispatch_semaphore_create(0);
    __block bool ready = false;
    nw_connection_set_queue(connection, queue);
    nw_connection_set_state_changed_handler(connection, ^(nw_connection_state_t state,
                                                          nw_error_t error) {
        (void) error;
        if (state == nw_connection_state_ready) {
            ready = true;
            dispatch_semaphore_signal(settled);
        } else if (state == nw_connection_state_failed ||
                   state == nw_connection_state_cancelled) {
            dispatch_semaphore_signal(settled);
        }
    });
    nw_connection_start(connection);
    if (dispatch_semaphore_wait(settled, vexDeadline(timeoutMs)) != 0) {
        nw_connection_cancel(connection); // unblock the handler we are abandoning
        return false;
    }
    return ready;
}

/**
 * Send one buffer, bounded. A deadline cancels the connection, because a
 * pending send handler must not fire into a caller that has already returned.
 */
static bool vexSendBounded(nw_connection_t connection, const void *bytes,
                           size_t length, uint32_t timeoutMs) {
    dispatch_data_t payload = dispatch_data_create(bytes, length, nullptr,
                                                   DISPATCH_DATA_DESTRUCTOR_DEFAULT);
    dispatch_semaphore_t settled = dispatch_semaphore_create(0);
    __block bool sent = false;
    nw_connection_send(connection, payload, NW_CONNECTION_DEFAULT_MESSAGE_CONTEXT, true,
                       ^(nw_error_t error) {
        sent = (error == nullptr);
        dispatch_semaphore_signal(settled);
    });
    if (dispatch_semaphore_wait(settled, vexDeadline(timeoutMs)) != 0) {
        nw_connection_cancel(connection);
        return false;
    }
    return sent;
}

/**
 * Receive one buffer, bounded. A clean end of stream reports false with a zero
 * count, which is the seam's "nothing more will arrive".
 */
static bool vexReceiveBounded(nw_connection_t connection, void *dest, size_t capacity,
                              size_t *gotOut, uint32_t timeoutMs) {
    *gotOut = 0u;
    dispatch_semaphore_t settled = dispatch_semaphore_create(0);
    __block bool received = false;
    __block size_t got = 0u;
    nw_connection_receive(connection, 1u, (uint32_t) capacity,
                          ^(dispatch_data_t content, nw_content_context_t context,
                            bool is_complete, nw_error_t error) {
        (void) context;
        (void) is_complete;
        if (error == nullptr && content != nullptr) {
            dispatch_data_apply(content, ^bool(dispatch_data_t region, size_t offset,
                                               const void *buffer, size_t size) {
                (void) region;
                size_t room = capacity - offset;
                size_t copy = size < room ? size : room;
                memcpy((uint8_t*) dest + offset, buffer, copy);
                got = offset + copy;
                return true;
            });
            received = got > 0u;
        }
        dispatch_semaphore_signal(settled);
    });
    if (dispatch_semaphore_wait(settled, vexDeadline(timeoutMs)) != 0) {
        nw_connection_cancel(connection);
        return false;
    }
    *gotOut = got;
    return received;
}

/** Borrow the session a TlsConn carries, or null when the handle is unusable. */
static inline VexTlsSession *vexSession(const TlsConn *conn) {
    if (conn == nullptr || (*conn).opaque == nullptr ||
        (*conn).backend != (int) TLS_BACKEND_APPLE)
        return nullptr;
    return (__bridge VexTlsSession*) (*conn).opaque;
}

// THE SEAM

/**
 * Open a TLS connection and complete the handshake within the budget. The
 * framework performs certificate verification against the system trust anchors;
 * an untrusted peer fails here and yields no connection.
 */
bool Tls_connect(const char *host, int port, TlsConn **connOut) {
    if (connOut == nullptr) {
        THROW("Tls_connect: null output");
        return false;
    }
    *connOut = nullptr;
    if (host == nullptr || host[0] == '\0' || port <= 0 || port > 65535) {
        THROW("Tls_connect: invalid host or port");
        return false;
    }
    @autoreleasepool {
        char service[16];
        snprintf(service, sizeof(service), "%d", port);
        nw_endpoint_t endpoint = nw_endpoint_create_host(host, service);
        if (endpoint == nullptr) {
            THROW("Tls_connect: endpoint creation failed");
            return false;
        }
        // Secure TCP: TLS over TCP with the default (verifying) TLS options.
        nw_parameters_t parameters = nw_parameters_create_secure_tcp(
            NW_PARAMETERS_DEFAULT_CONFIGURATION, NW_PARAMETERS_DEFAULT_CONFIGURATION);
        if (parameters == nullptr) {
            THROW("Tls_connect: parameters creation failed");
            return false;
        }
        nw_connection_t connection = nw_connection_create(endpoint, parameters);
        if (connection == nullptr) {
            THROW("Tls_connect: connection creation failed");
            return false;
        }
        VexTlsSession *session = [[VexTlsSession alloc] init];
        session.connection = connection;
        session.queue = dispatch_queue_create("vex.tls.apple", DISPATCH_QUEUE_SERIAL);

        if (!vexWaitForReady(connection, session.queue, VEX_TLS_CONNECT_TIMEOUT_MS)) {
            nw_connection_cancel(connection);
            THROW("Tls_connect: handshake did not complete");
            return false;
        }
        TlsConn *conn = (TlsConn*) malloc(sizeof(TlsConn));
        if (conn == nullptr) {
            nw_connection_cancel(connection);
            THROW("Tls_connect: connection record allocation failed");
            return false;
        }
        (*conn).backend = (int) TLS_BACKEND_APPLE;
        (*conn).opaque = (__bridge_retained void*) session; // released by Tls_close
        s_lastBackend = TLS_BACKEND_APPLE;
        *connOut = conn;
    }
    return true;
}

/** Send a whole buffer, bounded. False on a null/closing connection or timeout. */
bool Tls_send(TlsConn *conn, const char *buf, size_t len) {
    VexTlsSession *session = vexSession(conn);
    if (session == nullptr || (buf == nullptr && len != 0u))
        return false;
    if (len == 0u)
        return true;
    @autoreleasepool {
        return vexSendBounded(session.connection, buf, len, VEX_TLS_IO_TIMEOUT_MS);
    }
}

/** Receive at most cap Bytes, bounded; gotOut receives the count actually read. */
bool Tls_recv(TlsConn *conn, char *buf, size_t cap, size_t *gotOut) {
    if (gotOut != nullptr)
        *gotOut = 0u;
    VexTlsSession *session = vexSession(conn);
    if (session == nullptr || buf == nullptr || cap == 0u || gotOut == nullptr)
        return false;
    @autoreleasepool {
        return vexReceiveBounded(session.connection, buf, cap, gotOut,
                                 VEX_TLS_IO_TIMEOUT_MS);
    }
}

/** Cancel the connection, release the session and free the record. Null-safe. */
void Tls_close(TlsConn *conn) {
    if (conn == nullptr)
        return;
    @autoreleasepool {
        if ((*conn).opaque != nullptr) {
            VexTlsSession *session = (__bridge_transfer VexTlsSession*) (*conn).opaque;
            if (session.connection != nullptr)
                nw_connection_cancel(session.connection);
            session = nil; // ARC releases the session and its connection here
        }
    }
    (*conn).opaque = nullptr;
    (*conn).backend = (int) TLS_BACKEND_NONE;
    free(conn);
}

/** Which backend answered; TLS_BACKEND_NONE until one completes a handshake. */
TlsBackendKind Tls_backend(void) {
    return s_lastBackend;
}
