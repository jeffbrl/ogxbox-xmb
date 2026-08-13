# OG Xbox XMB Dashboard: Project Context & Status

## Project Overview
Developing a minimalist custom dashboard for the original Xbox, styled after Sony's Cross Media Bar (XMB), using the open-source `nxdk` SDK and `SDL2`. 

## Implementation Milestones
*   **Input Handling:** Implemented `input.c` for reading standard Xbox controller digital D-pad and face buttons via SDL GameController APIs.
*   **XBE Scanner & Launcher:** Built `xbe_scanner.c` to scan configured partitions (`E:`, `F:`, `G:`) for `default.xbe` files, parse certificate headers, and scan for custom artwork (`icon.png`, `boxart.png`, `poster.png`, `cover.png`).
*   **Procedural Audio Engine (`feat/audio-sfx`):** Real-time synthesized XMB sound effects (`audio.c`) with zero-latency sine wave generators for cursor ticks (1800Hz click), category glide swooshes, melodic dual-tone selection chimes, and cancel sounds.
*   **Hierarchical Submenu System (`feat/submenu-navigation`):** Stack-based node navigation (`menu_tree.c`) supporting deep submenus for System Information (RAM, CPU, Encoder, Kernel), Video Settings (720p/480p/16:9), Audio Settings, and System Utilities (Reboot/Power down/MS Dash) with breadcrumbs and subtitles.
*   **Cover Art & Texture Caching (`feat/cover-art`):** Dynamic per-game thumbnail and icon scanning with an integrated LRU texture cache in `ui_renderer.c`.
*   **Dynamic Theme & Wave Switcher (`feat/theme-customization`):** 5 selectable color palettes (PS3 Smoked Obsidian, Xbox Emerald Green, Cobalt Sapphire, Ruby Crimson, Cyberpunk Gold) with real-time wave ribbon, particle, and gradient recoloring.
*   **HDD Image Deployment:** Fully integrated automated FATX `qcow2` HDD image creation via `ogxbox-image-builder`.

## Git Branches
*   `main`: Baseline working dashboard with PS3 wave engine and HDD game launcher.
*   `feat/audio-sfx`: Procedural zero-latency XMB audio synthesis engine.
*   `feat/submenu-navigation`: Stack-based hierarchical submenu navigation, breadcrumbs, and Xbox system actions.
*   `feat/cover-art`: Per-game cover art and custom icon scanning with texture caching.
*   `feat/theme-customization`: Real-time wave color theme selector and 5 curated presets.
