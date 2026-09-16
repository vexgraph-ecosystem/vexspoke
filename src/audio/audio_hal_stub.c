#include "audio/audio_hal.h"

#include <stdlib.h>

#include "annotation/overview.h"
#include "annotation/intention.h"

;;OVERVIEW
/**
 * ============================================================================
 * MODULE: AudioHal_stub (audio/audio_hal_stub.c)
 * LEVEL: L4 — Platform Abstraction (Audio HAL Fallback)
 * ============================================================================
 * Fallback stub for Audio HAL on non-Apple systems without CoreAudio.
 * ============================================================================
 */

;;INTENTION("Audio HAL fallback stub")

struct AudioHal {
    bool is_running;
};

AudioHalConfig AudioHalConfig_default(void) {
    AudioHalConfig cfg;
    cfg.sample_rate = 48000.0;
    cfg.channels = 2;
    cfg.buffer_frames = 256;
    cfg.callback = NULL;
    cfg.user_data = NULL;
    return cfg;
}

bool AudioHal_create(const AudioHalConfig *config, AudioHal **hal_out) {
    (void) config;
    if (hal_out) {
        *hal_out = NULL;
    }
    return false;
}

bool AudioHal_start(AudioHal *hal) {
    (void) hal;
    return false;
}

bool AudioHal_stop(AudioHal *hal) {
    (void) hal;
    return false;
}

void AudioHal_destroy(AudioHal *hal) {
    if (hal) {
        free(hal);
    }
}

bool AudioHal_is_running(const AudioHal *hal) {
    (void) hal;
    return false;
}

double AudioHal_get_sample_rate(const AudioHal *hal) {
    (void) hal;
    return 0.0;
}

uint32_t AudioHal_get_channels(const AudioHal *hal) {
    (void) hal;
    return 0;
}

uint32_t AudioHal_get_buffer_frames(const AudioHal *hal) {
    (void) hal;
    return 0;
}

double AudioHal_get_latency_ms(const AudioHal *hal) {
    (void) hal;
    return 0.0;
}
