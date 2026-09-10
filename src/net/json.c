// net/json.c — off-heap JSON documents (Legacy: net/JSON.java port).
//
// A single-pass recursive-descent parser over the caller's text. Depth is
// bounded by JSON_MAX_DEPTH; every pool exhaustion or syntax error flips
// (*doc).ok once and the parse unwinds. Escaped strings decode forward into
// the scratch arena so value views stay pointer-stable.

#include "net/json.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "annotation/overview.h"

;;OVERVIEW
/**
 * ============================================================================
 * CLASS: Json (net/json.c)
 * LEVEL: L2 — Behavior (JSON document behavior API)
 * ============================================================================
 * off-heap JSON documents (Legacy: net/JSON.java).
 *
 * STRUCT FIELDS (Mirroring net/json.h + local to this file):
 * ----------------------------------------------------------------------------
 *   JsonNode {
 *     JsonType type; // node type tag
 *     JsonRef child; // first element/member
 *     JsonRef next; // next sibling
 *     uint32_t offset; // string/key start (in src, or scratch when inScratch)
 *     uint32_t length; // string/key length (decoded)
 *     bool inScratch; // true when offset points into the decode arena
 *     double number; // JSON_NUMBER payload
 *   }
 *   JsonDoc {
 *     JsonNode *nodes; // caller-owned pool
 *     uint32_t nodeCap; // node pool state
 *     uint32_t nodeCount; // live entry count
 *     const char *src; // input text (must outlive all string views)
 *     char *scratch; // caller-owned: decoded escape copies
 *     uint32_t scratchCap; // input/decode buffer state
 *     uint32_t scratchUsed; // input/decode buffer state
 *     JsonRef root; // root node ref
 *     bool ok; // parse result
 *   }
 *   Parser {
 *     JsonDoc *doc;              // destination document being built
 *     const char *p;             // input cursor
 *   }
 *
 * FUNCTION REGISTRY:
 * ----------------------------------------------------------------------------
 * Core Functions:
 *   - Json_parse(doc, nodes, nodeCap, scratch, scratchCap, text)
 *   - Json_root(doc)
 *   - Json_ok(doc)
 *   - Json_member(doc, obj, key)
 *   - Json_at(doc, node, index)
 *   - Json_count(doc, node)
 *   - Json_type(doc, ref)
 *   - Json_bool(doc, ref)
 *   - Json_number(doc, ref)
 *   - Json_string(doc, ref, lengthOut)
 *   - Json_key(doc, ref, lengthOut)
 *   - Json_has(doc, obj, key)
 *   - Json_path(doc, root, path)
 *   - Json_getBool(doc, ref, def)
 *   - Json_getNumber(doc, ref, def)
 *   - Json_getString(doc, ref, dest)
 *   - Json_writeNumber(out, cap, v)
 *   - Json_writeString(out, cap, s)
 *   - Json_writeBool(out, cap, v)
 *   - JsonWriter_init(w, out, cap) + JsonWriter_ok/used
 *   - Json_beginObject/endObject, Json_beginArray/endArray
 *   - Json_addKey(w, k) + Json_stringVal/numberVal/boolVal/nullVal/rawVal
 * ============================================================================
 */


typedef struct Parser {
    JsonDoc *doc;
    const char *p;   // cursor
} Parser;

static JsonRef allocNode(Parser *ps) {
    JsonDoc *d = (*ps).doc;
    if ((*d).nodeCount >= (*d).nodeCap) return -1;
    JsonNode *n = &(*d).nodes[(*d).nodeCount];
    memset(n, 0, sizeof(*n));
    // Zero is a VALID ref (the root); terminals must be none-refs.
    (*n).child = -1;
    (*n).next = -1;
    return (JsonRef)(*d).nodeCount++;
}

static void skipWs(Parser *ps) {
    while (*(*ps).p == ' ' || *(*ps).p == '\t' || *(*ps).p == '\n' || *(*ps).p == '\r')
        (*ps).p++;
}

static bool fail(JsonDoc *d) {
    (*d).ok = false;
    return false;
}

// --- Strings ---

// Decode a JSON string body starting after the opening quote. Unescaped
// input is viewed in place; any escape forces a scratch copy of the whole
// string so the returned view is always contiguous.
static bool parseStringBody(Parser *ps, uint32_t *offOut, uint32_t *lenOut, bool *scratchFlagOut) {
    JsonDoc *d = (*ps).doc;
    const char *start = (*ps).p;

    // Fast path: scan for the closing quote with no escapes.
    const char *q = start;
    while (*q && *q != '"' && *q != '\\') q++;
    if (*q == '"') {
        // No escapes: the view points straight into the caller's text.
        *offOut = (uint32_t)(start - (*d).src);
        *lenOut = (uint32_t)(q - start);
        *scratchFlagOut = false;
        (*ps).p = q + 1;
        return true;
    }
    if (!*q) return fail(d);

    // Escape present: copy-decode into scratch.
    char *dst = (*d).scratch + (*d).scratchUsed;
    size_t capLeft = (*d).scratchCap - (*d).scratchUsed;
    char *out = dst;
    (*ps).p = start;
    while (*(*ps).p && *(*ps).p != '"') {
        char c = *(*ps).p;
        if (c == '\\') {
            (*ps).p++; // step onto the escape designator
            switch (*(*ps).p) {
                case '"': c = '"'; (*ps).p++; break;
                case '\\': c = '\\'; (*ps).p++; break;
                case '/': c = '/'; (*ps).p++; break;
                case 'b': c = '\b'; (*ps).p++; break;
                case 'f': c = '\f'; (*ps).p++; break;
                case 'n': c = '\n'; (*ps).p++; break;
                case 'r': c = '\r'; (*ps).p++; break;
                case 't': c = '\t'; (*ps).p++; break;
                case 'u': {
                    // \uXXXX => UTF-8 (BMP only)
                    unsigned int cp = 0;
                    for (int i = 0; i < 4; i++) {
                        (*ps).p++;
                        char h = *(*ps).p;
                        cp <<= 4;
                        if (h >= '0' && h <= '9') cp |= (unsigned)(h - '0');
                        else if (h >= 'a' && h <= 'f') cp |= (unsigned)(h - 'a' + 10);
                        else if (h >= 'A' && h <= 'F') cp |= (unsigned)(h - 'A' + 10);
                        else return fail(d);
                    }
                    (*ps).p++; // past the last hex digit
                    if (cp < 0x80) {
                        if ((size_t)(out - dst) + 1 > capLeft) return fail(d);
                        *out++ = (char)cp;
                    } else if (cp < 0x800) {
                        if ((size_t)(out - dst) + 2 > capLeft) return fail(d);
                        *out++ = (char)(0xC0 | (cp >> 6));
                        *out++ = (char)(0x80 | (cp & 0x3F));
                    } else {
                        if ((size_t)(out - dst) + 3 > capLeft) return fail(d);
                        *out++ = (char)(0xE0 | (cp >> 12));
                        *out++ = (char)(0x80 | ((cp >> 6) & 0x3F));
                        *out++ = (char)(0x80 | (cp & 0x3F));
                    }
                    continue;
                }
                default: return fail(d);
            }
        } else {
            (*ps).p++; // consume the plain character
        }
        if ((size_t)(out - dst) + 1 > capLeft) return fail(d);
        *out++ = c;
    }
    if (*(*ps).p != '"') return fail(d);
    (*ps).p++; // closing quote

    *offOut = (uint32_t)(dst - (*d).scratch);
    *lenOut = (uint32_t)(out - dst);
    *scratchFlagOut = true;
    (*d).scratchUsed += (uint32_t)(out - dst) + 1; // keep NUL space per string
    if ((*d).scratchUsed < (*d).scratchCap) (*d).scratch[(*d).scratchUsed - 1] = '\0';
    return true;
}

// --- Values ---

static bool parseValue(Parser *ps, JsonRef *out, int depth);

static bool parseObject(Parser *ps, JsonRef *out, int depth) {
    JsonDoc *d = (*ps).doc;
    JsonRef obj = allocNode(ps);
    if (obj < 0) return fail(d);
    (*d).nodes[obj].type = JSON_OBJECT;
    (*ps).p++; // '{'
    skipWs(ps);

    JsonRef tail = -1;
    if (*(*ps).p == '}') { (*ps).p++; *out = obj; return true; }

    while (true) {
        skipWs(ps);
        if (*(*ps).p != '"') return fail(d);
        (*ps).p++;
        uint32_t kOff, kLen;
        bool kScratch;
        if (!parseStringBody(ps, &kOff, &kLen, &kScratch)) return fail(d);

        skipWs(ps);
        if (*(*ps).p != ':') return fail(d);
        (*ps).p++;

        JsonRef member = allocNode(ps);
        if (member < 0) return fail(d);
        (*d).nodes[member].offset = kOff;
        (*d).nodes[member].length = kLen;
        (*d).nodes[member].inScratch = kScratch;
        if (!parseValue(ps, &(*d).nodes[member].child, depth + 1)) return fail(d);

        if (tail < 0) (*d).nodes[obj].child = member;
        else (*d).nodes[tail].next = member;
        tail = member;

        skipWs(ps);
        if (*(*ps).p == ',') { (*ps).p++; continue; }
        if (*(*ps).p == '}') { (*ps).p++; break; }
        return fail(d);
    }
    *out = obj;
    return true;
}

static bool parseArray(Parser *ps, JsonRef *out, int depth) {
    JsonDoc *d = (*ps).doc;
    JsonRef arr = allocNode(ps);
    if (arr < 0) return fail(d);
    (*d).nodes[arr].type = JSON_ARRAY;
    (*ps).p++; // '['
    skipWs(ps);

    JsonRef tail = -1;
    if (*(*ps).p == ']') { (*ps).p++; *out = arr; return true; }

    while (true) {
        JsonRef element = allocNode(ps);
        if (element < 0) return fail(d);
        if (!parseValue(ps, &(*d).nodes[element].child, depth + 1)) return fail(d);

        if (tail < 0) (*d).nodes[arr].child = element;
        else (*d).nodes[tail].next = element;
        tail = element;

        skipWs(ps);
        if (*(*ps).p == ',') { (*ps).p++; continue; }
        if (*(*ps).p == ']') { (*ps).p++; break; }
        return fail(d);
    }
    *out = arr;
    return true;
}

static bool parseValue(Parser *ps, JsonRef *out, int depth) {
    JsonDoc *d = (*ps).doc;
    if (depth > JSON_MAX_DEPTH) return fail(d);
    skipWs(ps);

    char c = *(*ps).p;
    if (c == '{') return parseObject(ps, out, depth);
    if (c == '[') return parseArray(ps, out, depth);

    if (c == '"') {
        (*ps).p++;
        uint32_t off, len;
        bool scratch;
        if (!parseStringBody(ps, &off, &len, &scratch)) return fail(d);
        JsonRef s = allocNode(ps);
        if (s < 0) return fail(d);
        (*d).nodes[s].type = JSON_STRING;
        (*d).nodes[s].offset = off;
        (*d).nodes[s].length = len;
        (*d).nodes[s].inScratch = scratch;
        *out = s;
        return true;
    }

    if (c == 't') {
        if (strncmp((*ps).p, "true", 4) != 0) return fail(d);
        (*ps).p += 4;
        JsonRef v = allocNode(ps);
        if (v < 0) return fail(d);
        (*d).nodes[v].type = JSON_TRUE;
        *out = v;
        return true;
    }
    if (c == 'f') {
        if (strncmp((*ps).p, "false", 5) != 0) return fail(d);
        (*ps).p += 5;
        JsonRef v = allocNode(ps);
        if (v < 0) return fail(d);
        (*d).nodes[v].type = JSON_FALSE;
        *out = v;
        return true;
    }
    if (c == 'n') {
        if (strncmp((*ps).p, "null", 4) != 0) return fail(d);
        (*ps).p += 4;
        JsonRef v = allocNode(ps);
        if (v < 0) return fail(d);
        (*d).nodes[v].type = JSON_NULL;
        *out = v;
        return true;
    }

    // Number: strtod handles sign/exponent/fraction in one bite.
    char *end = nullptr;
    double num = strtod((*ps).p, &end);
    if (end == (*ps).p) return fail(d);
    JsonRef v = allocNode(ps);
    if (v < 0) return fail(d);
    (*d).nodes[v].type = JSON_NUMBER;
    (*d).nodes[v].number = num;
    (*ps).p = end;
    *out = v;
    return true;
}

bool Json_parse(JsonDoc *doc, JsonNode *nodes, uint32_t nodeCap,
                char *scratch, uint32_t scratchCap, const char *text) {
    (*doc).nodes = nodes;
    (*doc).nodeCap = nodeCap;
    (*doc).nodeCount = 0;
    (*doc).src = text;
    (*doc).scratch = scratch;
    (*doc).scratchCap = scratchCap;
    (*doc).scratchUsed = 0;
    (*doc).root = -1;
    (*doc).ok = true;

    Parser ps = { .doc = doc, .p = text };
    JsonRef root = -1;
    if (!parseValue(&ps, &root, 0)) {
        (*doc).ok = false;
        return false;
    }
    skipWs(&ps);
    if (*ps.p != '\0')
        (*doc).ok = false; // trailing garbage
    (*doc).root = root;
    return (*doc).ok;
}

JsonRef Json_root(const JsonDoc *doc) {
    return (*doc).root;
}

bool Json_ok(const JsonDoc *doc) {
    return (*doc).ok;
}

JsonRef Json_member(const JsonDoc *doc, JsonRef obj, const char *key) {
    if (obj < 0 || (*doc).nodes[obj].type != JSON_OBJECT) return -1;
    size_t klen = strlen(key);
    for (JsonRef m = (*doc).nodes[obj].child; m >= 0; m = (*doc).nodes[m].next) {
        uint32_t len = 0;
        const char *base = Json_key(doc, m, &len);
        if (len == klen && memcmp(base, key, klen) == 0)
            return (*doc).nodes[m].child;
    }
    return -1;
}

JsonRef Json_at(const JsonDoc *doc, JsonRef node, uint32_t index) {
    if (node < 0) return -1;
    JsonType t = (*doc).nodes[node].type;
    if (t != JSON_ARRAY && t != JSON_OBJECT) return -1;
    uint32_t i = 0;
    for (JsonRef e = (*doc).nodes[node].child; e >= 0; e = (*doc).nodes[e].next, i++)
        if (i == index) return (*doc).nodes[e].child;
    return -1;
}

uint32_t Json_count(const JsonDoc *doc, JsonRef node) {
    if (node < 0) return 0;
    JsonType t = (*doc).nodes[node].type;
    if (t != JSON_ARRAY && t != JSON_OBJECT) return 0;
    uint32_t n = 0;
    for (JsonRef e = (*doc).nodes[node].child; e >= 0; e = (*doc).nodes[e].next) n++;
    return n;
}

JsonType Json_type(const JsonDoc *doc, JsonRef ref) {
    if (ref < 0) return JSON_NULL;
    return (*doc).nodes[ref].type;
}

bool Json_bool(const JsonDoc *doc, JsonRef ref) {
    if (ref < 0) return false;
    return (*doc).nodes[ref].type == JSON_TRUE;
}

double Json_number(const JsonDoc *doc, JsonRef ref) {
    if (ref < 0) return 0.0;
    return (*doc).nodes[ref].number;
}

const char *Json_string(const JsonDoc *doc, JsonRef ref, uint32_t *lengthOut) {
    if (lengthOut) *lengthOut = 0;
    if (ref < 0 || (*doc).nodes[ref].type != JSON_STRING) return "";
    if (lengthOut) *lengthOut = (*doc).nodes[ref].length;
    if ((*doc).nodes[ref].inScratch)
        return (*doc).scratch + (*doc).nodes[ref].offset;
    return (*doc).src + (*doc).nodes[ref].offset;
}

const char *Json_key(const JsonDoc *doc, JsonRef ref, uint32_t *lengthOut) {
    if (lengthOut) *lengthOut = 0;
    if (ref < 0) return "";
    if (lengthOut) *lengthOut = (*doc).nodes[ref].length;
    if ((*doc).nodes[ref].inScratch)
        return (*doc).scratch + (*doc).nodes[ref].offset;
    return (*doc).src + (*doc).nodes[ref].offset;
}

// --- Writer ---

int64_t Json_writeNumber(char *out, size_t cap, double v) {
    long long asInt = (long long)v;
    if ((double)asInt == v)
        return (int64_t)snprintf(out, cap, "%lld", asInt);
    return (int64_t)snprintf(out, cap, "%.17g", v);
}

int64_t Json_writeString(char *out, size_t cap, const char *s) {
    size_t used = 0;
    if (cap < 2) return -1;
    out[used++] = '"';
    for (const char *p = s; *p; p++) {
        char c = *p;
        const char *esc = nullptr;
        switch (c) {
            case '"': esc = "\\\""; break;
            case '\\': esc = "\\\\"; break;
            case '\n': esc = "\\n"; break;
            case '\r': esc = "\\r"; break;
            case '\t': esc = "\\t"; break;
            case '\b': esc = "\\b"; break;
            case '\f': esc = "\\f"; break;
            default: break;
        }
        if (esc) {
            size_t l = strlen(esc);
            if (used + l >= cap) return -1;
            memcpy(out + used, esc, l);
            used += l;
        } else {
            if (used + 1 >= cap) return -1;
            out[used++] = c;
        }
    }
    if (used + 2 >= cap) return -1;
    out[used++] = '"';
    out[used] = '\0';
    return (int64_t)used;
}

int64_t Json_writeBool(char *out, size_t cap, bool v) {
    return (int64_t)snprintf(out, cap, "%s", v ? "true" : "false");
}

// --- JS-flex helpers (defaults + paths, still zero-alloc) ---

bool Json_has(const JsonDoc *doc, JsonRef obj, const char *key) {
    if (!doc || obj < 0 || !key)
        return false;
    return Json_member(doc, obj, key) >= 0;
}

JsonRef Json_path(const JsonDoc *doc, JsonRef root, const char *path) {
    if (!doc || root < 0 || !path || (*path) == '\0')
        return -1;
    JsonRef cur = root;
    const char *p = path;
    char keyBuf[128];
    while (*p) {
        if ((*p) == '.') {
            p++;
            continue;
        }
        if ((*p) != '[') {
            size_t klen = 0;
            while (*p && (*p) != '.' && (*p) != '[') {
                if (klen + 1 < sizeof(keyBuf))
                    keyBuf[klen++] = *p;
                else
                    return -1;
                p++;
            }
            keyBuf[klen] = '\0';
            if (klen > 0) {
                if (cur < 0 || (*doc).nodes[cur].type != JSON_OBJECT)
                    return -1;
                cur = Json_member(doc, cur, keyBuf);
                if (cur < 0)
                    return -1;
            }
            continue;
        }
        p++;
        uint32_t idx = 0;
        bool anyDigit = false;
        while (*p >= '0' && (*p) <= '9') {
            anyDigit = true;
            idx = idx * 10 + (uint32_t)((*p) - '0');
            p++;
        }
        if (!anyDigit || (*p) != ']')
            return -1;
        p++;
        cur = Json_at(doc, cur, idx);
        if (cur < 0)
            return -1;
    }
    return cur;
}

bool Json_getBool(const JsonDoc *doc, JsonRef ref, bool def) {
    if (!doc || ref < 0)
        return def;
    JsonType t = (*doc).nodes[ref].type;
    if (t == JSON_TRUE)
        return true;
    if (t == JSON_FALSE)
        return false;
    return def;
}

double Json_getNumber(const JsonDoc *doc, JsonRef ref, double def) {
    if (!doc || ref < 0)
        return def;
    if ((*doc).nodes[ref].type != JSON_NUMBER)
        return def;
    return (*doc).nodes[ref].number;
}

bool Json_getString(const JsonDoc *doc, JsonRef ref, char *dest, size_t cap) {
    if (!doc || !dest || cap == 0)
        return false;
    if (ref < 0 || (*doc).nodes[ref].type != JSON_STRING)
        return false;
    uint32_t len = 0;
    const char *view = Json_string(doc, ref, &len);
    if ((size_t)len + 1 > cap)
        return false;
    memcpy(dest, view, len);
    dest[len] = '\0';
    return true;
}

// --- Streaming builder (~ JSON.stringify) ---

static bool writerFail(JsonWriter *w) {
    (*w).ok = false;
    return false;
}

static bool writerReserve(JsonWriter *w, size_t n) {
    if (!(*w).ok)
        return false;
    if (!(*w).out || (*w).used + n >= (*w).cap)
        return writerFail(w);
    return true;
}

static bool writerPut(JsonWriter *w, char c) {
    if (!writerReserve(w, 1))
        return false;
    (*w).out[(*w).used++] = c;
    (*w).out[(*w).used] = '\0';
    return true;
}

static bool writerAppend(JsonWriter *w, const char *s, size_t n) {
    if (n == 0)
        return true;
    if (!writerReserve(w, n))
        return false;
    memcpy((*w).out + (*w).used, s, n);
    (*w).used += n;
    (*w).out[(*w).used] = '\0';
    return true;
}

static bool writerStringBody(JsonWriter *w, const char *s) {
    for (const char *p = s; *p; p++) {
        char c = *p;
        const char *esc = NULL;
        switch (c) {
            case '"': esc = "\\\""; break;
            case '\\': esc = "\\\\"; break;
            case '\n': esc = "\\n"; break;
            case '\r': esc = "\\r"; break;
            case '\t': esc = "\\t"; break;
            case '\b': esc = "\\b"; break;
            case '\f': esc = "\\f"; break;
            default: break;
        }
        if (esc) {
            if (!writerAppend(w, esc, strlen(esc)))
                return false;
        } else {
            if (!writerPut(w, c))
                return false;
        }
    }
    return true;
}

void JsonWriter_init(JsonWriter *w, char *out, size_t cap) {
    if (!w)
        return;
    (*w).out = out;
    (*w).cap = cap;
    (*w).used = 0;
    (*w).ok = (out != NULL && cap > 0);
    (*w).depth = 0;
    for (int i = 0; i < JSON_MAX_DEPTH; i++)
        (*w).needComma[i] = false;
    if ((*w).ok)
        out[0] = '\0';
}

bool JsonWriter_ok(const JsonWriter *w) {
    if (!w)
        return false;
    return (*w).ok;
}

size_t JsonWriter_used(const JsonWriter *w) {
    if (!w)
        return 0;
    return (*w).used;
}

static bool writerValueOpen(JsonWriter *w) {
    if (!(*w).ok)
        return false;
    if ((*w).depth == 0)
        return true;
    if ((*w).depth < 0 || (*w).depth > JSON_MAX_DEPTH)
        return writerFail(w);
    int parent = (*w).depth - 1;
    if ((*w).needComma[parent]) {
        if (!writerPut(w, ','))
            return false;
        (*w).needComma[parent] = false;
    }
    return true;
}

static bool writerValueClose(JsonWriter *w) {
    if ((*w).depth == 0)
        return (*w).ok;
    int parent = (*w).depth - 1;
    (*w).needComma[parent] = true;
    return (*w).ok;
}

bool Json_beginObject(JsonWriter *w) {
    if (!w || !writerValueOpen(w))
        return false;
    if (!writerPut(w, '{'))
        return false;
    if ((*w).depth < 0 || (*w).depth >= JSON_MAX_DEPTH)
        return writerFail(w);
    (*w).needComma[(*w).depth] = false;
    (*w).depth++;
    return true;
}

bool Json_endObject(JsonWriter *w) {
    if (!w || !(*w).ok)
        return false;
    if ((*w).depth <= 0)
        return writerFail(w);
    if (!writerPut(w, '}'))
        return false;
    (*w).depth--;
    return writerValueClose(w);
}

bool Json_beginArray(JsonWriter *w) {
    if (!w || !writerValueOpen(w))
        return false;
    if (!writerPut(w, '['))
        return false;
    if ((*w).depth < 0 || (*w).depth >= JSON_MAX_DEPTH)
        return writerFail(w);
    (*w).needComma[(*w).depth] = false;
    (*w).depth++;
    return true;
}

bool Json_endArray(JsonWriter *w) {
    if (!w || !(*w).ok)
        return false;
    if ((*w).depth <= 0)
        return writerFail(w);
    if (!writerPut(w, ']'))
        return false;
    (*w).depth--;
    return writerValueClose(w);
}

bool Json_addKey(JsonWriter *w, const char *k) {
    if (!w || !k || !(*w).ok)
        return false;
    if ((*w).depth <= 0)
        return writerFail(w);
    int idx = (*w).depth - 1;
    if ((*w).needComma[idx]) {
        if (!writerPut(w, ','))
            return false;
    }
    if (!writerPut(w, '"'))
        return false;
    if (!writerStringBody(w, k))
        return false;
    if (!writerAppend(w, "\":", 2))
        return false;
    (*w).needComma[idx] = false;
    return true;
}

bool Json_stringVal(JsonWriter *w, const char *s) {
    if (!w || !s || !writerValueOpen(w))
        return false;
    if (!writerPut(w, '"'))
        return false;
    if (!writerStringBody(w, s))
        return false;
    if (!writerPut(w, '"'))
        return false;
    return writerValueClose(w);
}

bool Json_numberVal(JsonWriter *w, double v) {
    if (!w || !writerValueOpen(w))
        return false;
    char tmp[32];
    long long asInt = (long long)v;
    int n = 0;
    if ((double)asInt == v)
        n = snprintf(tmp, sizeof(tmp), "%lld", asInt);
    else
        n = snprintf(tmp, sizeof(tmp), "%.17g", v);
    if (n <= 0 || (size_t)n >= sizeof(tmp))
        return writerFail(w);
    if (!writerAppend(w, tmp, (size_t)n))
        return false;
    return writerValueClose(w);
}

bool Json_boolVal(JsonWriter *w, bool v) {
    if (!w || !writerValueOpen(w))
        return false;
    if (!writerAppend(w, v ? "true" : "false", v ? 4 : 5))
        return false;
    return writerValueClose(w);
}

bool Json_nullVal(JsonWriter *w) {
    if (!w || !writerValueOpen(w))
        return false;
    if (!writerAppend(w, "null", 4))
        return false;
    return writerValueClose(w);
}

bool Json_rawVal(JsonWriter *w, const char *raw) {
    if (!w || !raw || !writerValueOpen(w))
        return false;
    if (!writerAppend(w, raw, strlen(raw)))
        return false;
    return writerValueClose(w);
}
