# GEMINI.md: OG Xbox XMB Dashboard Implementation Plan

## Project Overview
This document serves as the master technical specification and step-by-step implementation plan for an LLM/Agent to develop a minimalist custom dashboard for the original Xbox, styled after Sony's Cross Media Bar (XMB).

- **Target SDK:** `nxdk` (Open-source C/C++ SDK for OG Xbox)
- **Graphics & Input Library:** `SDL2` (built into `nxdk`)
- **Environment:** Hardmod Original Xbox / `xemu` emulator
- **Primary Objective:** Provide a lightweight, responsive XMB game launcher that scans Xbox drive partitions (`E:`, `F:`, `G:`) for `.xbe` titles, renders a smooth horizontal/vertical category menu, and launches selected games/apps.

---

## Technical Stack & Dependencies
- **SDK:** `nxdk` (must be set in environment via `NXDK_DIR`)
- **Core Libraries:**
  - `SDL2` for display rendering, timing, and gamepad/controller input handling
  - `stb_truetype` or `SDL_ttf` for font rendering
  - `stb_image` for PNG/JPG icon loading
  - `nxdk` C runtime and kernel wrappers (`nxLaunchXBE` for XBE launching, partition mounting)
- **Build System:** GNU `make`
- **Packaging Tool:** `extract-xiso` or `mkiso` for creating xemu-compatible `.iso` images

---

## Directory Structure
```text
xbox-xmb-dash/
├── Makefile
├── README.md
├── scripts/
│   └── run_xemu.sh          # Build & auto-launch script for xemu
├── assets/
│   ├── fonts/
│   │   └── Roboto-Regular.ttf
│   └── icons/
│       ├── cat_games.png
│       ├── cat_apps.png
│       ├── cat_settings.png
│       └── default_game.png
├── include/
│   ├── config.h             # Global constants & search paths
│   ├── xmb_types.h          # Data structures for categories & menu items
│   ├── ui_renderer.h        # Graphics & animation rendering API
│   ├── xbe_scanner.h        # Partition mounting & XBE header parser
│   ├── xbe_launcher.h       # System cleanup & XBE launch execution
│   └── input.h              # Gamepad input polling & mapping
└── src/
    ├── main.c
    ├── ui_renderer.c
    ├── xbe_scanner.c
    ├── xbe_launcher.c
    └── input.c
