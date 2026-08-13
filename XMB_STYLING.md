# Sony XrossMediaBar (XMB) Architecture

## TL;DR Summary
The Sony XrossMediaBar (XMB) is a 2D spatial navigation interface structured as an orthogonal array (X/Y matrix). Primary domain categories populate the horizontal axis, while executable items or sub-nodes populate the vertical axis. The selection point remains locked at a static screen origin, with the entire coordinate plane dynamically translating around it.

---

## Key Architectural Concepts

### Orthogonal Axis Separation
* **Horizontal Axis (X):** Root system categories (e.g., Settings, Photo, Music, Video, Game, Network).
* **Vertical Axis (Y):** Executable binaries, files, or sub-menu arrays bound to the active horizontal node.

### Static Selection Reticle (Viewport Translation)
* The active focus frame does not travel across a static background.
* The cursor remains locked at a fixed spatial target.
* Input events trigger non-blocking translation vectors (Δx, Δy) that move the UI tree relative to the viewport origin.

### Hierarchical State Machine
* Entering a sub-folder pushes a new node tree onto the stack.
* The parent horizontal column shifts left to retain structural context, displaying the nested child tree at X + 1.

---

## Technical Features

* **Deterministic Digital Input:** Direct mapping to D-Pad/discrete digital inputs via unit increments across the grid, eliminating 2D pointer tracking.
* **Asynchronous Asset Preloading:** Dwelling on an active node triggers asynchronous loading of contextual media (background textures, audio samples, or video loops) into the primary render loop.
* **Multi-Layer Compositing:** Renders semi-transparent vector icons over dynamic real-time shaders (e.g., the PS3 procedural wave background).
