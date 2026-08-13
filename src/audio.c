#include "audio.h"
#include <SDL.h>
#include <math.h>
#include <hal/debug.h>

#define SAMPLE_RATE 44100
#define PI 3.14159265358979323846f

static SoundEffect current_sfx = SFX_NONE;
static int sfx_sample_idx = 0;
static int sfx_total_samples = 0;

static void audio_callback(void* userdata, Uint8* stream, int len) {
    (void)userdata;
    Sint16* buffer = (Sint16*)stream;
    int samples = len / (sizeof(Sint16) * 2); // stereo

    for (int i = 0; i < samples; i++) {
        Sint16 val = 0;
        if (current_sfx != SFX_NONE && sfx_sample_idx < sfx_total_samples) {
            float t = (float)sfx_sample_idx / (float)SAMPLE_RATE;
            float env = 0.0f;
            float freq = 0.0f;

            if (current_sfx == SFX_TICK) {
                // Short crisp click (15ms)
                float dur = 0.015f;
                if (t < dur) {
                    env = (1.0f - t / dur) * (1.0f - t / dur);
                    freq = 2400.0f - (t / dur) * 1000.0f;
                    val = (Sint16)(sinf(2.0f * PI * freq * t) * env * 14000.0f);
                }
            } else if (current_sfx == SFX_CATEGORY) {
                // Soft horizontal glide swoosh (65ms)
                float dur = 0.065f;
                if (t < dur) {
                    env = sinf((t / dur) * PI);
                    freq = 650.0f + sinf((t / dur) * PI) * 400.0f;
                    val = (Sint16)(sinf(2.0f * PI * freq * t) * env * 12000.0f);
                }
            } else if (current_sfx == SFX_SELECT) {
                // Melodic two-tone chime (160ms)
                float dur = 0.160f;
                if (t < dur) {
                    env = 1.0f - (t / dur);
                    float f1 = (t < 0.08f) ? 1046.5f : 1318.5f; // C6 to E6
                    val = (Sint16)(sinf(2.0f * PI * f1 * t) * env * 18000.0f);
                }
            } else if (current_sfx == SFX_BACK) {
                // Soft downward tone (120ms)
                float dur = 0.120f;
                if (t < dur) {
                    env = 1.0f - (t / dur);
                    freq = 880.0f - (t / dur) * 350.0f;
                    val = (Sint16)(sinf(2.0f * PI * freq * t) * env * 15000.0f);
                }
            }

            sfx_sample_idx++;
            if (sfx_sample_idx >= sfx_total_samples) {
                current_sfx = SFX_NONE;
            }
        }
        buffer[i * 2] = val;     // Left
        buffer[i * 2 + 1] = val; // Right
    }
}

int audio_init(void) {
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
        debugPrint("SDL Audio init failed: %s\n", SDL_GetError());
        return -1;
    }

    SDL_AudioSpec wanted, obtained;
    SDL_zero(wanted);
    wanted.freq = SAMPLE_RATE;
    wanted.format = AUDIO_S16SYS;
    wanted.channels = 2;
    wanted.samples = 1024;
    wanted.callback = audio_callback;

    if (SDL_OpenAudio(&wanted, &obtained) < 0) {
        debugPrint("Failed to open audio: %s\n", SDL_GetError());
        return -1;
    }

    SDL_PauseAudio(0); // Start audio playback
    return 0;
}

void audio_play_sfx(SoundEffect sfx) {
    SDL_LockAudio();
    current_sfx = sfx;
    sfx_sample_idx = 0;
    if (sfx == SFX_TICK) {
        sfx_total_samples = (int)(0.020f * SAMPLE_RATE);
    } else if (sfx == SFX_CATEGORY) {
        sfx_total_samples = (int)(0.070f * SAMPLE_RATE);
    } else if (sfx == SFX_SELECT) {
        sfx_total_samples = (int)(0.170f * SAMPLE_RATE);
    } else if (sfx == SFX_BACK) {
        sfx_total_samples = (int)(0.130f * SAMPLE_RATE);
    } else {
        sfx_total_samples = 0;
    }
    SDL_UnlockAudio();
}

void audio_cleanup(void) {
    SDL_CloseAudio();
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
}
