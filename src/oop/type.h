#ifndef OOP_TYPE_H
#define OOP_TYPE_H

#include <stdint.h>

// oop/type.h — the TypeRegister, ported from oop/TypeRegister.java.
//
// Every allocated object in vexspoke carries a 64-bit type id in its
// header. The id is bit-packed thus:
//
//     0x F PRPR M W1 W2 PDPD CCCCCCCC
//        | |    | |  |  |    `-------- class        (32 bits: which struct this is, per-project)
//        | |    | |  |  |
//        | |    | |  |  `------------- padding      (8 bits, reserved 0)
//        | |    | |  `---------------- wrapper 2    (probable/future/choice)
//        | |    | `------------------- wrapper 1    (proactive/reactive)
//        | |    `---------------------- modifier     (global/locale/transient)
//        | `--------------------------- project      (8 bits: owning repo;
//        |                                            64+ projects per stack)
//        `------------------------------ form         (singleton/array/...,
//                                                      struct layouts)
//
// Written with C23 digit separators grouping each field, e.g.
// FORM_SINGLETON is 0x1'00'0'0'0'00'00000000ULL. Reading the fields lets
// code decide *shape* without a switch: is it an array? a struct? The
// class id then says *what kind*, the project byte says *whose*. This
// is what lets one allocator serve every type across the whole stack
// (see nio/mem.h).
//
// Previous layout was 32-bit (0x F M W1 W2 CCCC); the project byte and
// the 32-bit class space are new. Bare ID_* constants carry no project
// bits — Type_arch falls back to the legacy class-range table for them,
// so old call sites keep working while TYPE_* macros carry explicit
// PROJ_* bits.

#define MASK_FORM       0xF'00'0'0'0'00'00000000ULL
#define MASK_PROJECT    0x0'FF'0'0'0'00'00000000ULL
#define MASK_MODIFIER   0x0'00'F'0'0'00'00000000ULL
#define MASK_WRAPPER_1  0x0'00'0'F'0'00'00000000ULL
#define MASK_WRAPPER_2  0x0'00'0'0'F'00'00000000ULL
#define MASK_PAD        0x0'00'0'0'0'FF'00000000ULL
#define MASK_CLASS      0x0'00'0'0'0'00'FFFFFFFFULL

#define FORM_SINGLETON          0x1'00'0'0'0'00'00000000ULL
#define FORM_ARRAY              0x2'00'0'0'0'00'00000000ULL
#define FORM_POINTER            0x3'00'0'0'0'00'00000000ULL
#define FORM_STRUCT_SINGLETON   0x4'00'0'0'0'00'00000000ULL
#define FORM_STRUCT_ARRAY       0x5'00'0'0'0'00'00000000ULL
#define FORM_STRUCT_POINTER     0x6'00'0'0'0'00'00000000ULL
#define FORM_ARRAY_SOA          0x7'00'0'0'0'00'00000000ULL
#define FORM_ARRAY_AOS          0x8'00'0'0'0'00'00000000ULL
#define FORM_STRUCT_COEXISTENT  0x9'00'0'0'0'00'00000000ULL

// Project byte: which repo owns the class. Matches ARCH_* numbering;
// 256 projects fit the same stack (relentless dogfooding).
#define PROJ_GENERIC    0x0'00'0'0'0'00'00000000ULL  // zero lol
#define PROJ_VEXSPOKE   0x0'01'0'0'0'00'00000000ULL
#define PROJ_GRAPHVEX   0x0'02'0'0'0'00'00000000ULL
#define PROJ_HOTCWAP    0x0'03'0'0'0'00'00000000ULL
#define PROJ_DARLING    0x0'04'0'0'0'00'00000000ULL
#define PROJ_API_HAVEN  0x0'05'0'0'0'00'00000000ULL

#define MOD_GLOBAL     0x0'00'1'0'0'00'00000000ULL
#define MOD_LOCALE     0x0'00'2'0'0'00'00000000ULL
#define MOD_TRANSIENT  0x0'00'3'0'0'00'00000000ULL

#define WRAP_PROACTIVE  0x0'00'0'1'0'00'00000000ULL
#define WRAP_REACTIVE   0x0'00'0'2'0'00'00000000ULL

#define WRAP2_PROBABLE          0x0'00'0'0'1'00'00000000ULL
#define WRAP2_PROBABLE_OBJECTS  0x0'00'0'0'2'00'00000000ULL
#define WRAP2_FUTURE            0x0'00'0'0'3'00'00000000ULL
#define WRAP2_CHOICE            0x0'00'0'0'4'00'00000000ULL

#define ID_INT          0x0001u
#define ID_LONG         0x0002u
#define ID_DOUBLE       0x0003u
#define ID_FIXED64      0x0004u
#define ID_FLOAT        0x0005u
#define ID_SHORT        0x0006u
#define ID_SPINLOCK     0x0007u
#define ID_RING_BUFFER  0x0008u
#define ID_BIT8         0x0009u
#define ID_BIT16        0x000Au
#define ID_BIT32        0x000Bu
#define ID_BIT64        0x000Cu
#define ID_VEC2         0x000Du
#define ID_VEC3         0x000Eu
#define ID_ENTITY       0x000Fu

// --- MATH / GEOMETRY CLASSES (added in the C port) ---
#define ID_VEC4        0x0029u
#define ID_MAT3        0x002Au
#define ID_MAT4        0x002Bu
#define ID_QUATERNION  0x002Cu

// --- SCALAR / WRAPPER CLASSES (Legacy TypeRegister raw IDs) ---
#define ID_BYTE         0x0010u
#define ID_STRING       0x0011u
#define ID_INT_FLOAT    0x0012u
#define ID_INT_DOUBLE   0x0013u
#define ID_LONG_FLOAT   0x0014u
#define ID_LONG_DOUBLE  0x0015u
#define ID_VARIABLE     0x0016u
#define ID_PACK         0x0017u
#define ID_ARRAYS       0x0018u
#define ID_HASH         0x0019u
#define ID_CLASS        0x001Au
#define ID_STRIDE       0x001Bu
#define ID_FILE         0x0030u
#define ID_BOOL         0x0031u
#define ID_BRAIN        0x0032u
#define ID_FIXED32      0x0033u
#define ID_COMMAND      0x0064u

// --- COLLECTION CLASSES ---
#define ID_LIST              0x001Cu
#define ID_MAP               0x001Du
#define ID_SET               0x001Eu
#define ID_STACK             0x001Fu
#define ID_DEQUE             0x0020u
#define ID_MIN_HEAP          0x0021u
#define ID_SPARSE_SET        0x0022u
#define ID_RANDOM            0x0023u
#define ID_INDEX_RANDOM      0x0024u
#define ID_PROBABLE          0x0025u
#define ID_PROBABLE_OBJECTS  0x0026u
#define ID_QUEUE             0x0027u
#define ID_ARGUMENTS         0x0028u

// --- OBJECT WRAPPERS & MODIFIERS (Legacy 0x006F..0x0075) ---
#define ID_CHOICE     0x006Fu
#define ID_FUTURE     0x0070u
#define ID_PASSIVE    0x0071u
#define ID_REACTIVE   0x0072u
#define ID_TRANSIENT  0x0073u
#define ID_LOCAL      0x0074u
#define ID_GLOBAL     0x0075u

// Base ID for runtime-defined custom structs (Legacy CUSTOM_STRUCT).
// Custom structs are CUSTOM_STRUCT + n; the Struct registry owns the stride.
#define ID_CUSTOM_STRUCT	0x4000u

// --- BUFFER FAMILY (Legacy 0x4A / 0x50..0x63) ---
#define ID_BUFFER                  0x004Au
#define ID_ACCUMULUATION_BUFFER    0x0050u
#define ID_AMBIENT_BUFFER          0x0051u
#define ID_COLOR_BUFFER            0x0052u
#define ID_DEFAULT_PIXEL_BUFFER    0x0053u
#define ID_DEPTH_BUFFER            0x0054u
#define ID_FILTER_BUFFER           0x0055u
#define ID_FRAME_BUFFER            0x0056u
#define ID_HEIGHT_BUFFER           0x0057u
#define ID_LIGHT_BUFFER            0x0058u
#define ID_MATERIAL_RESOLVE        0x0059u
#define ID_MOTION_VECTOR_BUFFER    0x005Au
#define ID_NORMAL_BUFFER           0x005Bu
#define ID_PHYSICAL_BUFFER         0x005Cu
#define ID_POST_PROCESSING_BUFFER  0x005Du
#define ID_REFLECTIVITY_BUFFER     0x005Eu
#define ID_SHADOW_BUFFER           0x005Fu
#define ID_SPECULAR_BUFFER         0x0060u
#define ID_STENCIL_BUFFER          0x0061u
#define ID_TRANSPARENCY_BUFFER     0x0062u
#define ID_VISIBILITY_BUFFER       0x0063u

// --- SYSTEM CLASSES ---
#define ID_DISPLAY_MONITOR              0x0066u
#define TYPE_DISPLAY_MONITOR_SINGLETON  (PROJ_VEXSPOKE | FORM_SINGLETON | ID_DISPLAY_MONITOR)

// --- THREAD CLASSES (the real OS threads; sync primitives live in atomic/) ---
#define ID_THREAD             0x0080u
#define ID_THREAD_NETWORKING  0x0081u
#define ID_THREAD_EVENT       0x0082u
#define ID_THREAD_DRAW        0x0083u
#define ID_THREAD_SCRIPTING   0x0084u
#define ID_THREAD_UI          0x0085u

#define TYPE_THREAD_SINGLETON             (PROJ_HOTCWAP | FORM_SINGLETON | ID_THREAD)
#define TYPE_THREAD_NETWORKING_SINGLETON  (PROJ_HOTCWAP | FORM_SINGLETON | ID_THREAD_NETWORKING)
#define TYPE_THREAD_EVENT_SINGLETON       (PROJ_HOTCWAP | FORM_SINGLETON | ID_THREAD_EVENT)
#define TYPE_THREAD_DRAW_SINGLETON        (PROJ_HOTCWAP | FORM_SINGLETON | ID_THREAD_DRAW)
#define TYPE_THREAD_SCRIPTING_SINGLETON   (PROJ_HOTCWAP | FORM_SINGLETON | ID_THREAD_SCRIPTING)
#define TYPE_THREAD_UI_SINGLETON          (PROJ_HOTCWAP | FORM_SINGLETON | ID_THREAD_UI)

// --- AUDIO CLASSES (native playback seam) ---
#define ID_AUDIO_CLIP   0x0086u
#define ID_AUDIO_VOICE  0x0087u
#define ID_AUDIO        0x0088u

#define TYPE_AUDIO_CLIP_SINGLETON   (PROJ_VEXSPOKE | FORM_SINGLETON | ID_AUDIO_CLIP)
#define TYPE_AUDIO_VOICE_SINGLETON  (PROJ_VEXSPOKE | FORM_SINGLETON | ID_AUDIO_VOICE)
#define TYPE_AUDIO_SINGLETON        (PROJ_VEXSPOKE | FORM_SINGLETON | ID_AUDIO)

// --- DOWNSTREAM CLASS SPACE (owned per project, NOT listed here) ---
// Darling classes (0x0065-0x00FF) live in darling/darling-type.h;
// graphvex classes (0x0100-0x01FF) live in graphvex/src/graphvex/type.h;
// hotcwap module constants live in hotcwap/hotcwap-type.h. This file
// keeps vexspoke-owned IDs only. Rule 17: central logic below dispatches
// on project byte + class ranges and must never include downstream ID
// files (upstream builds standalone). New projects claim a class range
// here and ship their own *-type.h. Bare-ID fallback ranges live in
// Type_arch; parent defaults live in Type_getParentClass.

// --- ARCHITECTURE LAYER (which repo owns the class) ---
// Primary key is the project byte (PROJ_* above): one macro, no table.
// Bare ID_* constants carry no project byte, so Type_arch falls back to
// class ranges (vexspoke 0x0001-0x0064, darling 0x0065-0x00FF, threads
// by exact id). Range fallback exists for compat only — new code ships
// full TYPE_* ids.
#define ARCH_VEXSPOKE  1u
#define ARCH_HOTCWAP   2u
#define ARCH_DARLING   3u
#define ARCH_GRAPHVEX  4u
#define ARCH_APIHAVEN  5u

#define TYPE_INT_SINGLETON  (PROJ_VEXSPOKE | FORM_SINGLETON | ID_INT)
#define TYPE_INT_ARRAY      (PROJ_VEXSPOKE | FORM_ARRAY     | ID_INT)
#define TYPE_INT_POINTER    (PROJ_VEXSPOKE | FORM_POINTER   | ID_INT)
#define TYPE_LONG_ARRAY     (PROJ_VEXSPOKE | FORM_ARRAY     | ID_LONG)
#define TYPE_FLOAT_ARRAY    (PROJ_VEXSPOKE | FORM_ARRAY     | ID_FLOAT)
#define TYPE_DOUBLE_ARRAY   (PROJ_VEXSPOKE | FORM_ARRAY     | ID_DOUBLE)
#define TYPE_BYTE_ARRAY     (PROJ_VEXSPOKE | FORM_ARRAY     | ID_BYTE)
#define TYPE_STRING_ARRAY   (PROJ_VEXSPOKE | FORM_ARRAY     | ID_STRING)

#define TYPE_SPIN_LOCK               (PROJ_VEXSPOKE | FORM_SINGLETON | ID_SPINLOCK)
#define TYPE_RING_BUFFER             (PROJ_VEXSPOKE | FORM_ARRAY     | ID_RING_BUFFER)
#define TYPE_LIST                    (PROJ_VEXSPOKE | FORM_ARRAY     | ID_LIST)
#define TYPE_ARRAY                   (PROJ_VEXSPOKE | FORM_ARRAY     | ID_ARRAYS)
#define TYPE_VEC2_SINGLETON          (PROJ_VEXSPOKE | FORM_SINGLETON | ID_VEC2)
#define TYPE_VEC3_SINGLETON          (PROJ_VEXSPOKE | FORM_SINGLETON | ID_VEC3)
#define TYPE_VEC4_SINGLETON          (PROJ_VEXSPOKE | FORM_SINGLETON | ID_VEC4)
#define TYPE_MAT3_SINGLETON          (PROJ_VEXSPOKE | FORM_SINGLETON | ID_MAT3)
#define TYPE_MAT4_SINGLETON          (PROJ_VEXSPOKE | FORM_SINGLETON | ID_MAT4)
#define TYPE_QUATERNION_SINGLETON    (PROJ_VEXSPOKE | FORM_SINGLETON | ID_QUATERNION)
#define TYPE_STACK                   (PROJ_VEXSPOKE | FORM_ARRAY     | ID_STACK)
#define TYPE_DEQUE                   (PROJ_VEXSPOKE | FORM_ARRAY     | ID_DEQUE)
#define TYPE_QUEUE                   (PROJ_VEXSPOKE | FORM_ARRAY     | ID_QUEUE)
#define TYPE_MAP                     (PROJ_VEXSPOKE | FORM_POINTER   | ID_MAP)
#define TYPE_SET                     (PROJ_VEXSPOKE | FORM_POINTER   | ID_SET)
#define TYPE_MIN_HEAP                (PROJ_VEXSPOKE | FORM_SINGLETON | ID_MIN_HEAP)
#define TYPE_RANDOM                  (PROJ_VEXSPOKE | FORM_SINGLETON | ID_RANDOM)
#define TYPE_PROBABLE                (PROJ_VEXSPOKE | FORM_SINGLETON | WRAP2_PROBABLE | ID_PROBABLE)
#define TYPE_PROBABLE_ARRAY          (PROJ_VEXSPOKE | FORM_ARRAY     | WRAP2_PROBABLE | ID_PROBABLE)
#define TYPE_PROBABLE_OBJECTS_ARRAY  (PROJ_VEXSPOKE | FORM_ARRAY | WRAP2_PROBABLE_OBJECTS | ID_PROBABLE_OBJECTS)
#define TYPE_CHOICE                  (PROJ_VEXSPOKE | FORM_SINGLETON | WRAP2_CHOICE | ID_CHOICE)
#define TYPE_CHOICE_ARRAY            (PROJ_VEXSPOKE | FORM_ARRAY | WRAP2_CHOICE | ID_CHOICE)
#define TYPE_FUTURE                  (PROJ_VEXSPOKE | FORM_SINGLETON | WRAP2_FUTURE | ID_FUTURE)
#define TYPE_FUTURE_ARRAY            (PROJ_VEXSPOKE | FORM_ARRAY | WRAP2_FUTURE | ID_FUTURE)
#define TYPE_GLOBAL                  (PROJ_VEXSPOKE | FORM_SINGLETON | MOD_GLOBAL | ID_GLOBAL)
#define TYPE_GLOBAL_ARRAY            (PROJ_VEXSPOKE | FORM_ARRAY | MOD_GLOBAL | ID_GLOBAL)
#define TYPE_LOCAL                   (PROJ_VEXSPOKE | FORM_SINGLETON | MOD_LOCALE | ID_LOCAL)
#define TYPE_LOCAL_ARRAY             (PROJ_VEXSPOKE | FORM_ARRAY | MOD_LOCALE | ID_LOCAL)
#define TYPE_PASSIVE                 (PROJ_VEXSPOKE | FORM_SINGLETON | WRAP_PROACTIVE | ID_PASSIVE)
#define TYPE_PASSIVE_ARRAY           (PROJ_VEXSPOKE | FORM_ARRAY | WRAP_PROACTIVE | ID_PASSIVE)
#define TYPE_REACTIVE                (PROJ_VEXSPOKE | FORM_SINGLETON | WRAP_REACTIVE | ID_REACTIVE)
#define TYPE_REACTIVE_ARRAY          (PROJ_VEXSPOKE | FORM_ARRAY | WRAP_REACTIVE | ID_REACTIVE)
#define TYPE_PROBABLE_OBJECTS        (PROJ_VEXSPOKE | FORM_SINGLETON | WRAP2_PROBABLE_OBJECTS | ID_PROBABLE_OBJECTS)
#define TYPE_FILE_SINGLETON          (PROJ_VEXSPOKE | FORM_SINGLETON | ID_FILE)
#define TYPE_COMMAND_SINGLETON       (PROJ_VEXSPOKE | FORM_SINGLETON | ID_COMMAND)

#define TYPE_INT_SINGLETON  (PROJ_VEXSPOKE | FORM_SINGLETON | ID_INT)
#define TYPE_INT_ARRAY      (PROJ_VEXSPOKE | FORM_ARRAY     | ID_INT)
#define TYPE_INT_POINTER    (PROJ_VEXSPOKE | FORM_POINTER   | ID_INT)

#define TYPE_SPIN_LOCK    (PROJ_VEXSPOKE | FORM_SINGLETON | ID_SPINLOCK)
#define TYPE_RING_BUFFER  (PROJ_VEXSPOKE | FORM_ARRAY     | ID_RING_BUFFER)

// The header prefixing every allocated block: [typeId][length].
// 16 bytes keeps payloads 8-byte aligned, so doubles/pointers sit naturally.
typedef struct TypeHeader {
    uint64_t typeId;
    uint32_t length;
    uint32_t pad;
} TypeHeader;

_Static_assert(sizeof(TypeHeader) == 16, "TypeHeader must stay 16 bytes");

// Compose a full type id from project + form + class id. Project owns
// the high byte, shape the top nibble, identity the low 32 bits.
static inline uint64_t Type_make(uint64_t proj, uint64_t form, uint32_t classId) {
    return (proj & MASK_PROJECT) | (form & MASK_FORM) | (classId & MASK_CLASS);
}

static inline uint64_t Type_class(uint64_t typeId) {
    return typeId & MASK_CLASS;
}

static inline uint64_t Type_form(uint64_t typeId) {
    return typeId & MASK_FORM;
}

static inline uint64_t Type_project(uint64_t typeId) {
    return typeId & MASK_PROJECT;
}

static inline int Type_isStruct(uint64_t form) {
    return form == FORM_STRUCT_SINGLETON || form == FORM_STRUCT_ARRAY
        || form == FORM_STRUCT_POINTER;
}

static inline int Type_isSingleton(uint64_t typeId) {
    return (typeId & MASK_FORM) == FORM_SINGLETON;
}

static inline int Type_isArray(uint64_t typeId) {
    uint64_t form = typeId & MASK_FORM;
    return form == FORM_ARRAY || form == FORM_ARRAY_SOA || form == FORM_ARRAY_AOS || form == FORM_STRUCT_COEXISTENT;
}

static inline int Type_isPointer(uint64_t typeId) {
    return (typeId & MASK_FORM) == FORM_POINTER;
}

static inline int Type_isStructSingleton(uint64_t typeId) {
    return (typeId & MASK_FORM) == FORM_STRUCT_SINGLETON;
}

static inline int Type_isStructArray(uint64_t typeId) {
    uint64_t form = typeId & MASK_FORM;
    return form == FORM_STRUCT_ARRAY || form == FORM_ARRAY_SOA || form == FORM_ARRAY_AOS || form == FORM_STRUCT_COEXISTENT;
}

static inline int Type_isStructSOA(uint64_t typeId) {
    return (typeId & MASK_FORM) == FORM_ARRAY_SOA;
}

static inline int Type_isStructAOS(uint64_t typeId) {
    return (typeId & MASK_FORM) == FORM_ARRAY_AOS;
}

static inline int Type_isStructCoexistent(uint64_t typeId) {
    return (typeId & MASK_FORM) == FORM_STRUCT_COEXISTENT;
}

static inline int Type_isStructPointer(uint64_t typeId) {
    return (typeId & MASK_FORM) == FORM_STRUCT_POINTER;
}

static inline int Type_isPrimitive(uint64_t typeId) {
    uint64_t form = typeId & MASK_FORM;
    return form == FORM_SINGLETON || form == FORM_ARRAY || form == FORM_POINTER;
}

static inline int Type_isGlobal(uint64_t typeId) {
    return (typeId & MASK_MODIFIER) == MOD_GLOBAL;
}

static inline int Type_isLocale(uint64_t typeId) {
    return (typeId & MASK_MODIFIER) == MOD_LOCALE;
}

static inline int Type_isTransient(uint64_t typeId) {
    return (typeId & MASK_MODIFIER) == MOD_TRANSIENT;
}

static inline int Type_isProactive(uint64_t typeId) {
    return (typeId & MASK_WRAPPER_1) == WRAP_PROACTIVE;
}

static inline int Type_isReactive(uint64_t typeId) {
    return (typeId & MASK_WRAPPER_1) == WRAP_REACTIVE;
}

static inline int Type_isProbable(uint64_t typeId) {
    return (typeId & MASK_WRAPPER_2) == WRAP2_PROBABLE;
}

static inline int Type_isProbableObjects(uint64_t typeId) {
    return (typeId & MASK_WRAPPER_2) == WRAP2_PROBABLE_OBJECTS;
}

static inline int Type_isFuture(uint64_t typeId) {
    return (typeId & MASK_WRAPPER_2) == WRAP2_FUTURE;
}

static inline int Type_isChoice(uint64_t typeId) {
    return (typeId & MASK_WRAPPER_2) == WRAP2_CHOICE;
}

// Parent-class walk (Legacy getParentClass). Takes a full id or a bare
// class id (masks to class first); returns the parent class id, or the
// class id itself when it is a root. Used by Type_isA.
uint64_t Type_getParentClass(uint64_t classId);

// Architecture layer owning an id (ARCH_* — one per project byte).
// Reads the project byte when present; falls back to the legacy
// class-range table for bare ID_* constants (project byte zero).
// Used by Darling_addAny.
uint64_t Type_arch(uint64_t classId);

// True if classId belongs to the given architecture layer.
static inline int Type_isVexspoke(uint64_t classId) {
    return Type_arch(classId) == ARCH_VEXSPOKE;
}

static inline int Type_isHotcwap(uint64_t classId) {
    return Type_arch(classId) == ARCH_HOTCWAP;
}

static inline int Type_isDarling(uint64_t classId) {
    return Type_arch(classId) == ARCH_DARLING;
}

// True if classId is ancestorId or any descendant of it (walks parents;
// both sides masked to class first so full and bare ids mix freely).
int Type_isA(uint64_t classId, uint64_t ancestorId);

#endif
