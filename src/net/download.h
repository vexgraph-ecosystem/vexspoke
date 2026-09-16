#ifndef NET_DOWNLOAD_H
#define NET_DOWNLOAD_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct DownloadOptions {
    uint32_t timeout_seconds;
    bool follow_redirects;
    bool silent;
    const char *user_agent;
} DownloadOptions;

typedef struct DownloadResponse {
    int exit_code;
    uint32_t http_status;
    char *data;
    size_t size;
} DownloadResponse;

DownloadOptions DownloadOptions_default(void);

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
