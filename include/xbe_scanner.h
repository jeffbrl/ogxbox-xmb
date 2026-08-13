#ifndef XBE_SCANNER_H
#define XBE_SCANNER_H

#include "xmb_types.h"

// Scans all configured game paths and populates the items list for a category
int xbe_scanner_get_items(XMBCategory category, XMBItem* items, int max_items);

#endif // XBE_SCANNER_H
