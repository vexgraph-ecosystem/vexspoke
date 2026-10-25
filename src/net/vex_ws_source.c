#include "net/vex_ws_source.h"

#include "annotation/definition.h"
#include "annotation/overview.h"
#include "annotation/intention.h"
#include "exception/throw.h"
#include "lang/str.h"

#include <string.h>

;;DEFINITION
/**
 * ============================================================================
 * DEFINITION: VexWsSource (net/vex_ws_source.c)
 * ============================================================================
 * The socket adapter behind a host's transport seam. A host registry keeps a
 * borrowed handle plus four callbacks — connect, poll, send, close — and drives
 * them with a millisecond budget, so the host owns scheduling and this class
 * owns exactly the socket and the protocol on it.
 *
 * The seam carries no payload and reports no error, which shapes the whole
 * design: the handle holds the last delivered message record so the host can
 * read what a poll delivered, and a close or failure is *recorded* rather than
 * returned as a count. poll() therefore delivers at most one complete message
 * per call and returns 1 only for text or binary. That is deliberately
 * lossless: nothing is read off the socket unless it is handed over, so a host
 * that polls more slowly than the peer sends cannot drop a message it never
 * saw. The alternative — draining the socket into a ring — would make the
 * return value a claim about frames the host can no longer find.
 *
 * The four operations are declared with the host seam's own signatures, and no
 * R3 header is included: the compatibility is structural, so the host that binds
 * them is the one place a compile-time check on the function pointers belongs.
 * That keeps one owner per seam — R2 owns the protocol and its socket, R3 owns
 * the shape, and the host owns the binding — with no reverse include in either
 * direction (the Vertical Integration Law).
 *
 * Only `ws://` is reachable because net/websocket refuses `wss://` until the TLS
 * backend behind net/tls.h reports ready; a peer that needs transport security
 * cannot be served by this adapter yet, and that is stated rather than assumed.
 * ============================================================================
 */

;;OVERVIEW
/**
 * ============================================================================
 * CLASS: VexWsSource (net/vex_ws_source.c)
 * ============================================================================
 * A live connection plus the four callbacks a host transport table binds.
 *
 * STRUCT FIELDS (Mirroring net/vex_ws_source.h):
 * ----------------------------------------------------------------------------
 *   VexWsSource {
 *     WebSocket socket;        // the embedded RFC 6455 connection
 *     char url[2048];          // owned copy of the target, re-used by connect
 *     WebSocketConfig config;  // budgets applied to every connect
 *     bool configured;         // true once open() has stored a target
 *     WebSocketMessage last;   // the most recent delivered message record
 *   }
 *
 * FUNCTION REGISTRY:
 * ----------------------------------------------------------------------------
 * Constructors:      VexWsSource_0() (inline, in the header)
 * Seam operations:   _connect / _poll / _send / _close (host table targets)
 * Public Core:       VexWsSource_open
 * Public Getters:    _last / _socket / _url / _isOpen
 * Public Projections:_toString / _toStringStruct
 * CONTRACT: VexWsSource() yields the empty handle; every seam operation is
 * null-safe and returns false/0 rather than dereferencing a null handle. poll()
 * delivers at most one complete message and never consumes one it does not hand
 * over, so no message is lost by polling late. last() borrows the client: the
 * payload stays valid until the next poll or close on that handle, and a close
 * or failure appears there rather than as a delivered count. send() transmits a
 * TEXT frame and refuses a payload that is not valid UTF-8 instead of silently
 * choosing another opcode. One handle is caller-serialized: the class holds no
 * lock and is not safe for concurrent callers.
 * ============================================================================
 */

// CONSTRUCTORS

/** Store a target and connect, so a later _connect can retry through the table. */
bool VexWsSource_open(VexWsSource *self, const char *url, WebSocketConfig config) {
    if (self == nullptr || url == nullptr) {
        THROW("VexWsSource_open: null argument");
        return false;
    }
    size_t length = strlen(url);
    if (length + 1u > sizeof((*self).url)) {
        THROW("VexWsSource_open: target URL does not fit");
        return false;
    }
    memcpy((*self).url, url, length + 1u);
    (*self).config = config;
    (*self).configured = true;
    WebSocketMessage *last = &(*self).last;
    (*last).kind = WS_MESSAGE_NONE;
    (*last).closeCode = 1005u;
    (*last).payload = nullptr;
    (*last).length = 0u;
    (*last).error[0] = '\0';
    return VexWsSource_connect(self);
}

// THE FOUR SEAM OPERATIONS

/** Connect (or reconnect) to the stored target; the host's retry path. */
bool VexWsSource_connect(void *handle) {
    VexWsSource *self = (VexWsSource*) handle;
    if (self == nullptr || !(*self).configured) {
        if (self != nullptr)
            THROW("VexWsSource_connect: no target was configured");
        return false;
    }
    if (!WebSocket_open(&(*self).socket, (*self).url, (*self).config)) {
        WebSocketMessage *last = &(*self).last;
        (*last).kind = WS_MESSAGE_ERROR;
        (*last).payload = nullptr;
        (*last).length = 0u;
        size_t cap = sizeof((*last).error);
        const char *reason = WebSocket_lastError(&(*self).socket);
        size_t reasonLength = strlen(reason);
        size_t copy = reasonLength < cap - 1u ? reasonLength : cap - 1u;
        memcpy((*last).error, reason, copy);
        (*last).error[copy] = '\0';
        return false;
    }
    return true;
}

/** Deliver at most one complete message; never consume one we do not hand over. */
uint32_t VexWsSource_poll(void *handle, uint64_t budgetMs) {
    VexWsSource *self = (VexWsSource*) handle;
    if (self == nullptr)
        return 0u;
    uint32_t budget = budgetMs > UINT32_MAX ? UINT32_MAX : (uint32_t) budgetMs;
    WebSocketMessage delivered = WebSocket_poll(&(*self).socket, budget);
    (*self).last = delivered;
    return (delivered.kind == WS_MESSAGE_TEXT ||
            delivered.kind == WS_MESSAGE_BINARY) ? 1u : 0u;
}

/** Send one TEXT frame; a non-UTF-8 payload is refused, not re-typed. */
bool VexWsSource_send(void *handle, const uint8_t *bytes, uint32_t length) {
    VexWsSource *self = (VexWsSource*) handle;
    if (self == nullptr || (bytes == nullptr && length != 0u))
        return false;
    if (!WebSocket_writeText(&(*self).socket, (const char*) bytes, (size_t) length))
        return false;
    return true;
}

/** Close the connection and release the owned buffer. */
void VexWsSource_close(void *handle) {
    VexWsSource *self = (VexWsSource*) handle;
    if (self == nullptr)
        return;
    WebSocket_close(&(*self).socket);
    (*self).last.payload = nullptr;
    (*self).last.length = 0u;
}

// GETTERS (the Symmetric Getter/Setter Completeness Law: null-safe)

/** The last delivered record, or null for a null handle. */
const WebSocketMessage *VexWsSource_last(const VexWsSource *self) {
    return self == nullptr ? nullptr : &(*self).last;
}

/** The embedded client, for what the four-callback seam cannot express. */
WebSocket *VexWsSource_socket(VexWsSource *self) {
    return self == nullptr ? nullptr : &(*self).socket;
}

/** The stored target, or "" when none was configured. */
const char *VexWsSource_url(const VexWsSource *self) {
    return self == nullptr ? "" : (*self).url;
}

/** True only while the embedded connection is OPEN. */
bool VexWsSource_isOpen(const VexWsSource *self) {
    return self != nullptr && WebSocket_isOpen(&(*self).socket);
}

// STRING PROJECTIONS (the toString Law)

/** The VALUE string: the seam-facing summary. */
void VexWsSource_toString(const VexWsSource *self, char *dest, size_t cap,
                          bool *outTruncated) {
    Str out = Str(dest, cap);
    if (self == nullptr) {
        Str_put(&out, "nullptr");
    } else {
        Str_printf(&out, "VexWsSource { url=%s, open=%s, last=%d, closeCode=%u }",
                   (*self).configured ? (*self).url : "",
                   WebSocket_isOpen(&(*self).socket) ? "true" : "false",
                   (int) (*self).last.kind, (unsigned) (*self).last.closeCode);
    }
    if (outTruncated != nullptr)
        *outTruncated = Str_isTruncated(&out);
}

/** The STRUCTURE string: this class's own fields, one layer, no recursion. */
void VexWsSource_toStringStruct(const VexWsSource *self, char *dest, size_t cap,
                                bool *outTruncated) {
    Str out = Str(dest, cap);
    if (self == nullptr) {
        Str_put(&out, "nullptr");
    } else {
        Str_printf(&out,
            "VexWsSource { socket=%s, url=%s, configured=%s, timeoutMs=%u, "
            "maxMessageBytes=%llu, last=%d, lastLength=%llu, closeCode=%u }",
            WebSocket_isOpen(&(*self).socket) ? "OPEN" : "not-open",
            (*self).configured ? (*self).url : "",
            (*self).configured ? "true" : "false", (*self).config.timeoutMs,
            (unsigned long long) (*self).config.maxMessageBytes,
            (int) (*self).last.kind, (unsigned long long) (*self).last.length,
            (unsigned) (*self).last.closeCode);
    }
    if (outTruncated != nullptr)
        *outTruncated = Str_isTruncated(&out);
}
