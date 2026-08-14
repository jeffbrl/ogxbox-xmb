#include <hal/debug.h>
#include <hal/video.h>
#include <windows.h>
#include <nxdk/mount.h>
#include <hal/xbox.h>
#include <SDL.h>

#include "xmb_config.h"
#include "xmb_types.h"
#include "menu_tree.h"
#include "ui_renderer.h"
#include "input.h"
#include "audio.h"
#include "xbe_launcher.h"

int main(void) {
    XVideoSetMode(WINDOW_WIDTH, WINDOW_HEIGHT, 32, REFRESH_DEFAULT);
    
    // Explicitly mount all standard partitions
    nxMountDrive('C', "\\Device\\Harddisk0\\Partition2\\");
    nxMountDrive('E', "\\Device\\Harddisk0\\Partition1\\");
    nxMountDrive('F', "\\Device\\Harddisk0\\Partition6\\");
    nxMountDrive('G', "\\Device\\Harddisk0\\Partition7\\");

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER | SDL_INIT_AUDIO) < 0) {
        debugPrint("Failed to initialize SDL: %s\n", SDL_GetError());
        return -1;
    }

    if (ui_init() < 0) {
        SDL_Quit();
        return -1;
    }

    audio_init();
    input_init();

    // Initialize Root Menu Tree and Scanned Game Nodes
    static XMBNode root_categories[CATEGORY_COUNT];
    menu_tree_init(root_categories, 64);

    XMBCategory current_category = CATEGORY_GAMES;
    XMBNavContext nav_ctx = { .depth = 0 };
    int selected_indices[MAX_NAV_DEPTH + 1] = {0};

    int running = 1;
    InputState state;
    
    while (running) {
        input_update(&state);
        
        if (state.quit) {
            running = 0;
        }
        
        // Category switching only allowed at root navigation level
        if (nav_ctx.depth == 0) {
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
        }
        
        int item_count = 0;
        XMBNode* active_items = menu_tree_get_active_list(&nav_ctx, current_category, root_categories, &item_count);
        int cur_sel = selected_indices[nav_ctx.depth];

        if (state.up) {
            if (cur_sel > 0) {
                selected_indices[nav_ctx.depth]--;
                audio_play_sfx(SFX_TICK);
            }
        }
        if (state.down) {
            if (cur_sel < item_count - 1) {
                selected_indices[nav_ctx.depth]++;
                audio_play_sfx(SFX_TICK);
            }
        }
        
        cur_sel = selected_indices[nav_ctx.depth];

        if (state.a) {
            if (item_count > 0 && active_items != NULL) {
                XMBNode* selected_node = &active_items[cur_sel];
                
                if (selected_node->type == NODE_TYPE_SUBMENU && selected_node->children != NULL && nav_ctx.depth < MAX_NAV_DEPTH) {
                    audio_play_sfx(SFX_SELECT);
                    nav_ctx.stack[nav_ctx.depth] = selected_node;
                    nav_ctx.depth++;
                    selected_indices[nav_ctx.depth] = 0; // reset sub-level cursor
                } else if (selected_node->type == NODE_TYPE_LAUNCH && selected_node->path[0] != '\0') {
                    audio_play_sfx(SFX_SELECT);
                    SDL_Delay(90);
                    xbe_launcher_launch(selected_node->path);
                    running = 0;
                } else if (selected_node->type == NODE_TYPE_THEME_CYCLE) {
                    audio_play_sfx(SFX_SELECT);
                    XMBTheme next_theme = (ui_get_theme() + 1) % THEME_COUNT;
                    ui_set_theme(next_theme);
                    snprintf(selected_node->subtitle, 64, "Active: %s", ui_get_theme_name(next_theme));
                } else if (selected_node->type == NODE_TYPE_ACTION) {
                    audio_play_sfx(SFX_SELECT);
                    if (strcmp(selected_node->title, "Theme: Xbox Emerald") == 0) {
                        ui_set_theme(THEME_XBOX_EMERALD);
                    } else if (strcmp(selected_node->title, "Theme: PS3 Obsidian") == 0) {
                        ui_set_theme(THEME_PS3_OBSIDIAN);
                    } else if (strcmp(selected_node->title, "Theme: Cobalt Sapphire") == 0) {
                        ui_set_theme(THEME_COBALT_BLUE);
                    } else if (strcmp(selected_node->title, "Theme: Ruby Crimson") == 0) {
                        ui_set_theme(THEME_RUBY_CRIMSON);
                    } else if (strcmp(selected_node->title, "Reboot Console") == 0) {
                        HalReturnToFirmware(HalRebootRoutine);
                    } else if (strcmp(selected_node->title, "Power Off") == 0) {
                        HalReturnToFirmware(HalHaltRoutine);
                    }
                } else {
                    audio_play_sfx(SFX_SELECT);
                }
            }
        }
        
        if (state.b) {
            if (nav_ctx.depth > 0) {
                audio_play_sfx(SFX_BACK);
                nav_ctx.depth--;
            } else {
                audio_play_sfx(SFX_BACK);
            }
        }
        
        audio_update();

        const char* breadcrumb = (nav_ctx.depth > 0) ? nav_ctx.stack[nav_ctx.depth - 1]->title : NULL;
        ui_render(current_category, active_items, item_count, selected_indices[nav_ctx.depth], nav_ctx.depth, breadcrumb);
        
        audio_update();

        SDL_Delay(16); // ~60fps
    }
    
    input_cleanup();
    audio_cleanup();
    ui_cleanup();
    SDL_Quit();
    return 0;
}
