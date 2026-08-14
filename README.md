# OGX-XMB: Cross Media Bar Dashboard for Original Xbox

A sleek, minimalist, high-performance custom dashboard for the ogxbox, inspired by Sony's Cross Media Bar (XMB) interface. Built natively with the open-source **[nxdk](https://github.com/XboxDev/nxdk)** SDK and SDL2, **OGX-XMB** delivers a modern, lightweight, 60 FPS launcher experience with fluid animations, real-time system diagnostics, and instant game launching.

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

## ⚡ Cerbios BIOS Customization & `cerbios.ini` Injection

OGX-XMB provides built-in support for **Cerbios** (both v2.x and v3.x+). You can define and customize your console's BIOS behavior directly from the repository root using `cerbios.ini`.

### 1. How It Works
When you build the hard drive image using `./scripts/build_hdd.sh`, the script automatically:
1. Converts the root `cerbios.ini` to standard DOS/Windows **CRLF (`\r\n`)** line endings expected by the Cerbios BIOS parser.
2. Injects the configuration into all locations recognized by Cerbios:
   * `C:\cerbios.ini` (Legacy / standard location)
   * `C:\Cerbios\cerbios.ini` (Subfolder fallback)
   * `E:\Cerbios\cerbios.ini` (Primary location for Cerbios v3.0+)

### 2. Customizing `cerbios.ini`
Edit `cerbios.ini` in the project root to adjust boot paths and hardware options:

```ini
; ==============================================================================
; Cerbios Configuration File (cerbios.ini)
; ==============================================================================

[Dash]
; Dashboard Boot Priority (1 to 5)
DashPath1 = C:\evoxdash.xbe
DashPath2 = C:\default.xbe
DashPath3 = E:\evoxdash.xbe
DashPath4 = E:\default.xbe
DashPath5 = C:\xboxdash.xbe

[General]
; Enable/Disable custom boot animation (0 = Off, 1 = On)
CustomBootAnimation = 1

[Video]
; Force Video Modes (0 = Off / Auto, 1 = On)
ForceProgressive = 0
Force480p = 0
Force720p = 0
ForceWidescreen = 0

[Fan]
; Custom Fan Speed percentage (20 to 100)
FanSpeed = 20

[Audio]
; Play Cerbios Startup Sound (0 = Off, 1 = On)
PlayBootSound = 1
```

### 3. Deploying Changes
After modifying `cerbios.ini`:
```bash
./scripts/build_hdd.sh
```
This re-injects the configuration across partitions and regenerates `xbox_hdd.qcow2` with user-level permissions.

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
├── ind-bios.cfg          # [Optional] Ind-BIOS configuration file
├── cerbios.ini           # [Optional] Cerbios BIOS configuration file
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
├── Cerbios/
│   └── cerbios.ini       # Cerbios v3.x boot configuration
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

### 2. Launch in Xemu (Fast Development Workflow)
```bash
./scripts/run_xemu.sh
```
* Compiles the dashboard with `make` and launches `xemu` instantly (< 1s).

### 3. Full HDD Image Rebuild
```bash
./scripts/build_hdd.sh
```
* Rebuilds `xbox_hdd.qcow2` with updated partitions, injected `cerbios.ini`, and custom games/apps.

---

## 📜 License
This project is open-source software licensed under the **MIT License**. Built using the open-source `nxdk` toolchain and `SDL2`.
