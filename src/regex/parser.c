// regex/parser.c — the RegexParser: pattern bytes to a flat AST plus bitmaps.

#include "regex/parser.h"

#include <string.h>

#include "annotation/definition.h"
#include "annotation/overview.h"
#include "nio/mem.h"
#include "oop/type.h"

;;DEFINITION
/**
 * ============================================================================
 * DEFINITION: RegexParser
 * ============================================================================
 * A recursive-descent parser that turns pattern bytes into the flat AST in
 * regex/nodelist.h plus the character-class bitmaps in regex/charclass.h. It
 * exists as its own unit because parsing is where every hostile input arrives:
 * an unbalanced bracket, a malformed quantifier, an inverted range, an
 * unsupported group — all of them are decided here, once, before any program
 * is emitted. Each rejection is recorded as one reason string and the parse
 * stops producing anything usable (the Cold-Strict, Hot-Minimal Validation
 * Law's cold half: validate exhaustively where input enters).
 *
 * Precedence, lowest to highest: alternation, concatenation, repetition, atom.
 * Concatenation and alternation are built by LOOPS (left-nested), so only
 * group nesting recurses — and that depth is capped at REGEX_MAX_DEPTH, which
 * keeps the C stack bounded on a pathological pattern like '((((...'.
 *
 * memory: owns the node list and a doubling class-bitmap array, both in the
 * engine arena stamped TYPE_REGEX. Case folding is resolved here from
 * ignoreCase and baked into the nodes and bitmaps, so nothing downstream ever
 * folds. Lifetime: one parser per compile call; the caller takes the classes
 * and frees the rest.
 * ============================================================================
 */

;;OVERVIEW
/**
 * ============================================================================
 * CLASS: RegexParser (regex/parser.c — defined in regex/parser.h)
 * ============================================================================
 * Pattern bytes -> AST nodes + class bitmaps, with one recorded reason per
 * rejection.
 *
 * STRUCT FIELDS (Mirroring regex/parser.h):
 * ----------------------------------------------------------------------------
 *   RegexParser {
 *     const char     *pattern;       // borrowed pattern text (not owned)
 *     size_t          length;        // pattern byte length
 *     size_t          pos;           // parse cursor
 *     bool            ignoreCase;    // fold ASCII case while building the AST
 *     RegexNodeList   nodes;         // the AST being built (owned)
 *     RegexCharClass *classes;       // growable class bitmaps (owned)
 *     uint32_t        classCount;    // classes in use
 *     uint32_t        classCapacity; // classes allocated
 *     uint32_t        groupCount;    // capturing groups seen
 *     uint32_t        depth;         // current group nesting depth
 *     bool            failed;        // a rejection was recorded
 *     const char     *error;         // first rejection reason (static text)
 *   }
 *
 * FUNCTION REGISTRY:
 * ----------------------------------------------------------------------------
 * Public Core Functions: (.h)
 *   - RegexParser(pattern, length, ignoreCase)
 *   - RegexParser_parse(parser)
 *   - RegexParser_ok(parser)
 *   - RegexParser_error(parser)
 *   - RegexParser_nodes(parser)
 *   - RegexParser_groupCount(parser)
 *   - RegexParser_releaseClasses(parser, outCount)
 *   - RegexParser_free(parser)
 *
 * Private Core Functions: (.c static)
 *   - lowerByte(c) / isAsciiDigit(c) / parserFail(parser, message)
 *   - parserNode(parser, kind) / parserAddClass(parser)
 *   - parseEscapeByte(parser, outByte)
 *   - parseGroup / parseClass / parseAtom / parseBraces / parseRepeat /
 *     parseConcat / parseAlternation
 *
 * Admission, ownership, failure:
 *   - init zeroes the parser and allocates nothing; a null pattern is the
 *     caller's misuse and outside this unit's contract.
 *   - parse returns the root index or REGEX_NODE_NONE; ok/error report the
 *     single recorded reason. Unsupported alphabetic escapes and stacked
 *     quantifiers reject instead of changing meaning; punctuation stays literal.
 *     A failed parse leaves nodes/classes allocated
 *     for free() and nothing usable for the caller.
 *   - releaseClasses transfers ownership once; a second call yields nullptr.
 *   - free releases the AST and any un-transferred class array, then zeroes
 *     the parser; null-safe.
 * ============================================================================
 */

// ASCII-only lowercase; deliberately not locale-sensitive (byte patterns).
static uint8_t lowerByte(uint8_t c) {
    if (c >= (uint8_t) 'A' && c <= (uint8_t) 'Z')
        return (uint8_t)(c + 32u);
    return c;
}

static bool isAsciiDigit(char c) {
    return c >= '0' && c <= '9';
}

// Records the first rejection reason; later ones are ignored.
static void parserFail(RegexParser *parser, const char *message) {
    if (!(*parser).failed) {
        (*parser).failed = true;
        (*parser).error = message;
    }
}

// Appends a zeroed AST node under the node ceiling.
static uint32_t parserNode(RegexParser *parser, uint8_t kind) {
    if ((*parser).nodes.count >= REGEX_MAX_NODES) {
        parserFail(parser, "pattern too complex (node ceiling)");
        return REGEX_NODE_NONE;
    }
    uint32_t index = RegexNodeList_add(&(*parser).nodes, kind);
    if (index == REGEX_NODE_NONE)
        parserFail(parser, "out of memory");
    return index;
}

// Adds one cleared class bitmap under the class ceiling; UINT32_MAX on refusal.
static uint32_t parserAddClass(RegexParser *parser) {
    if ((*parser).classCount >= REGEX_MAX_CLASSES) {
        parserFail(parser, "too many character classes");
        return UINT32_MAX;
    }
    if ((*parser).classCount == (*parser).classCapacity) {
        uint32_t next = (*parser).classCapacity ? (*parser).classCapacity * 2u : 8u;
        if (next > REGEX_MAX_CLASSES)
            next = REGEX_MAX_CLASSES;
        RegexCharClass *grown = (RegexCharClass*) Memory_realloc(
            (*parser).classes, (size_t) next * sizeof(RegexCharClass));
        if (!grown) {
            parserFail(parser, "out of memory");
            return UINT32_MAX;
        }
        (*parser).classes = grown;
        (*parser).classCapacity = next;
    }
    uint32_t index = (*parser).classCount++;
    RegexCharClass_clear(&(*parser).classes[index]);
    return index;
}

// Reads the escape body after a backslash and yields a literal byte. A
// predefined family is recognised by the caller before this runs.
static bool parseEscapeByte(RegexParser *parser, uint8_t *outByte) {
    if ((*parser).pos >= (*parser).length) {
        parserFail(parser, "trailing backslash");
        return false;
    }
    char c = (*parser).pattern[(*parser).pos++];
    switch (c) {
    case 'n': *outByte = (uint8_t) '\n'; return true;
    case 'r': *outByte = (uint8_t) '\r'; return true;
    case 't': *outByte = (uint8_t) '\t'; return true;
    case 'f': *outByte = (uint8_t) '\f'; return true;
    case 'v': *outByte = (uint8_t) '\v'; return true;
    case '0': *outByte = (uint8_t) '\0'; return true;
    case 'x': {
        if ((*parser).pos + 1u >= (*parser).length) {
            parserFail(parser, "malformed \\x escape");
            return false;
        }
        char h = (*parser).pattern[(*parser).pos];
        char l = (*parser).pattern[(*parser).pos + 1u];
        uint32_t hi, lo;
        if (h >= '0' && h <= '9') hi = (uint32_t)(h - '0');
        else if (h >= 'a' && h <= 'f') hi = (uint32_t)(h - 'a' + 10);
        else if (h >= 'A' && h <= 'F') hi = (uint32_t)(h - 'A' + 10);
        else { parserFail(parser, "malformed \\x escape"); return false; }
        if (l >= '0' && l <= '9') lo = (uint32_t)(l - '0');
        else if (l >= 'a' && l <= 'f') lo = (uint32_t)(l - 'a' + 10);
        else if (l >= 'A' && l <= 'F') lo = (uint32_t)(l - 'A' + 10);
        else { parserFail(parser, "malformed \\x escape"); return false; }
        (*parser).pos += 2u;
        *outByte = (uint8_t)((hi << 4) | lo);
        return true;
    }
    default:
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
            (c >= '1' && c <= '9')) {
            parserFail(parser, "unsupported escape");
            return false;
        }
        // Every other escaped byte is that literal byte (\\. \\* \\\\ ...).
        *outByte = (uint8_t) c;
        return true;
    }
}

static uint32_t parseAlternation(RegexParser *parser);

// Parses "(...)" or "(?:...)" into a group node.
static uint32_t parseGroup(RegexParser *parser) {
    (*parser).pos++; // consume '('
    bool capturing = true;
    if ((*parser).pos + 1u < (*parser).length && (*parser).pattern[(*parser).pos] == '?' &&
        (*parser).pattern[(*parser).pos + 1u] == ':') {
        capturing = false;
        (*parser).pos += 2u;
    } else if ((*parser).pos < (*parser).length && (*parser).pattern[(*parser).pos] == '?') {
        parserFail(parser, "unsupported group construct (only (?:...) is supported)");
        return REGEX_NODE_NONE;
    }
    uint32_t group = 0;
    if (capturing)
        group = ++(*parser).groupCount;
    if ((*parser).depth >= REGEX_MAX_DEPTH) {
        parserFail(parser, "pattern nesting too deep");
        return REGEX_NODE_NONE;
    }
    (*parser).depth++;
    uint32_t body = parseAlternation(parser);
    (*parser).depth--;
    if (body == REGEX_NODE_NONE)
        return REGEX_NODE_NONE;
    if ((*parser).pos >= (*parser).length || (*parser).pattern[(*parser).pos] != ')') {
        parserFail(parser, "missing closing ')'");
        return REGEX_NODE_NONE;
    }
    (*parser).pos++;
    if (!capturing)
        return body;
    uint32_t node = parserNode(parser, REGEX_NODE_GROUP);
    if (node == REGEX_NODE_NONE)
        return REGEX_NODE_NONE;
    RegexNode *group_node = RegexNodeList_get(&(*parser).nodes, node);
    (*group_node).a = body;
    (*group_node).group = group;
    return node;
}

// Parses "[...]" into a class bitmap index; UINT32_MAX on failure.
static uint32_t parseClass(RegexParser *parser) {
    (*parser).pos++; // consume '['
    bool negate = false;
    if ((*parser).pos < (*parser).length && (*parser).pattern[(*parser).pos] == '^') {
        negate = true;
        (*parser).pos++;
    }
    uint32_t index = parserAddClass(parser);
    if (index == UINT32_MAX)
        return UINT32_MAX;
    RegexCharClass *cls = &(*parser).classes[index];
    bool insensitive = (*parser).ignoreCase;
    bool first = true;
    bool closed = false;
    while ((*parser).pos < (*parser).length) {
        char c = (*parser).pattern[(*parser).pos];
        if (c == ']' && !first) {
            (*parser).pos++;
            closed = true;
            break;
        }
        first = false;
        uint8_t lo = 0;
        bool predefined = false;
        if (c == '\\') {
            (*parser).pos++;
            if ((*parser).pos < (*parser).length &&
                RegexCharClass_isPredefined((*parser).pattern[(*parser).pos])) {
                RegexCharClass_addPredefined(cls, (*parser).pattern[(*parser).pos]);
                (*parser).pos++;
                predefined = true;
            } else if (!parseEscapeByte(parser, &lo)) {
                return UINT32_MAX;
            }
        } else {
            lo = (uint8_t) c;
            (*parser).pos++;
        }
        if (predefined)
            continue;
        if ((*parser).pos + 1u < (*parser).length && (*parser).pattern[(*parser).pos] == '-' &&
            (*parser).pattern[(*parser).pos + 1u] != ']') {
            (*parser).pos++; // consume '-'
            uint8_t hi = 0;
            char hc = (*parser).pattern[(*parser).pos];
            if (hc == '\\') {
                (*parser).pos++;
                if ((*parser).pos < (*parser).length &&
                    RegexCharClass_isPredefined((*parser).pattern[(*parser).pos])) {
                    parserFail(parser, "character class in a range endpoint");
                    return UINT32_MAX;
                }
                if (!parseEscapeByte(parser, &hi))
                    return UINT32_MAX;
            } else {
                hi = (uint8_t) hc;
                (*parser).pos++;
            }
            if (hi < lo) {
                parserFail(parser, "invalid character range (end before start)");
                return UINT32_MAX;
            }
            RegexCharClass_addRange(cls, lo, hi, insensitive);
        } else {
            RegexCharClass_addByte(cls, lo, insensitive);
        }
    }
    if (!closed) {
        parserFail(parser, "missing closing ']'");
        return UINT32_MAX;
    }
    if (negate)
        RegexCharClass_negate(cls);
    return index;
}

// Parses one atom: a literal, '.', an anchor, a group, a class or an escape.
static uint32_t parseAtom(RegexParser *parser) {
    char c = (*parser).pattern[(*parser).pos];
    switch (c) {
    case '(':
        return parseGroup(parser);
    case '[': {
        uint32_t index = parseClass(parser);
        if (index == UINT32_MAX)
            return REGEX_NODE_NONE;
        uint32_t node = parserNode(parser, REGEX_NODE_CLASS);
        if (node == REGEX_NODE_NONE)
            return REGEX_NODE_NONE;
        RegexNode *cls_node = RegexNodeList_get(&(*parser).nodes, node);
        (*cls_node).clsIndex = index;
        return node;
    }
    case '.':
        (*parser).pos++;
        return parserNode(parser, REGEX_NODE_ANY);
    case '^':
        (*parser).pos++;
        return parserNode(parser, REGEX_NODE_BOL);
    case '$':
        (*parser).pos++;
        return parserNode(parser, REGEX_NODE_EOL);
    case '\\': {
        (*parser).pos++;
        if ((*parser).pos < (*parser).length &&
            RegexCharClass_isPredefined((*parser).pattern[(*parser).pos])) {
            uint32_t index = parserAddClass(parser);
            if (index == UINT32_MAX)
                return REGEX_NODE_NONE;
            RegexCharClass_addPredefined(&(*parser).classes[index],
                                         (*parser).pattern[(*parser).pos]);
            (*parser).pos++;
            uint32_t node = parserNode(parser, REGEX_NODE_CLASS);
            if (node == REGEX_NODE_NONE)
                return REGEX_NODE_NONE;
            RegexNode *cls_node = RegexNodeList_get(&(*parser).nodes, node);
            (*cls_node).clsIndex = index;
            return node;
        }
        uint8_t byte = 0;
        if (!parseEscapeByte(parser, &byte))
            return REGEX_NODE_NONE;
        uint32_t node = parserNode(parser, REGEX_NODE_CHAR);
        if (node == REGEX_NODE_NONE)
            return REGEX_NODE_NONE;
        RegexNode *char_node = RegexNodeList_get(&(*parser).nodes, node);
        (*char_node).byte = (*parser).ignoreCase ? lowerByte(byte) : byte;
        return node;
    }
    case '*':
    case '+':
    case '?':
        parserFail(parser, "nothing to repeat");
        return REGEX_NODE_NONE;
    default: {
        (*parser).pos++;
        uint32_t node = parserNode(parser, REGEX_NODE_CHAR);
        if (node == REGEX_NODE_NONE)
            return REGEX_NODE_NONE;
        RegexNode *char_node = RegexNodeList_get(&(*parser).nodes, node);
        uint8_t byte = (uint8_t) c;
        (*char_node).byte = (*parser).ignoreCase ? lowerByte(byte) : byte;
        return node;
    }
    }
}

// Parses "{m}", "{m,}" or "{m,n}". 1 = quantifier consumed, 0 = a literal
// '{', -1 = a malformed quantifier (already recorded).
static int parseBraces(RegexParser *parser, uint32_t *outMin, uint32_t *outMax, bool *outHasMax) {
    size_t i = (*parser).pos + 1u;
    if (i >= (*parser).length || !isAsciiDigit((*parser).pattern[i]))
        return 0;
    uint32_t lo = 0;
    while (i < (*parser).length && isAsciiDigit((*parser).pattern[i])) {
        lo = lo * 10u + (uint32_t)((*parser).pattern[i] - '0');
        if (lo > REGEX_MAX_REPEAT) {
            parserFail(parser, "repetition count too large");
            return -1;
        }
        i++;
    }
    bool hasMax = true;
    uint32_t hi = lo;
    if (i < (*parser).length && (*parser).pattern[i] == ',') {
        i++;
        if (i < (*parser).length && isAsciiDigit((*parser).pattern[i])) {
            hi = 0;
            while (i < (*parser).length && isAsciiDigit((*parser).pattern[i])) {
                hi = hi * 10u + (uint32_t)((*parser).pattern[i] - '0');
                if (hi > REGEX_MAX_REPEAT) {
                    parserFail(parser, "repetition count too large");
                    return -1;
                }
                i++;
            }
        } else {
            hasMax = false;
        }
    }
    if (i >= (*parser).length || (*parser).pattern[i] != '}') {
        parserFail(parser, "malformed repetition");
        return -1;
    }
    if (hasMax && hi < lo) {
        parserFail(parser, "invalid repetition range (end before start)");
        return -1;
    }
    (*parser).pos = i + 1u;
    *outMin = lo;
    *outMax = hasMax ? hi : REGEX_REP_INF;
    *outHasMax = hasMax;
    return 1;
}

// Parses an atom plus any trailing quantifiers into one node.
static uint32_t parseRepeat(RegexParser *parser) {
    uint32_t atom = parseAtom(parser);
    if (atom == REGEX_NODE_NONE)
        return REGEX_NODE_NONE;
    for (;;) {
        if ((*parser).pos >= (*parser).length)
            break;
        char c = (*parser).pattern[(*parser).pos];
        uint8_t wrapKind = 0;
        uint32_t min = 0, max = 0;
        bool hasMax = false;
        if (c == '*') {
            wrapKind = REGEX_NODE_STAR;
            (*parser).pos++;
        } else if (c == '+') {
            wrapKind = REGEX_NODE_PLUS;
            (*parser).pos++;
        } else if (c == '?') {
            wrapKind = REGEX_NODE_QUEST;
            (*parser).pos++;
        } else if (c == '{') {
            int braces = parseBraces(parser, &min, &max, &hasMax);
            if (braces < 0)
                return REGEX_NODE_NONE;
            if (braces == 0)
                break; // a literal '{' — leave it for the concatenation loop
            wrapKind = REGEX_NODE_REPEAT;
        } else {
            break;
        }
        uint32_t wrap = parserNode(parser, wrapKind);
        if (wrap == REGEX_NODE_NONE)
            return REGEX_NODE_NONE;
        RegexNode *n = RegexNodeList_get(&(*parser).nodes, wrap);
        (*n).a = atom;
        if (wrapKind == REGEX_NODE_REPEAT) {
            (*n).repMin = min;
            (*n).repMax = hasMax ? max : REGEX_REP_INF;
        }
        atom = wrap;
        if ((*parser).pos < (*parser).length) {
            char next = (*parser).pattern[(*parser).pos];
            if (next == '*' || next == '+' || next == '?' ||
                (next == '{' && (*parser).pos + 1u < (*parser).length &&
                 isAsciiDigit((*parser).pattern[(*parser).pos + 1u]))) {
                parserFail(parser, "stacked, lazy or possessive quantifier unsupported");
                return REGEX_NODE_NONE;
            }
        }
        break;
    }
    return atom;
}

// Parses a run of repeat atoms into a left-nested concatenation.
static uint32_t parseConcat(RegexParser *parser) {
    uint32_t head = REGEX_NODE_NONE;
    for (;;) {
        if ((*parser).pos >= (*parser).length)
            break;
        char c = (*parser).pattern[(*parser).pos];
        if (c == '|' || c == ')')
            break;
        uint32_t piece = parseRepeat(parser);
        if (piece == REGEX_NODE_NONE)
            return REGEX_NODE_NONE;
        if (head == REGEX_NODE_NONE) {
            head = piece;
            continue;
        }
        uint32_t cat = parserNode(parser, REGEX_NODE_CONCAT);
        if (cat == REGEX_NODE_NONE)
            return REGEX_NODE_NONE;
        RegexNode *n = RegexNodeList_get(&(*parser).nodes, cat);
        (*n).a = head;
        (*n).b = piece;
        head = cat;
    }
    if (head == REGEX_NODE_NONE)
        return parserNode(parser, REGEX_NODE_EMPTY);
    return head;
}

// Parses a run of concatenations separated by '|' into a left-nested alternation.
static uint32_t parseAlternation(RegexParser *parser) {
    uint32_t left = parseConcat(parser);
    if (left == REGEX_NODE_NONE)
        return REGEX_NODE_NONE;
    while ((*parser).pos < (*parser).length && (*parser).pattern[(*parser).pos] == '|') {
        (*parser).pos++;
        uint32_t right = parseConcat(parser);
        if (right == REGEX_NODE_NONE)
            return REGEX_NODE_NONE;
        uint32_t alt = parserNode(parser, REGEX_NODE_ALT);
        if (alt == REGEX_NODE_NONE)
            return REGEX_NODE_NONE;
        RegexNode *n = RegexNodeList_get(&(*parser).nodes, alt);
        (*n).a = left;
        (*n).b = right;
        left = alt;
    }
    return left;
}

RegexParser RegexParser_3(const char *pattern, size_t length, bool ignoreCase) {
    RegexParser parser;
    parser.pattern = pattern;
    parser.length = length;
    parser.pos = 0u;
    parser.ignoreCase = ignoreCase;
    parser.nodes = RegexNodeList();
    parser.classes = nullptr;
    parser.classCount = 0u;
    parser.classCapacity = 0u;
    parser.groupCount = 0u;
    parser.depth = 0u;
    parser.failed = false;
    parser.error = nullptr;
    return parser;
}

uint32_t RegexParser_parse(RegexParser *parser) {
    if ((*parser).length == 0)
        return parseAlternation(parser); // the empty pattern is the empty node
    uint32_t root = parseAlternation(parser);
    if (root == REGEX_NODE_NONE || (*parser).failed)
        return REGEX_NODE_NONE;
    if ((*parser).pos != (*parser).length) {
        parserFail(parser, "unexpected ')'");
        return REGEX_NODE_NONE;
    }
    return root;
}

bool RegexParser_ok(const RegexParser *parser) {
    return !(*parser).failed;
}

const char *RegexParser_error(const RegexParser *parser) {
    return (*parser).error;
}

RegexNodeList *RegexParser_nodes(RegexParser *parser) {
    return &(*parser).nodes;
}

uint32_t RegexParser_groupCount(const RegexParser *parser) {
    return (*parser).groupCount;
}

RegexCharClass *RegexParser_releaseClasses(RegexParser *parser, uint32_t *outCount) {
    RegexCharClass *classes = (*parser).classes;
    if (outCount)
        *outCount = (*parser).classCount;
    (*parser).classes = nullptr;
    (*parser).classCount = 0;
    (*parser).classCapacity = 0;
    return classes;
}

void RegexParser_free(RegexParser *parser) {
    RegexNodeList_free(&(*parser).nodes);
    if ((*parser).classes)
        Memory_free((*parser).classes);
    *parser = RegexParser(nullptr, 0u, false);
}
