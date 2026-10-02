// reactive/reactive_probable_tmpl.inc — the ReactiveProbable DEFINITION template.
//
// TEMPLATE, NOT A SOURCE: included once per instantiation's own `.c`. The caller
// defines VEX_NAME, VEX_T, VEX_TYPE_ID, VEX_TO_WORD, VEX_FROM_WORD, VEX_ZERO,
// and VEX_VALUE_OF (formats the value for the engine's valueOf).
//
// get() is the roll: with probability `chance` it yields the held value, else the
// empty value. _set publishes a new value+chance (atomic, never fires — the owner
// drains, same engine contract). The typed channels are the shared trampolines.

// The typed stringifier: bound to the engine so Reactive_valueOf finds it.
static void VEX_CAT(VEX_NAME, _valueOf)(uintptr_t word, char *out, size_t cap) {
    VEX_T value = VEX_FROM_WORD(word);
    VEX_VALUE_OF(out, cap, value);
}

VEX_NAME *VEX_CAT(VEX_NAME, _2)(VEX_T value, float chance) {
    VEX_NAME *self = (VEX_NAME*) Memory_alloc(VEX_TYPE_ID, sizeof(VEX_NAME));
    if (self == nullptr)
        return nullptr;
    Reactive_init(&(*self).base, VEX_TO_WORD(value));
    Reactive_setValueOf(&(*self).base, VEX_CAT(VEX_NAME, _valueOf));
    (*self).chance = chance;
    return self;
}

VEX_NAME *VEX_CAT(VEX_NAME, _0)(void) {
    return VEX_CAT(VEX_NAME, _2)(VEX_ZERO, 0.0f);
}

void VEX_CAT(VEX_NAME, _free)(VEX_NAME *self) {
    if (self == nullptr)
        return;
    Reactive_shutdown(&(*self).base);
    Memory_free(self);
}

void VEX_CAT(VEX_NAME, _set)(VEX_NAME *self, VEX_T value, float chance) {
    if (self == nullptr)
        return;
    (*self).chance = chance;
    Reactive_store(&(*self).base, VEX_TO_WORD(value));
}

float VEX_CAT(VEX_NAME, _getChance)(const VEX_NAME *self) {
    return self ? (*self).chance : 0.0f;
}

VEX_T VEX_CAT(VEX_NAME, _get)(const VEX_NAME *self) {
    if (self == nullptr)
        return VEX_ZERO;
    // The one-fire read delivers the held value, then the roll decides whether
    // this read is a hit (the value) or a miss (the empty value).
    VEX_T held = VEX_FROM_WORD(Reactive_load((Reactive*) &(*self).base));
    double roll = Random_nextDouble(Random_system());
    if (roll < (double) (*self).chance)
        return held;
    return VEX_ZERO;
}

VEX_CHANNELS_DEF(VEX_NAME, VEX_FROM_WORD);
