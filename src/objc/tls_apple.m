#import "net/tls.h"

#import <Foundation/Foundation.h>
#import <Network/Network.h>
#import <dispatch/dispatch.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "annotation/definition.h"
#include "annotation/overview.h"
#include "annotation/intention.h"

;;DEFINITION
/**
 * ============================================================================
 * DEFINITION: Tls_apple
 * ============================================================================
 * Apple-native TLS transport backend using Network.framework and system
 * trust roots: wraps nw_connection_t in an AppleTlsContext (connection,
 * dispatch queue, ready/failed flags) behind the opaque TlsConn handle.
 * Exists because the engine needs TLS without vendoring OpenSSL —
 * Network.framework provides system-managed trust and ALPN. Memory: context
 * is malloc'd per connection, freed in Tls_close; callbacks hop onto the
 * caller's queue. Lifetime: the Tls_connect/Tls_close pair; bounded by the
 * Bounded Wait Law via nw_connection state callbacks.
 * ============================================================================
 */

;;OVERVIEW
/**
 * ============================================================================
 * MODULE: Tls_apple (objc/tls_apple.m)
 * LEVEL: L4 — Self-Management (Native TLS via Network.framework)
 * ============================================================================
 * Native Apple TLS transport backend using Network.framework and system trust roots.
 *
 * STRUCT FIELDS:
 *   - AppleTlsContext: nw_connection_t, dispatch_queue_t, is_ready
 *
 * FUNCTION REGISTRY:
 * ----------------------------------------------------------------------------
 * TLS Operations:
 *   - Tls_connect(host, port, connOut)
 *   - Tls_send(conn, buf, len)
 *   - Tls_recv(conn, buf, cap, gotOut)
 *   - Tls_close(conn)
 * ============================================================================
 */

;;INTENTION("Apple native TLS using Network.framework nw_connection_t")

typedef struct AppleTlsContext {
    nw_connection_t nw_conn;
    dispatch_queue_t queue;
    bool is_ready;
    bool is_failed;
} AppleTlsContext;

bool Tls_connect(const char *host, int port, TlsConn **connOut) {
    if (!host || !connOut || port <= 0 || port > 65535) {
        if (connOut) {
            *connOut = NULL;
        }
        return false;
    }

    char port_str[16];
    snprintf(port_str, sizeof(port_str), "%d", port);

    nw_endpoint_t endpoint = nw_endpoint_create_host(host, port_str);
    if (!endpoint) {
        *connOut = NULL;
        return false;
    }

    nw_parameters_configure_protocol_block_t configure_tls = NW_PARAMETERS_DEFAULT_CONFIGURATION;
    nw_parameters_t params = nw_parameters_create_secure_tcp(
        configure_tls,
        NW_PARAMETERS_DEFAULT_CONFIGURATION
    );
    if (!params) {
        *connOut = NULL;
        return false;
    }

    nw_connection_t connection = nw_connection_create(endpoint, params);
    if (!connection) {
        *connOut = NULL;
        return false;
    }

    AppleTlsContext *ctx = (AppleTlsContext*) malloc(sizeof(AppleTlsContext));
    if (!ctx) {
        *connOut = NULL;
        return false;
    }

    (*ctx).nw_conn = connection;
    (*ctx).queue = dispatch_queue_create("com.vexgraph.vexspoke.tls", DISPATCH_QUEUE_SERIAL);
    (*ctx).is_ready = false;
    (*ctx).is_failed = false;

    dispatch_semaphore_t sema = dispatch_semaphore_create(0);
    __block bool ready = false;

    nw_connection_set_queue(connection, (*ctx).queue);
    nw_connection_set_state_changed_handler(connection, ^(nw_connection_state_t state, nw_error_t error) {
        (void) error;
        if (state == nw_connection_state_ready) {
            ready = true;
            dispatch_semaphore_signal(sema);
        } else if (state == nw_connection_state_failed || state == nw_connection_state_cancelled) {
            ready = false;
            dispatch_semaphore_signal(sema);
        }
    });

    nw_connection_start(connection);

    // Timeout handshake after 4 seconds to avoid hanging indefinitely
    dispatch_time_t timeout = dispatch_time(DISPATCH_TIME_NOW, (int64_t) (4LL * NSEC_PER_SEC));
    intptr_t waited = dispatch_semaphore_wait(sema, timeout);

    if (waited != 0 || !ready) {
        nw_connection_cancel(connection);
        free(ctx);
        *connOut = NULL;
        return false;
    }

    (*ctx).is_ready = true;
    TlsConn *conn = (TlsConn*) malloc(sizeof(TlsConn));
    if (!conn) {
        nw_connection_cancel(connection);
        free(ctx);
        *connOut = NULL;
        return false;
    }

    (*conn).backend = 1; // 1 = Network.framework Apple backend
    (*conn).opaque = (void*) ctx;
    *connOut = conn;
    return true;
}

bool Tls_send(TlsConn *conn, const char *buf, size_t len) {
    if (!conn || !(*conn).opaque || !buf) {
        return false;
    }
    if (len == 0) {
        return true;
    }

    AppleTlsContext *ctx = (AppleTlsContext*) (*conn).opaque;
    if (!(*ctx).is_ready || (*ctx).is_failed) {
        return false;
    }

    dispatch_data_t data = dispatch_data_create(buf, len, (*ctx).queue, DISPATCH_DATA_DESTRUCTOR_DEFAULT);
    dispatch_semaphore_t sema = dispatch_semaphore_create(0);
    __block bool send_ok = false;

    nw_connection_send((*ctx).nw_conn, data, NW_CONNECTION_DEFAULT_MESSAGE_CONTEXT, true, ^(nw_error_t error) {
        if (!error) {
            send_ok = true;
        }
        dispatch_semaphore_signal(sema);
    });

    dispatch_time_t timeout = dispatch_time(DISPATCH_TIME_NOW, (int64_t) (5LL * NSEC_PER_SEC));
    if (dispatch_semaphore_wait(sema, timeout) != 0) {
        return false;
    }

    return send_ok;
}

bool Tls_recv(TlsConn *conn, char *buf, size_t cap, size_t *gotOut) {
    if (!conn || !(*conn).opaque || !buf || !gotOut || cap == 0) {
        if (gotOut) {
            *gotOut = 0;
        }
        return false;
    }

    AppleTlsContext *ctx = (AppleTlsContext*) (*conn).opaque;
    if (!(*ctx).is_ready || (*ctx).is_failed) {
        *gotOut = 0;
        return false;
    }

    dispatch_semaphore_t sema = dispatch_semaphore_create(0);
    __block size_t bytes_read = 0;
    __block bool recv_ok = false;

    nw_connection_receive((*ctx).nw_conn, 1, (uint32_t) cap, ^(dispatch_data_t content, nw_content_context_t context, bool is_complete, nw_error_t error) {
        (void) context;
        (void) is_complete;
        if (!error && content) {
            const void *src_buf = NULL;
            size_t src_size = 0;
            dispatch_data_t contig = dispatch_data_create_map(content, &src_buf, &src_size);
            (void) contig;
            if (src_buf && src_size > 0) {
                size_t to_copy = (src_size < cap) ? src_size : cap;
                memcpy(buf, src_buf, to_copy);
                bytes_read = to_copy;
                recv_ok = true;
            }
        }
        dispatch_semaphore_signal(sema);
    });

    dispatch_time_t timeout = dispatch_time(DISPATCH_TIME_NOW, (int64_t) (5LL * NSEC_PER_SEC));
    if (dispatch_semaphore_wait(sema, timeout) != 0) {
        *gotOut = 0;
        return false;
    }

    *gotOut = bytes_read;
    return recv_ok;
}

void Tls_close(TlsConn *conn) {
    if (!conn) {
        return;
    }
    if ((*conn).opaque) {
        AppleTlsContext *ctx = (AppleTlsContext*) (*conn).opaque;
        if ((*ctx).nw_conn) {
            nw_connection_cancel((*ctx).nw_conn);
        }
        free(ctx);
        (*conn).opaque = NULL;
    }
    free(conn);
}
