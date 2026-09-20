#include "audio/audio_hal.h"

#include <stdlib.h>

#include "annotation/definition.h"
#include "annotation/overview.h"
#include "annotation/intention.h"

;;DEFINITION
/**
 * ============================================================================
 * DEFINITION: AudioHal (stub)
 * ============================================================================
 * Fallback implementation of the AudioHal contract for platforms without
 * CoreAudio: every create/start/stop call fails closed (returns false) and
 * every getter returns a safe zero default, so audio-dependent subsystems
 * degrade gracefully instead of crashing. The opaque AudioHal struct carries
 * a single is_running flag that is never set true here. This is the L4
 * platform-abstraction seam: the real CoreAudio HAL replaces this file on
 * Apple platforms, and both must satisfy the same audio/audio_hal.h contract.
 * ============================================================================
 */

;;OVERVIEW
/**
 * ============================================================================
 * MODULE: AudioHal_stub (audio/audio_hal_stub.c)
 * LEVEL: L4 — Platform Abstraction (Audio HAL Fallback)
 * ============================================================================
 * Fallback stub for Audio HAL on non-Apple systems without CoreAudio.
 *
 * STRUCT FIELDS (Mirroring audio/audio_hal.h + local AudioHal):
 * ----------------------------------------------------------------------------
 *   AudioHalConfig {
 *     double sample_rate;             // e.g. 44100.0, 48000.0, 96000.0
 *     uint32_t channels;              // 1 = mono, 2 = stereo
 *     uint32_t buffer_frames;         // e.g. 128, 256, 512
 *     AudioHalRenderCallback callback;// Real-time pull render callback
 *     void *user_data;
 *   }
 *   AudioHal {                        // opaque; defined locally in this .c
 *     bool is_running;                // never set true by the stub
 *   }
 *
 * FUNCTION REGISTRY:
 * ----------------------------------------------------------------------------
 * Public Core Functions: (.h)
 *   - AudioHalConfig_default(void)
 *   - AudioHal_create(config, hal_out)
 *   - AudioHal_start(hal)
 *   - AudioHal_stop(hal)
 *   - AudioHal_destroy(hal)
 * Public Getters: (.h)
 *   - AudioHal_is_running(hal)
 *   - AudioHal_get_sample_rate(hal)
 *   - AudioHal_get_channels(hal)
 *   - AudioHal_get_buffer_frames(hal)
 *   - AudioHal_get_latency_ms(hal)
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
