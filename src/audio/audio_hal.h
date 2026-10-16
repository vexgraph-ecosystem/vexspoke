#ifndef VEXSPOKE_AUDIO_AUDIO_HAL_H
#define VEXSPOKE_AUDIO_AUDIO_HAL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "c23/constructor.h"

#ifdef __cplusplus
extern "C" {
#endif

// Low-latency CoreAudio HAL Render Callback
// Invoked by the high-priority real-time audio thread.
typedef void (*AudioHalRenderCallback)(
    float *interleaved_output,
    uint32_t frame_count,
    uint32_t channels,
    void *user_data
);

typedef struct AudioHalConfig {
    double sample_rate;             // e.g. 44100.0, 48000.0, 96000.0
    uint32_t channels;              // 1 = mono, 2 = stereo
    uint32_t buffer_frames;         // e.g. 128, 256, 512
    AudioHalRenderCallback callback;// Real-time pull render callback
    void *user_data;
} AudioHalConfig;

typedef struct AudioHal AudioHal;

AudioHalConfig AudioHalConfig_0(void);
AudioHalConfig AudioHalConfig_1(AudioHalRenderCallback callback);
AudioHalConfig AudioHalConfig_2(AudioHalRenderCallback callback, void *user_data);
#define AudioHalConfig(...) CONSTRUCTOR_DISPATCH(AudioHalConfig, __VA_ARGS__)
bool AudioHalConfig_set_sample_rate(AudioHalConfig *config, double sample_rate);
bool AudioHalConfig_set_channels(AudioHalConfig *config, uint32_t channels);
bool AudioHalConfig_set_buffer_frames(AudioHalConfig *config, uint32_t buffer_frames);

AudioHal *AudioHal_0(void);
AudioHal *AudioHal_1(const AudioHalConfig *config);
#define AudioHal(...) CONSTRUCTOR_DISPATCH(AudioHal, __VA_ARGS__)

bool AudioHal_start(AudioHal *hal);
bool AudioHal_stop(AudioHal *hal);
void AudioHal_destroy(AudioHal *hal);

bool AudioHal_is_running(const AudioHal *hal);
double AudioHal_get_sample_rate(const AudioHal *hal);
uint32_t AudioHal_get_channels(const AudioHal *hal);
uint32_t AudioHal_get_buffer_frames(const AudioHal *hal);
double AudioHal_get_latency_ms(const AudioHal *hal);

#ifdef __cplusplus
}
#endif

#endif // VEXSPOKE_AUDIO_AUDIO_HAL_H
