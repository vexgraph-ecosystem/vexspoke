#include "oop/type.h"
#include "annotation/overview.h"

;;OVERVIEW
/**
 * ============================================================================
 * CLASS: Type (oop/type.c)
 * LEVEL: L1 — File Metadata (type-id metadata registry)
 * ============================================================================
 * the TypeRegister, ported from oop/TypeRegister.java.
 *
 * STRUCT FIELDS (Mirroring oop/type.h):
 * ----------------------------------------------------------------------------
 *   TypeHeader {
 *     uint32_t typeId; // block-header type id
 *     uint32_t length; // payload length
 *   }
 *
 * FUNCTION REGISTRY:
 * ----------------------------------------------------------------------------
 * Core Functions:
 *   - Type_make(proj, form, classId)
 *   - Type_class(typeId)
 *   - Type_form(typeId)
 *
 * Getters:
 *   - Type_isStruct(form)
 *   - Type_isSingleton(typeId)
 *   - Type_isArray(typeId)
 *   - Type_isPointer(typeId)
 *   - Type_isStructSingleton(typeId)
 *   - Type_isStructArray(typeId)
 *   - Type_isStructSOA(typeId)
 *   - Type_isStructAOS(typeId)
 *   - Type_isStructCoexistent(typeId)
 *   - Type_isStructPointer(typeId)
 *   - Type_isPrimitive(typeId)
 *   - Type_isGlobal(typeId)
 *   - Type_isLocale(typeId)
 *   - Type_isTransient(typeId)
 *   - Type_isProactive(typeId)
 *   - Type_isReactive(typeId)
 *   - Type_isProbable(typeId)
 *   - Type_isProbableObjects(typeId)
 *   - Type_isFuture(typeId)
 *   - Type_isChoice(typeId)
 *   - Type_getParentClass(classId)
 *   - Type_isA(classId, ancestorId)
 *   - Type_arch(classId)
 * ============================================================================
 */


// type.c — TypeRegister port (Legacy: oop/TypeRegister.java).
//
// The class table lives in type.h as macros; this file carries only the two
// parent-chain helpers that need real code.

uint64_t Type_getParentClass(uint64_t classId) {
    uint64_t cls = classId & MASK_CLASS;
    // Buffer family: 0x50..0x63 in legacy all descend from ID_BUFFER.
    if (cls >= 0x0050u && cls <= 0x0063u)
        return ID_BUFFER;
    // Darling class space (0x0065-0x00FF, see darling/darling-type.h):
    // everything panels-up descends from Panel. Raw numbers mirror that
    // file — central logic may not include downstream ID headers.
    if (cls == 0x0079u)             // ID_CONTAINER
        return 0x0079u;             // root
    if (cls == 0x0065u)             // ID_CANVAS
        return 0x0065u;             // root
    if (cls == 0x00C1u)             // ID_RICHTEXT
        return 0x00C1u;             // root (text engine object, not a node)
    if (cls == 0x0078u)             // ID_PANEL
        return 0x0079u;             // ID_CONTAINER
    if (cls == 0x007Du || cls == 0x007Eu)  // ID_SCENE2D/ID_SCENE3D
        return 0x007Cu;             // ID_SCENE
    if (cls == 0x00A1u)             // ID_ALERTDIALOG
        return 0x00A0u;             // ID_DIALOG
    if (cls == 0x009Eu)             // ID_COLORDIALOG
        return 0x00A0u;             // ID_DIALOG
    if (cls >= 0x0065u && cls <= 0x00FFu)
        return 0x0078u;             // ID_PANEL
    return cls;
}

uint64_t Type_arch(uint64_t classId) {
    uint64_t proj = classId & MASK_PROJECT;
    if (proj == PROJ_VEXSPOKE)
        return ARCH_VEXSPOKE;
    if (proj == PROJ_GRAPHVEX)
        return ARCH_GRAPHVEX;
    if (proj == PROJ_HOTCWAP)
        return ARCH_HOTCWAP;
    if (proj == PROJ_DARLING)
        return ARCH_DARLING;
    if (proj == PROJ_APIHAVEN)
        return ARCH_APIHAVEN;
    // Bare ID_* constants carry no project byte: class-space ranges.
    // (Per-class ID_* constants live in their project's *-type.h, which
    // central logic may not include — ranges only here.)
    uint64_t cls = classId & MASK_CLASS;
    if (cls >= 0x0065u && cls <= 0x00FFu)
        return ARCH_DARLING;
    if (cls >= 0x0100u && cls <= 0x01FFu)
        return ARCH_GRAPHVEX;
    switch (cls) {
        case ID_THREAD:
        case ID_THREAD_NETWORKING:
        case ID_THREAD_EVENT:
        case ID_THREAD_DRAW:
        case ID_THREAD_SCRIPTING:
        case ID_THREAD_UI:
            return ARCH_HOTCWAP;
        default:
            return ARCH_VEXSPOKE;
    }
}

int Type_isA(uint64_t classId, uint64_t ancestorId) {
    uint64_t current = classId & MASK_CLASS;
    uint64_t target = ancestorId & MASK_CLASS;
    while (current != target) {
        uint64_t parent = Type_getParentClass(current);
        if (parent == current)
            return 0;
        current = parent;
    }
    return 1;
}
