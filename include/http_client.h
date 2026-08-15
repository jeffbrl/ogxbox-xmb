#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#include <stddef.h>

// Initialize lwIP networking via DHCP asynchronously on console boot (non-blocking)
int net_init(void);

// Returns 1 if DHCP lease has been acquired and IP is assigned, 0 otherwise
int net_is_connected(void);

// Returns active IP string or "Acquiring DHCP..." / "Not Connected"
const char* net_get_ip_str(void);

// Returns the latest network / scraper status message to display on the HUD / UI
const char* net_get_last_log(void);

// Fetch a remote HTTP file and save to local disk
int http_download_file(const char* host, const char* path, const char* out_filepath);

// Background / on-demand art scraper helper
int scrape_game_cover(const char* game_title, const char* target_folder, char* out_cover_path, size_t max_len);

// Start async background box art scraper thread
struct XMBNode;
void scraper_start_background(struct XMBNode* root_categories);

#endif // HTTP_CLIENT_H
