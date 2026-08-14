# OGX-XMB: Cross Media Bar Dashboard for Original Xbox

A sleek, minimalist, high-performance custom dashboard for the Original Xbox, inspired by Sony's Cross Media Bar (XMB) interface. Built natively with the open-source **[nxdk](https://github.com/XboxDev/nxdk)** SDK and SDL2, **OGX-XMB** delivers a modern, lightweight, 60 FPS launcher experience with fluid animations, real-time system diagnostics, and instant game launching.

---

## 🌟 Key Features

### 🎮 1. Dual-Axis XMB Navigation
* **Horizontal Category Carousel:** Seamlessly glide between main console functions (**Games**, **Apps**, **Info**, **Settings**).
* **Vertical Item Stacks:** Deep hierarchical submenu support with smooth cursor tracking, focal expansion, and breadcrumb trails.
* **Fluid 60 FPS Visuals:** Dynamic sine wave background ribbons with floating particle dust and smooth alpha-blended transitions.
* **Dynamic Color Themes:** Instant switching between curated themes:
  * 🟢 **Xbox Emerald:** Classic OG Xbox glowing neon green.
  * ⬛ **PS3 Obsidian:** Minimalist monochrome slate with subtle platinum accents.
  * 🔷 **Cobalt Sapphire:** Electric deep blue gradient.
  * 🔴 **Ruby Crimson:** Warm, high-contrast crimson red.

---

### 📂 2. Automated Game & App Library
* **Deep Partition Scanning:** Automatically discovers and indexes all `.xbe` titles across `E:\Games\`, `F:\Games\`, and `G:\Games\`.
* **Artwork & Thumbnail Fallbacks:** Automatically loads custom cover art (`icon.png`, `icon.jpg`, `default.png`, `artwork/icon.png`) with clean procedural fallback icons for unskinned titles.
* **Instant XBE Launching:** Cleanly shuts down hardware contexts and executes games directly via native Xbox kernel execution routines (`XLaunchXBE`).

---

### 📊 3. Live Hardware Diagnostics (`Info` Category)
Dedicated, read-only system telemetry displaying live console statistics:
* **Accurate Memory Probing:** True detection of both **64 MB stock** and **128 MB RAM-upgraded** consoles with active physical page breakdown (Free RAM vs. Used RAM).
* **Real-Time Clock (RTC):** Top HUD and system clocks query the hardware SMC RTC directly (`GetLocalTime`), keeping accurate local date and time.
* **Partition Free Space:** Live capacity and free byte counters across `C:`, `E:`, `F:`, and `G:` drives.
* **Video & Display Mode:** Active screen resolution (480p/720p/1080i), refresh rate, and widescreen aspect ratio.
* **Network & Kernel:** Current IP address, link state, and Xbox kernel / nxdk build revisions.

---

### ⚙️ 4. Configurable Console Preferences (`Settings` Category)
All user-adjustable dashboard and hardware settings grouped into a clean settings menu:
* **Theme Customizer:** Live theme switcher with instant visual feedback.
* **Video Mode Settings:** Enable/disable 480p, 720p, 1080i high-definition modes and 16:9 Widescreen aspect ratios.
* **Audio & Network Preferences:** Output mode toggles and network setup shortcuts.

---

### 🛠️ 5. System Utilities (`Apps` Category)
* **Reboot Console:** Gracefully terminates the dashboard and triggers a soft/cold reboot through the BIOS and SMC.
* **Power Off:** Safely shuts down system hardware and cuts power via the Xbox System Management Controller (`0x80` SMBus command).
* **Stock MS Dashboard:** Direct launcher for the stock Microsoft Xbox Dashboard (`C:\msxboxdash.xbe`).

---

## 🎮 Controller Layout

| Button | Action |
| :--- | :--- |
| **D-Pad Left / Right** | Switch horizontal category (at root level) |
| **D-Pad Up / Down** | Navigate vertical items / submenus |
| **Left Analog Stick** | Smooth directional navigation |
| **A Button** | Select / Launch Game / Enter Submenu / Toggle Option |
| **B Button** | Back / Return to parent menu level |
| **Start / Back** | Dashboard menu shortcuts |

---

## ⚖️ Clean-Room Policy & Copyright Notice

**OGX-XMB is 100% open-source and free of proprietary, copyrighted Microsoft assets.**

To launch the **Stock Microsoft Dashboard** from OGX-XMB (`Apps -> System Utilities -> Launch MS Dashboard`), you must supply your own legitimate files dumped from your Original Xbox console.

---

## 📦 Setting Up `c.zip` (C: Partition Payload)

When preparing the `c.zip` archive for disk image generation or deployment, populate it with the following structure:

### 1. OGX-XMB Dashboard Files (Generated from Build)
* `evoxdash.xbe` & `default.xbe`: The compiled `bin/default.xbe` binary.
* `assets/`: The dashboard UI assets folder containing fonts (`Roboto-Regular.ttf`) and category icons.

### 2. User-Supplied Stock Microsoft Dashboard Files (Build 5960)
Extract the following stock 5960 files from your console dump into the root of `C:\`:
* `msxboxdash.xbe`: **Your console's original `xboxdash.xbe` executable, renamed to `msxboxdash.xbe`** (this allows OGX-XMB to serve as primary boot target while keeping the stock dash accessible).
* `Xbox.xtf` & `XBox Book.xtf`: Microsoft UI system fonts.
* `xboxdashdata.185ead00/`: Microsoft Dashboard skins, scripts, and interface data.
* `Audio/`: Microsoft Dashboard ambient soundscapes and audio cues.
* `xodash/`: Xbox Live dashboard and network update files.

### Example `c.zip` Directory Tree:
```text
c.zip
├── evoxdash.xbe          # OGX-XMB Dashboard (Compiled binary)
├── default.xbe           # OGX-XMB Fallback
├── msxboxdash.xbe        # [User-Provided] Stock Microsoft Dashboard 5960
├── Xbox.xtf              # [User-Provided] Stock font
├── XBox Book.xtf         # [User-Provided] Stock font
├── ind-bios.cfg          # [Optional] BIOS configuration file
├── assets/               # OGX-XMB UI assets
│   ├── fonts/
│   │   └── Roboto-Regular.ttf
│   └── icons/
│       ├── cat_games.png
│       ├── cat_apps.png
│       ├── cat_info.png
│       ├── cat_settings.png
│       └── default_game.png
├── Audio/                # [User-Provided] Stock audio files
├── xboxdashdata.185ead00/# [User-Provided] Stock dashboard UI data
└── xodash/               # [User-Provided] Stock Xbox Live applet
```

---

## 📁 E: Partition Structure (`e.zip`)

```text
e.zip
├── Games/                # Game directories (e.g., Games/Halo/default.xbe)
└── Apps/                 # Homebrew utilities and standalone apps
```

---

## 🏗️ Building from Source

### Prerequisites
* **Linux / WSL2** environment
* **[nxdk](https://github.com/XboxDev/nxdk)** (Open-source Original Xbox SDK)
* **Docker** (for `ogxbox-image-builder`) or **extract-xiso**
* **[xemu](https://xemu.app/)** emulator for testing

### 1. Compile Dashboard
Set your `NXDK_DIR` environment variable and run `make`:
```bash
export NXDK_DIR=/path/to/nxdk
PATH=$PATH:$NXDK_DIR/bin make
```
The compiled Xbox executable will be produced at `bin/default.xbe`.

### 2. Launch in Xemu
To build the disk image and launch automatically in `xemu`:
```bash
./scripts/run_xemu.sh
```

---

## 📜 License
This project is open-source software licensed under the **MIT License**. Built using the open-source `nxdk` toolchain and `SDL2`.
