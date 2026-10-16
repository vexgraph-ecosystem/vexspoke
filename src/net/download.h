#ifndef NET_DOWNLOAD_H
#define NET_DOWNLOAD_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "c23/constructor.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct DownloadOptions {
    uint32_t timeout_seconds;
    bool follow_redirects;
    bool silent;
    const char *user_agent;
} DownloadOptions;

static inline DownloadOptions DownloadOptions_0(void) {
    return (DownloadOptions) { .timeout_seconds = 30u, .follow_redirects = true,
        .silent = true, .user_agent = "vexgraph/1.0 (Homebrew/Tooling Client)" };
}
#define DownloadOptions(...) CONSTRUCTOR_DISPATCH(DownloadOptions, __VA_ARGS__)
static inline void DownloadOptions_setTimeout(DownloadOptions *options, uint32_t seconds) {
    if (options) (*options).timeout_seconds = seconds;
}
static inline void DownloadOptions_setFollowRedirects(DownloadOptions *options, bool enabled) {
    if (options) (*options).follow_redirects = enabled;
}
static inline void DownloadOptions_setSilent(DownloadOptions *options, bool enabled) {
    if (options) (*options).silent = enabled;
}
static inline void DownloadOptions_setUserAgent(DownloadOptions *options, const char *agent) {
    if (options) (*options).user_agent = agent;
}

typedef struct DownloadResponse {
    int exit_code;
    uint32_t http_status;
    char *data;
    size_t size;
} DownloadResponse;

static inline DownloadResponse DownloadResponse_0(void) { return (DownloadResponse) {0}; }
#define DownloadResponse(...) CONSTRUCTOR_DISPATCH(DownloadResponse, __VA_ARGS__)

// Download URL to an output file path on disk.
bool Download_to_file(const char *url, const char *output_file, const DownloadOptions *opts);

// Download URL directly to memory buffer (caller frees with DownloadResponse_free).
bool Download_to_memory(const char *url, const DownloadOptions *opts, DownloadResponse *response_out);

void DownloadResponse_free(DownloadResponse *res);

// Fetch URL through the cache system: checks cache first, and if missing,
// downloads and caches it under <subsystem>. Populates cached_path_out.
bool Download_fetch_cached(
    const char *url,
    const char *subsystem,
    uint64_t ttl_sec,
    const DownloadOptions *opts,
    char *cached_path_out,
    size_t path_cap
);

#ifdef __cplusplus
}
#endif

#endif // NET_DOWNLOAD_H
