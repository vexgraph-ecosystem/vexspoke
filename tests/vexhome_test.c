// tests/vexhome_test.c — verify VexHome cache factory and layout.
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "io/vexhome.h"
#include "io/file.h"

static int g_failures = 0;
#define CHECK(cond) do{ if(!(cond)){ printf("FAIL %s:%d %s\n",__FILE__,__LINE__,#cond); g_failures++; } }while(0)

static void testVexHomeCache(void) {
    // Create a temporary directory for HOME
    char tmpdir[FILE_PATH_MAX];
    snprintf(tmpdir, sizeof(tmpdir), "/tmp/vexhome_test_%d", getpid());
    File_delete(tmpdir);
    CHECK(File_mkdirs(tmpdir));

    // Save original HOME
    const char *old_home = getenv("HOME");
    setenv("HOME", tmpdir, 1);

    // Test 1: cacheEnsure creates cache dir and dictionary.ini
    bool ok = VexHome_cacheEnsure("testsub");
    CHECK(ok);

    const char *cache_dir = VexHome_cache("testsub");
    CHECK(cache_dir != nullptr);
    CHECK(File_exists(cache_dir));
    CHECK(File_isDirectory(cache_dir));

    const char *index_path = VexHome_cacheIndex("testsub");
    CHECK(index_path != nullptr);
    CHECK(File_exists(index_path));

    // Verify dictionary.ini content
    File *f = File_open(index_path, FILE_MODE_READ);
    CHECK(f != nullptr);
    char buf[512];
    int64_t n = File_read(f, buf, sizeof(buf) - 1);
    CHECK(n > 0);
    buf[n] = '\0';
    File_close(f);

    CHECK(strstr(buf, "# vexgraph cache index") != nullptr);
    CHECK(strstr(buf, "subsystem = testsub") != nullptr);
    CHECK(strstr(buf, "version = 1") != nullptr);
    CHECK(strstr(buf, "created = ") != nullptr);

    // Test 2: Second cacheEnsure does not overwrite edited dictionary.ini
    // Edit the dictionary.ini
    f = File_open(index_path, FILE_MODE_WRITE | FILE_MODE_TRUNCATE);
    CHECK(f != nullptr);
    const char *edited = "# edited by test\n[cache]\nsubsystem = testsub\ncreated = 2024-01-01\nversion = 99\n";
    int64_t w = File_write(f, edited, (int64_t)strlen(edited));
    CHECK(w == (int64_t)strlen(edited));
    File_close(f);

    // Call cacheEnsure again - should NOT overwrite
    ok = VexHome_cacheEnsure("testsub");
    CHECK(ok);

    f = File_open(index_path, FILE_MODE_READ);
    CHECK(f != nullptr);
    n = File_read(f, buf, sizeof(buf) - 1);
    CHECK(n > 0);
    buf[n] = '\0';
    File_close(f);

    CHECK(strstr(buf, "# edited by test") != nullptr);
    CHECK(strstr(buf, "version = 99") != nullptr);
    CHECK(strstr(buf, "created = 2024-01-01") != nullptr);

    // Test 3: NULL/empty subsystem falls back to root cache dir
    const char *root_cache = VexHome_cache(NULL);
    CHECK(root_cache != nullptr);
    CHECK(strstr(root_cache, "/vex/cache") != nullptr);
    CHECK(strstr(root_cache, "/testsub") == nullptr);

    const char *empty_cache = VexHome_cache("");
    CHECK(empty_cache != nullptr);
    CHECK(strcmp(root_cache, empty_cache) == 0);

    const char *root_index = VexHome_cacheIndex(NULL);
    CHECK(root_index != nullptr);
    CHECK(strstr(root_index, "/vex/cache/dictionary.ini") != nullptr);

    // Test 4: VexHome_ensure creates cache dir too
    // Reset HOME to a fresh temp dir
    char tmpdir2[FILE_PATH_MAX];
    snprintf(tmpdir2, sizeof(tmpdir2), "/tmp/vexhome_test2_%d", getpid());
    File_delete(tmpdir2);
    CHECK(File_mkdirs(tmpdir2));
    setenv("HOME", tmpdir2, 1);

    ok = VexHome_ensure();
    CHECK(ok);

    const char *cache_after_ensure = VexHome_cache(NULL);
    CHECK(File_exists(cache_after_ensure));
    CHECK(File_isDirectory(cache_after_ensure));

    // Restore HOME
    if (old_home)
        setenv("HOME", old_home, 1);
    else
        unsetenv("HOME");

    // Cleanup
    File_delete(tmpdir);
    File_delete(tmpdir2);
}

int main(void) {
    testVexHomeCache();
    if (g_failures == 0)
        printf("vexhome_test: all checks passed\n");
    else
        printf("vexhome_test: %d FAILURES\n", g_failures);
    return g_failures == 0 ? 0 : 1;
}