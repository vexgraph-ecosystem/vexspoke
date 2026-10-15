#ifndef NET_WEBSOCKET_H
#define NET_WEBSOCKET_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "c23/constructor.h"

// net/websocket.h — RFC 6455 WebSocket client over BSD sockets
//
// One client connection, one frame codec. This module owns the PROTOCOL and its
// socket adapter: the handshake (Upgrade / Sec-WebSocket-Key / accept hash), the
// frame codec with client masking, the ping/pong/close control path and
// continuation-frame reassembly. Like net/http.c it allocates nothing on the
// caller's behalf beyond the one reassembly buffer this class owns, and it owns
// no threads: every wait is bounded by the caller's budget, and a timeout
// mid-frame fails the connection rather than blocking. `wss://` is refused
// loudly until a TLS backend lands behind net/tls.h; only `ws://` is implemented.
//
// R3 does not include this header. api-haven's WsSource is a duck-typed
// fn-table shape; a host that can see both binds this client to it.

// Bounded handshake response we will read before failing the connection.
#define WS_HANDSHAKE_MAX_BYTES 8192u
// Receive chunk: header Bytes and payload chunks are read through this.
#define WS_RX_BYTES_DEFAULT 4096u
// Default cap on one reassembled message (named default, caller-overridable).
#define WS_MESSAGE_BYTES_DEFAULT (16u * 1024u * 1024u)
// RFC 6455: a control frame's payload is at most 125 Bytes.
#define WS_CONTROL_MAX_BYTES 125u
// …so a close reason is at most 123 Bytes (2 Bytes go to the close code).
#define WS_CLOSE_REASON_BYTES 123u
// Bounded host/path/error/reason text.
#define WS_HOST_BYTES 256u
#define WS_PATH_BYTES 2048u
#define WS_TEXT_BYTES 128u

// The RFC 6455 handshake GUID appended to the client key before hashing.
#define WS_HANDSHAKE_GUID "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"

typedef enum WebSocketState {
    WEBSOCKET_CLOSED = 0,  // never opened, or the close handshake completed
    WEBSOCKET_OPEN = 1,    // handshake accepted and no close seen
    WEBSOCKET_CLOSING = 2, // we sent close and await the peer's
    WEBSOCKET_FAILED = 3,  // protocol/transport error; no further frames
} WebSocketState;

typedef enum WebSocketOpcode {
    WEBSOCKET_CONTINUATION = 0x0,
    WEBSOCKET_TEXT = 0x1,
    WEBSOCKET_BINARY = 0x2,
    WEBSOCKET_CLOSE = 0x8,
    WEBSOCKET_PING = 0x9,
    WEBSOCKET_PONG = 0xA,
} WebSocketOpcode;

// What one poll produced. A record, not a destination argument: the payload
// borrows the client's reassembly buffer and is valid until the next poll.
typedef enum WebSocketMessageKind {
    WS_MESSAGE_NONE = 0,   // budget elapsed with nothing complete
    WS_MESSAGE_TEXT = 1,   // reassembled and validated as UTF-8
    WS_MESSAGE_BINARY = 2, // opaque Bytes
    WS_MESSAGE_CLOSE = 3,  // the peer closed; closeCode/closeReason are set
    WS_MESSAGE_ERROR = 4,  // the connection failed; error says why
} WebSocketMessageKind;

typedef struct WebSocketMessage {
    WebSocketMessageKind kind;
    const uint8_t *payload; // borrowed; null unless TEXT/BINARY
    size_t length;
    bool truncated;         // a bounded text field in this record was cut
    uint16_t closeCode;     // 1005 when the peer sent no code
    char error[WS_TEXT_BYTES];
} WebSocketMessage;

// Connection budgets. Both are caller-selected; the defaults are named above.
typedef struct WebSocketConfig {
    uint32_t timeoutMs;     // transport budget for one read or write burst
    size_t maxMessageBytes; // reassembly cap for one complete message
} WebSocketConfig;

typedef struct WebSocket {
    int fd;                        // connected socket, -1 once released
    WebSocketState state;
    uint32_t timeoutMs;            // transport budget for one read/write burst
    size_t maxMessageBytes;        // reassembly cap
    uint8_t *message;              // owned reassembly buffer
    size_t messageCap;             // allocated Bytes
    size_t messageLength;          // Bytes accumulated for the current message
    uint32_t messageOpcode;        // TEXT/BINARY while a fragmented message builds
    uint8_t control[WS_CONTROL_MAX_BYTES]; // ping/pong/close payload
    uint16_t closeCode;            // finalized close code (1005 when absent)
    char closeReason[WS_TEXT_BYTES]; // bounded close reason text
    bool closeReasonTruncated;     // the reason was cut to fit
    char error[WS_TEXT_BYTES];     // bounded last-failure text
    uint64_t framesIn;             // complete frames decoded
    uint64_t framesOut;            // complete frames encoded and sent
    uint64_t pongsSent;            // automatic pong replies
} WebSocket;

// --- Constructors ---
// The empty instance: no socket, CLOSED, no owned buffer. Inline, like
// HttpResponse, so the caller owns the storage and nothing is allocated here.
static inline WebSocket WebSocket_0(void) {
    return (WebSocket) { .fd = -1, .state = WEBSOCKET_CLOSED, .closeCode = 1005u,
                         .timeoutMs = 5000u, .maxMessageBytes = WS_MESSAGE_BYTES_DEFAULT };
}
#define WebSocket(...) CONSTRUCTOR_DISPATCH(WebSocket, __VA_ARGS__)

// Connect, handshake and leave the client OPEN. The URL must be ws://; a
// wss:// or malformed URL, a refused connect, or a rejected handshake each
// return false with a cold diagnostic and leave the client CLOSED. On a
// reconnect the previous connection is closed first.
bool WebSocket_open(WebSocket *self, const char *url, WebSocketConfig config);

// Send a close frame when still open, close the socket and release the owned
// buffer. Idempotent; safe on a zeroed instance.
void WebSocket_close(WebSocket *self);

// --- Core functions (all bounded; no thread, no unbounded wait) ---

// Read frames for at most budgetMs, answering pings and returning the first
// complete data message or close. Control frames are handled inline and never
// surface as a message. Returns WS_MESSAGE_NONE when the budget elapses.
WebSocketMessage WebSocket_poll(WebSocket *self, uint32_t budgetMs);

// Encode and send one final frame with fresh client masking. Text payloads are
// validated as UTF-8 before they leave; an invalid text payload is refused.
bool WebSocket_writeText(WebSocket *self, const char *text, size_t length);
bool WebSocket_writeBinary(WebSocket *self, const uint8_t *bytes, size_t length);
bool WebSocket_writePing(WebSocket *self, const uint8_t *payload, size_t length);
// Send a close frame and move to CLOSING. A zero code sends no close payload.
// The reason is cut to WS_CLOSE_REASON_BYTES and the cut is observable.
bool WebSocket_writeClose(WebSocket *self, uint16_t code, const char *reason);

// --- Getters (the Symmetric Getter/Setter Completeness Law: null-safe) ---
WebSocketState WebSocket_state(const WebSocket *self);
bool WebSocket_isOpen(const WebSocket *self);
const char *WebSocket_lastError(const WebSocket *self);
uint16_t WebSocket_closeCode(const WebSocket *self);
const char *WebSocket_closeReason(const WebSocket *self);
size_t WebSocket_maxMessageBytes(const WebSocket *self);
uint64_t WebSocket_framesIn(const WebSocket *self);
uint64_t WebSocket_framesOut(const WebSocket *self);
uint64_t WebSocket_pongsSent(const WebSocket *self);

// --- String projections (the toString Law) ---
void WebSocket_toString(const WebSocket *self, char *dest, size_t cap, bool *outTruncated);
void WebSocket_toStringStruct(const WebSocket *self, char *dest, size_t cap, bool *outTruncated);

#endif
