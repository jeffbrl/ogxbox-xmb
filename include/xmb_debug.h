#ifndef XMB_DEBUG_H
#define XMB_DEBUG_H

#include <stdio.h>
#include <stdarg.h>
#include <xboxkrnl/xboxkrnl.h>

// Outport byte to x86 I/O port (COM1 = 0x3F8, QEMU debug = 0xE9)
static inline void outb_port(unsigned short port, unsigned char val) {
    __asm__ __volatile__ ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline void xmb_log(const char* fmt, ...) {
    char buf[512];
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    // 1. Write characters to Xbox COM1 Serial Port (0x3F8) -> piped to terminal via -serial stdio
    for (int i = 0; i < len; i++) {
        outb_port(0x3F8, (unsigned char)buf[i]);
    }

    // 2. Kernel Debugger DbgPrint
    DbgPrint("%s", buf);

    // 3. Write directly to C:\xmb_debug.log on disk (system root)
    FILE* f = fopen("C:\\xmb_debug.log", "a");
    if (f) {
        fputs(buf, f);
        fflush(f);
        fclose(f);
    }
}

#endif // XMB_DEBUG_H
