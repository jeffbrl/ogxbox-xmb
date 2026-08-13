# OG Xbox XMB Dashboard: Project Context & Status

## Project Overview
Developing a minimalist custom dashboard for the original Xbox, styled after Sony's Cross Media Bar (XMB), using the open-source `nxdk` SDK and `SDL2`. 

## Implementation Milestones
*   **Input Handling:** Implemented `input.c` for reading standard Xbox controller digital D-pad and face buttons via SDL GameController APIs.
*   **XBE Scanner & Launcher:** Built `xbe_scanner.c` to scan configured partitions (`E:`, `F:`, `G:`) for `default.xbe` files and parse internal certificate headers for game titles. Created `xbe_launcher.c` to gracefully clean up SDL and use `XLaunchXBE()` to jump execution to a selected game.
*   **True XMB Geometry & PS3 Wave Styling:** Implemented full PS3-style Cross Media Bar engine in `ui_renderer.c`:
    *   Dynamic multi-layered procedural wave ribbons with harmonic sine/cosine motion and particle effects.
    *   Delta-time exponential decay smoothing (`lerp`) for continuous camera/plane translation along both axes.
    *   Node scaling and distance-based alpha falloff.
    *   Live digital clock HUD and drop-shadowed typography.
    *   Custom PNG icons loaded via `stb_image` for categories and game titles.
*   **HDD Image Deployment:** Transitioned from ISO to a native FATX HDD (`qcow2`) image pipeline using `ogxbox-image-builder`, properly populated with clean MS Dash signatures, our dashboard, and partitioned game libraries.

## Current Status
*   **Working:** Full dashboard boot from HDD (`C:\evoxdash.xbe`), true XMB orthogonal navigation, game scanning from `E:\Games`, and successful game launching in `xemu`.

## Next Steps & Roadmap
1.  **Apps & Settings Submenus:** Implement functional sub-node menus for system settings (video mode, IP config, fan speed) and applications.
2.  **Sound Effects:** Add classic XMB UI navigation sounds (tick / click / swoosh) on D-Pad movements and item selections.
3.  **Cover Art & Custom Icons:** Support game-specific thumbnail icons (`icon.png` / `titleimage.xbx`) from game directories.
4.  **Theme Customization:** Color palette / wave tint options (Xbox Emerald, Classic PS3 Smoked Obsidian, Cobalt Blue).
