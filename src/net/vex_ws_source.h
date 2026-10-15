#ifndef NET_VEX_WS_SOURCE_H
#define NET_VEX_WS_SOURCE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "c23/constructor.h"
#include "net/websocket.h"

// net/vex_ws_source.h — the socket adapter behind a host's WsSource seam.
//
// Vexspoke owns the network protocols AND their socket adapters, so the adapter
// that turns a live `WebSocket` connection into the four-callback transport a
// host expects lives here, next to the protocol it wraps. The four operations
// below are declared with the shape a host's transport seam already uses
// (`bool connect(void*)`, `uint32_t poll(void*, uint64_t)`,
// `bool send(void*, const uint8_t*, uint32_t)`, `void close(void*)`), so a host
// that can see both this header and its own seam assigns them directly:
//
//     const HavenWsSource source = {
//         .connect = VexWsSource_connect, .poll = VexWsSource_poll,
//         .send = VexWsSource_send, .close = VexWsSource_close,
//     };
//
// No R3 header is included here and none includes this one: the compatibility is
// structural (equal signatures), and the host that binds them is the only place
// that can check it — which is exactly where a `_Static_assert` on the function
// pointers belongs.

// The seam offers no payload out-parameter and no error channel, so the handle
// carries the last delivered message for the host to read after a poll.
typedef struct VexWsSource {
    WebSocket socket;          // the embedded RFC 6455 connection
    char url[WS_PATH_BYTES];   // owned copy of the target, re-used by connect
    WebSocketConfig config;    // budgets applied to every connect
    bool configured;           // true once open() has stored a target
    WebSocketMessage last;     // the most recent delivered message record
} VexWsSource;

// --- Constructors ---
// The empty handle: no socket, no target. Inline, so nothing is allocated here.
static inline VexWsSource VexWsSource_0(void) {
    return (VexWsSource) { .socket = { .fd = -1, .state = WEBSOCKET_CLOSED,
                                       .closeCode = 1005u },
                           .last = { .kind = WS_MESSAGE_NONE, .closeCode = 1005u } };
}
#define VexWsSource(...) CONSTRUCTOR_DISPATCH(VexWsSource, __VA_ARGS__)

// Store the target and connect. Returns false with a cold diagnostic when the
// URL is unsupported or the handshake is refused, leaving the handle closed.
bool VexWsSource_open(VexWsSource *self, const char *url, WebSocketConfig config);

// --- The four seam operations (signatures a host's transport table binds) ---

// Connect (or reconnect) to the target a previous open() stored, so a host may
// retry through the table alone. False when nothing was configured or the
// attempt fails.
bool VexWsSource_connect(void *handle);

// Deliver at most ONE complete message within the budget, and never discard a
// message it did not deliver: unread Bytes stay in the socket, so a host that
// polls slower than the peer still loses nothing. Returns 1 when a text or
// binary message was delivered (read it with VexWsSource_last), 0 otherwise —
// a close or a failure is reported through the record, not as a count.
uint32_t VexWsSource_poll(void *handle, uint64_t budgetMs);

// Send one TEXT frame. A payload that is not valid UTF-8 is refused rather than
// silently re-typed; use VexWsSource_socket for binary, ping and close frames.
bool VexWsSource_send(void *handle, const uint8_t *bytes, uint32_t length);

// Close the connection and release the owned buffer. Safe on a null handle.
void VexWsSource_close(void *handle);

// --- Getters (the Symmetric Getter/Setter Completeness Law: null-safe) ---
// The last delivered record; its payload borrows the client and stays valid
// until the next poll or close on the same handle.
const WebSocketMessage *VexWsSource_last(const VexWsSource *self);
// The embedded client, for operations the four-callback seam cannot express.
WebSocket *VexWsSource_socket(VexWsSource *self);
const char *VexWsSource_url(const VexWsSource *self);
bool VexWsSource_isOpen(const VexWsSource *self);

// --- String projections (the toString Law) ---
void VexWsSource_toString(const VexWsSource *self, char *dest, size_t cap, bool *outTruncated);
void VexWsSource_toStringStruct(const VexWsSource *self, char *dest, size_t cap, bool *outTruncated);

#endif
