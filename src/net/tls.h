#ifndef NET_TLS_H
#define NET_TLS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// net/tls.h — TLS backend seam (Legacy: net/TransportProtocol.java).
//
// http:// stays on plain TCP. https:// (and wss://) require a backend:
//   - APPLE: src/net/tls_apple.m — Network.framework, system trust anchors.
//            Implemented; this is the proven backend.
//   - other: src/net/tls_curl.c — libcurl. Still a fail-closed stub: it returns
//            false without sending a Byte until a libcurl backend is wired.
// The Apple and curl translation units are alternatives, never both linked: the
// build selects one per platform, and Tls_backend() reports which answered.
// Verification is never optional: a peer whose chain does not validate against
// the platform trust store is refused, and there is deliberately no switch to
// accept an untrusted certificate.

typedef enum TlsBackendKind {
    TLS_BACKEND_NONE = 0,  // no backend answered (stub or unavailable)
    TLS_BACKEND_APPLE = 1, // Network.framework
    TLS_BACKEND_CURL = 2,  // libcurl
} TlsBackendKind;

typedef struct TlsConn {
    int backend;   // TlsBackendKind that produced this connection
    void *opaque;  // backend-owned session; released only by Tls_close
} TlsConn;

// Connect with TLS to host:port. connOut is filled on success.
// Returns false when scheme is unsupported or handshake fails.
bool Tls_connect(const char *host, int port, TlsConn **connOut);

// Send Bytes over an established TLS connection.
bool Tls_send(TlsConn *conn, const char *buf, size_t len);

// Receive up to cap Bytes. gotOut receives the count.
bool Tls_recv(TlsConn *conn, char *buf, size_t cap, size_t *gotOut);

// Close and release a connection Tls_connect produced. Null-safe. NEVER call it
// on a caller-owned TlsConn: the record is owned by the backend that created it,
// and closing one the backend did not allocate is undefined.
void Tls_close(TlsConn *conn);

// Non-consuming readiness: true when at least one Byte can be read within the
// budget, false on a quiet budget or a broken connection. A poll loop uses it so
// a quiet transport is not read as a failure — which is why `wss://` needs it,
// since a TLS connection has no descriptor to poll.
bool Tls_readable(TlsConn *conn, uint32_t budgetMs);

// The per-connection IO budget: every send and receive waits at most this long.
// A caller driving a bounded poll loop sets it so the transport honours the same
// slice the loop promises its own caller — without it a TLS read has no fd to
// carry SO_RCVTIMEO and would outlive the caller's budget. Zero restores the
// backend default; the getter proves the budget believed in is the budget in
// force.
void Tls_setBudget(TlsConn *conn, uint32_t timeoutMs);
uint32_t Tls_getBudget(const TlsConn *conn);

// Which backend answered the last successful connect; TLS_BACKEND_NONE until
// one does. A host reports the transport it actually got, never the one it
// hoped for.
TlsBackendKind Tls_backend(void);

#endif
