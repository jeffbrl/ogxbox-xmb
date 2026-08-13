#include <hal/debug.h>
#include <hal/video.h>
#include <windows.h>
#include <SDL.h>
#include <nxdk/mount.h>

#include "xmb_config.h"
#include "xmb_types.h"
#include "ui_renderer.h"
#include "input.h"
#include "audio.h"
#include "xbe_scanner.h"
#include "xbe_launcher.h"

#define MAX_ITEMS_PER_CAT 50

int main(void) {
    XVideoSetMode(WINDOW_WIDTH, WINDOW_HEIGHT, 32, REFRESH_DEFAULT);
    
    // Explicitly mount all standard partitions
    nxMountDrive('C', "\\Device\\Harddisk0\\Partition2\\");
    nxMountDrive('E', "\\Device\\Harddisk0\\Partition1\\");
    nxMountDrive('F', "\\Device\\Harddisk0\\Partition6\\");
    nxMountDrive('G', "\\Device\\Harddisk0\\Partition7\\");

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) < 0) {
        debugPrint("Failed to initialize SDL: %s\n", SDL_GetError());
        return -1;
    }

    if (ui_init() < 0) {
        SDL_Quit();
        return -1;
    }

    audio_init();
    input_init();

    static XMBItem items[CATEGORY_COUNT][MAX_ITEMS_PER_CAT];
    int counts[CATEGORY_COUNT] = {0};
    
    for (int i = 0; i < CATEGORY_COUNT; i++) {
        counts[i] = xbe_scanner_get_items(i, items[i], MAX_ITEMS_PER_CAT);
    }

    XMBCategory current_category = CATEGORY_GAMES;
    int selected_index[CATEGORY_COUNT] = {0};
    int running = 1;
    InputState state;
    
    while (running) {
        input_update(&state);
        
        if (state.quit) {
            running = 0;
        }
        
        if (state.left) {
            if (current_category > 0) {
                current_category--;
                audio_play_sfx(SFX_CATEGORY);
            }
        }
        if (state.right) {
            if (current_category < CATEGORY_COUNT - 1) {
                current_category++;
                audio_play_sfx(SFX_CATEGORY);
            }
        }
        
        int count = counts[current_category];
        if (state.up) {
            if (selected_index[current_category] > 0) {
                selected_index[current_category]++;
                selected_index[current_category]--; // clamp logic
                selected_index[current_category]--;
                audio_play_sfx(SFX_TICK);
            }
        }
        if (state.down) {
            if (selected_index[current_category] < count - 1) {
                selected_index[current_category]++;
                audio_play_sfx(SFX_TICK);
            }
        }
        if (state.a) {
            if (count > 0) {
                audio_play_sfx(SFX_SELECT);
                if (current_category == CATEGORY_GAMES) {
                    SDL_Delay(80); // Brief audio cue grace period before hardware handoff
                    xbe_launcher_launch(items[current_category][selected_index[current_category]].path);
                    running = 0;
                }
            }
        }
        if (state.b) {
            audio_play_sfx(SFX_BACK);
        }
        
        ui_render(current_category, items[current_category], count, selected_index[current_category]);
        
        SDL_Delay(16); // ~60fps
    }
    
    input_cleanup();
    audio_cleanup();
    ui_cleanup();
    SDL_Quit();
    return 0;
}
