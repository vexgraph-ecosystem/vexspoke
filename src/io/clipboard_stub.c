#include "io/clipboard.h"
#include <string.h>

#if !defined(__APPLE__)

static char s_stubText[4096] = {0};

bool Clipboard_hasText(void) {
    return s_stubText[0] != '\0';
}

bool Clipboard_hasImage(void) {
    return false;
}

size_t Clipboard_getText(char *dest, size_t maxBytes) {
    if (dest == nullptr || maxBytes == 0) return 0;
    size_t len = strlen(s_stubText);
    if (len >= maxBytes) len = maxBytes - 1;
    memcpy(dest, s_stubText, len);
    dest[len] = '\0';
    return len;
}

bool Clipboard_setText(const char *text) {
    if (text == nullptr) return false;
    size_t len = strlen(text);
    if (len >= sizeof(s_stubText)) len = sizeof(s_stubText) - 1;
    memcpy(s_stubText, text, len);
    s_stubText[len] = '\0';
    return true;
}

bool Clipboard_getImage(void *dest, size_t maxBytes, int32_t *outWidth, int32_t *outHeight) {
    (void) dest; (void) maxBytes; (void) outWidth; (void) outHeight;
    return false;
}

bool Clipboard_setImage(const void *pixels, int32_t width, int32_t height) {
    (void) pixels; (void) width; (void) height;
    return false;
}

void Clipboard_clear(void) {
    s_stubText[0] = '\0';
}

#endif
