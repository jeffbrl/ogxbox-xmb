#include "input.h"
#include <hal/debug.h>
#include <SDL.h>

static SDL_GameController *controller = NULL;
static SDL_Joystick *joystick = NULL;

#define ANALOG_DEADZONE 16000
static int stick_up_pressed = 0;
static int stick_down_pressed = 0;
static int stick_left_pressed = 0;
static int stick_right_pressed = 0;

static void try_open_controller(int device_index) {
    if (!controller && SDL_IsGameController(device_index)) {
        controller = SDL_GameControllerOpen(device_index);
        if (controller) {
            debugPrint("GameController %d opened successfully\n", device_index);
            return;
        }
    }

    if (!controller && !joystick) {
        joystick = SDL_JoystickOpen(device_index);
        if (joystick) {
            debugPrint("Raw Joystick %d opened successfully\n", device_index);
        }
    }
}

int input_init(void) {
    stick_up_pressed = 0;
    stick_down_pressed = 0;
    stick_left_pressed = 0;
    stick_right_pressed = 0;

    int num_joysticks = SDL_NumJoysticks();
    for (int i = 0; i < num_joysticks; i++) {
        try_open_controller(i);
        if (controller || joystick) break;
    }
    return 0;
}

void input_cleanup(void) {
    if (controller) {
        SDL_GameControllerClose(controller);
        controller = NULL;
    }
    if (joystick) {
        SDL_JoystickClose(joystick);
        joystick = NULL;
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
        switch (event.type) {
            case SDL_QUIT:
                state->quit = 1;
                break;

            // Hotplugging support
            case SDL_CONTROLLERDEVICEADDED:
                if (!controller) {
                    try_open_controller(event.cdevice.which);
                }
                break;
            case SDL_CONTROLLERDEVICEREMOVED:
                if (controller) {
                    SDL_GameControllerClose(controller);
                    controller = NULL;
                }
                break;
            case SDL_JOYDEVICEADDED:
                if (!controller && !joystick) {
                    try_open_controller(event.jdevice.which);
                }
                break;
            case SDL_JOYDEVICEREMOVED:
                if (joystick) {
                    SDL_JoystickClose(joystick);
                    joystick = NULL;
                }
                break;

            // GameController button events
            case SDL_CONTROLLERBUTTONDOWN:
                switch (event.cbutton.button) {
                    case SDL_CONTROLLER_BUTTON_DPAD_UP: state->up = 1; break;
                    case SDL_CONTROLLER_BUTTON_DPAD_DOWN: state->down = 1; break;
                    case SDL_CONTROLLER_BUTTON_DPAD_LEFT: state->left = 1; break;
                    case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: state->right = 1; break;
                    case SDL_CONTROLLER_BUTTON_A: state->a = 1; break;
                    case SDL_CONTROLLER_BUTTON_B: state->b = 1; break;
                    case SDL_CONTROLLER_BUTTON_BACK: state->back = 1; break;
                    case SDL_CONTROLLER_BUTTON_START: state->start = 1; break;
                    default: break;
                }
                break;

            // GameController Axis motion (Thumbsticks)
            case SDL_CONTROLLERAXISMOTION:
                if (event.caxis.axis == SDL_CONTROLLER_AXIS_LEFTY) {
                    if (event.caxis.value < -ANALOG_DEADZONE) {
                        if (!stick_up_pressed) { state->up = 1; stick_up_pressed = 1; }
                    } else if (event.caxis.value > ANALOG_DEADZONE) {
                        if (!stick_down_pressed) { state->down = 1; stick_down_pressed = 1; }
                    } else {
                        stick_up_pressed = 0;
                        stick_down_pressed = 0;
                    }
                } else if (event.caxis.axis == SDL_CONTROLLER_AXIS_LEFTX) {
                    if (event.caxis.value < -ANALOG_DEADZONE) {
                        if (!stick_left_pressed) { state->left = 1; stick_left_pressed = 1; }
                    } else if (event.caxis.value > ANALOG_DEADZONE) {
                        if (!stick_right_pressed) { state->right = 1; stick_right_pressed = 1; }
                    } else {
                        stick_left_pressed = 0;
                        stick_right_pressed = 0;
                    }
                }
                break;

            // Raw Joystick button events (Xbox mappings: 0=A, 1=B, 6=Back, 7=Start)
            case SDL_JOYBUTTONDOWN:
                switch (event.jbutton.button) {
                    case 0: state->a = 1; break;
                    case 1: state->b = 1; break;
                    case 6: state->back = 1; break;
                    case 7: state->start = 1; break;
                    default: break;
                }
                break;

            // Raw Joystick Hat motion (D-Pad)
            case SDL_JOYHATMOTION:
                if (event.jhat.value & SDL_HAT_UP) state->up = 1;
                if (event.jhat.value & SDL_HAT_DOWN) state->down = 1;
                if (event.jhat.value & SDL_HAT_LEFT) state->left = 1;
                if (event.jhat.value & SDL_HAT_RIGHT) state->right = 1;
                break;

            // Raw Joystick Axis motion
            case SDL_JOYAXISMOTION:
                if (event.jaxis.axis == 1) { // Left Stick Y
                    if (event.jaxis.value < -ANALOG_DEADZONE) {
                        if (!stick_up_pressed) { state->up = 1; stick_up_pressed = 1; }
                    } else if (event.jaxis.value > ANALOG_DEADZONE) {
                        if (!stick_down_pressed) { state->down = 1; stick_down_pressed = 1; }
                    } else {
                        stick_up_pressed = 0;
                        stick_down_pressed = 0;
                    }
                } else if (event.jaxis.axis == 0) { // Left Stick X
                    if (event.jaxis.value < -ANALOG_DEADZONE) {
                        if (!stick_left_pressed) { state->left = 1; stick_left_pressed = 1; }
                    } else if (event.jaxis.value > ANALOG_DEADZONE) {
                        if (!stick_right_pressed) { state->right = 1; stick_right_pressed = 1; }
                    } else {
                        stick_left_pressed = 0;
                        stick_right_pressed = 0;
                    }
                }
                break;

            // Keyboard fallback for debugging
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                    case SDLK_UP: state->up = 1; break;
                    case SDLK_DOWN: state->down = 1; break;
                    case SDLK_LEFT: state->left = 1; break;
                    case SDLK_RIGHT: state->right = 1; break;
                    case SDLK_RETURN:
                    case SDLK_SPACE: state->a = 1; break;
                    case SDLK_ESCAPE:
                    case SDLK_BACKSPACE: state->b = 1; break;
                    default: break;
                }
                break;

            default:
                break;
        }
    }
}
