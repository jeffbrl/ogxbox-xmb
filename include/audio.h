#ifndef AUDIO_H
#define AUDIO_H

typedef enum {
    SFX_NONE = 0,
    SFX_TICK,
    SFX_CATEGORY,
    SFX_SELECT,
    SFX_BACK
} SoundEffect;

int audio_init(void);
void audio_play_sfx(SoundEffect sfx);
void audio_update(void);
void audio_cleanup(void);

#endif // AUDIO_H
