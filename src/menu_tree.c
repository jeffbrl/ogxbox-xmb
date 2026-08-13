#include "menu_tree.h"
#include "xbe_scanner.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <hal/xbox.h>
#include <hal/video.h>
#include <xboxkrnl/xboxkrnl.h>

// Static storage for hierarchical nodes
static XMBNode settings_theme_children[5];
static XMBNode settings_sysinfo_children[5];
static XMBNode settings_video_children[5];
static XMBNode settings_audio_children[3];
static XMBNode settings_network_children[3];
static XMBNode settings_root_children[5];

static XMBNode apps_utils_children[3];
static XMBNode apps_storage_children[4];
static XMBNode apps_root_children[2];

static XMBNode games_root_children[64];

void menu_tree_refresh_system_info(void) {
    // 1. Dynamic System Memory Query
    MM_STATISTICS mm_stats;
    memset(&mm_stats, 0, sizeof(mm_stats));
    mm_stats.Length = sizeof(MM_STATISTICS);
    
    if (NT_SUCCESS(MmQueryStatistics(&mm_stats))) {
        unsigned long total_mb = (mm_stats.TotalPhysicalPages * 4) / 1024;
        unsigned long free_mb = (mm_stats.AvailablePages * 4) / 1024;
        snprintf(settings_sysinfo_children[1].subtitle, 64, "%lu MB Total / %lu MB Free", total_mb, free_mb);
    } else {
        snprintf(settings_sysinfo_children[1].subtitle, 64, "64 MB DDR RAM");
    }

    // 2. Dynamic Kernel Version Query
    if (XboxKrnlVersion.Major != 0 || XboxKrnlVersion.Build != 0) {
        snprintf(settings_sysinfo_children[2].subtitle, 64, "Kernel v%u.%u.%u.%u",
            XboxKrnlVersion.Major, XboxKrnlVersion.Minor, XboxKrnlVersion.Build, XboxKrnlVersion.Qfe);
    } else {
        snprintf(settings_sysinfo_children[2].subtitle, 64, "Xbox Kernel 1.0 (Retail/Debug)");
    }

    // 3. Dynamic Video Mode & Hardware Query
    VIDEO_MODE vm = XVideoGetMode();
    DWORD dwEnc = XVideoGetEncoderSettings();

    // Active Display Output
    snprintf(settings_video_children[0].title, 64, "Active Video Mode");
    snprintf(settings_video_children[0].subtitle, 64, "%dx%d @ %dHz (%dbpp)",
        vm.width, vm.height, vm.refresh ? vm.refresh : 60, vm.bpp ? vm.bpp : 32);

    // 720p HD Status
    snprintf(settings_video_children[1].title, 64, "720p HD Mode");
    snprintf(settings_video_children[1].subtitle, 64, (dwEnc & VIDEO_MODE_720P) ? "Enabled in EEPROM" : "Disabled");

    // 480p Progressive Status
    snprintf(settings_video_children[2].title, 64, "480p Progressive");
    snprintf(settings_video_children[2].subtitle, 64, (dwEnc & VIDEO_MODE_480P) ? "Enabled in EEPROM" : "Disabled");

    // 1080i High-Def Status
    snprintf(settings_video_children[3].title, 64, "1080i High-Def");
    snprintf(settings_video_children[3].subtitle, 64, (dwEnc & VIDEO_MODE_1080I) ? "Enabled in EEPROM" : "Disabled");

    // Widescreen Status
    snprintf(settings_video_children[4].title, 64, "Aspect Ratio");
    snprintf(settings_video_children[4].subtitle, 64, (dwEnc & VIDEO_WIDESCREEN) ? "16:9 Widescreen" : "4:3 Standard");

    // 4. Dynamic Hard Drive Partition Free Space
    const char* drive_letters[] = { "C:\\", "E:\\", "F:\\", "G:\\" };
    const char* drive_labels[] = { "Drive C: (System)", "Drive E: (Dashboard & Games)", "Drive F: (Extended)", "Drive G: (Extended 2)" };

    for (int d = 0; d < 4; d++) {
        ULARGE_INTEGER free_bytes, total_bytes, total_free_bytes;
        strncpy(apps_storage_children[d].title, drive_labels[d], 64);
        if (GetDiskFreeSpaceExA(drive_letters[d], &free_bytes, &total_bytes, &total_free_bytes)) {
            unsigned long total_mb = (unsigned long)(total_bytes.QuadPart / (1024 * 1024));
            unsigned long free_mb = (unsigned long)(free_bytes.QuadPart / (1024 * 1024));
            snprintf(apps_storage_children[d].subtitle, 64, "%lu MB Total / %lu MB Free", total_mb, free_mb);
        } else {
            snprintf(apps_storage_children[d].subtitle, 64, "Not mounted");
        }
        apps_storage_children[d].type = NODE_TYPE_INFO;
    }
}

void menu_tree_init(XMBNode* root_categories, int max_games) {
    (void)max_games;

    // ==========================================
    // 1. INFO CATEGORY TREE
    // ==========================================
    
    // Theme Customization Submenu
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

    // System Information Submenu
    strncpy(settings_sysinfo_children[0].title, "Console Hardware", 64);
    strncpy(settings_sysinfo_children[0].subtitle, "Original Xbox (x86 Coppermine 733MHz)", 64);
    settings_sysinfo_children[0].type = NODE_TYPE_INFO;

    strncpy(settings_sysinfo_children[1].title, "System Memory", 64);
    strncpy(settings_sysinfo_children[1].subtitle, "Detecting RAM...", 64);
    settings_sysinfo_children[1].type = NODE_TYPE_INFO;

    strncpy(settings_sysinfo_children[2].title, "BIOS / Kernel", 64);
    strncpy(settings_sysinfo_children[2].subtitle, "Querying Kernel...", 64);
    settings_sysinfo_children[2].type = NODE_TYPE_INFO;

    strncpy(settings_sysinfo_children[3].title, "Video Encoder", 64);
    strncpy(settings_sysinfo_children[3].subtitle, "Conexant / Focus CX25871 HDTV", 64);
    settings_sysinfo_children[3].type = NODE_TYPE_INFO;

    strncpy(settings_sysinfo_children[4].title, "Dashboard Version", 64);
    strncpy(settings_sysinfo_children[4].subtitle, "OGX-XMB v1.0 (Sony PS3 Style)", 64);
    settings_sysinfo_children[4].type = NODE_TYPE_INFO;

    // Display Settings Submenu
    for (int v = 0; v < 5; v++) {
        settings_video_children[v].type = NODE_TYPE_INFO;
    }

    // Audio Settings Submenu
    strncpy(settings_audio_children[0].title, "Digital Output", 64);
    strncpy(settings_audio_children[0].subtitle, "Dolby Digital 5.1 (Optical)", 64);
    settings_audio_children[0].type = NODE_TYPE_ACTION;

    strncpy(settings_audio_children[1].title, "DTS Surround", 64);
    strncpy(settings_audio_children[1].subtitle, "Enabled", 64);
    settings_audio_children[1].type = NODE_TYPE_ACTION;

    strncpy(settings_audio_children[2].title, "UI Navigation Sounds", 64);
    strncpy(settings_audio_children[2].subtitle, "Enabled (100% Volume)", 64);
    settings_audio_children[2].type = NODE_TYPE_ACTION;

    // Network Settings Submenu
    strncpy(settings_network_children[0].title, "Network Protocol", 64);
    strncpy(settings_network_children[0].subtitle, "DHCP (Automatic)", 64);
    settings_network_children[0].type = NODE_TYPE_INFO;

    strncpy(settings_network_children[1].title, "IP Address", 64);
    strncpy(settings_network_children[1].subtitle, "192.168.0.100", 64);
    settings_network_children[1].type = NODE_TYPE_INFO;

    strncpy(settings_network_children[2].title, "Gateway / Subnet", 64);
    strncpy(settings_network_children[2].subtitle, "192.168.0.1 / 255.255.255.0", 64);
    settings_network_children[2].type = NODE_TYPE_INFO;

    // Info Root Menu
    strncpy(settings_root_children[0].title, "Theme Settings", 64);
    strncpy(settings_root_children[0].subtitle, "PS3 Wave & Accent Color Palettes", 64);
    settings_root_children[0].type = NODE_TYPE_SUBMENU;
    settings_root_children[0].children = settings_theme_children;
    settings_root_children[0].child_count = 5;

    strncpy(settings_root_children[1].title, "System Information", 64);
    strncpy(settings_root_children[1].subtitle, "Kernel, Memory, Hardware specs", 64);
    settings_root_children[1].type = NODE_TYPE_SUBMENU;
    settings_root_children[1].children = settings_sysinfo_children;
    settings_root_children[1].child_count = 5;

    strncpy(settings_root_children[2].title, "Display Settings", 64);
    strncpy(settings_root_children[2].subtitle, "Resolution, Widescreen 16:9", 64);
    settings_root_children[2].type = NODE_TYPE_SUBMENU;
    settings_root_children[2].children = settings_video_children;
    settings_root_children[2].child_count = 5;

    strncpy(settings_root_children[3].title, "Audio Settings", 64);
    strncpy(settings_root_children[3].subtitle, "Dolby Digital 5.1 & UI sounds", 64);
    settings_root_children[3].type = NODE_TYPE_SUBMENU;
    settings_root_children[3].children = settings_audio_children;
    settings_root_children[3].child_count = 3;

    strncpy(settings_root_children[4].title, "Network Settings", 64);
    strncpy(settings_root_children[4].subtitle, "Ethernet, IP address, Gateway", 64);
    settings_root_children[4].type = NODE_TYPE_SUBMENU;
    settings_root_children[4].children = settings_network_children;
    settings_root_children[4].child_count = 3;

    root_categories[CATEGORY_INFO].children = settings_root_children;
    root_categories[CATEGORY_INFO].child_count = 5;

    // ==========================================
    // 2. APPS CATEGORY TREE
    // ==========================================
    strncpy(apps_utils_children[0].title, "Reboot Console", 64);
    strncpy(apps_utils_children[0].subtitle, "Soft-reset Xbox system", 64);
    apps_utils_children[0].type = NODE_TYPE_ACTION;

    strncpy(apps_utils_children[1].title, "Power Off", 64);
    strncpy(apps_utils_children[1].subtitle, "Shutdown hardware", 64);
    apps_utils_children[1].type = NODE_TYPE_ACTION;

    strncpy(apps_utils_children[2].title, "Launch MS Dashboard", 64);
    strncpy(apps_utils_children[2].subtitle, "C:\\xboxdash.xbe", 64);
    strncpy(apps_utils_children[2].path, "C:\\xboxdash.xbe", 256);
    apps_utils_children[2].type = NODE_TYPE_LAUNCH;

    strncpy(apps_root_children[0].title, "System Utilities", 64);
    strncpy(apps_root_children[0].subtitle, "Reboot, Power, Stock Dash", 64);
    apps_root_children[0].type = NODE_TYPE_SUBMENU;
    apps_root_children[0].children = apps_utils_children;
    apps_root_children[0].child_count = 3;

    strncpy(apps_root_children[1].title, "Hard Drive Partitions", 64);
    strncpy(apps_root_children[1].subtitle, "Live drive capacity and free space", 64);
    apps_root_children[1].type = NODE_TYPE_SUBMENU;
    apps_root_children[1].children = apps_storage_children;
    apps_root_children[1].child_count = 4;

    root_categories[CATEGORY_APPS].children = apps_root_children;
    root_categories[CATEGORY_APPS].child_count = 2;

    // ==========================================
    // 3. GAMES CATEGORY TREE
    // ==========================================
    static XMBItem scanned_games[64];
    int game_count = xbe_scanner_get_items(CATEGORY_GAMES, scanned_games, 64);
    for (int i = 0; i < game_count; i++) {
        strncpy(games_root_children[i].title, scanned_games[i].title, 64);
        strncpy(games_root_children[i].path, scanned_games[i].path, 256);
        strncpy(games_root_children[i].icon_path, scanned_games[i].icon_path, 256);
        strncpy(games_root_children[i].subtitle, "Original Xbox Title", 64);
        games_root_children[i].type = NODE_TYPE_LAUNCH;
    }

    root_categories[CATEGORY_GAMES].children = games_root_children;
    root_categories[CATEGORY_GAMES].child_count = game_count;

    // Populate live statistics
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
