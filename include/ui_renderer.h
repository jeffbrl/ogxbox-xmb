#ifndef UI_RENDERER_H
#define UI_RENDERER_H

#include <SDL.h>
#include "xmb_types.h"

int ui_init(void);
void ui_render(XMBCategory current_category, XMBItem* items, int item_count, int selected_index);
void ui_cleanup(void);

#endif // UI_RENDERER_H
