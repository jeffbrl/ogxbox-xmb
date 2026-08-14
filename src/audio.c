#include "audio.h"
#include <hal/audio.h>
#include <hal/debug.h>
#include <xboxkrnl/xboxkrnl.h>
#include <math.h>
#include <string.h>

#define SAMPLE_RATE 48000
#define BUFFER_SAMPLES 1024
#define NUM_DMA_BUFFERS 4
#define PI 3.14159265358979323846f

// Static double-buffered DMA pages locked in RAM
static short dma_buffers[NUM_DMA_BUFFERS][BUFFER_SAMPLES * 2] __attribute__((aligned(128)));
static int current_dma_buf = 0;

// Precomputed integer PCM Wave Tables (Interleaved Stereo)
#define SFX_TICK_LEN 960
#define SFX_CAT_LEN 2880
#define SFX_SELECT_LEN 8160
#define SFX_BACK_LEN 6240

static short pcm_tick[SFX_TICK_LEN * 2];
static short pcm_cat[SFX_CAT_LEN * 2];
static short pcm_select[SFX_SELECT_LEN * 2];
static short pcm_back[SFX_BACK_LEN * 2];

static const short* volatile active_sfx_ptr = NULL;
static volatile int active_sfx_len = 0;
static volatile int active_sfx_pos = 0;

static void precompute_tables(void) {
    // 1. Tick (Click)
    for (int i = 0; i < SFX_TICK_LEN; i++) {
        float t = (float)i / (float)SAMPLE_RATE;
        float dur = 0.020f;
        float env = (1.0f - t / dur) * (1.0f - t / dur);
        float freq = 2800.0f - (t / dur) * 1600.0f;
        short s = (short)(sinf(2.0f * PI * freq * t) * env * 28000.0f);
        pcm_tick[i * 2] = s;
        pcm_tick[i * 2 + 1] = s;
    }

    // 2. Category (Swoosh)
    for (int i = 0; i < SFX_CAT_LEN; i++) {
        float t = (float)i / (float)SAMPLE_RATE;
        float dur = 0.060f;
        float env = sinf((t / dur) * PI);
        float freq = 550.0f + sinf((t / dur) * PI) * 550.0f;
        short s = (short)(sinf(2.0f * PI * freq * t) * env * 24000.0f);
        pcm_cat[i * 2] = s;
        pcm_cat[i * 2 + 1] = s;
    }

    // 3. Select (Chime C6 -> E6)
    for (int i = 0; i < SFX_SELECT_LEN; i++) {
        float t = (float)i / (float)SAMPLE_RATE;
        float dur = 0.170f;
        float env = 1.0f - (t / dur);
        float freq = (t < 0.08f) ? 1046.5f : 1318.5f;
        short s = (short)(sinf(2.0f * PI * freq * t) * env * 28000.0f);
        pcm_select[i * 2] = s;
        pcm_select[i * 2 + 1] = s;
    }

    // 4. Back (Tone A5 -> D5)
    for (int i = 0; i < SFX_BACK_LEN; i++) {
        float t = (float)i / (float)SAMPLE_RATE;
        float dur = 0.130f;
        float env = 1.0f - (t / dur);
        float freq = 880.0f - (t / dur) * 320.0f;
        short s = (short)(sinf(2.0f * PI * freq * t) * env * 26000.0f);
        pcm_back[i * 2] = s;
        pcm_back[i * 2 + 1] = s;
    }
}

static void feed_next_buffer(void) {
    short* dst = dma_buffers[current_dma_buf];

    // Zero-FPU pure integer memory copy/mix
    for (int i = 0; i < BUFFER_SAMPLES; i++) {
        short sample = 0;
        if (active_sfx_ptr != NULL && active_sfx_pos < active_sfx_len) {
            sample = active_sfx_ptr[active_sfx_pos * 2];
            active_sfx_pos++;
            if (active_sfx_pos >= active_sfx_len) {
                active_sfx_ptr = NULL;
            }
        }
        dst[i * 2]     = sample;
        dst[i * 2 + 1] = sample;
    }

    XAudioProvideSamples((unsigned char*)dst, BUFFER_SAMPLES * 2 * sizeof(short), 0);
    current_dma_buf = (current_dma_buf + 1) % NUM_DMA_BUFFERS;
}

static void native_audio_callback(void* pac97Device, void* data) {
    (void)pac97Device;
    (void)data;
    feed_next_buffer();
}

int audio_init(void) {
    precompute_tables();

    MmLockUnlockBufferPages((PVOID)dma_buffers, sizeof(dma_buffers), FALSE);

    XAudioInit(16, 2, native_audio_callback, NULL);

    for (int i = 0; i < NUM_DMA_BUFFERS; i++) {
        feed_next_buffer();
    }

    XAudioPlay();
    return 0;
}

void audio_play_sfx(SoundEffect sfx) {
    if (sfx == SFX_TICK) {
        active_sfx_ptr = pcm_tick;
        active_sfx_len = SFX_TICK_LEN;
        active_sfx_pos = 0;
    } else if (sfx == SFX_CATEGORY) {
        active_sfx_ptr = pcm_cat;
        active_sfx_len = SFX_CAT_LEN;
        active_sfx_pos = 0;
    } else if (sfx == SFX_SELECT) {
        active_sfx_ptr = pcm_select;
        active_sfx_len = SFX_SELECT_LEN;
        active_sfx_pos = 0;
    } else if (sfx == SFX_BACK) {
        active_sfx_ptr = pcm_back;
        active_sfx_len = SFX_BACK_LEN;
        active_sfx_pos = 0;
    }
}

void audio_cleanup(void) {
    XAudioPause();
}
