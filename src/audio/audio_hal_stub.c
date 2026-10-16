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
 * Public Constructors: (.h)
 *   - AudioHalConfig_0()
 *   - AudioHalConfig_1(callback)
 *   - AudioHalConfig_2(callback, user_data)
 *   - AudioHal_0()
 *   - AudioHal_1(config)
 * Public Core Functions: (.h)
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

AudioHalConfig AudioHalConfig_0(void) {
    AudioHalConfig cfg;
    cfg.sample_rate = 48000.0;
    cfg.channels = 2;
    cfg.buffer_frames = 256;
    cfg.callback = nullptr;
    cfg.user_data = nullptr;
    return cfg;
}

AudioHalConfig AudioHalConfig_1(AudioHalRenderCallback callback) {
    return AudioHalConfig_2(callback, nullptr);
}

AudioHalConfig AudioHalConfig_2(AudioHalRenderCallback callback, void *user_data) {
    AudioHalConfig cfg = AudioHalConfig_0();
    cfg.callback = callback;
    cfg.user_data = user_data;
    return cfg;
}

bool AudioHalConfig_set_sample_rate(AudioHalConfig *config, double sample_rate) {
    if (!config || sample_rate <= 0.0) return false;
    (*config).sample_rate = sample_rate;
    return true;
}

bool AudioHalConfig_set_channels(AudioHalConfig *config, uint32_t channels) {
    if (!config || channels == 0) return false;
    (*config).channels = channels;
    return true;
}

bool AudioHalConfig_set_buffer_frames(AudioHalConfig *config, uint32_t buffer_frames) {
    if (!config || buffer_frames == 0) return false;
    (*config).buffer_frames = buffer_frames;
    return true;
}

AudioHal *AudioHal_0(void) {
    return AudioHal_1(nullptr);
}

AudioHal *AudioHal_1(const AudioHalConfig *config) {
    (void) config;
    return nullptr;
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
