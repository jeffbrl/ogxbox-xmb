#include "input.h"
#include <hal/debug.h>

static SDL_GameController *controller = NULL;

int input_init(void) {
    if (SDL_NumJoysticks() > 0) {
        if (SDL_IsGameController(0)) {
            controller = SDL_GameControllerOpen(0);
            if (controller) {
                debugPrint("Controller initialized\n");
            }
        }
    }
    return 0;
}

void input_cleanup(void) {
    if (controller) {
        SDL_GameControllerClose(controller);
        controller = NULL;
    }
}

void input_update(InputState* state) {
    state->up = 0;
    state->down = 0;
    state->left = 0;
    state->right = 0;
    state->a = 0;
    state->b = 0;
    state->back = 0;
    state->start = 0;
    state->quit = 0;

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            state->quit = 1;
        } else if (event.type == SDL_CONTROLLERBUTTONDOWN) {
            switch (event.cbutton.button) {
                case SDL_CONTROLLER_BUTTON_DPAD_UP: state->up = 1; break;
                case SDL_CONTROLLER_BUTTON_DPAD_DOWN: state->down = 1; break;
                case SDL_CONTROLLER_BUTTON_DPAD_LEFT: state->left = 1; break;
                case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: state->right = 1; break;
                case SDL_CONTROLLER_BUTTON_A: state->a = 1; break;
                case SDL_CONTROLLER_BUTTON_B: state->b = 1; break;
                case SDL_CONTROLLER_BUTTON_BACK: state->back = 1; break;
                case SDL_CONTROLLER_BUTTON_START: state->start = 1; break;
            }
        } else if (event.type == SDL_KEYDOWN) {
            // Also support keyboard for testing
            switch (event.key.keysym.sym) {
                case SDLK_UP: state->up = 1; break;
                case SDLK_DOWN: state->down = 1; break;
                case SDLK_LEFT: state->left = 1; break;
                case SDLK_RIGHT: state->right = 1; break;
                case SDLK_RETURN: state->a = 1; break;
                case SDLK_ESCAPE: state->b = 1; break;
            }
        }
    }
}
