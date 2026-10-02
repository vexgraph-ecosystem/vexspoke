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
//   VEX_VALUE_OF(out, cap, v) formats v into out — the typed valueOf() the engine
//                    exposes for type-erased consumers (a Label [] slot).
//   VEX_ZERO         the additive identity for VEX_T (the Construction and arity clause).
//
// The engine contract is unchanged: _set is atomic; delivery is scheduled by the
// engine's hook (the owner drains). _free tears the engine down before releasing
// the block. The typed channels are trampolines over the engine's uniform channels.

// The typed stringifier: bound to the engine so Reactive_valueOf finds it.
static void VEX_CAT(VEX_NAME, _valueOf)(uintptr_t word, char *out, size_t cap) {
    VEX_T value = VEX_FROM_WORD(word);
    VEX_VALUE_OF(out, cap, value);
}

VEX_NAME *VEX_CAT(VEX_NAME, _1)(VEX_T initial) {
    VEX_NAME *self = (VEX_NAME*) Memory_alloc(VEX_TYPE_ID, sizeof(VEX_NAME));
    if (self == nullptr)
        return nullptr;
    Reactive_init(&(*self).base, VEX_TO_WORD(initial));
    Reactive_setValueOf(&(*self).base, VEX_CAT(VEX_NAME, _valueOf));
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
    Reactive_store(&(*self).base, VEX_TO_WORD(value));
}

VEX_T VEX_CAT(VEX_NAME, _get)(const VEX_NAME *self) {
    if (self == nullptr)
        return VEX_ZERO;
    return VEX_FROM_WORD(Reactive_load((Reactive*) &(*self).base));
}

// The four typed channels: trampolines over the engine's uniform channels.
VEX_CHANNELS_DEF(VEX_NAME, VEX_FROM_WORD);
