#ifndef MENU_TREE_H
#define MENU_TREE_H

#include "xmb_types.h"

#define MAX_NAV_DEPTH 4

typedef struct {
    XMBNode* stack[MAX_NAV_DEPTH];
    int depth;
} XMBNavContext;

void menu_tree_init(XMBNode* root_categories, int max_games);
void menu_tree_refresh_system_info(void);
void menu_tree_background_scraper_tick(XMBNode* root_categories);
XMBNode* menu_tree_get_active_list(XMBNavContext* ctx, XMBCategory cat, XMBNode* root_categories, int* out_count);

#endif // MENU_TREE_H
