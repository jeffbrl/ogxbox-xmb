#include "http_client.h"
#include "xmb_types.h"
#include <windows.h>
#include <nxdk/net.h>
#include <lwip/sockets.h>
#include <lwip/netdb.h>
#include <lwip/tcpip.h>
#include <lwip/dhcp.h>
#include <lwip/netifapi.h>
#include <lwip/dns.h>
#include <lwip/prot/icmp.h>
#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int s_net_initialized = 0;
static char s_ip_str[32] = "Initializing...";
static char s_last_log[128] = "Network Initializing...";
extern struct netif *g_pnetif;

// Standard Internet Checksum for ICMP Header
static uint16_t standard_checksum(const void* buf, int len) {
    const uint16_t* p = (const uint16_t*)buf;
    uint32_t sum = 0;
    while (len > 1) {
        sum += *p++;
        len -= 2;
    }
    if (len == 1) {
        sum += *(const uint8_t*)p;
    }
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    return (uint16_t)(~sum);
}

// Sends 5 test UDP packets (port 53 DNS probe) to target IP (standard unprivileged socket)
static void send_test_pings(const char* target_ip, int count) {
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0) {
        snprintf(s_last_log, sizeof(s_last_log), "UDP socket error (%d)", sock);
        return;
    }

    struct sockaddr_in dest;
    memset(&dest, 0, sizeof(dest));
    dest.sin_family = AF_INET;
    dest.sin_port = htons(53); // Port 53 DNS
    dest.sin_addr.s_addr = inet_addr(target_ip);

    for (int i = 0; i < count; i++) {
        // Standard DNS Header Query probe for "thumbnails.libretro.com"
        const unsigned char dns_query[] = {
            0x12, 0x34, // Transaction ID
            0x01, 0x00, // Standard query
            0x00, 0x01, // Questions: 1
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            // Query: "thumbnails.libretro.com"
            10, 't','h','u','m','b','n','a','i','l','s',
            8,  'l','i','b','r','e','t','r','o',
            3,  'c','o','m',
            0,          // End of name
            0x00, 0x01, // Type A
            0x00, 0x01  // Class IN
        };

        sendto(sock, (const char*)dns_query, sizeof(dns_query), 0, (struct sockaddr*)&dest, sizeof(dest));
        snprintf(s_last_log, sizeof(s_last_log), "Sent DNS Probe %d/5 to %s", i + 1, target_ip);
        
        SDL_Delay(100);
    }

    closesocket(sock);
}

static XMBNode* s_scraper_categories = NULL;
static int s_scraper_running = 0;

static DWORD WINAPI scraper_worker_thread_proc(LPVOID lpParam) {
    (void)lpParam;
    s_scraper_running = 1;

    // Wait until network DHCP lease is acquired before attempting any download
    while (!net_is_connected()) {
        SwitchToThread();
        Sleep(100);
    }

    // Give network stack 1.5 seconds to settle after DHCP
    for (int s = 0; s < 15; s++) {
        SwitchToThread();
        Sleep(100);
    }

    if (s_scraper_categories) {
        int game_count = s_scraper_categories[CATEGORY_GAMES].child_count;
        XMBNode* games = s_scraper_categories[CATEGORY_GAMES].children;

        if (games && game_count > 0) {
            snprintf(s_last_log, sizeof(s_last_log), "Found %d titles", game_count);
            for (int i = 0; i < game_count; i++) {
                if (games[i].icon_path[0] == '\0' || strcmp(games[i].icon_path, "NONE") == 0) {
                    char game_dir[256];
                    strncpy(game_dir, games[i].path, sizeof(game_dir));
                    char* last_slash = strrchr(game_dir, '\\');
                    if (last_slash) *last_slash = '\0';

                    char* folder_name = strrchr(game_dir, '\\');
                    const char* name_to_scrape = (folder_name && *(folder_name + 1) != '\0') ? (folder_name + 1) : games[i].title;

                    char downloaded_path[256] = {0};
                    int res = scrape_game_cover(name_to_scrape, game_dir, downloaded_path, sizeof(downloaded_path));
                    if (res == 0) {
                        strncpy(games[i].icon_path, downloaded_path, sizeof(games[i].icon_path));
                    } else {
                        strncpy(games[i].icon_path, "NONE", sizeof(games[i].icon_path));
                    }

                    // Yield execution between downloads
                    SwitchToThread();
                    SDL_Delay(50);
                }
            }
        }
    }

    s_scraper_running = 0;
    return 0;
}

void scraper_start_background(XMBNode* root_categories) {
    s_scraper_categories = root_categories;
    if (s_scraper_running) return;

    HANDLE hScraper = CreateThread(NULL, 0, scraper_worker_thread_proc, NULL, 0, NULL);
    if (hScraper) {
        SetThreadPriority(hScraper, THREAD_PRIORITY_LOWEST);
        CloseHandle(hScraper);
    }
}

static DWORD WINAPI net_init_thread_proc(LPVOID lpParam) {
    (void)lpParam;
    snprintf(s_last_log, sizeof(s_last_log), "DHCP: Requesting lease...");

    // Initialize lwIP DHCP
    nx_net_parameters_t params;
    memset(&params, 0, sizeof(params));
    params.ipv4_mode = NX_NET_DHCP;

    int res = nxNetInit(&params);
    if (res == 0) {
        // Wait up to 5 seconds for DHCP lease to be fully assigned
        for (int retry = 0; retry < 50; retry++) {
            if (g_pnetif && dhcp_supplied_address(g_pnetif)) {
                snprintf(s_ip_str, sizeof(s_ip_str), "%s", ip4addr_ntoa(netif_ip4_addr(g_pnetif)));
                snprintf(s_last_log, sizeof(s_last_log), "Online: IP %s", s_ip_str);
                return 0;
            }
            SDL_Delay(100);
        }
    }

    if (g_pnetif && dhcp_supplied_address(g_pnetif)) {
        snprintf(s_ip_str, sizeof(s_ip_str), "%s", ip4addr_ntoa(netif_ip4_addr(g_pnetif)));
        snprintf(s_last_log, sizeof(s_last_log), "Online: IP %s", s_ip_str);
    } else {
        snprintf(s_ip_str, sizeof(s_ip_str), "Offline");
        snprintf(s_last_log, sizeof(s_last_log), "DHCP Offline (Err %d)", res);
    }
    return 0;
}

int net_init(void) {
    if (s_net_initialized) return 0;
    s_net_initialized = 1;

    snprintf(s_last_log, sizeof(s_last_log), "DHCP: Initializing...");

    HANDLE hThread = CreateThread(NULL, 0, net_init_thread_proc, NULL, 0, NULL);
    if (hThread) {
        CloseHandle(hThread);
    }
    return 0;
}

int net_is_connected(void) {
    if (g_pnetif && dhcp_supplied_address(g_pnetif)) {
        return 1;
    }
    return 0;
}

const char* net_get_ip_str(void) {
    if (g_pnetif && dhcp_supplied_address(g_pnetif)) {
        snprintf(s_ip_str, sizeof(s_ip_str), "%s", ip4addr_ntoa(netif_ip4_addr(g_pnetif)));
    }
    return s_ip_str;
}

const char* net_get_last_log(void) {
    return s_last_log;
}

// URL encode special characters
static void url_encode(const char* src, char* dst, size_t dst_size) {
    size_t d = 0;
    for (size_t s = 0; src[s] != '\0' && d + 4 < dst_size; s++) {
        unsigned char c = (unsigned char)src[s];
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' || c == '~' || c == '/') {
            dst[d++] = c;
        } else if (c == ' ') {
            dst[d++] = '%';
            dst[d++] = '2';
            dst[d++] = '0';
        } else {
            snprintf(&dst[d], 4, "%%%02X", c);
            d += 3;
        }
    }
    dst[d] = '\0';
}

int http_download_file(const char* host, const char* path, const char* out_filepath) {
    if (!net_is_connected()) {
        return -10;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(80);
    // Direct IPv4: 5.9.202.203 (thumbnails.libretro.com CDN)
    server_addr.sin_addr.s_addr = inet_addr("5.9.202.203");

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        snprintf(s_last_log, sizeof(s_last_log), "Socket create error (%d)", sock);
        return -2;
    }

    // 2 second socket timeout so recv never blocks indefinitely on the final chunk
    struct timeval tv;
    tv.tv_sec = 2;
    tv.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&tv, sizeof(tv));

    int rcvbuf = 65536;
    setsockopt(sock, SOL_SOCKET, SO_RCVBUF, (const char*)&rcvbuf, sizeof(rcvbuf));

    snprintf(s_last_log, sizeof(s_last_log), "Connecting 5.9.202.203:80...");
    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        snprintf(s_last_log, sizeof(s_last_log), "TCP Connect Failed");
        closesocket(sock);
        return -3;
    }

    // Build HTTP GET Request
    char request[1024];
    snprintf(request, sizeof(request),
        "GET %s HTTP/1.1\r\n"
        "Host: %s\r\n"
        "User-Agent: OGX-XMB/1.0 (Xbox)\r\n"
        "Accept: image/png, */*\r\n"
        "Connection: close\r\n\r\n",
        path, host);

    if (send(sock, request, strlen(request), 0) < 0) {
        closesocket(sock);
        return -4;
    }

    // Allocate 1.5MB download buffer in RAM
    size_t ram_capacity = 1536 * 1024;
    char* ram_buf = (char*)malloc(ram_capacity);
    if (!ram_buf) {
        closesocket(sock);
        return -5;
    }

    size_t total_received = 0;
    char chunk[8192];
    int bytes_received;
    int expected_content_len = -1;
    size_t header_size = 0;
    int chunk_count = 0;
    char* body_start = NULL;

    snprintf(s_last_log, sizeof(s_last_log), "HTTP: Starting recv...");

    while ((bytes_received = recv(sock, chunk, sizeof(chunk), 0)) > 0) {
        chunk_count++;
        if (total_received + bytes_received < ram_capacity) {
            memcpy(ram_buf + total_received, chunk, bytes_received);
            total_received += bytes_received;
            ram_buf[total_received] = '\0';

            // Check if header is complete
            if (expected_content_len < 0) {
                char* hdr_end = strstr(ram_buf, "\r\n\r\n");
                if (hdr_end) {
                    header_size = (hdr_end + 4) - ram_buf;
                    char* cl = strstr(ram_buf, "Content-Length:");
                    if (!cl) cl = strstr(ram_buf, "content-length:");
                    if (!cl) cl = strstr(ram_buf, "Content-length:");
                    if (cl) {
                        expected_content_len = atoi(cl + 15);
                    }
                }
            }

            snprintf(s_last_log, sizeof(s_last_log), "Recv: %u B (%d chunks)", (unsigned int)total_received, chunk_count);

            // If we received all payload bytes according to Content-Length, terminate immediately
            if (expected_content_len > 0 && header_size > 0) {
                if (total_received >= header_size + (size_t)expected_content_len) {
                    snprintf(s_last_log, sizeof(s_last_log), "Recv complete: %u B", (unsigned int)total_received);
                    break;
                }
            }
        } else {
            break;
        }

        // Cooperative yield
        Sleep(2);
    }

    snprintf(s_last_log, sizeof(s_last_log), "Closing socket...");
    closesocket(sock);

    if (total_received == 0) {
        free(ram_buf);
        snprintf(s_last_log, sizeof(s_last_log), "Download empty (0 B)");
        return -6;
    }

    // Parse HTTP Status Code safely
    snprintf(s_last_log, sizeof(s_last_log), "Parsing HTTP status...");
    int status_code = 0;
    if (strncmp(ram_buf, "HTTP/1.1 200", 12) == 0 || strncmp(ram_buf, "HTTP/1.0 200", 12) == 0) {
        status_code = 200;
    } else {
        char* status_ptr = strchr(ram_buf, ' ');
        if (status_ptr && *(status_ptr + 1) != '\0') {
            status_code = atoi(status_ptr + 1);
        }
    }

    if (status_code != 200) {
        free(ram_buf);
        snprintf(s_last_log, sizeof(s_last_log), "HTTP Status %d", status_code);
        return -7;
    }

    // Find Header boundary safely
    if (header_size == 0) {
        char* hdr_end = strstr(ram_buf, "\r\n\r\n");
        if (hdr_end) {
            header_size = (hdr_end + 4) - ram_buf;
        }
    }

    if (header_size == 0 || header_size >= total_received) {
        free(ram_buf);
        snprintf(s_last_log, sizeof(s_last_log), "Error: No header boundary");
        return -8;
    }

    char* body_start_ptr = ram_buf + header_size;
    size_t body_len = total_received - header_size;

    if (expected_content_len > 0 && body_len > (size_t)expected_content_len) {
        body_len = (size_t)expected_content_len;
    }

    if (body_len == 0) {
        free(ram_buf);
        snprintf(s_last_log, sizeof(s_last_log), "Error: Empty body");
        return -9;
    }

    snprintf(s_last_log, sizeof(s_last_log), "Writing to disk: %u B...", (unsigned int)body_len);

    // Write complete image to disk at once
    FILE* out_file = fopen(out_filepath, "wb");
    if (!out_file) {
        free(ram_buf);
        snprintf(s_last_log, sizeof(s_last_log), "Error: Disk fopen failed");
        return -10;
    }

    fwrite(body_start_ptr, 1, body_len, out_file);
    fclose(out_file);
    free(ram_buf);

    snprintf(s_last_log, sizeof(s_last_log), "Disk write done (%u B)", (unsigned int)body_len);

    return 0;
}

// Scrape box art from Libretro Thumbnails HTTP CDN
int scrape_game_cover(const char* game_title, const char* target_folder, char* out_cover_path, size_t max_len) {
    if (!game_title || game_title[0] == '\0' || !target_folder) return -1;

    // Check if cover/icon already exists locally in any standard naming format
    const char* art_candidates[] = { "cover.png", "icon.png", "boxart.png", "poster.png", "default.png", "artwork\\icon.png" };
    for (int a = 0; a < 6; a++) {
        char check_path[256];
        snprintf(check_path, sizeof(check_path), "%s\\%s", target_folder, art_candidates[a]);
        FILE* f = fopen(check_path, "rb");
        if (f) {
            fclose(f);
            strncpy(out_cover_path, check_path, max_len);
            return 0;
        }
    }

    char local_path[256];
    snprintf(local_path, sizeof(local_path), "%s\\cover.png", target_folder);

    if (!net_is_connected()) {
        return -10;
    }

    const char* cdn_host = "thumbnails.libretro.com";
    char raw_url_path[512];
    char encoded_url_path[1024];

    snprintf(s_last_log, sizeof(s_last_log), "Scraping: %s", game_title);

    // Attempt 1: Exact Directory / Title Name (e.g. "Halo - Combat Evolved (USA)")
    snprintf(raw_url_path, sizeof(raw_url_path), "/Microsoft - Xbox/Named_Boxarts/%s.png", game_title);
    url_encode(raw_url_path, encoded_url_path, sizeof(encoded_url_path));

    int res = http_download_file(cdn_host, encoded_url_path, local_path);

    // Attempt 2: If Title has region parenthesis, try variant with/without
    if (res != 0) {
        char clean_title[128];
        strncpy(clean_title, game_title, sizeof(clean_title));
        char* paren = strchr(clean_title, '(');
        if (paren && paren != clean_title) {
            *(paren - 1) = '\0';
            snprintf(raw_url_path, sizeof(raw_url_path), "/Microsoft - Xbox/Named_Boxarts/%s (USA).png", clean_title);
            url_encode(raw_url_path, encoded_url_path, sizeof(encoded_url_path));
            res = http_download_file(cdn_host, encoded_url_path, local_path);
            
            if (res != 0) {
                snprintf(raw_url_path, sizeof(raw_url_path), "/Microsoft - Xbox/Named_Boxarts/%s.png", clean_title);
                url_encode(raw_url_path, encoded_url_path, sizeof(encoded_url_path));
                res = http_download_file(cdn_host, encoded_url_path, local_path);
            }
        }
    }

    // Attempt 3: Replace hyphens with colons (e.g. "Halo: Combat Evolved (USA)")
    if (res != 0) {
        char alt_title[128];
        strncpy(alt_title, game_title, sizeof(alt_title));
        char* dash = strstr(alt_title, " - ");
        if (dash) {
            *dash = ':';
            memmove(dash + 1, dash + 3, strlen(dash + 3) + 1);
            snprintf(raw_url_path, sizeof(raw_url_path), "/Microsoft - Xbox/Named_Boxarts/%s.png", alt_title);
            url_encode(raw_url_path, encoded_url_path, sizeof(encoded_url_path));
            res = http_download_file(cdn_host, encoded_url_path, local_path);
        }
    }

    if (res == 0) {
        strncpy(out_cover_path, local_path, max_len);
        snprintf(s_last_log, sizeof(s_last_log), "Downloaded: %s", game_title);
        return 0;
    }

    snprintf(s_last_log, sizeof(s_last_log), "Scrape Failed: %s (Err %d)", game_title, res);
    return res;
}
