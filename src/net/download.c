#include "net/download.h"
#include "io/cache.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <spawn.h>
#include <sys/wait.h>

#include "annotation/overview.h"
#include "annotation/intention.h"

extern char **environ;

;;OVERVIEW
/**
 * ============================================================================
 * MODULE: Download (net/download.c)
 * LEVEL: L4 — External Systems (Curl / Package Download Integration)
 * ============================================================================
 * Safe download bridge utilizing system curl to retrieve packages (Homebrew, assets)
 * to disk or in-memory buffers.
 *
 * STRUCT FIELDS:
 *   - DownloadOptions: timeout_seconds, follow_redirects, silent, user_agent
 *   - DownloadResponse: exit_code, http_status, data, size
 *
 * FUNCTION REGISTRY:
 * ----------------------------------------------------------------------------
 * Options & Lifecycle:
 *   - DownloadOptions_default()
 *   - DownloadResponse_free(resources)
 * Execution:
 *   - Download_to_file(url, output_file, opts)
 *   - Download_to_memory(url, opts, response_out)
 * ============================================================================
 */

;;INTENTION("Reliable download manager leveraging system curl and HTTP/TLS stacks")

DownloadOptions DownloadOptions_default(void) {
    DownloadOptions opts;
    opts.timeout_seconds = 30;
    opts.follow_redirects = true;
    opts.silent = true;
    opts.user_agent = "vexgraph/1.0 (Homebrew/Tooling Client)";
    return opts;
}

void DownloadResponse_free(DownloadResponse *res) {
    if (!res) {
        return;
    }
    if ((*res).data) {
        free((*res).data);
        (*res).data = NULL;
    }
    (*res).size = 0;
    (*res).http_status = 0;
    (*res).exit_code = 0;
}

bool Download_to_file(const char *url, const char *output_file, const DownloadOptions *opts) {
    if (!url || !output_file) {
        return false;
    }

    DownloadOptions local_opts = opts ? (*opts) : DownloadOptions_default();

    char timeout_str[16];
    snprintf(timeout_str, sizeof(timeout_str), "%u", local_opts.timeout_seconds);

    const char *argv[16];
    int argc = 0;
    argv[argc++] = "curl";
    argv[argc++] = "-f"; // Fail fast on HTTP errors (>=400)
    if (local_opts.silent) {
        argv[argc++] = "-s";
    }
    if (local_opts.follow_redirects) {
        argv[argc++] = "-L";
    }
    argv[argc++] = "--max-time";
    argv[argc++] = timeout_str;
    if (local_opts.user_agent) {
        argv[argc++] = "-A";
        argv[argc++] = local_opts.user_agent;
    }
    argv[argc++] = "-o";
    argv[argc++] = output_file;
    argv[argc++] = url;
    argv[argc] = NULL;

    pid_t pid = 0;
    int status = posix_spawnp(&pid, "curl", NULL, NULL, (char *const *) argv, environ);
    if (status != 0) {
        return false;
    }

    int wait_status = 0;
    if (waitpid(pid, &wait_status, 0) < 0) {
        return false;
    }

    if (WIFEXITED(wait_status) && WEXITSTATUS(wait_status) == 0) {
        return true;
    }

    return false;
}

bool Download_to_memory(const char *url, const DownloadOptions *opts, DownloadResponse *response_out) {
    if (!url || !response_out) {
        return false;
    }

    (*response_out).exit_code = -1;
    (*response_out).http_status = 0;
    (*response_out).data = NULL;
    (*response_out).size = 0;

    char temp_path[] = "/tmp/vex_dl_XXXXXX";
    int fd = mkstemp(temp_path);
    if (fd < 0) {
        return false;
    }
    close(fd);

    bool ok = Download_to_file(url, temp_path, opts);
    if (!ok) {
        unlink(temp_path);
        return false;
    }

    FILE *f = fopen(temp_path, "rb");
    if (!f) {
        unlink(temp_path);
        return false;
    }

    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (sz < 0) {
        fclose(f);
        unlink(temp_path);
        return false;
    }

    char *buf = (char*) malloc((size_t) sz + 1);
    if (!buf) {
        fclose(f);
        unlink(temp_path);
        return false;
    }

    size_t read_bytes = fread(buf, 1, (size_t) sz, f);
    buf[read_bytes] = '\0';
    fclose(f);
    unlink(temp_path);

    (*response_out).exit_code = 0;
    (*response_out).http_status = 200;
    (*response_out).data = buf;
    (*response_out).size = read_bytes;

    return true;
}

bool Download_fetch_cached(
    const char *url,
    const char *subsystem,
    uint64_t ttl_sec,
    const DownloadOptions *opts,
    char *cached_path_out,
    size_t path_cap
) {
    if (!url || !cached_path_out || path_cap == 0) {
        return false;
    }

    Cache *cache = NULL;
    if (!Cache_open(subsystem, &cache)) {
        return false;
    }

    if (Cache_has(cache, url)) {
        bool ok = Cache_get_path(cache, url, cached_path_out, path_cap);
        Cache_close(cache);
        return ok;
    }

    char temp_download[] = "/tmp/vex_cachedl_XXXXXX";
    int fd = mkstemp(temp_download);
    if (fd < 0) {
        Cache_close(cache);
        return false;
    }
    close(fd);

    if (!Download_to_file(url, temp_download, opts)) {
        unlink(temp_download);
        Cache_close(cache);
        return false;
    }

    bool stored = Cache_put_file(cache, url, temp_download, ttl_sec);
    unlink(temp_download);

    if (!stored) {
        Cache_close(cache);
        return false;
    }

    bool ok = Cache_get_path(cache, url, cached_path_out, path_cap);
    Cache_close(cache);
    return ok;
}
