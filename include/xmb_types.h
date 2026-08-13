#ifndef XMB_TYPES_H
#define XMB_TYPES_H

typedef enum {
    CATEGORY_GAMES = 0,
    CATEGORY_APPS,
    CATEGORY_SETTINGS,
    CATEGORY_COUNT
} XMBCategory;

typedef enum {
    NODE_TYPE_LAUNCH = 0,
    NODE_TYPE_SUBMENU,
    NODE_TYPE_INFO,
    NODE_TYPE_ACTION,
    NODE_TYPE_THEME_CYCLE
} XMBNodeType;

typedef enum {
    THEME_PS3_OBSIDIAN = 0,
    THEME_XBOX_EMERALD,
    THEME_COBALT_BLUE,
    THEME_RUBY_CRIMSON,
    THEME_CYBERPUNK_GOLD,
    THEME_COUNT
} XMBTheme;

typedef struct XMBNode {
    char title[64];
    char subtitle[64];
    char path[256];
    char icon_path[256];
    XMBNodeType type;
    
    struct XMBNode* children;
    int child_count;
    int selected_child;
} XMBNode;

typedef struct {
    char title[64];
    char path[256];
    char icon_path[256];
} XMBItem;

#endif // XMB_TYPES_H
