#ifndef UI_RENDERER_H
#define UI_RENDERER_H

#include <SDL.h>
#include "xmb_types.h"

int ui_init(void);
void ui_render(XMBCategory current_category, XMBNode* items, int item_count, int selected_index, int nav_depth, const char* breadcrumb);
void ui_cleanup(void);

#endif // UI_RENDERER_H
