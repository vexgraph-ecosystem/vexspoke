// reactive/reactive_tmpl.inc — the typed-reactive DEFINITION template.
//
// TEMPLATE, NOT A SOURCE: included once per instantiation's own `.c`. The caller
// defines VEX_NAME, VEX_T, VEX_TYPE_ID, VEX_TO_WORD, VEX_FROM_WORD, and VEX_ZERO
// before the include and undefs them after.
//
//   VEX_TO_WORD(v)   packs the typed value into the engine's word
//                    (the value itself for integers; a bit-cast for floats, so
//                    every bit pattern round-trips; the pointer for pointer types).
//   VEX_FROM_WORD(w) unpacks it back.
//   VEX_ZERO         the additive identity for VEX_T (the Construction and arity clause).
//
// The engine contract is unchanged: _set is atomic and never fires; the owner
// drains. _free tears the engine down before releasing the block. The typed
// channels are trampolines over the engine's uniform channels (reactive/generic.h).

VEX_NAME *VEX_CAT(VEX_NAME, _1)(VEX_T initial) {
    VEX_NAME *self = (VEX_NAME*) Memory_alloc(VEX_TYPE_ID, sizeof(VEX_NAME));
    if (self == nullptr)
        return nullptr;
    Reactive_init(&(*self).base, VEX_TO_WORD(initial));
    return self;
}

VEX_NAME *VEX_CAT(VEX_NAME, _0)(void) {
    return VEX_CAT(VEX_NAME, _1)(VEX_ZERO);
}

void VEX_CAT(VEX_NAME, _free)(VEX_NAME *self) {
    if (self == nullptr)
        return;
    Reactive_shutdown(&(*self).base);
    Memory_free(self);
}

void VEX_CAT(VEX_NAME, _set)(VEX_NAME *self, VEX_T value) {
    if (self == nullptr)
        return;
    Reactive_set(&(*self).base, VEX_TO_WORD(value));
}

VEX_T VEX_CAT(VEX_NAME, _get)(const VEX_NAME *self) {
    if (self == nullptr)
        return VEX_ZERO;
    return VEX_FROM_WORD(Reactive_get((Reactive*) &(*self).base));
}

// The four typed channels: trampolines over the engine's uniform channels.
VEX_CHANNELS_DEF(VEX_NAME, VEX_FROM_WORD);
