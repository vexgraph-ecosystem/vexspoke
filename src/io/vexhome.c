#include "io/vexhome.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "io/file.h"
#include "annotation/overview.h"

;;OVERVIEW
/**
 * ============================================================================
 * CLASS: VexHome (io/vexhome.c)
 * LEVEL: L4 — Self-Management (Rule 28: owns the per-user filesystem layout)
 * ============================================================================
 * The VexHome class (was AntiHome, renamed on the vexspoke/darling split).
 * Manages the per-user engine home directory layout on disk.
 *
 * STRUCT FIELDS: none — procedural (operates on VexHome directory layout on disk)
 *
 * PRIVATE HELPERS:
 * ----------------------------------------------------------------------------
 *   static char root_buf[FILE_PATH_MAX];        // VexHome_root buffer
 *   static char projects_buf[FILE_PATH_MAX];    // VexHome_projects buffer
 *   static char logs_buf[FILE_PATH_MAX];        // VexHome_logs / defaultLogPath buffer
 *   static char fonts_buf[FILE_PATH_MAX];       // VexHome_fonts buffer
 *   static char placeholder_buf[FILE_PATH_MAX]; // VexHome_placeholder buffer
 *   static char cache_buf[FILE_PATH_MAX];       // VexHome_cache buffer
 *   static char cache_index_buf[FILE_PATH_MAX]; // VexHome_cacheIndex buffer
 *   static bool ensured;                        // VexHome_ensure idempotency flag
 *
 * FUNCTION REGISTRY:
 * ----------------------------------------------------------------------------
 * Core Functions:
 *   - VexHome_root(void)
 *   - VexHome_projects(void)
 *   - VexHome_logs(void)
 *   - VexHome_fonts(void)
 *   - VexHome_placeholder(void)
 *   - VexHome_cache(subsystem)
 *   - VexHome_cacheIndex(subsystem)
 *   - VexHome_cacheEnsure(subsystem)
 *   - VexHome_ensure(void)
 *   - VexHome_defaultLogPath(void)
 * ============================================================================
 */

// vexhome.c — VexHome port (was AntiHome; Legacy: io/AntiHome.java). ~/vex layout.

#define ROOT_NAME      "vex"
#define PROJECTS_NAME  "projects"
#define LOGS_NAME      "logs"
#define FONTS_NAME     "fonts"
#define PLACEHOLDER    "placeholder"
#define CACHE_NAME     "cache"
#define LOG_FILE       "engine.bin"
#define INDEX_FILE     "dictionary.ini"

static char root_buf[FILE_PATH_MAX];
static char projects_buf[FILE_PATH_MAX];
static char logs_buf[FILE_PATH_MAX];
static char fonts_buf[FILE_PATH_MAX];
static char placeholder_buf[FILE_PATH_MAX];
static char cache_buf[FILE_PATH_MAX];
static char cache_index_buf[FILE_PATH_MAX];
static bool ensured;

static void build_path(char *out, const char *sub) {
    const char *home = getenv("HOME");
    if (!home || *home == '\0')
        home = ".";
    if (*sub == '\0')
        snprintf(out, FILE_PATH_MAX, "%s/%s", home, ROOT_NAME);
    else
        snprintf(out, FILE_PATH_MAX, "%s/%s/%s", home, ROOT_NAME, sub);
}

static void build_cache_path(char *out, const char *subsystem) {
    const char *home = getenv("HOME");
    if (!home || *home == '\0')
        home = ".";
    if (!subsystem || *subsystem == '\0')
        snprintf(out, FILE_PATH_MAX, "%s/%s/%s", home, ROOT_NAME, CACHE_NAME);
    else
        snprintf(out, FILE_PATH_MAX, "%s/%s/%s/%s", home, ROOT_NAME, CACHE_NAME, subsystem);
}

const char *VexHome_root(void) {
    build_path(root_buf, "");
    return root_buf;
}

const char *VexHome_projects(void) {
    build_path(projects_buf, PROJECTS_NAME);
    return projects_buf;
}

const char *VexHome_logs(void) {
    build_path(logs_buf, LOGS_NAME);
    return logs_buf;
}

const char *VexHome_fonts(void) {
    build_path(fonts_buf, FONTS_NAME);
    return fonts_buf;
}

const char *VexHome_placeholder(void) {
    build_path(placeholder_buf, PLACEHOLDER);
    return placeholder_buf;
}

const char *VexHome_cache(const char *subsystem) {
    build_cache_path(cache_buf, subsystem);
    return cache_buf;
}

const char *VexHome_cacheIndex(const char *subsystem) {
    const char *cache_dir = VexHome_cache(subsystem);
    snprintf(cache_index_buf, FILE_PATH_MAX, "%s/%s", cache_dir, INDEX_FILE);
    return cache_index_buf;
}

bool VexHome_cacheEnsure(const char *subsystem) {
    const char *cache_dir = VexHome_cache(subsystem);
    if (!File_mkdirs(cache_dir))
        return false;

    const char *index_path = VexHome_cacheIndex(subsystem);
    if (File_exists(index_path))
        return true;

    File *f = File_open(index_path, FILE_MODE_WRITE | FILE_MODE_CREATE | FILE_MODE_TRUNCATE);
    if (!f)
        return false;

    time_t now = time(NULL);
    struct tm tm_buf;
    localtime_r(&now, &tm_buf);
    char date_buf[32];
    strftime(date_buf, sizeof(date_buf), "%Y-%m-%d", &tm_buf);

    const char *sub = subsystem ? subsystem : "";
    char content[256];
    snprintf(content, sizeof(content),
             "# vexgraph cache index — created by VexHome_cacheEnsure\n"
             "[cache]\n"
             "subsystem = %s\n"
             "created = %s\n"
             "version = 1\n",
             sub, date_buf);

    int64_t written = File_write(f, content, (int64_t)strlen(content));
    bool ok = (written == (int64_t)strlen(content));
    File_close(f);
    return ok;
}

bool VexHome_ensure(void) {
    if (ensured)
        return true;
    if (!File_mkdirs(VexHome_root()))
        return false;
    if (!File_mkdirs(VexHome_projects()))
        return false;
    if (!File_mkdirs(VexHome_logs()))
        return false;
    if (!File_mkdirs(VexHome_fonts()))
        return false;
    if (!File_mkdirs(VexHome_placeholder()))
        return false;
    if (!File_mkdirs(VexHome_cache(NULL)))
        return false;
    ensured = true;
    return true;
}

const char *VexHome_defaultLogPath(void) {
    VexHome_ensure();
    snprintf(logs_buf, FILE_PATH_MAX, "%s/%s", VexHome_logs(), LOG_FILE);
    return logs_buf;
}
