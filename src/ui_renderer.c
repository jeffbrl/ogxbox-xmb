#include "ui_renderer.h"
#include "xmb_config.h"
#include <SDL.h>
#include <SDL_ttf.h>
#include <hal/debug.h>
#include <math.h>
#include <time.h>
#include <stdlib.h>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_STDIO
#define STBI_ONLY_PNG
#include "stb_image.h"

static SDL_Window* window = NULL;
static SDL_Renderer* renderer = NULL;
static TTF_Font* font_main = NULL;
static TTF_Font* font_small = NULL;

// Textures for Categories & Items
static SDL_Texture* tex_cat_games = NULL;
static SDL_Texture* tex_cat_apps = NULL;
static SDL_Texture* tex_cat_settings = NULL;
static SDL_Texture* tex_default_game = NULL;

// PS3 Wave Particle System
#define NUM_PARTICLES 24
typedef struct {
    float x, y;
    float speed;
    float size;
    float alpha;
} WaveParticle;

static WaveParticle particles[NUM_PARTICLES];
static float wave_time = 0.0f;

// Kinematic Motion Interpolation
static float anim_cat_x = 0.0f;
static float anim_item_y = 0.0f;
static float anim_cat_elev = 0.0f;
static int anim_initialized = 0;

static SDL_Texture* load_texture(const char* filename) {
    const char* prefixes[] = {
        "D:\\assets\\icons\\",
        "C:\\assets\\icons\\",
        "E:\\assets\\icons\\",
        "assets/icons/"
    };

    for (int p = 0; p < 4; p++) {
        char full_path[256];
        snprintf(full_path, sizeof(full_path), "%s%s", prefixes[p], filename);
        
        FILE* f = fopen(full_path, "rb");
        if (f) {
            fseek(f, 0, SEEK_END);
            long size = ftell(f);
            fseek(f, 0, SEEK_SET);
            
            unsigned char* buf = (unsigned char*)malloc(size);
            if (buf) {
                fread(buf, 1, size, f);
                int w, h, channels;
                unsigned char* data = stbi_load_from_memory(buf, (int)size, &w, &h, &channels, 4);
                free(buf);
                fclose(f);
                
                if (data) {
                    SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormatFrom(
                        data, w, h, 32, w * 4, SDL_PIXELFORMAT_RGBA32);
                    if (surface) {
                        SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surface);
                        SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
                        SDL_FreeSurface(surface);
                        stbi_image_free(data);
                        return tex;
                    }
                    stbi_image_free(data);
                }
                return NULL;
            }
            fclose(f);
        }
    }
    return NULL;
}

#define MAX_CUSTOM_ICONS 32
typedef struct {
    char path[256];
    SDL_Texture* texture;
} CustomIconCache;

static CustomIconCache icon_cache[MAX_CUSTOM_ICONS];
static int icon_cache_count = 0;

static SDL_Texture* get_custom_icon(const char* file_path) {
    if (!file_path || file_path[0] == '\0') return NULL;
    
    for (int i = 0; i < icon_cache_count; i++) {
        if (strcmp(icon_cache[i].path, file_path) == 0) {
            return icon_cache[i].texture;
        }
    }
    
    FILE* f = fopen(file_path, "rb");
    if (!f) return NULL;
    
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    unsigned char* buf = (unsigned char*)malloc(size);
    if (!buf) {
        fclose(f);
        return NULL;
    }
    
    fread(buf, 1, size, f);
    fclose(f);
    
    int w, h, channels;
    unsigned char* data = stbi_load_from_memory(buf, (int)size, &w, &h, &channels, 4);
    free(buf);
    
    if (data) {
        SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormatFrom(
            data, w, h, 32, w * 4, SDL_PIXELFORMAT_RGBA32);
        if (surface) {
            SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surface);
            SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
            SDL_FreeSurface(surface);
            stbi_image_free(data);
            
            if (tex && icon_cache_count < MAX_CUSTOM_ICONS) {
                strncpy(icon_cache[icon_cache_count].path, file_path, 256);
                icon_cache[icon_cache_count].texture = tex;
                icon_cache_count++;
            }
            return tex;
        }
        stbi_image_free(data);
    }
    return NULL;
}

static void init_particles(void) {
    for (int i = 0; i < NUM_PARTICLES; i++) {
        particles[i].x = (float)(rand() % WINDOW_WIDTH);
        particles[i].y = (float)(rand() % WINDOW_HEIGHT);
        particles[i].speed = 0.4f + ((float)rand() / (float)RAND_MAX) * 0.7f;
        particles[i].size = 2.0f + ((float)rand() / (float)RAND_MAX) * 2.0f;
        particles[i].alpha = 60.0f + ((float)rand() / (float)RAND_MAX) * 100.0f;
    }
}

int ui_init(void) {
    window = SDL_CreateWindow("Xbox XMB",
                              SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED,
                              WINDOW_WIDTH, WINDOW_HEIGHT,
                              SDL_WINDOW_SHOWN);
    if (!window) {
        debugPrint("Window creation failed: %s\n", SDL_GetError());
        return -1;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
        if (!renderer) {
            debugPrint("Renderer creation failed: %s\n", SDL_GetError());
            return -1;
        }
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    if (TTF_Init() == -1) {
        debugPrint("TTF_Init failed: %s\n", TTF_GetError());
        return -1;
    }

    const char* font_paths[] = {
        "D:\\assets\\fonts\\Roboto-Regular.ttf",
        "C:\\assets\\fonts\\Roboto-Regular.ttf",
        "E:\\assets\\fonts\\Roboto-Regular.ttf",
        "assets/fonts/Roboto-Regular.ttf"
    };

    for (int i = 0; i < 4; i++) {
        font_main = TTF_OpenFont(font_paths[i], 22);
        font_small = TTF_OpenFont(font_paths[i], 16);
        if (font_main && font_small) break;
    }

    if (!font_main) {
        debugPrint("Failed to load font: %s\n", TTF_GetError());
    }

    // Load Category and Default Icons
    tex_cat_games = load_texture("cat_games.png");
    tex_cat_apps = load_texture("cat_apps.png");
    tex_cat_settings = load_texture("cat_settings.png");
    tex_default_game = load_texture("default_game.png");

    init_particles();

    return 0;
}

void ui_cleanup(void) {
    if (tex_cat_games) SDL_DestroyTexture(tex_cat_games);
    if (tex_cat_apps) SDL_DestroyTexture(tex_cat_apps);
    if (tex_cat_settings) SDL_DestroyTexture(tex_cat_settings);
    if (tex_default_game) SDL_DestroyTexture(tex_default_game);

    if (font_main) TTF_CloseFont(font_main);
    if (font_small) TTF_CloseFont(font_small);
    if (TTF_WasInit()) TTF_Quit();

    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
}

static void render_text_shadow(TTF_Font* f, const char* text, int x, int y, SDL_Color color, Uint8 alpha) {
    if (!f || !text || text[0] == '\0') return;

    // Soft drop shadow
    SDL_Color shadow_col = { 0, 0, 0, (Uint8)(alpha * 0.70f) };
    SDL_Surface* s_shadow = TTF_RenderText_Blended(f, text, shadow_col);
    if (s_shadow) {
        SDL_Texture* t_shadow = SDL_CreateTextureFromSurface(renderer, s_shadow);
        if (t_shadow) {
            SDL_SetTextureAlphaMod(t_shadow, (Uint8)(alpha * 0.70f));
            SDL_Rect dest = { x + 2, y + 2, s_shadow->w, s_shadow->h };
            SDL_RenderCopy(renderer, t_shadow, NULL, &dest);
            SDL_DestroyTexture(t_shadow);
        }
        SDL_FreeSurface(s_shadow);
    }

    // Foreground Text
    SDL_Surface* surface = TTF_RenderText_Blended(f, text, color);
    if (surface) {
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        if (texture) {
            SDL_SetTextureAlphaMod(texture, alpha);
            SDL_Rect dest = { x, y, surface->w, surface->h };
            SDL_RenderCopy(renderer, texture, NULL, &dest);
            SDL_DestroyTexture(texture);
        }
        SDL_FreeSurface(surface);
    }
}

static XMBTheme current_theme = THEME_PS3_OBSIDIAN;

typedef struct {
    Uint8 bg_top_r, bg_top_g, bg_top_b;
    Uint8 bg_bot_r, bg_bot_g, bg_bot_b;
    Uint8 w1_r, w1_g, w1_b;
    Uint8 w2_r, w2_g, w2_b;
    Uint8 w3_r, w3_g, w3_b;
    Uint8 p_r, p_g, p_b;
    Uint8 glow_r, glow_g, glow_b;
} ThemeColors;

static const ThemeColors theme_palettes[THEME_COUNT] = {
    // 0: PS3 Smoked Obsidian
    { 10, 15, 22,   22, 41, 56,   90, 180, 240,  100, 220, 180,  130, 150, 255,  210, 240, 255,  100, 200, 255 },
    // 1: Xbox Emerald
    { 8, 18, 10,    14, 48, 20,   60, 235, 90,   120, 255, 140,  30, 180, 70,    180, 255, 200,  80, 240, 100 },
    // 2: Cobalt Blue
    { 6, 12, 28,    12, 28, 65,   50, 140, 255,  80, 180, 255,   40, 100, 240,   190, 225, 255,  70, 170, 255 },
    // 3: Ruby Crimson
    { 24, 8, 12,    55, 14, 22,   240, 70, 80,   255, 140, 80,   210, 50, 70,    255, 210, 200,  255, 90, 100 },
    // 4: Cyberpunk Gold
    { 18, 8, 26,    42, 16, 60,   255, 200, 50,  255, 160, 40,   200, 120, 240,  255, 235, 170,  255, 215, 60 }
};

void ui_set_theme(XMBTheme theme) {
    if (theme < THEME_COUNT) {
        current_theme = theme;
    }
}

XMBTheme ui_get_theme(void) {
    return current_theme;
}

const char* ui_get_theme_name(XMBTheme theme) {
    switch (theme) {
        case THEME_PS3_OBSIDIAN: return "PS3 Smoked Obsidian";
        case THEME_XBOX_EMERALD: return "Xbox Emerald Green";
        case THEME_COBALT_BLUE: return "Cobalt Sapphire";
        case THEME_RUBY_CRIMSON: return "Ruby Crimson";
        case THEME_CYBERPUNK_GOLD: return "Cyberpunk Gold";
        default: return "Default";
    }
}

static void render_ps3_wave(void) {
    const ThemeColors* tc = &theme_palettes[current_theme];

    // 1. Vertical background gradient
    for (int y = 0; y < WINDOW_HEIGHT; y += 4) {
        float factor = (float)y / (float)WINDOW_HEIGHT;
        Uint8 r = (Uint8)(tc->bg_top_r + factor * (tc->bg_bot_r - tc->bg_top_r));
        Uint8 g = (Uint8)(tc->bg_top_g + factor * (tc->bg_bot_g - tc->bg_top_g));
        Uint8 b = (Uint8)(tc->bg_top_b + factor * (tc->bg_bot_b - tc->bg_top_b));
        SDL_Rect line = { 0, y, WINDOW_WIDTH, 4 };
        SDL_SetRenderDrawColor(renderer, r, g, b, 255);
        SDL_RenderFillRect(renderer, &line);
    }

    // 2. Animated PS3 Multi-Layered Harmonic Ribbons
    wave_time += 0.020f;

    struct WaveLayer {
        float base_y;
        float freq1, amp1, speed1;
        float freq2, amp2, speed2;
        Uint8 r, g, b, alpha;
        int thickness;
    } layers[3] = {
        { 250.0f, 0.007f, 38.0f, 0.7f, 0.014f, 20.0f, 1.1f, tc->w1_r, tc->w1_g, tc->w1_b, 55, 24 },
        { 280.0f, 0.005f, 50.0f, 0.4f, 0.010f, 28.0f, 0.8f, tc->w2_r, tc->w2_g, tc->w2_b, 45, 36 },
        { 305.0f, 0.008f, 34.0f, 0.9f, 0.016f, 16.0f, 1.4f, tc->w3_r, tc->w3_g, tc->w3_b, 35, 44 }
    };

    for (int w = 0; w < 3; w++) {
        int step = 8;
        for (int x = 0; x < WINDOW_WIDTH; x += step) {
            float t = wave_time;
            float y1 = layers[w].base_y
                     + sinf(x * layers[w].freq1 + t * layers[w].speed1) * layers[w].amp1
                     + cosf(x * layers[w].freq2 - t * layers[w].speed2) * layers[w].amp2;

            float x2 = (float)(x + step);
            float y2 = layers[w].base_y
                     + sinf(x2 * layers[w].freq1 + t * layers[w].speed1) * layers[w].amp1
                     + cosf(x2 * layers[w].freq2 - t * layers[w].speed2) * layers[w].amp2;

            for (int dy = -layers[w].thickness / 2; dy <= layers[w].thickness / 2; dy += 2) {
                float falloff = 1.0f - (float)abs(dy) / (float)(layers[w].thickness / 2);
                Uint8 a = (Uint8)(layers[w].alpha * falloff * falloff);
                SDL_SetRenderDrawColor(renderer, layers[w].r, layers[w].g, layers[w].b, a);
                SDL_RenderDrawLine(renderer, x, (int)(y1 + dy), (int)x2, (int)(y2 + dy));
            }
        }
    }

    // 3. Floating Sparkle Particles
    for (int i = 0; i < NUM_PARTICLES; i++) {
        particles[i].x += particles[i].speed;
        particles[i].y += sinf(wave_time + particles[i].x * 0.01f) * 0.25f;
        if (particles[i].x > WINDOW_WIDTH + 10) {
            particles[i].x = -10.0f;
            particles[i].y = (float)(rand() % WINDOW_HEIGHT);
        }

        SDL_Rect p_rect = { (int)particles[i].x, (int)particles[i].y, (int)particles[i].size, (int)particles[i].size };
        SDL_SetRenderDrawColor(renderer, tc->p_r, tc->p_g, tc->p_b, (Uint8)particles[i].alpha);
        SDL_RenderFillRect(renderer, &p_rect);
    }
}

static void render_hud(void) {
    // Elegant Top Header Line
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 25);
    SDL_RenderDrawLine(renderer, 40, 52, WINDOW_WIDTH - 40, 52);

    // Clock
    time_t rawtime;
    struct tm* timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);

    char time_str[64];
    if (timeinfo && timeinfo->tm_year > 70) {
        strftime(time_str, sizeof(time_str), "%I:%M %p", timeinfo);
    } else {
        snprintf(time_str, sizeof(time_str), "12:00 PM");
    }

    SDL_Color hud_color = { 230, 240, 255, 255 };
    render_text_shadow(font_small, time_str, WINDOW_WIDTH - 115, 24, hud_color, 210);

    // Dashboard Branding
    render_text_shadow(font_small, "Xbox XMB", 45, 24, hud_color, 160);
}

void ui_render(XMBCategory current_category, XMBNode* items, int item_count, int selected_index, int nav_depth, const char* breadcrumb) {
    // 1. Initialize Animation state on first run
    float target_cat_x = (float)current_category;
    float target_item_y = (float)selected_index;
    float target_cat_elev = (item_count > 0) ? 1.0f : 0.0f;

    if (!anim_initialized) {
        anim_cat_x = target_cat_x;
        anim_item_y = target_item_y;
        anim_cat_elev = target_cat_elev;
        anim_initialized = 1;
    } else {
        // Continuous Exponential Decay Smoothing (Lerp)
        anim_cat_x += (target_cat_x - anim_cat_x) * 0.18f;
        anim_item_y += (target_item_y - anim_item_y) * 0.22f;
        anim_cat_elev += (target_cat_elev - anim_cat_elev) * 0.18f;
    }

    // 2. Render Procedural Background
    render_ps3_wave();

    // 3. Render Top HUD
    render_hud();

    // Breadcrumb path for nested submenus
    if (nav_depth > 0 && breadcrumb && breadcrumb[0] != '\0') {
        char bc_text[128];
        snprintf(bc_text, sizeof(bc_text), "< %s", breadcrumb);
        SDL_Color bc_col = { 130, 200, 255, 255 };
        render_text_shadow(font_small, bc_text, 50, 68, bc_col, 200);
    }

    // 4. Coordinate Constants
    const int origin_x = 210;
    const int origin_y = 240;
    const int cat_spacing = 130;
    const int item_spacing = 62;

    SDL_Color white = { 255, 255, 255, 255 };
    SDL_Color silver = { 190, 210, 230, 255 };
    SDL_Color dim_text = { 140, 160, 180, 255 };
    SDL_Color glow_blue = { 100, 200, 255, 255 };

    const char* cat_names[] = { "Games", "Apps", "Settings" };
    SDL_Texture* cat_textures[] = { tex_cat_games, tex_cat_apps, tex_cat_settings };

    // 5. Render Horizontal Category Row (dimmed if inside a deep sub-menu)
    float cat_dim_mult = (nav_depth > 0) ? 0.45f : 1.0f;

    for (int c = 0; c < CATEGORY_COUNT; c++) {
        float rel_c = (float)c - anim_cat_x;
        float x = (float)origin_x + rel_c * cat_spacing;
        
        float lift = (1.0f - fminf(1.0f, fabsf(rel_c))) * (anim_cat_elev * 85.0f);
        float y = (float)origin_y - lift;

        float dist = fabsf(rel_c);
        float alpha_factor = fmaxf(0.0f, 1.0f - dist * 0.60f);
        Uint8 alpha = (Uint8)((90 + alpha_factor * 165) * cat_dim_mult);

        int icon_size = (int)(38.0f + alpha_factor * 12.0f);
        SDL_Rect dest = { (int)(x - icon_size / 2), (int)(y - icon_size / 2), icon_size, icon_size };

        if (cat_textures[c]) {
            SDL_SetTextureAlphaMod(cat_textures[c], alpha);
            SDL_RenderCopy(renderer, cat_textures[c], NULL, &dest);
        } else {
            SDL_SetRenderDrawColor(renderer, 220, 235, 255, alpha);
            SDL_RenderFillRect(renderer, &dest);
        }

        if (dist < 0.8f && nav_depth == 0) {
            Uint8 text_alpha = (Uint8)((1.0f - dist / 0.8f) * 255);
            render_text_shadow(font_small, cat_names[c], (int)(x - 22), (int)(y + icon_size / 2 + 6), white, text_alpha);
        }
    }

    // 6. Render Vertical Items Column for Active Node List
    if (item_count > 0 && items != NULL) {
        for (int i = 0; i < item_count; i++) {
            float rel_i = (float)i - anim_item_y;
            float y = (float)origin_y + rel_i * item_spacing;

            if (y > 70 && y < WINDOW_HEIGHT + 40) {
                float dist = fabsf(rel_i);
                float focus = fmaxf(0.0f, 1.0f - dist * 0.55f);
                Uint8 alpha = (Uint8)(70 + focus * 185);

                int icon_size = (int)(30.0f + focus * 12.0f);
                SDL_Rect dest = { origin_x - icon_size / 2, (int)(y - icon_size / 2), icon_size, icon_size };

                // Glow Ring on Selected Item
                if (dist < 0.4f) {
                    float glow_strength = 1.0f - (dist / 0.4f);
                    SDL_Rect glow1 = { dest.x - 4, dest.y - 4, dest.w + 8, dest.h + 8 };
                    SDL_Rect glow2 = { dest.x - 2, dest.y - 2, dest.w + 4, dest.h + 4 };
                    SDL_SetRenderDrawColor(renderer, glow_blue.r, glow_blue.g, glow_blue.b, (Uint8)(glow_strength * 70));
                    SDL_RenderDrawRect(renderer, &glow1);
                    SDL_SetRenderDrawColor(renderer, 220, 245, 255, (Uint8)(glow_strength * 120));
                    SDL_RenderDrawRect(renderer, &glow2);
                }

                // Choose icon based on custom artwork, category, or node type
                SDL_Texture* item_tex = NULL;
                if (items[i].icon_path[0] != '\0') {
                    item_tex = get_custom_icon(items[i].icon_path);
                }

                if (!item_tex) {
                    if (items[i].type == NODE_TYPE_SUBMENU) {
                        item_tex = tex_cat_apps;
                    } else if (items[i].type == NODE_TYPE_INFO) {
                        item_tex = tex_cat_settings;
                    } else {
                        item_tex = tex_default_game;
                    }
                }

                if (item_tex) {
                    SDL_SetTextureAlphaMod(item_tex, alpha);
                    SDL_RenderCopy(renderer, item_tex, NULL, &dest);
                } else {
                    SDL_SetRenderDrawColor(renderer, 180, 200, 225, alpha);
                    SDL_RenderFillRect(renderer, &dest);
                }

                // Title & Subtitle
                SDL_Color title_col = (dist < 0.4f) ? white : silver;
                render_text_shadow(font_main, items[i].title, origin_x + 36, (int)(y - 14), title_col, alpha);

                if (items[i].subtitle[0] != '\0') {
                    render_text_shadow(font_small, items[i].subtitle, origin_x + 38, (int)(y + 10), dim_text, (Uint8)(alpha * 0.85f));
                }
            }
        }
    } else {
        render_text_shadow(font_main, "No items found", origin_x + 38, origin_y - 12, silver, 140);
    }

    SDL_RenderPresent(renderer);
}
