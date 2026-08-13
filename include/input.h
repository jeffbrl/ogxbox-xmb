#ifndef INPUT_H
#define INPUT_H

#include <SDL.h>

// Initializes controller mapping
int input_init(void);
void input_cleanup(void);

typedef struct {
    int up;
    int down;
    int left;
    int right;
    int a;
    int b;
    int back;
    int start;
    int quit; // 1 if SDL_QUIT occurred
} InputState;

// Process all pending events and update the state.
// State flags are momentary (only 1 for the frame they were pressed).
void input_update(InputState* state);

#endif // INPUT_H
