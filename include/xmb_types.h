#ifndef XMB_TYPES_H
#define XMB_TYPES_H

typedef enum {
    CATEGORY_GAMES,
    CATEGORY_APPS,
    CATEGORY_SETTINGS,
    CATEGORY_COUNT
} XMBCategory;

typedef struct {
    char title[64];
    char path[256];
    char icon_path[256];
} XMBItem;

#endif // XMB_TYPES_H
