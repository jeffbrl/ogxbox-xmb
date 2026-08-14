#include "audio.h"
#include <SDL.h>
#include <math.h>
#include <string.h>
#include <hal/debug.h>

#define SAMPLE_RATE 48000
#define PI 3.14159265358979323846f

static volatile SoundEffect current_sfx = SFX_NONE;
static volatile int sfx_sample_idx = 0;
static volatile int sfx_total_samples = 0;
static int audio_opened = 0;

static void audio_callback(void* userdata, Uint8* stream, int len) {
    (void)userdata;
    memset(stream, 0, len);

    if (current_sfx == SFX_NONE) {
        return;
    }

    Sint16* buffer = (Sint16*)stream;
    int num_samples = len / (int)(sizeof(Sint16) * 2); // 16-bit stereo

    for (int i = 0; i < num_samples; i++) {
        if (sfx_sample_idx >= sfx_total_samples) {
            current_sfx = SFX_NONE;
            break;
        }

        float t = (float)sfx_sample_idx / (float)SAMPLE_RATE;
        Sint16 val = 0;

        if (current_sfx == SFX_TICK) {
            // Tactile 18ms click pulse
            float dur = 0.018f;
            if (t < dur) {
                float env = (1.0f - t / dur) * (1.0f - t / dur);
                float freq = 2600.0f - (t / dur) * 1400.0f;
                val = (Sint16)(sinf(2.0f * PI * freq * t) * env * 22000.0f);
            }
        } else if (current_sfx == SFX_CATEGORY) {
            // Smooth horizontal glide (60ms)
            float dur = 0.060f;
            if (t < dur) {
                float env = sinf((t / dur) * PI);
                float freq = 550.0f + sinf((t / dur) * PI) * 500.0f;
                val = (Sint16)(sinf(2.0f * PI * freq * t) * env * 18000.0f);
            }
        } else if (current_sfx == SFX_SELECT) {
            // Crisp two-tone chime (170ms: C6 1046Hz -> E6 1318Hz)
            float dur = 0.170f;
            if (t < dur) {
                float env = 1.0f - (t / dur);
                float freq = (t < 0.075f) ? 1046.5f : 1318.5f;
                val = (Sint16)(sinf(2.0f * PI * freq * t) * env * 24000.0f);
            }
        } else if (current_sfx == SFX_BACK) {
            // Warm descending tone (130ms: A5 880Hz -> D5 587Hz)
            float dur = 0.130f;
            if (t < dur) {
                float env = 1.0f - (t / dur);
                float freq = 880.0f - (t / dur) * 320.0f;
                val = (Sint16)(sinf(2.0f * PI * freq * t) * env * 20000.0f);
            }
        }

        buffer[i * 2] = val;     // Left
        buffer[i * 2 + 1] = val; // Right
        sfx_sample_idx++;
    }
}

int audio_init(void) {
    SDL_AudioSpec wanted, obtained;
    SDL_zero(wanted);
    wanted.freq = SAMPLE_RATE;
    wanted.format = AUDIO_S16SYS;
    wanted.channels = 2;
    wanted.samples = 1024;
    wanted.callback = audio_callback;

    if (SDL_OpenAudio(&wanted, &obtained) < 0) {
        debugPrint("SDL_OpenAudio failed: %s\n", SDL_GetError());
        audio_opened = 0;
        return -1;
    }

    debugPrint("Audio initialized: %d Hz, %d channels, buffer %d\n",
        obtained.freq, obtained.channels, obtained.samples);

    SDL_PauseAudio(0); // Unpause audio device
    audio_opened = 1;
    return 0;
}

void audio_play_sfx(SoundEffect sfx) {
    if (!audio_opened || sfx == SFX_NONE) return;

    SDL_LockAudio();
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
    SDL_UnlockAudio();
}

void audio_cleanup(void) {
    if (audio_opened) {
        SDL_CloseAudio();
        audio_opened = 0;
    }
}
