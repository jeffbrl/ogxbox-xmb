#include "menu_tree.h"
#include "xbe_scanner.h"
#include "xmb_config.h"
#include "http_client.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <hal/xbox.h>
#include <hal/video.h>
#include <xboxkrnl/xboxkrnl.h>

// Static storage for Info submenus
static XMBNode info_hardware_children[5];
static XMBNode info_storage_children[4];
static XMBNode info_root_children[3];

// Static storage for Settings submenus
static XMBNode settings_theme_children[5];
static XMBNode settings_video_children[5];
static XMBNode settings_audio_children[3];
static XMBNode settings_network_children[2];
static XMBNode settings_root_children[4];

// Static storage for Apps submenus
static XMBNode apps_utils_children[3];
static XMBNode apps_root_children[1];

// Static storage for Games (expanded for Xemu 128MB memory budget)
static XMBNode games_root_children[MAX_GAMES];

void menu_tree_refresh_system_info(void) {
    // 1. Safe Dynamic System Memory Query via Kernel MM API
    MM_STATISTICS mm_stats;
    memset(&mm_stats, 0, sizeof(mm_stats));
    mm_stats.Length = sizeof(MM_STATISTICS);
    
    if (NT_SUCCESS(MmQueryStatistics(&mm_stats))) {
        unsigned long total_mb = (mm_stats.TotalPhysicalPages * 4) / 1024;
        unsigned long free_mb = (mm_stats.AvailablePages * 4) / 1024;
        snprintf(info_hardware_children[1].subtitle, 64, "%lu MB Total (%lu MB Free)", total_mb, free_mb);
    } else {
        snprintf(info_hardware_children[1].subtitle, 64, "128 MB Unified RAM");
    }

    // 2. Dynamic Kernel Version Query
    if (XboxKrnlVersion.Major != 0 || XboxKrnlVersion.Build != 0) {
        snprintf(info_hardware_children[2].subtitle, 64, "Kernel v%u.%u.%u.%u",
            XboxKrnlVersion.Major, XboxKrnlVersion.Minor, XboxKrnlVersion.Build, XboxKrnlVersion.Qfe);
    } else {
        snprintf(info_hardware_children[2].subtitle, 64, "Xbox Kernel (Homebrew/Retail)");
    }

    // 3. Dynamic Video Mode Query
    VIDEO_MODE vm = XVideoGetMode();
    DWORD dwEnc = XVideoGetEncoderSettings();

    snprintf(info_hardware_children[3].title, 64, "Active Video Output");
    snprintf(info_hardware_children[3].subtitle, 64, "%dx%d @ %dHz (%dbpp)",
        vm.width ? vm.width : 640,
        vm.height ? vm.height : 480,
        vm.refresh ? vm.refresh : 60,
        vm.bpp ? vm.bpp : 32);

    // 4. Dynamic Hard Drive Partition Free Space
    const char* drive_letters[] = { "C:\\", "E:\\", "F:\\", "G:\\" };
    const char* drive_labels[] = { "Drive C: (System)", "Drive E: (Dashboard & Games)", "Drive F: (Extended)", "Drive G: (Extended 2)" };

    for (int d = 0; d < 4; d++) {
        ULARGE_INTEGER free_bytes, total_bytes, total_free_bytes;
        strncpy(info_storage_children[d].title, drive_labels[d], 64);
        if (GetDiskFreeSpaceExA(drive_letters[d], &free_bytes, &total_bytes, &total_free_bytes)) {
            unsigned long total_mb = (unsigned long)(total_bytes.QuadPart / (1024 * 1024));
            unsigned long free_mb = (unsigned long)(free_bytes.QuadPart / (1024 * 1024));
            snprintf(info_storage_children[d].subtitle, 64, "%lu MB Total / %lu MB Free", total_mb, free_mb);
        } else {
            snprintf(info_storage_children[d].subtitle, 64, "Not mounted");
        }
        info_storage_children[d].type = NODE_TYPE_INFO;
    }

    // 5. Video Settings Status
    snprintf(settings_video_children[0].title, 64, "720p HD Mode");
    snprintf(settings_video_children[0].subtitle, 64, (dwEnc & VIDEO_MODE_720P) ? "Enabled" : "Disabled");

    snprintf(settings_video_children[1].title, 64, "480p Progressive");
    snprintf(settings_video_children[1].subtitle, 64, (dwEnc & VIDEO_MODE_480P) ? "Enabled" : "Disabled");

    snprintf(settings_video_children[2].title, 64, "1080i High-Def");
    snprintf(settings_video_children[2].subtitle, 64, (dwEnc & VIDEO_MODE_1080I) ? "Enabled" : "Disabled");

    snprintf(settings_video_children[3].title, 64, "Aspect Ratio");
    snprintf(settings_video_children[3].subtitle, 64, (dwEnc & VIDEO_WIDESCREEN) ? "16:9 Widescreen" : "4:3 Standard");

    snprintf(settings_video_children[4].title, 64, "Display Refresh");
    snprintf(settings_video_children[4].subtitle, 64, "60Hz NTSC");

    // 6. Live Network Status Refresh
    snprintf(info_root_children[2].subtitle, 64, "%s", net_get_ip_str());
}

void menu_tree_init(XMBNode* root_categories, int max_games) {
    (void)max_games;

    // ==========================================
    // 1. GAMES CATEGORY TREE
    // ==========================================
    static XMBItem scanned_games[64];
    int game_count = xbe_scanner_get_items(CATEGORY_GAMES, scanned_games, 64);
    for (int i = 0; i < game_count; i++) {
        strncpy(games_root_children[i].title, scanned_games[i].title, 64);
        strncpy(games_root_children[i].path, scanned_games[i].path, 256);
        strncpy(games_root_children[i].icon_path, scanned_games[i].icon_path, 256);
        games_root_children[i].subtitle[0] = '\0';
        games_root_children[i].type = NODE_TYPE_LAUNCH;
    }

    root_categories[CATEGORY_GAMES].children = games_root_children;
    root_categories[CATEGORY_GAMES].child_count = game_count;

    // ==========================================
    // 2. APPS CATEGORY TREE
    // ==========================================
    strncpy(apps_utils_children[0].title, "Reboot Console", 64);
    strncpy(apps_utils_children[0].subtitle, "Soft-reset Xbox system", 64);
    apps_utils_children[0].type = NODE_TYPE_ACTION;

    strncpy(apps_utils_children[1].title, "Exit to Desktop", 64);
    strncpy(apps_utils_children[1].subtitle, "Quit Xemu and return to host", 64);
    apps_utils_children[1].type = NODE_TYPE_ACTION;

    strncpy(apps_utils_children[2].title, "Launch MS Dashboard", 64);
    strncpy(apps_utils_children[2].subtitle, "C:\\msxboxdash.xbe", 64);
    strncpy(apps_utils_children[2].path, "C:\\msxboxdash.xbe", 256);
    apps_utils_children[2].type = NODE_TYPE_LAUNCH;

    strncpy(apps_root_children[0].title, "System Utilities", 64);
    strncpy(apps_root_children[0].subtitle, "Reboot, Exit Xemu, Stock Dash", 64);
    apps_root_children[0].type = NODE_TYPE_SUBMENU;
    apps_root_children[0].children = apps_utils_children;
    apps_root_children[0].child_count = 3;

    root_categories[CATEGORY_APPS].children = apps_root_children;
    root_categories[CATEGORY_APPS].child_count = 1;

    // ==========================================
    // 3. INFO CATEGORY TREE (Read-Only Stats)
    // ==========================================
    strncpy(info_hardware_children[0].title, "Console Architecture", 64);
    strncpy(info_hardware_children[0].subtitle, "Intel Pentium III Coppermine (733MHz)", 64);
    info_hardware_children[0].type = NODE_TYPE_INFO;

    strncpy(info_hardware_children[1].title, "System Memory (RAM)", 64);
    strncpy(info_hardware_children[1].subtitle, "Detecting memory...", 64);
    info_hardware_children[1].type = NODE_TYPE_INFO;

    strncpy(info_hardware_children[2].title, "BIOS / Kernel", 64);
    strncpy(info_hardware_children[2].subtitle, "Reading version...", 64);
    info_hardware_children[2].type = NODE_TYPE_INFO;

    strncpy(info_hardware_children[3].title, "Active Video Output", 64);
    strncpy(info_hardware_children[3].subtitle, "640x480 @ 60Hz", 64);
    info_hardware_children[3].type = NODE_TYPE_INFO;

    strncpy(info_hardware_children[4].title, "Dashboard Version", 64);
    strncpy(info_hardware_children[4].subtitle, "OGX-XMB v1.0 (Sony PS3 Cross Media Bar)", 64);
    info_hardware_children[4].type = NODE_TYPE_INFO;

    strncpy(info_root_children[0].title, "Hardware & Kernel", 64);
    strncpy(info_root_children[0].subtitle, "CPU, RAM, GPU, and BIOS diagnostics", 64);
    info_root_children[0].type = NODE_TYPE_SUBMENU;
    info_root_children[0].children = info_hardware_children;
    info_root_children[0].child_count = 5;

    strncpy(info_root_children[1].title, "Storage Partitions", 64);
    strncpy(info_root_children[1].subtitle, "Live capacity and free disk space", 64);
    info_root_children[1].type = NODE_TYPE_SUBMENU;
    info_root_children[1].children = info_storage_children;
    info_root_children[1].child_count = 4;

    strncpy(info_root_children[2].title, "Network Status", 64);
    strncpy(info_root_children[2].subtitle, "DHCP Auto (192.168.0.100)", 64);
    info_root_children[2].type = NODE_TYPE_INFO;

    root_categories[CATEGORY_INFO].children = info_root_children;
    root_categories[CATEGORY_INFO].child_count = 3;

    // ==========================================
    // 4. SETTINGS CATEGORY TREE (Configurable)
    // ==========================================
    
    // Theme Settings
    strncpy(settings_theme_children[0].title, "Cycle Wave Theme", 64);
    strncpy(settings_theme_children[0].subtitle, "PS3 Obsidian / Xbox Green / Blue / Ruby", 64);
    settings_theme_children[0].type = NODE_TYPE_THEME_CYCLE;

    strncpy(settings_theme_children[1].title, "Theme: Xbox Emerald", 64);
    strncpy(settings_theme_children[1].subtitle, "Signature Xbox Green & Jade Wave", 64);
    settings_theme_children[1].type = NODE_TYPE_ACTION;

    strncpy(settings_theme_children[2].title, "Theme: PS3 Obsidian", 64);
    strncpy(settings_theme_children[2].subtitle, "Smoked Charcoal & Ice Blue Wave", 64);
    settings_theme_children[2].type = NODE_TYPE_ACTION;

    strncpy(settings_theme_children[3].title, "Theme: Cobalt Sapphire", 64);
    strncpy(settings_theme_children[3].subtitle, "Electric Ocean Blue Wave", 64);
    settings_theme_children[3].type = NODE_TYPE_ACTION;

    strncpy(settings_theme_children[4].title, "Theme: Ruby Crimson", 64);
    strncpy(settings_theme_children[4].subtitle, "Warm Scarlet & Amber Wave", 64);
    settings_theme_children[4].type = NODE_TYPE_ACTION;

    for (int s = 0; s < 5; s++) {
        settings_video_children[s].type = NODE_TYPE_ACTION;
    }

    // Audio Settings
    strncpy(settings_audio_children[0].title, "Digital Output", 64);
    strncpy(settings_audio_children[0].subtitle, "Dolby Digital 5.1 (Optical)", 64);
    settings_audio_children[0].type = NODE_TYPE_ACTION;

    strncpy(settings_audio_children[1].title, "DTS Surround", 64);
    strncpy(settings_audio_children[1].subtitle, "Enabled", 64);
    settings_audio_children[1].type = NODE_TYPE_ACTION;

    strncpy(settings_audio_children[2].title, "UI Navigation Sounds", 64);
    strncpy(settings_audio_children[2].subtitle, "Enabled (100% Volume)", 64);
    settings_audio_children[2].type = NODE_TYPE_ACTION;

    // Network Settings
    strncpy(settings_network_children[0].title, "IP Assignment", 64);
    strncpy(settings_network_children[0].subtitle, "DHCP (Automatic)", 64);
    settings_network_children[0].type = NODE_TYPE_ACTION;

    strncpy(settings_network_children[1].title, "DNS Configuration", 64);
    strncpy(settings_network_children[1].subtitle, "Automatic from Router", 64);
    settings_network_children[1].type = NODE_TYPE_ACTION;

    // Settings Root Nodes
    strncpy(settings_root_children[0].title, "Theme & Appearance", 64);
    strncpy(settings_root_children[0].subtitle, "PS3 Wave & Color Palette presets", 64);
    settings_root_children[0].type = NODE_TYPE_SUBMENU;
    settings_root_children[0].children = settings_theme_children;
    settings_root_children[0].child_count = 5;

    strncpy(settings_root_children[1].title, "Display Settings", 64);
    strncpy(settings_root_children[1].subtitle, "Resolution, 720p, Widescreen 16:9", 64);
    settings_root_children[1].type = NODE_TYPE_SUBMENU;
    settings_root_children[1].children = settings_video_children;
    settings_root_children[1].child_count = 5;

    strncpy(settings_root_children[2].title, "Audio Settings", 64);
    strncpy(settings_root_children[2].subtitle, "Dolby Digital 5.1 & UI sounds", 64);
    settings_root_children[2].type = NODE_TYPE_SUBMENU;
    settings_root_children[2].children = settings_audio_children;
    settings_root_children[2].child_count = 3;

    strncpy(settings_root_children[3].title, "Network Settings", 64);
    strncpy(settings_root_children[3].subtitle, "DHCP, Static IP, DNS", 64);
    settings_root_children[3].type = NODE_TYPE_SUBMENU;
    settings_root_children[3].children = settings_network_children;
    settings_root_children[3].child_count = 2;

    root_categories[CATEGORY_SETTINGS].children = settings_root_children;
    root_categories[CATEGORY_SETTINGS].child_count = 4;

    // Refresh live stats safely
    menu_tree_refresh_system_info();
}

XMBNode* menu_tree_get_active_list(XMBNavContext* ctx, XMBCategory cat, XMBNode* root_categories, int* out_count) {
    if (!ctx || ctx->depth == 0) {
        *out_count = root_categories[cat].child_count;
        return root_categories[cat].children;
    }

    XMBNode* current = ctx->stack[ctx->depth - 1];
    *out_count = current->child_count;
    return current->children;
}

void menu_tree_background_scraper_tick(XMBNode* root_categories) {
    if (!net_is_connected()) return;

    int game_count = root_categories[CATEGORY_GAMES].child_count;
    XMBNode* games = root_categories[CATEGORY_GAMES].children;
    if (!games || game_count <= 0) return;

    // Scan for first game missing local artwork
    for (int i = 0; i < game_count; i++) {
        if (games[i].icon_path[0] == '\0' && games[i].path[0] != '\0') {
            // games[i].path is e.g. "E:\Games\Halo - Combat Evolved (USA)\default.xbe"
            char game_dir[256];
            strncpy(game_dir, games[i].path, sizeof(game_dir));
            char* last_slash = strrchr(game_dir, '\\');
            if (last_slash) *last_slash = '\0'; // "E:\Games\Halo - Combat Evolved (USA)"

            char* folder_name = strrchr(game_dir, '\\');
            const char* name_to_scrape = (folder_name && *(folder_name + 1) != '\0') ? (folder_name + 1) : games[i].title;

            int res = scrape_game_cover(name_to_scrape, game_dir, games[i].icon_path, sizeof(games[i].icon_path));
            if (res != 0) {
                // Mark as attempted so we don't retry endlessly
                strncpy(games[i].icon_path, "NONE", sizeof(games[i].icon_path));
            }
            break; // Process at most one per tick
        }
    }
}
