#import "audio/audio_hal.h"

#import <AudioToolbox/AudioToolbox.h>
#import <CoreAudio/CoreAudio.h>
#include <stdlib.h>
#include <string.h>

#include "annotation/definition.h"
#include "annotation/overview.h"
#include "annotation/intention.h"

;;DEFINITION
/**
 * ============================================================================
 * DEFINITION: AudioHal_mac
 * ============================================================================
 * macOS native CoreAudio HAL output unit bridge: creates a default-output
 * AudioUnit with float PCM stream format and a render callback that feeds
 * real-time DSP straight into hardware output at minimal buffer latency. The
 * AudioHal struct is heap-allocated (malloc/free) because it must outlive
 * the caller's stack and is owned by the audio subsystem; destroy stops the
 * unit, uninitializes and disposes the AudioComponentInstance, then frees.
 * The render callback runs on the audio thread and must never block or
 * allocate — it calls the registered AudioHalRenderCallback or zero-fills.
 * ============================================================================
 */

;;OVERVIEW
/**
 * ============================================================================
 * MODULE: AudioHal_mac (objc/audio_hal_mac.m)
 * LEVEL: L4 — Hardware & Platform Integration (CoreAudio Low-Latency Output)
 * ============================================================================
 * macOS native CoreAudio HAL output unit bridge for real-time DSP feeding
 * into hardware output with minimal buffer latency.
 *
 * STRUCT FIELDS:
 *   - AudioHal: AudioUnit, is_running, sample_rate, channels, buffer_frames,
 *               callback, user_data
 *
 * FUNCTION REGISTRY:
 * ----------------------------------------------------------------------------
 * Lifecycle & Stream Control:
 *   - AudioHalConfig_default()
 *   - AudioHal_create(config, hal_out)
 *   - AudioHal_start(hal)
 *   - AudioHal_stop(hal)
 *   - AudioHal_destroy(hal)
 * Metrics:
 *   - AudioHal_is_running(hal)
 *   - AudioHal_get_sample_rate(hal)
 *   - AudioHal_get_channels(hal)
 *   - AudioHal_get_buffer_frames(hal)
 *   - AudioHal_get_latency_ms(hal)
 * ============================================================================
 */

;;INTENTION("Direct CoreAudio AudioUnit HAL output engine for real-time low-latency DSP")

struct AudioHal {
    AudioComponentInstance audio_unit;
    bool is_running;
    double sample_rate;
    uint32_t channels;
    uint32_t buffer_frames;
    AudioHalRenderCallback callback;
    void *user_data;
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

static OSStatus audio_hal_render_proc(
    void *inRefCon,
    AudioUnitRenderActionFlags *ioActionFlags,
    const AudioTimeStamp *inTimeStamp,
    UInt32 inBusNumber,
    UInt32 inNumberFrames,
    AudioBufferList *ioData
) {
    (void) ioActionFlags;
    (void) inTimeStamp;
    (void) inBusNumber;

    AudioHal *hal = (AudioHal*) inRefCon;
    if (!hal || !ioData || (*ioData).mNumberBuffers == 0) {
        return noErr;
    }

    float *out_buf = (float*) (*ioData).mBuffers[0].mData;
    uint32_t channels = (*hal).channels;

    if ((*hal).callback) {
        (*hal).callback(out_buf, (uint32_t) inNumberFrames, channels, (*hal).user_data);
    } else {
        size_t bytes = (size_t) inNumberFrames * channels * sizeof(float);
        memset(out_buf, 0, bytes);
    }

    return noErr;
}

bool AudioHal_create(const AudioHalConfig *config, AudioHal **hal_out) {
    if (!hal_out) {
        return false;
    }
    *hal_out = NULL;

    AudioHalConfig cfg = config ? (*config) : AudioHalConfig_default();
    if (cfg.channels == 0 || cfg.sample_rate <= 0.0 || cfg.buffer_frames == 0) {
        return false;
    }

    AudioComponentDescription desc;
    desc.componentType = kAudioUnitType_Output;
    desc.componentSubType = kAudioUnitSubType_DefaultOutput;
    desc.componentManufacturer = kAudioUnitManufacturer_Apple;
    desc.componentFlags = 0;
    desc.componentFlagsMask = 0;

    AudioComponent comp = AudioComponentFindNext(NULL, &desc);
    if (!comp) {
        return false;
    }

    AudioComponentInstance au = NULL;
    OSStatus err = AudioComponentInstanceNew(comp, &au);
    if (err != noErr || !au) {
        return false;
    }

    AudioStreamBasicDescription asbd;
    memset(&asbd, 0, sizeof(asbd));
    asbd.mSampleRate = cfg.sample_rate;
    asbd.mFormatID = kAudioFormatLinearPCM;
    asbd.mFormatFlags = kAudioFormatFlagIsFloat | kAudioFormatFlagIsPacked;
    asbd.mBytesPerPacket = cfg.channels * sizeof(float);
    asbd.mFramesPerPacket = 1;
    asbd.mBytesPerFrame = cfg.channels * sizeof(float);
    asbd.mChannelsPerFrame = cfg.channels;
    asbd.mBitsPerChannel = 32;

    err = AudioUnitSetProperty(
        au,
        kAudioUnitProperty_StreamFormat,
        kAudioUnitScope_Input,
        0,
        &asbd,
        sizeof(asbd)
    );
    if (err != noErr) {
        AudioComponentInstanceDispose(au);
        return false;
    }

    AudioHal *hal = (AudioHal*) malloc(sizeof(AudioHal));
    if (!hal) {
        AudioComponentInstanceDispose(au);
        return false;
    }

    (*hal).audio_unit = au;
    (*hal).is_running = false;
    (*hal).sample_rate = cfg.sample_rate;
    (*hal).channels = cfg.channels;
    (*hal).buffer_frames = cfg.buffer_frames;
    (*hal).callback = cfg.callback;
    (*hal).user_data = cfg.user_data;

    AURenderCallbackStruct cb_struct;
    cb_struct.inputProc = audio_hal_render_proc;
    cb_struct.inputProcRefCon = (void*) hal;

    err = AudioUnitSetProperty(
        au,
        kAudioUnitProperty_SetRenderCallback,
        kAudioUnitScope_Input,
        0,
        &cb_struct,
        sizeof(cb_struct)
    );
    if (err != noErr) {
        AudioComponentInstanceDispose(au);
        free(hal);
        return false;
    }

    err = AudioUnitInitialize(au);
    if (err != noErr) {
        AudioComponentInstanceDispose(au);
        free(hal);
        return false;
    }

    *hal_out = hal;
    return true;
}

bool AudioHal_start(AudioHal *hal) {
    if (!hal || !(*hal).audio_unit) {
        return false;
    }
    if ((*hal).is_running) {
        return true;
    }

    OSStatus err = AudioOutputUnitStart((*hal).audio_unit);
    if (err == noErr) {
        (*hal).is_running = true;
        return true;
    }
    return false;
}

bool AudioHal_stop(AudioHal *hal) {
    if (!hal || !(*hal).audio_unit) {
        return false;
    }
    if (!(*hal).is_running) {
        return true;
    }

    OSStatus err = AudioOutputUnitStop((*hal).audio_unit);
    if (err == noErr) {
        (*hal).is_running = false;
        return true;
    }
    return false;
}

void AudioHal_destroy(AudioHal *hal) {
    if (!hal) {
        return;
    }
    if ((*hal).is_running) {
        AudioHal_stop(hal);
    }
    if ((*hal).audio_unit) {
        AudioUnitUninitialize((*hal).audio_unit);
        AudioComponentInstanceDispose((*hal).audio_unit);
        (*hal).audio_unit = NULL;
    }
    free(hal);
}

bool AudioHal_is_running(const AudioHal *hal) {
    return hal ? (*hal).is_running : false;
}

double AudioHal_get_sample_rate(const AudioHal *hal) {
    return hal ? (*hal).sample_rate : 0.0;
}

uint32_t AudioHal_get_channels(const AudioHal *hal) {
    return hal ? (*hal).channels : 0;
}

uint32_t AudioHal_get_buffer_frames(const AudioHal *hal) {
    return hal ? (*hal).buffer_frames : 0;
}

double AudioHal_get_latency_ms(const AudioHal *hal) {
    if (!hal || (*hal).sample_rate <= 0.0) {
        return 0.0;
    }
    return (1000.0 * (double) (*hal).buffer_frames) / (*hal).sample_rate;
}
