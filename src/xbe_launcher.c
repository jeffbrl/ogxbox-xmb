#include "xbe_launcher.h"
#include "input.h"
#include "ui_renderer.h"
#include <hal/xbox.h>
#include <hal/debug.h>
#include <SDL.h>

void xbe_launcher_launch(const char* xbe_path) {
    debugPrint("Launching XBE: %s\n", xbe_path);
    
    // Cleanup any systems before launch
    input_cleanup();
    ui_cleanup();
    SDL_Quit();
    
    // Launch XBE
    XLaunchXBE(xbe_path);
    
    // XLaunchXBE should not return if successful. 
    // If it does, we are probably in a bad state, but we can reboot.
    debugPrint("Failed to launch %s\n", xbe_path);
    XReboot();
}
