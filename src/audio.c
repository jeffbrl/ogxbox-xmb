#include "audio.h"
#include <hal/audio.h>
#include <hal/debug.h>
#include <xboxkrnl/xboxkrnl.h>
#include <math.h>
#include <string.h>

#define SAMPLE_RATE 48000
#define BUFFER_SAMPLES 1024 // 1024 samples = ~21.3ms at 48kHz
#define NUM_DMA_BUFFERS 4
#define PI 3.14159265358979323846f

static short dma_buffers[NUM_DMA_BUFFERS][BUFFER_SAMPLES * 2] __attribute__((aligned(128)));
static int current_dma_buf = 0;

static volatile SoundEffect current_sfx = SFX_NONE;
static volatile int sfx_sample_idx = 0;
static volatile int sfx_total_samples = 0;

static void synthesize_samples(short* buffer, int samples) {
    memset(buffer, 0, samples * 2 * sizeof(short));

    if (current_sfx == SFX_NONE) {
        return;
    }

    for (int i = 0; i < samples; i++) {
        if (sfx_sample_idx >= sfx_total_samples) {
            current_sfx = SFX_NONE;
            break;
        }

        float t = (float)sfx_sample_idx / (float)SAMPLE_RATE;
        short val = 0;

        if (current_sfx == SFX_TICK) {
            // Crisp tactile tick (18ms)
            float dur = 0.018f;
            if (t < dur) {
                float env = (1.0f - t / dur) * (1.0f - t / dur);
                float freq = 2800.0f - (t / dur) * 1600.0f;
                val = (short)(sinf(2.0f * PI * freq * t) * env * 24000.0f);
            }
        } else if (current_sfx == SFX_CATEGORY) {
            // Smooth horizontal glide (60ms)
            float dur = 0.060f;
            if (t < dur) {
                float env = sinf((t / dur) * PI);
                float freq = 600.0f + sinf((t / dur) * PI) * 500.0f;
                val = (short)(sinf(2.0f * PI * freq * t) * env * 20000.0f);
            }
        } else if (current_sfx == SFX_SELECT) {
            // Bright harmonic two-tone chime (170ms: C6 1046Hz -> E6 1318Hz)
            float dur = 0.170f;
            if (t < dur) {
                float env = 1.0f - (t / dur);
                float freq = (t < 0.08f) ? 1046.5f : 1318.5f;
                val = (short)(sinf(2.0f * PI * freq * t) * env * 26000.0f);
            }
        } else if (current_sfx == SFX_BACK) {
            // Warm descending tone (130ms: A5 880Hz -> D5 587Hz)
            float dur = 0.130f;
            if (t < dur) {
                float env = 1.0f - (t / dur);
                float freq = 880.0f - (t / dur) * 320.0f;
                val = (short)(sinf(2.0f * PI * freq * t) * env * 22000.0f);
            }
        }

        buffer[i * 2] = val;     // Left Channel
        buffer[i * 2 + 1] = val; // Right Channel
        sfx_sample_idx++;
    }
}

static void native_audio_callback(void* pac97Device, void* data) {
    (void)pac97Device;
    (void)data;

    short* target = dma_buffers[current_dma_buf];
    synthesize_samples(target, BUFFER_SAMPLES);

    XAudioProvideSamples((unsigned char*)target, BUFFER_SAMPLES * 2 * sizeof(short), 0);
    current_dma_buf = (current_dma_buf + 1) % NUM_DMA_BUFFERS;
}

int audio_init(void) {
    // Lock DMA buffer memory pages so physical addresses remain static
    MmLockUnlockBufferPages((PVOID)dma_buffers, sizeof(dma_buffers), FALSE);

    // Initialise native Xbox AC97 hardware driver
    XAudioInit(16, 2, native_audio_callback, NULL);

    // Pre-feed silent initial DMA buffers to prime the ring descriptors
    for (int i = 0; i < NUM_DMA_BUFFERS; i++) {
        memset(dma_buffers[i], 0, sizeof(dma_buffers[i]));
        XAudioProvideSamples((unsigned char*)dma_buffers[i], BUFFER_SAMPLES * 2 * sizeof(short), 0);
    }
    current_dma_buf = 0;

    // Start hardware playback
    XAudioPlay();
    return 0;
}

void audio_play_sfx(SoundEffect sfx) {
    if (sfx == SFX_NONE) return;

    current_sfx = sfx;
    sfx_sample_idx = 0;
    if (sfx == SFX_TICK) {
        sfx_total_samples = (int)(0.025f * SAMPLE_RATE);
    } else if (sfx == SFX_CATEGORY) {
        sfx_total_samples = (int)(0.070f * SAMPLE_RATE);
    } else if (sfx == SFX_SELECT) {
        sfx_total_samples = (int)(0.180f * SAMPLE_RATE);
    } else if (sfx == SFX_BACK) {
        sfx_total_samples = (int)(0.140f * SAMPLE_RATE);
    } else {
        sfx_total_samples = 0;
    }
}

void audio_cleanup(void) {
    XAudioPause();
}
