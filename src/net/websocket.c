#include "net/websocket.h"

#include "annotation/definition.h"
#include "annotation/overview.h"
#include "annotation/intention.h"
#include "exception/throw.h"
#include "lang/str.h"
#include "net/url.h"
#include "nio/mem.h"
#include "oop/type.h"
#include "security/secure_random.h"
#include "security/sha1.h"

#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <poll.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

;;DEFINITION
/**
 * ============================================================================
 * DEFINITION: WebSocket (net/websocket.c)
 * ============================================================================
 * RFC 6455 in one client connection. Vexspoke `net/` owns the ecosystem's
 * network protocols AND their socket adapters (the same arrangement net/http.c
 * already has), so the protocol and the BSD socket it rides on both live here.
 * R3 never includes this header: api-haven owns the `WsSource` fn-table SHAPE
 * and the fan-out/SSE state machines, and a host that can see both layers binds
 * this client to that shape. That keeps one owner per seam and no reverse
 * include.
 *
 * The class is caller-stored, like HttpResponse: WebSocket() yields the empty
 * instance and WebSocket_open() fills it, so construction allocates nothing on
 * the caller's behalf. The ONE buffer this class owns is the reassembly buffer
 * for a fragmented data message, taken from the engine's Memory_ ABI (it owns
 * its storage; Memory_free releases it). No thread is created and no wait is
 * unbounded: every read and write is bounded by the caller's transport budget,
 * the frame loop runs in bounded slices and re-checks that budget, and a stall
 * mid-frame fails the connection instead of blocking forever (the Bounded Wait
 * Law).
 *
 * The handshake is the RFC's: GET with Upgrade/Connection, a fresh
 * Sec-WebSocket-Key of 16 random Bytes, the version 13 pin, and verification of
 * `Sec-WebSocket-Accept` against base64(SHA1(key || GUID)). A peer that answers
 * anything else is refused loudly and the connection is dropped. Frames are
 * strict: reserved bits must be zero, a 64-bit length must clear its high bit, a
 * server frame must not be masked, control frames must be final and within 125
 * Bytes, continuation frames must follow a start frame, and a text message must
 * be valid UTF-8. Every one of those is a refusal with a bounded diagnostic
 * rather than a guess, and the connection moves to FAILED so no later call can
 * act on a desynchronized stream.
 *
 * `wss://` is deliberately refused until a TLS backend lands behind net/tls.h;
 * claiming transport security without it would be worse than not offering it.
 * ============================================================================
 */

;;OVERVIEW
/**
 * ============================================================================
 * CLASS: WebSocket (net/websocket.c)
 * ============================================================================
 * RFC 6455 client: handshake, frame codec with client masking, control path and
 * continuation reassembly, over one BSD socket owned by this class.
 *
 * STRUCT FIELDS (Mirroring net/websocket.h):
 * ----------------------------------------------------------------------------
 *   WebSocket {
 *     int fd;                        // connected socket, -1 once released
 *     WebSocketState state;          // CLOSED/OPEN/CLOSING/FAILED
 *     uint32_t timeoutMs;            // transport budget for one read/write burst
 *     size_t maxMessageBytes;        // reassembly cap
 *     uint8_t *message;              // owned reassembly buffer
 *     size_t messageCap;             // allocated Bytes
 *     size_t messageLength;          // Bytes accumulated for the current message
 *     uint32_t messageOpcode;        // TEXT/BINARY while fragments accumulate
 *     uint8_t control[125];          // ping/pong/close payload
 *     uint16_t closeCode;            // finalized close code (1005 when absent)
 *     char closeReason[128];         // bounded close reason text
 *     bool closeReasonTruncated;     // the reason was cut to fit
 *     char error[128];               // bounded last-failure text
 *     uint64_t framesIn;             // complete frames decoded
 *     uint64_t framesOut;            // complete frames encoded and sent
 *     uint64_t pongsSent;            // automatic pong replies
 *   }
 *
 * PRIVATE HELPER RECORD (kept file-local, behaviorless):
 *   WsTarget {
 *     char host[256];  // authority host (userinfo stripped)
 *     char path[2048]; // request target, "/" when absent
 *     int port;        // 80 when the authority omits it
 *   }
 *
 * PRIVATE HELPERS (file-local, pure logic):
 *   wsNowMs()                          // monotonic milliseconds (static)
 *   wsSetTimeout(fd, ms)               // SO_RCVTIMEO/SO_SNDTIMEO (static)
 *   wsConnect(target, ms)              // bounded non-blocking connect (static)
 *   wsSendAll(fd, bytes, length)       // short-write-safe (static)
 *   wsReadExact(self, dst, length)     // short-read-safe, timeout-bounded (static)
 *   wsFindHeaderEnd(bytes, length)     // "\r\n\r\n" end index (static)
 *   wsHeaderValue(block, name, out, cap) // case-insensitive header fetch (static)
 *   wsParseUrl(url, out)               // ws:// authority/path split (static)
 *   wsFail(self, reason)               // record text + FAILED (static)
 *   wsUtf8Valid(bytes, length)         // RFC 3629 validator (static)
 *   wsAppendMessage(self, bytes, n)    // bounded reassembly append (static)
 *   wsSendFrame(self, op, fin, p, n)   // encode + fresh mask + send (static)
 *   wsReadFrameHeader(self, ...)       // decode the frame header (static)
 *   wsConsumeFrame(self)               // one frame -> message record (static)
 *
 * FUNCTION REGISTRY:
 * ----------------------------------------------------------------------------
 * Constructors:        WebSocket_0() (inline, in the header)
 * Public Core:         WebSocket_open / _close / _poll
 *                      _writeText / _writeBinary / _writePing / _writeClose
 * Public Getters:      _state / _isOpen / _lastError / _closeCode / _closeReason
 *                      _maxMessageBytes / _framesIn / _framesOut / _pongsSent
 * Public Projections:  _toString / _toStringStruct
 * CONTRACT: WebSocket() yields the empty instance; a null self is safe on every
 * getter and projection. open() connects and handshakes, leaving the client
 * OPEN, or returns false with the client CLOSED/FAILED and a bounded diagnostic.
 * Every failure inside the frame loop moves the client to FAILED, which is
 * terminal: poll() reports it and no further frame is trusted. poll() never
 * blocks longer than its budget, answers pings itself, and returns a record
 * whose payload borrows the reassembly buffer until the next poll or close.
 * close() sends a normal close when still open, releases the socket and the
 * owned buffer, and is idempotent. One connection is caller-serialized: the
 * class holds no lock and is not safe for concurrent callers.
 * ============================================================================
 */

// PRIVATE HELPER RECORD

typedef struct WsTarget {
    char host[WS_HOST_BYTES]; // authority host (userinfo stripped)
    char path[WS_PATH_BYTES]; // request target, "/" when absent
    int port;                 // 80 for ws://, 443 for wss://, when omitted
    bool secure;              // true for wss://, decided by the scheme alone
} WsTarget;

// PRIVATE HELPERS

/** Monotonic milliseconds; the only clock the frame loop trusts. */
static uint64_t wsNowMs(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t) ts.tv_sec * 1000u + (uint64_t) (ts.tv_nsec / 1000000);
}

/** Bound both directions of one socket by the same transport budget. */
static void wsSetTimeout(int fd, uint32_t timeoutMs) {
    struct timeval tv;
    tv.tv_sec = (time_t) (timeoutMs / 1000u);
    tv.tv_usec = (suseconds_t) ((timeoutMs % 1000u) * 1000u);
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
}

/**
 * Connect with a bounded wait: a non-blocking connect plus a poll armed with the
 * caller's budget, so an unreachable host consumes that budget and no more.
 */
static int wsConnect(const WsTarget *target, uint32_t timeoutMs) {
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    char service[8];
    snprintf(service, sizeof(service), "%d", (*target).port);
    struct addrinfo *resolved = nullptr;
    if (getaddrinfo((*target).host, service, &hints, &resolved) != 0 ||
        resolved == nullptr)
        return -1;

    int fd = -1;
    for (struct addrinfo *it = resolved; it != nullptr; it = (*it).ai_next) {
        fd = socket((*it).ai_family, (*it).ai_socktype, (*it).ai_protocol);
        if (fd < 0)
            continue;
        int flags = fcntl(fd, F_GETFL, 0);
        if (flags < 0 || fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
            close(fd);
            fd = -1;
            continue;
        }
        int state = connect(fd, (*it).ai_addr, (*it).ai_addrlen);
        if (state != 0 && errno == EINPROGRESS) {
            struct pollfd waiting = { .fd = fd, .events = POLLOUT };
            if (poll(&waiting, 1, (int) timeoutMs) > 0) {
                int socketError = 0;
                socklen_t errorLength = (socklen_t) sizeof(socketError);
                if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &socketError, &errorLength) == 0 &&
                    socketError == 0)
                    state = 0;
            }
        }
        if (state == 0) {
            fcntl(fd, F_SETFL, flags);
            break;
        }
        close(fd);
        fd = -1;
    }
    freeaddrinfo(resolved);
    if (fd >= 0)
        wsSetTimeout(fd, timeoutMs);
    return fd;
}

/** Write a whole buffer, retrying short writes. */
static bool wsSendAll(int fd, const uint8_t *bytes, size_t length) {
    size_t sent = 0u;
    while (sent < length) {
        ssize_t written = send(fd, bytes + sent, length - sent, 0);
        if (written <= 0)
            return false;
        sent += (size_t) written;
    }
    return true;
}

/** Write through whichever transport this client owns (socket or TLS). */
static bool wsWrite(WebSocket *self, const uint8_t *bytes, size_t length) {
    if (self == nullptr)
        return false;
    if ((*self).tls != nullptr)
        return Tls_send((*self).tls, (const char*) bytes, length);
    return (*self).fd >= 0 && wsSendAll((*self).fd, bytes, length);
}

/** Read exactly length Bytes, retrying short reads within the transport budget. */
static bool wsReadExact(WebSocket *self, uint8_t *dest, size_t length) {
    size_t got = 0u;
    while (got < length) {
        if ((*self).tls != nullptr) {
            size_t taken = 0u;
            if (!Tls_recv((*self).tls, (char*) dest + got, length - got, &taken) ||
                taken == 0u)
                return false;
            got += taken;
            continue;
        }
        ssize_t taken = recv((*self).fd, dest + got, length - got, 0);
        if (taken <= 0)
            return false;
        got += (size_t) taken;
    }
    return true;
}

/** Index just past a complete "\r\n\r\n", or SIZE_MAX when it is absent. */
static size_t wsFindHeaderEnd(const char *bytes, size_t length) {
    for (size_t i = 3u; i < length; i++) {
        if (bytes[i - 3u] == '\r' && bytes[i - 2u] == '\n' &&
            bytes[i - 1u] == '\r' && bytes[i] == '\n')
            return i + 1u;
    }
    return SIZE_MAX;
}

/** Case-insensitive ASCII compare of n Bytes. */
static bool wsCiEq(const char *a, const char *b, size_t n) {
    for (size_t i = 0u; i < n; i++) {
        char ca = a[i];
        char cb = b[i];
        if (ca >= 'A' && ca <= 'Z') ca = (char) (ca - 'A' + 'a');
        if (cb >= 'A' && cb <= 'Z') cb = (char) (cb - 'A' + 'a');
        if (ca != cb)
            return false;
    }
    return true;
}

/**
 * Copy one response header's trimmed value into out, or return false when the
 * header is absent or the value would not fit. Names are matched
 * case-insensitively, as the RFC requires.
 */
static bool wsHeaderValue(const char *block, const char *name, char *out, size_t cap) {
    size_t nameLength = strlen(name);
    const char *line = block;
    while (line != nullptr && *line != '\0') {
        const char *end = strstr(line, "\r\n");
        const char *stop = end == nullptr ? line + strlen(line) : end;
        const char *colon = memchr(line, ':', (size_t) (stop - line));
        if (colon != nullptr && (size_t) (colon - line) == nameLength &&
            wsCiEq(line, name, nameLength)) {
            const char *value = colon + 1;
            while (value < stop && (*value == ' ' || *value == '\t'))
                value++;
            const char *tail = stop;
            while (tail > value && (tail[-1] == ' ' || tail[-1] == '\t'))
                tail--;
            size_t valueLength = (size_t) (tail - value);
            if (valueLength + 1u > cap)
                return false;
            memcpy(out, value, valueLength);
            out[valueLength] = '\0';
            return true;
        }
        if (end == nullptr)
            break;
        line = end + 2;
    }
    return false;
}

/** Split a ws:// URL into authority host, port and request target. */
static bool wsParseUrl(const char *url, WsTarget *out) {
    if (url == nullptr)
        return false;
    bool secure = false;
    const char *rest = nullptr;
    if (strncmp(url, "wss://", 6u) == 0) {
        secure = true;
        rest = url + 6u;
    } else if (strncmp(url, "ws://", 5u) == 0) {
        rest = url + 5u;
    } else {
        return false;
    }
    const char *slash = strchr(rest, '/');
    size_t authorityLength = slash == nullptr ? strlen(rest) : (size_t) (slash - rest);
    if (authorityLength == 0u || authorityLength >= WS_HOST_BYTES + 8u)
        return false;
    char authority[WS_HOST_BYTES + 8u];
    memcpy(authority, rest, authorityLength);
    authority[authorityLength] = '\0';

    const char *userInfo = strrchr(authority, '@');
    const char *hostPort = userInfo == nullptr ? authority : userInfo + 1;

    int port = secure ? 443 : 80;
    size_t hostLength = strlen(hostPort);
    if (*hostPort == '[') {
        const char *close = strchr(hostPort, ']');
        if (close == nullptr)
            return false;
        hostLength = (size_t) (close - hostPort - 1);
        const char *after = close + 1;
        if (*after != '\0') {
            if (*after != ':')
                return false;
            int value = 0;
            for (const char *digit = after + 1; *digit != '\0'; digit++) {
                if (*digit < '0' || *digit > '9')
                    return false;
                value = value * 10 + (*digit - '0');
                if (value > 65535)
                    return false;
            }
            port = value;
        }
        if (hostLength == 0u || hostLength >= WS_HOST_BYTES)
            return false;
        memset(out, 0, sizeof(*out));
        memcpy((*out).host, hostPort + 1, hostLength);
        (*out).host[hostLength] = '\0';
    } else {
        const char *colon = strrchr(hostPort, ':');
        if (colon != nullptr) {
            hostLength = (size_t) (colon - hostPort);
            const char *digits = colon + 1;
            if (*digits == '\0')
                return false;
            int value = 0;
            for (const char *digit = digits; *digit != '\0'; digit++) {
                if (*digit < '0' || *digit > '9')
                    return false;
                value = value * 10 + (*digit - '0');
                if (value > 65535)
                    return false;
            }
            port = value;
        }
        if (hostLength == 0u || hostLength >= WS_HOST_BYTES)
            return false;
        memset(out, 0, sizeof(*out));
        memcpy((*out).host, hostPort, hostLength);
        (*out).host[hostLength] = '\0';
    }

    const char *path = slash == nullptr ? "/" : slash;
    if (strlen(path) >= WS_PATH_BYTES)
        return false;
    memcpy((*out).path, path, strlen(path) + 1u);
    (*out).port = port;
    (*out).secure = secure;
    return true;
}

/** Record a bounded diagnostic and make the connection terminal. */
static void wsFail(WebSocket *self, const char *reason) {
    if (self == nullptr)
        return;
    size_t cap = sizeof((*self).error);
    size_t length = strlen(reason);
    size_t copy = length < cap - 1u ? length : cap - 1u;
    memcpy((*self).error, reason, copy);
    (*self).error[copy] = '\0';
    (*self).state = WEBSOCKET_FAILED;
}

/** RFC 3629 validation: rejects overlong forms and surrogate halves. */
static bool wsUtf8Valid(const uint8_t *bytes, size_t length) {
    size_t i = 0u;
    while (i < length) {
        uint8_t lead = bytes[i];
        if (lead < 0x80u) {
            i++;
            continue;
        }
        size_t extra;
        uint32_t codepoint;
        uint32_t minimum;
        if ((lead & 0xE0u) == 0xC0u) {
            extra = 1u;
            codepoint = lead & 0x1Fu;
            minimum = 0x80u;
        } else if ((lead & 0xF0u) == 0xE0u) {
            extra = 2u;
            codepoint = lead & 0x0Fu;
            minimum = 0x800u;
        } else if ((lead & 0xF8u) == 0xF0u) {
            extra = 3u;
            codepoint = lead & 0x07u;
            minimum = 0x10000u;
        } else {
            return false;
        }
        if (i + extra >= length)
            return false;
        for (size_t k = 1u; k <= extra; k++) {
            if ((bytes[i + k] & 0xC0u) != 0x80u)
                return false;
            codepoint = (codepoint << 6) | (uint32_t) (bytes[i + k] & 0x3Fu);
        }
        if (codepoint < minimum || codepoint > 0x10FFFFu ||
            (codepoint >= 0xD800u && codepoint <= 0xDFFFu))
            return false;
        i += extra + 1u;
    }
    return true;
}

/** Append reassembled payload Bytes, refusing anything past the message cap. */
static bool wsAppendMessage(WebSocket *self, const uint8_t *bytes, size_t length) {
    if (length > (*self).maxMessageBytes - (*self).messageLength) {
        wsFail(self, "message exceeds the configured cap");
        return false;
    }
    size_t wanted = (*self).messageLength + length;
    if (wanted > (*self).messageCap) {
        size_t cap = (*self).messageCap == 0u ? 1024u : (*self).messageCap;
        while (cap < wanted)
            cap *= 2u;
        uint8_t *grown = (uint8_t*) Memory_realloc((*self).message, cap);
        if (grown == nullptr) {
            wsFail(self, "message buffer growth failed");
            return false;
        }
        (*self).message = grown;
        (*self).messageCap = cap;
    }
    memcpy((*self).message + (*self).messageLength, bytes, length);
    (*self).messageLength += length;
    return true;
}

/** True when this client owns a live transport — a socket or a TLS session. */
static inline bool wsConnected(const WebSocket *self) {
    return self != nullptr && ((*self).fd >= 0 || (*self).tls != nullptr);
}

/** Encode one final-or-fragmented frame with a fresh mask and send it. */
static bool wsSendFrame(WebSocket *self, uint8_t opcode, bool fin,
                        const uint8_t *payload, size_t length) {
    if (!wsConnected(self))
        return false;
    uint8_t header[14];
    size_t headerLength = 0u;
    header[headerLength++] = (uint8_t) ((fin ? 0x80u : 0x00u) | (opcode & 0x0Fu));
    if (length <= 125u) {
        header[headerLength++] = (uint8_t) (0x80u | length);
    } else if (length <= 0xFFFFu) {
        header[headerLength++] = (uint8_t) (0x80u | 126u);
        header[headerLength++] = (uint8_t) (length >> 8);
        header[headerLength++] = (uint8_t) (length & 0xFFu);
    } else {
        header[headerLength++] = (uint8_t) (0x80u | 127u);
        uint64_t value = (uint64_t) length;
        for (int shift = 56; shift >= 0; shift -= 8)
            header[headerLength++] = (uint8_t) ((value >> shift) & 0xFFu);
    }
    uint8_t mask[4];
    SecureRandom *random = SecureRandom_0();
    if (random == nullptr)
        return false;
    bool masked = SecureRandom_bytes(random, mask, sizeof(mask));
    SecureRandom_free(random);
    if (!masked)
        return false;
    memcpy(header + headerLength, mask, 4u);
    headerLength += 4u;
    if (!wsWrite(self, header, headerLength))
        return false;

    // Mask in bounded chunks: the mask index keeps running across chunks, so
    // chunking cannot change a single payload Byte.
    uint8_t chunk[WS_RX_BYTES_DEFAULT];
    size_t sent = 0u;
    while (sent < length) {
        size_t take = length - sent < sizeof(chunk) ? length - sent : sizeof(chunk);
        for (size_t i = 0u; i < take; i++)
            chunk[i] = (uint8_t) (payload[sent + i] ^ mask[(sent + i) & 3u]);
        if (!wsWrite(self, chunk, take))
            return false;
        sent += take;
    }
    (*self).framesOut++;
    return true;
}

/** Decode one frame header, refusing reserved bits and a bad 64-bit length. */
static bool wsReadFrameHeader(WebSocket *self, bool *fin, uint8_t *opcode,
                              bool *masked, uint64_t *length) {
    uint8_t head[2];
    if (!wsReadExact(self, head, 2u)) {
        wsFail(self, "frame header read failed");
        return false;
    }
    if ((head[0] & 0x70u) != 0u) {
        wsFail(self, "reserved frame bits must be zero");
        return false;
    }
    *fin = (head[0] & 0x80u) != 0u;
    *opcode = (uint8_t) (head[0] & 0x0Fu);
    *masked = (head[1] & 0x80u) != 0u;
    uint64_t value = (uint64_t) (head[1] & 0x7Fu);
    if (value == 126u) {
        uint8_t extended[2];
        if (!wsReadExact(self, extended, 2u)) {
            wsFail(self, "frame length read failed");
            return false;
        }
        value = ((uint64_t) extended[0] << 8) | (uint64_t) extended[1];
    } else if (value == 127u) {
        uint8_t extended[8];
        if (!wsReadExact(self, extended, 8u)) {
            wsFail(self, "frame length read failed");
            return false;
        }
        if ((extended[0] & 0x80u) != 0u) {
            wsFail(self, "64-bit frame length must clear its high bit");
            return false;
        }
        value = 0u;
        for (size_t i = 0u; i < 8u; i++)
            value = (value << 8) | (uint64_t) extended[i];
    }
    *length = value;
    return true;
}

/** Consume one frame: control frames act inline, data frames assemble. */
static WebSocketMessage wsConsumeFrame(WebSocket *self) {
    WebSocketMessage result;
    memset(&result, 0, sizeof(result));
    result.kind = WS_MESSAGE_NONE;
    result.closeCode = 1005u;

    bool fin = false;
    bool masked = false;
    uint8_t opcode = 0u;
    uint64_t length = 0u;
    if (!wsReadFrameHeader(self, &fin, &opcode, &masked, &length)) {
        result.kind = WS_MESSAGE_ERROR;
        memcpy(result.error, (*self).error, sizeof(result.error));
        return result;
    }
    if (masked) {
        wsFail(self, "server frames must not be masked");
        result.kind = WS_MESSAGE_ERROR;
        memcpy(result.error, (*self).error, sizeof(result.error));
        return result;
    }
    if (length > (*self).maxMessageBytes) {
        wsFail(self, "frame length exceeds the configured cap");
        result.kind = WS_MESSAGE_ERROR;
        memcpy(result.error, (*self).error, sizeof(result.error));
        return result;
    }

    if ((opcode & 0x08u) != 0u) {
        if (!fin || length > WS_CONTROL_MAX_BYTES) {
            wsFail(self, "control frames must be final and within 125 Bytes");
            result.kind = WS_MESSAGE_ERROR;
            memcpy(result.error, (*self).error, sizeof(result.error));
            return result;
        }
        if (length > 0u && !wsReadExact(self, (*self).control, (size_t) length)) {
            wsFail(self, "control payload read failed");
            result.kind = WS_MESSAGE_ERROR;
            memcpy(result.error, (*self).error, sizeof(result.error));
            return result;
        }
        (*self).framesIn++;
        if (opcode == WEBSOCKET_PING) {
            if (!wsSendFrame(self, WEBSOCKET_PONG, true, (*self).control, (size_t) length)) {
                wsFail(self, "pong reply failed");
                result.kind = WS_MESSAGE_ERROR;
                memcpy(result.error, (*self).error, sizeof(result.error));
                return result;
            }
            (*self).pongsSent++;
            return result;
        }
        if (opcode == WEBSOCKET_PONG)
            return result;
        if (opcode == WEBSOCKET_CLOSE) {
            uint16_t code = 1005u;
            if (length >= 2u)
                code = (uint16_t) (((uint16_t) (*self).control[0] << 8) |
                                   (uint16_t) (*self).control[1]);
            (*self).closeCode = code;
            (*self).closeReason[0] = '\0';
            (*self).closeReasonTruncated = false;
            if (length > 2u) {
                size_t textLength = (size_t) length - 2u;
                size_t cap = sizeof((*self).closeReason) - 1u;
                size_t copy = textLength < cap ? textLength : cap;
                memcpy((*self).closeReason, (*self).control + 2u, copy);
                (*self).closeReason[copy] = '\0';
                (*self).closeReasonTruncated = copy < textLength;
            }
            if ((*self).state == WEBSOCKET_OPEN)
                (void) wsSendFrame(self, WEBSOCKET_CLOSE, true, (*self).control, (size_t) length);
            (*self).state = WEBSOCKET_CLOSED;
            result.kind = WS_MESSAGE_CLOSE;
            result.closeCode = code;
            result.truncated = (*self).closeReasonTruncated;
            return result;
        }
        wsFail(self, "unknown control opcode");
        result.kind = WS_MESSAGE_ERROR;
        memcpy(result.error, (*self).error, sizeof(result.error));
        return result;
    }

    if (opcode == WEBSOCKET_CONTINUATION) {
        if ((*self).messageOpcode == 0u) {
            wsFail(self, "continuation frame without a start frame");
            result.kind = WS_MESSAGE_ERROR;
            memcpy(result.error, (*self).error, sizeof(result.error));
            return result;
        }
    } else if (opcode == WEBSOCKET_TEXT || opcode == WEBSOCKET_BINARY) {
        if ((*self).messageOpcode != 0u) {
            wsFail(self, "new data frame inside a fragmented message");
            result.kind = WS_MESSAGE_ERROR;
            memcpy(result.error, (*self).error, sizeof(result.error));
            return result;
        }
        (*self).messageOpcode = opcode;
        (*self).messageLength = 0u;
    } else {
        wsFail(self, "unknown data opcode");
        result.kind = WS_MESSAGE_ERROR;
        memcpy(result.error, (*self).error, sizeof(result.error));
        return result;
    }

    size_t remaining = (size_t) length;
    uint8_t chunk[WS_RX_BYTES_DEFAULT];
    while (remaining > 0u) {
        size_t take = remaining < sizeof(chunk) ? remaining : sizeof(chunk);
        if (!wsReadExact(self, chunk, take) || !wsAppendMessage(self, chunk, take)) {
            if ((*self).state != WEBSOCKET_FAILED)
                wsFail(self, "message payload read failed");
            result.kind = WS_MESSAGE_ERROR;
            memcpy(result.error, (*self).error, sizeof(result.error));
            return result;
        }
        remaining -= take;
    }
    (*self).framesIn++;
    if (!fin)
        return result; // a fragmented message continues on the next frame

    if ((*self).messageOpcode == WEBSOCKET_TEXT &&
        !wsUtf8Valid((*self).message, (*self).messageLength)) {
        wsFail(self, "text message is not valid UTF-8");
        result.kind = WS_MESSAGE_ERROR;
        memcpy(result.error, (*self).error, sizeof(result.error));
        return result;
    }
    result.kind = (*self).messageOpcode == WEBSOCKET_TEXT ? WS_MESSAGE_TEXT
                                                         : WS_MESSAGE_BINARY;
    result.payload = (*self).message;
    result.length = (*self).messageLength;
    (*self).messageOpcode = 0u;
    (*self).messageLength = 0u;
    return result;
}

/** Perform the RFC 6455 opening handshake and verify the accept hash. */
static bool wsHandshake(WebSocket *self, const WsTarget *target) {
    uint8_t keyBytes[16];
    SecureRandom *random = SecureRandom_0();
    if (random == nullptr || !SecureRandom_bytes(random, keyBytes, sizeof(keyBytes))) {
        SecureRandom_free(random);
        THROW("WebSocket_open: no secure random source for the client key");
        wsFail(self, "no secure random source");
        return false;
    }
    SecureRandom_free(random);

    char key[32];
    if (Url_base64(keyBytes, sizeof(keyBytes), key, sizeof(key)) < 0) {
        THROW("WebSocket_open: client key encoding failed");
        wsFail(self, "client key encoding failed");
        return false;
    }
    CryptoSha1 digest = CryptoSha1_0();
    Crypto_sha1Init(&digest);
    Crypto_sha1Update(&digest, key, strlen(key));
    Crypto_sha1Update(&digest, WS_HANDSHAKE_GUID, strlen(WS_HANDSHAKE_GUID));
    uint8_t hash[CRYPTO_SHA1_DIGEST_SIZE];
    Crypto_sha1Final(&digest, hash);
    char accept[64];
    if (Url_base64(hash, CRYPTO_SHA1_DIGEST_SIZE, accept, sizeof(accept)) < 0) {
        THROW("WebSocket_open: accept hash encoding failed");
        wsFail(self, "accept hash encoding failed");
        return false;
    }

    char request[WS_PATH_BYTES + 512u];
    int written = snprintf(request, sizeof(request),
        "GET %s HTTP/1.1\r\n"
        "Host: %s:%d\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Key: %s\r\n"
        "Sec-WebSocket-Version: 13\r\n"
        "\r\n",
        (*target).path, (*target).host, (*target).port, key);
    if (written < 0 || (size_t) written >= sizeof(request)) {
        THROW("WebSocket_open: handshake request does not fit");
        wsFail(self, "handshake request does not fit");
        return false;
    }
    if (!wsWrite(self, (const uint8_t*) request, (size_t) written)) {
        THROW("WebSocket_open: handshake request write failed");
        wsFail(self, "handshake request write failed");
        return false;
    }

    // Read the response one Byte at a time: a server may send its first frame in
    // the same segment, and a bulk read would swallow those Bytes. Nothing is
    // buffered past the header, so the frame stream resumes exactly at the next
    // recv.
    char response[WS_HANDSHAKE_MAX_BYTES + 1u];
    size_t used = 0u;
    bool complete = false;
    while (used < WS_HANDSHAKE_MAX_BYTES) {
        if (!wsReadExact(self, (uint8_t*) (response + used), 1u))
            break;
        used += 1u;
        response[used] = '\0';
        if (used >= 4u && wsFindHeaderEnd(response, used) != SIZE_MAX) {
            complete = true;
            break;
        }
    }
    response[used] = '\0';
    if (!complete) {
        THROW("WebSocket_open: handshake response incomplete");
        wsFail(self, "handshake response incomplete");
        return false;
    }
    if (strncmp(response, "HTTP/1.1 101", 12u) != 0 &&
        strncmp(response, "HTTP/1.0 101", 12u) != 0) {
        THROW("WebSocket_open: handshake rejected (expected 101)");
        wsFail(self, "handshake rejected: no 101");
        return false;
    }
    char value[256];
    if (!wsHeaderValue(response, "Upgrade", value, sizeof(value)) ||
        !wsCiEq(value, "websocket", strlen("websocket"))) {
        THROW("WebSocket_open: handshake lacks Upgrade: websocket");
        wsFail(self, "handshake lacks Upgrade");
        return false;
    }
    if (!wsHeaderValue(response, "Connection", value, sizeof(value)) ||
        !wsCiEq(value, "Upgrade", strlen("Upgrade"))) {
        THROW("WebSocket_open: handshake lacks Connection: Upgrade");
        wsFail(self, "handshake lacks Connection");
        return false;
    }
    if (!wsHeaderValue(response, "Sec-WebSocket-Accept", value, sizeof(value)) ||
        strcmp(value, accept) != 0) {
        THROW("WebSocket_open: handshake accept hash mismatch");
        wsFail(self, "handshake accept hash mismatch");
        return false;
    }
    return true;
}

// CONSTRUCTORS

/** Connect, handshake, and leave the client OPEN, or fail closed. */
bool WebSocket_open(WebSocket *self, const char *url, WebSocketConfig config) {
    if (self == nullptr || url == nullptr) {
        THROW("WebSocket_open: null argument");
        return false;
    }
    WebSocket_close(self);
    (*self).timeoutMs = config.timeoutMs == 0u ? 5000u : config.timeoutMs;
    (*self).maxMessageBytes = config.maxMessageBytes == 0u
        ? WS_MESSAGE_BYTES_DEFAULT : config.maxMessageBytes;
    (*self).error[0] = '\0';
    (*self).closeReason[0] = '\0';
    (*self).closeReasonTruncated = false;
    (*self).closeCode = 1005u;
    (*self).messageOpcode = 0u;
    (*self).messageLength = 0u;
    (*self).framesIn = 0u;
    (*self).framesOut = 0u;
    (*self).pongsSent = 0u;

    WsTarget target;
    if (!wsParseUrl(url, &target)) {
        THROW("WebSocket_open: malformed or unsupported WebSocket URL");
        wsFail(self, "unsupported URL");
        return false;
    }
    if (target.secure) {
        TlsConn *tls = nullptr;
        if (!Tls_connect(target.host, target.port, &tls)) {
            THROW("WebSocket_open: TLS connect failed (no backend, or the peer was refused)");
            wsFail(self, "TLS connect failed");
            return false;
        }
        Tls_setBudget(tls, (*self).timeoutMs);
        (*self).tls = tls;
    } else {
        int fd = wsConnect(&target, (*self).timeoutMs);
        if (fd < 0) {
            THROW("WebSocket_open: connect failed");
            wsFail(self, "connect failed");
            return false;
        }
        (*self).fd = fd;
    }
    (*self).message = (uint8_t*) Memory_alloc(TYPE_BYTE_ARRAY, 1024u);
    if ((*self).message == nullptr) {
        THROW("WebSocket_open: reassembly buffer allocation failed");
        wsFail(self, "reassembly buffer allocation failed");
        WebSocket_close(self);
        return false;
    }
    (*self).messageCap = 1024u;
    if (!wsHandshake(self, &target)) {
        WebSocket_close(self);
        return false;
    }
    (*self).state = WEBSOCKET_OPEN;
    return true;
}

/** Send a normal close when open, drop the socket, release the owned buffer. */
void WebSocket_close(WebSocket *self) {
    if (self == nullptr)
        return;
    if ((*self).state == WEBSOCKET_OPEN &&
        ((*self).fd >= 0 || (*self).tls != nullptr))
        (void) WebSocket_writeClose(self, 1000u, nullptr);
    if ((*self).tls != nullptr) {
        Tls_close((*self).tls);
        (*self).tls = nullptr;
    }
    if ((*self).fd >= 0) {
        close((*self).fd);
        (*self).fd = -1;
    }
    if ((*self).message != nullptr) {
        Memory_free((*self).message);
        (*self).message = nullptr;
    }
    (*self).messageCap = 0u;
    (*self).messageLength = 0u;
    (*self).messageOpcode = 0u;
    (*self).state = WEBSOCKET_CLOSED;
}

// CORE FUNCTIONS

/** Read frames within the budget; control frames act inline. */
WebSocketMessage WebSocket_poll(WebSocket *self, uint32_t budgetMs) {
    WebSocketMessage result;
    memset(&result, 0, sizeof(result));
    result.kind = WS_MESSAGE_NONE;
    result.closeCode = 1005u;
    if (self == nullptr || ((*self).fd < 0 && (*self).tls == nullptr)) {
        result.kind = WS_MESSAGE_ERROR;
        memcpy(result.error, "connection is not open", sizeof("connection is not open"));
        return result;
    }
    if ((*self).state == WEBSOCKET_FAILED) {
        result.kind = WS_MESSAGE_ERROR;
        memcpy(result.error, (*self).error, sizeof(result.error));
        return result;
    }
    if ((*self).state == WEBSOCKET_CLOSED) {
        result.kind = WS_MESSAGE_CLOSE;
        result.closeCode = (*self).closeCode;
        return result;
    }

    uint64_t deadline = wsNowMs() + (uint64_t) budgetMs;
    for (;;) {
        uint64_t now = wsNowMs();
        if (now >= deadline)
            return result;
        uint32_t slice = (uint32_t) (deadline - now);
        if (slice > 100u)
            slice = 100u; // bounded slices: re-check the budget, never park
        if ((*self).tls != nullptr) {
            // A TLS connection has no descriptor to poll, so ask the transport
            // for readiness instead. A quiet budget keeps the connection healthy
            // and simply yields nothing this step; a broken peer is reported by
            // the read or write that actually fails.
            Tls_setBudget((*self).tls, slice);
            if (!Tls_readable((*self).tls, slice))
                continue;
        } else {
            struct pollfd waiting = { .fd = (*self).fd, .events = POLLIN };
            int ready = poll(&waiting, 1, (int) slice);
            if (ready == 0)
                continue;
            if (ready < 0) {
                wsFail(self, "poll failed");
                result.kind = WS_MESSAGE_ERROR;
                memcpy(result.error, (*self).error, sizeof(result.error));
                return result;
            }
        }
        WebSocketMessage frame = wsConsumeFrame(self);
        if (frame.kind != WS_MESSAGE_NONE)
            return frame;
    }
}

/** Send one final text frame; the payload must already be valid UTF-8. */
bool WebSocket_writeText(WebSocket *self, const char *text, size_t length) {
    if (!wsConnected(self) || text == nullptr)
        return false;
    if (!wsUtf8Valid((const uint8_t*) text, length)) {
        // A locally rejected payload is not a transport fault: the connection
        // stays usable and the caller may send something valid next.
        THROW("WebSocket_writeText: payload is not valid UTF-8");
        return false;
    }
    if (!wsSendFrame(self, WEBSOCKET_TEXT, true, (const uint8_t*) text, length)) {
        wsFail(self, "text frame write failed");
        return false;
    }
    return true;
}

/** Send one final binary frame. */
bool WebSocket_writeBinary(WebSocket *self, const uint8_t *bytes, size_t length) {
    if (!wsConnected(self) || (bytes == nullptr && length != 0u))
        return false;
    if (!wsSendFrame(self, WEBSOCKET_BINARY, true, bytes, length)) {
        wsFail(self, "binary frame write failed");
        return false;
    }
    return true;
}

/** Send a ping; the peer's pong is consumed silently by poll. */
bool WebSocket_writePing(WebSocket *self, const uint8_t *payload, size_t length) {
    if (!wsConnected(self) || (payload == nullptr && length != 0u))
        return false;
    if (length > WS_CONTROL_MAX_BYTES) {
        THROW("WebSocket_writePing: control payload exceeds 125 Bytes");
        return false;
    }
    if (!wsSendFrame(self, WEBSOCKET_PING, true, payload, length)) {
        wsFail(self, "ping write failed");
        return false;
    }
    return true;
}

/** Send a close frame and move to CLOSING; the reason is cut and flagged. */
bool WebSocket_writeClose(WebSocket *self, uint16_t code, const char *reason) {
    if (!wsConnected(self))
        return false;
    uint8_t payload[2u + WS_CLOSE_REASON_BYTES];
    size_t length = 0u;
    if (code != 0u) {
        payload[0] = (uint8_t) (code >> 8);
        payload[1] = (uint8_t) (code & 0xFFu);
        length = 2u;
        if (reason != nullptr) {
            size_t textLength = strlen(reason);
            size_t copy = textLength < WS_CLOSE_REASON_BYTES ? textLength : WS_CLOSE_REASON_BYTES;
            memcpy(payload + 2u, reason, copy);
            length += copy;
            (*self).closeReasonTruncated = copy < textLength;
        }
    }
    if (!wsSendFrame(self, WEBSOCKET_CLOSE, true, payload, length)) {
        wsFail(self, "close frame write failed");
        return false;
    }
    (*self).state = WEBSOCKET_CLOSING;
    return true;
}

// GETTERS (the Symmetric Getter/Setter Completeness Law: null-safe)

/** The connection state, or CLOSED for a null client. */
WebSocketState WebSocket_state(const WebSocket *self) {
    return self == nullptr ? WEBSOCKET_CLOSED : (*self).state;
}

/** True only while the handshake is accepted and no close has been seen. */
bool WebSocket_isOpen(const WebSocket *self) {
    return self != nullptr && (*self).state == WEBSOCKET_OPEN;
}

/** The bounded last-failure text, or "" when there is none. */
const char *WebSocket_lastError(const WebSocket *self) {
    return self == nullptr ? "" : (*self).error;
}

/** The finalized close code, or 1005 when the peer sent none. */
uint16_t WebSocket_closeCode(const WebSocket *self) {
    return self == nullptr ? 1005u : (*self).closeCode;
}

/** The bounded close reason text, or "" when there is none. */
const char *WebSocket_closeReason(const WebSocket *self) {
    return self == nullptr ? "" : (*self).closeReason;
}

/** The reassembly cap in force, or zero for a null client. */
size_t WebSocket_maxMessageBytes(const WebSocket *self) {
    return self == nullptr ? 0u : (*self).maxMessageBytes;
}

/** Complete frames decoded since open. */
uint64_t WebSocket_framesIn(const WebSocket *self) {
    return self == nullptr ? 0u : (*self).framesIn;
}

/** Complete frames encoded and sent since open. */
uint64_t WebSocket_framesOut(const WebSocket *self) {
    return self == nullptr ? 0u : (*self).framesOut;
}

/** Automatic pong replies sent since open. */
uint64_t WebSocket_pongsSent(const WebSocket *self) {
    return self == nullptr ? 0u : (*self).pongsSent;
}

// STRING PROJECTIONS (the toString Law)

/** The state's stable name for the projections. */
static const char *wsStateName(WebSocketState state) {
    switch (state) {
        case WEBSOCKET_OPEN: return "OPEN";
        case WEBSOCKET_CLOSING: return "CLOSING";
        case WEBSOCKET_FAILED: return "FAILED";
        case WEBSOCKET_CLOSED: return "CLOSED";
    }
    return "UNKNOWN";
}

/** The VALUE string: the live connection summary. */
void WebSocket_toString(const WebSocket *self, char *dest, size_t cap, bool *outTruncated) {
    Str out = Str(dest, cap);
    if (self == nullptr) {
        Str_put(&out, "nullptr");
    } else {
        Str_printf(&out,
            "WebSocket { state=%s, fd=%d, closeCode=%u, framesIn=%llu, framesOut=%llu, "
            "pongs=%llu }",
            wsStateName((*self).state), (*self).fd, (unsigned) (*self).closeCode,
            (unsigned long long) (*self).framesIn,
            (unsigned long long) (*self).framesOut,
            (unsigned long long) (*self).pongsSent);
    }
    if (outTruncated != nullptr)
        *outTruncated = Str_isTruncated(&out);
}

/** The STRUCTURE string: this class's own fields, one layer, no recursion. */
void WebSocket_toStringStruct(const WebSocket *self, char *dest, size_t cap,
                              bool *outTruncated) {
    Str out = Str(dest, cap);
    if (self == nullptr) {
        Str_put(&out, "nullptr");
    } else {
        Str_printf(&out,
            "WebSocket { fd=%d, state=%s, timeoutMs=%u, maxMessageBytes=%llu, "
            "message=%s, messageCap=%llu, messageLength=%llu, messageOpcode=%u, "
            "closeCode=%u, closeReasonTruncated=%s, framesIn=%llu, framesOut=%llu, "
            "pongsSent=%llu }",
            (*self).fd, wsStateName((*self).state), (*self).timeoutMs,
            (unsigned long long) (*self).maxMessageBytes,
            (*self).message == nullptr ? "nullptr" : "owned",
            (unsigned long long) (*self).messageCap,
            (unsigned long long) (*self).messageLength, (*self).messageOpcode,
            (unsigned) (*self).closeCode,
            (*self).closeReasonTruncated ? "true" : "false",
            (unsigned long long) (*self).framesIn,
            (unsigned long long) (*self).framesOut,
            (unsigned long long) (*self).pongsSent);
    }
    if (outTruncated != nullptr)
        *outTruncated = Str_isTruncated(&out);
}
