#include "xbe_scanner.h"
#include "xmb_config.h"
#include "http_client.h"
#include <string.h>
#include <stdio.h>
#include <windows.h>
#include <hal/debug.h>

// A simple XBE Header definition for extracting the title
typedef struct {
    uint32_t magic;         // "XBEH"
    uint8_t  padding1[0x104-4];
    uint32_t base_addr;
    uint32_t headers_size;
    uint32_t image_size;
    uint32_t image_header_size;
    uint32_t time_date;
    uint32_t cert_addr;
    uint32_t num_sections;
    uint32_t section_headers_addr;
    uint32_t init_flags;
    uint32_t entry_point;
    uint32_t tls_addr;
    uint32_t pe_stack_commit;
    uint32_t pe_heap_reserve;
    uint32_t pe_heap_commit;
    uint32_t pe_base_addr;
    uint32_t pe_image_size;
    uint32_t pe_checksum;
    uint32_t pe_time_date;
    uint32_t debug_path_addr;
    uint32_t debug_file_addr;
    uint32_t debug_unicode_file_addr;
} XBE_HEADER;

typedef struct {
    uint32_t size;
    uint32_t time_date;
    uint32_t title_id;
    uint16_t title_name[40]; // UTF-16 LE
} XBE_CERTIFICATE;

static void get_xbe_title(const char* xbe_path, char* title_out, size_t max_len) {
    FILE* f = fopen(xbe_path, "rb");
    if (!f) {
        strncpy(title_out, "Unknown Game", max_len);
        return;
    }

    XBE_HEADER header;
    if (fread(&header, 1, sizeof(header), f) != sizeof(header)) {
        fclose(f);
        strncpy(title_out, "Invalid XBE", max_len);
        return;
    }

    if (header.magic != 0x48454258) { // "XBEH"
        fclose(f);
        strncpy(title_out, "Invalid XBE", max_len);
        return;
    }

    uint32_t cert_offset = header.cert_addr - header.base_addr;
    fseek(f, cert_offset, SEEK_SET);

    XBE_CERTIFICATE cert;
    if (fread(&cert, 1, sizeof(cert), f) != sizeof(cert)) {
        fclose(f);
        strncpy(title_out, "Unknown Game", max_len);
        return;
    }

    // Convert UTF-16 to ASCII
    int i;
    for (i = 0; i < 40 && i < max_len - 1; i++) {
        uint16_t c = cert.title_name[i];
        if (c == 0) {
            break;
        }
        title_out[i] = (char)c;
    }
    title_out[i] = '\0';

    fclose(f);
}

int xbe_scanner_get_items(XMBCategory category, XMBItem* items, int max_items) {
    int count = 0;

    if (category == CATEGORY_GAMES) {
        const char* scan_paths[] = { GAMES_PATH_E, GAMES_PATH_F, GAMES_PATH_G, "D:\\Games" };
        
        for (int p = 0; p < 4; p++) {
            WIN32_FIND_DATA fd;
            char search_path[256];
            snprintf(search_path, sizeof(search_path), "%s\\*", scan_paths[p]);
            
            HANDLE hFind = FindFirstFile(search_path, &fd);
            if (hFind == INVALID_HANDLE_VALUE) continue;
            
            do {
                if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                    if (strcmp(fd.cFileName, ".") != 0 && strcmp(fd.cFileName, "..") != 0) {
                        if (count >= max_items) break;
                        
                        char xbe_path[256];
                        snprintf(xbe_path, sizeof(xbe_path), "%s\\%s\\%s", scan_paths[p], fd.cFileName, DEFAULT_XBE);
                        
                        FILE* f = fopen(xbe_path, "rb");
                        if (!f) {
                            // Try uppercase DEFAULT.XBE fallback
                            snprintf(xbe_path, sizeof(xbe_path), "%s\\%s\\DEFAULT.XBE", scan_paths[p], fd.cFileName);
                            f = fopen(xbe_path, "rb");
                        }
                        
                        if (f) {
                            fclose(f);
                            strncpy(items[count].path, xbe_path, sizeof(items[count].path));
                            
                            char title[64] = {0};
                            get_xbe_title(xbe_path, title, sizeof(title));
                            if (title[0] == '\0' || strcmp(title, "Unknown Game") == 0 || strcmp(title, "Invalid XBE") == 0) {
                                strncpy(items[count].title, fd.cFileName, sizeof(items[count].title));
                            } else {
                                strncpy(items[count].title, title, sizeof(items[count].title));
                            }
                            
                            // Check for custom artwork/icon locally
                            items[count].icon_path[0] = '\0';
                            const char* art_names[] = { "cover.png", "icon.png", "boxart.png", "poster.png", "default.png", "artwork\\icon.png" };
                            for (int a = 0; a < 6; a++) {
                                char art_path[256];
                                snprintf(art_path, sizeof(art_path), "%s\\%s\\%s", scan_paths[p], fd.cFileName, art_names[a]);
                                FILE* af = fopen(art_path, "rb");
                                if (af) {
                                    fclose(af);
                                    strncpy(items[count].icon_path, art_path, sizeof(items[count].icon_path));
                                    break;
                                }
                            }
                            
                            count++;
                        }
                    }
                }
            } while (FindNextFile(hFind, &fd) != 0);
            FindClose(hFind);
        }
    } else if (category == CATEGORY_APPS) {
        strncpy(items[count].title, "File Manager", sizeof(items[count].title));
        count++;
    } else if (category == CATEGORY_INFO) {
        strncpy(items[count].title, "Network Info", sizeof(items[count].title));
        count++;
        strncpy(items[count].title, "System Info", sizeof(items[count].title));
        count++;
    }

    return count;
}
