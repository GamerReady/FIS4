# FIS4 — Frame Interpolation System for PS4

**Duplicate FPS ×2 on PS4. Native test bench + future AI interpolation.**

FIS4 is a PS4 homebrew that lets you **visually compare** native FPS vs. interpolated FPS. Today it ships with a classic motion-compensated interpolator (lerp + motion-vector). Tomorrow you drop in the neural model at `fis4_interpolator.cpp:interpolateAI()` and it becomes a true **AI frame-generation** layer (think DLSS Frame Gen / FSR Fluid Motion for PS4).

> **Goal:** Run a game at 30 FPS internally, display at 60 FPS (or 60 → 120) with FIS4 generating every second frame. This repo is the **lab** to prototype, measure, and tune that pipeline *before* hooking games.

---

## ✨ What you get when you open FIS4 on PS4

A full-screen 1080p demo with **no black screen** — moving content specifically designed to expose judder:

- **Bouncing Balls** — 5 balls with velocity vectors, highlights, and trail ghosts. Best for motion-clarity A/B.
- **Rotating Grid** — Infinite grid + orbiting ball. Makes stutter obvious.
- **Tunnel Runner** — Parallax tunnel + 3 balls. Tests large motion vectors.
- **Stress 12 Balls** — 12 simultaneous balls + rotating bar. Tests throughput.

### In-app FPS switching (controller)

| Button | Action |
|--------|--------|
| **X / Cross** | Toggle FIS4 **ON/OFF** (native vs. ×2) |
| **↑ / ↓ (D-Pad)** | Switch base FPS **30 ↔ 60** (effective becomes 60 / 120 when FIS4 on) |
| **← / →** | Previous / next scene |
| **Square** | Cycle interp mode: **Classic → AI → Hybrid** |
| **Triangle** | Toggle frame-time graph |
| **Circle** | Pause |
| **Options** | Toggle HUD |
| **L1+R1+Options** | Exit to PS4 menu |

HUD shows:
- FIS4 pill (green = ON, red = OFF)
- Base FPS vs. Effective FPS (e.g., `30 → 60`)
- Live frame-time graph (green = actual, yellow = target)
- Scene name and interpolation mode

The renderer marks **interpolated frames** with a green ring + “INTERPOLATED FRAME (FIS4)” label so you can see exactly which frames are synthetic.

---

## 🧠 Interpolation Architecture

### Current (ships today)
`FIS4::Interpolator::interpolateClassic()` — linear motion compensation:

```cpp
pos = lerp(prev.pos, curr.pos, t) + vel * 0.02*(t-0.5)
angle = lerpAngle(prev.angle, curr.angle, t)
```

Plus ghost-detection, hybrid blending, and frametime-aware scheduling.

### Future AI path (hook ready)
`Interpolator::interpolateAI()` is a **drop-in stub** that currently blends classic + smoothstep. Replace it with:

- Optical-flow estimation
- Depth-aware warping
- Occlusion mask inpainting
- ONNX/TensorRT-lite embedded inference (load via `loadAIModel("/app0/assets/fis4_nn.onnx")`)

No other code changes needed — `App::getRenderState(t)` and the main loop already dispatch to the AI path when `setMode(AI)` is active.

```
Native tick (30 Hz) ──► pushFrame() ──► Interpolator ──► getInterpolated(t) ──► Render (60/120 Hz)
                              ▲                              │
                              │                     ┌────────┴────────┐
                              │                     │ Classic (ship)  │
                              └─────────────────────┤ AI (future)     │
                                                    │ Hybrid (both)   │
                                                    └─────────────────┘
```

---

## 📁 Project layout

```
FIS4/
├── Makefile               # PS4 build (OpenOrbis toolchain)
├── Makefile.pc            # PC simulation build (SDL2)
├── source/
│   ├── main.cpp           # PS4 entry (Orbis Pad, VideoOut, flip queue)
│   ├── main_pc.cpp        # PC entry (SDL2 window)
│   ├── fis4_app.h/.cpp    # FPS switching, scene mgmt, stats
│   ├── fis4_interpolator.h/.cpp  # X2 core (classic + AI hook)
│   ├── demo_scene.h/.cpp  # 4 moving test scenes
│   ├── fis4_renderer.h/.cpp # PS4 (framebuffer) vs PC (SDL) renderer
│   └── common/
│       ├── graphics.h/.cpp # Vendored Scene2D from OpenOrbis _common
│       └── log.h
├── sce_sys/
│   ├── icon0.png          # 512×512 FIS4 icon
│   └── about/right.sprx   # required for PKG
├── web/
│   └── index.html         # Browser preview (same logic, Canvas)
├── .github/workflows/
│   └── build.yml          # Builds PS4 PKG + PC SDL2 on push
└── docs/
```

---

## 🔨 Build

### PS4 PKG (requires OpenOrbis)

```bash
# Install toolchain (Ubuntu 22.04)
git clone https://github.com/OpenOrbis/OpenOrbis-PS4-Toolchain.git /opt/orbis
# Follow its install.sh, then:
export OO_PS4_TOOLCHAIN=/opt/orbis
export PATH=$OO_PS4_TOOLCHAIN/bin/linux:$PATH

# Build
make          # → IV0000-FIS400001_00-FIS4LAB000000000.pkg
```

Install the PKG on a jailbroken PS4 (GoldHEN) via USB or `pkg_install`.

**Firmware:** 9.00–11.00 with GoldHEN tested. 5.05 should also work (repaid 0x3800000000000011).

### PC simulation (SDL2)

```bash
# Ubuntu/Debian
sudo apt install libsdl2-dev
make -f Makefile.pc        # → build/pc/FIS4_pc
./build/pc/FIS4_pc
# Controls: X/SPACE fis4, ↑/↓ fps, ←/→ scene, M mode, G graph, P pause, ESC exit
```

### Browser preview

Just open `web/index.html` — no build needed. Same physics + interpolation as PS4. Great for quick iteration.

---

## 🤖 GitHub Actions CI

Every push builds:

- **PS4 PKG** — installs OpenOrbis toolchain in container `ps4dev/ps4sdk` and runs `make`.
- **PC SDL2** — builds `FIS4_pc` and checks it links.
- **Validation** — syntax-checks interpolator logic headless.

Artifacts appear under Actions → *FIS4-PS4-PKG* and *FIS4-PC-SDL2*.

To manually trigger: Actions → *Build FIS4* → Run workflow.

---

## 🧪 How to compare native vs. FIS4

1. Set **Base = 30** (D-Pad ↑/↓). Observe at **FIS4 OFF** — 30 FPS judder is obvious on the grid tunnel.
2. Press **X** to turn **FIS4 ON** — effective jumps to 60 FPS. Grid becomes smooth; ball rings turn green.
3. Toggle **Square** to cycle Classic/AI/Hybrid. Watch the ball trail: AI adds smoothstep easing (15% blend).
4. Switch to **60 → 120** to stress-test: native 60 is already smooth, but FIS4 120 shows diminishing returns + ghost risk.
5. Open the **frame-time graph** (Triangle): green spikes should stay near the yellow target line. If you see double-humps, your native tick is unstable — fix game pacing before enabling FIS4.

Pro tip: **Bouncing Balls** is the most honest test. Count the trail dots per second at 30 vs. 60 — FIS4 should double them without duplicating ghosts.

---

## 🔮 Roadmap

- [x] PS4 homebrew scaffold + Scene2D framebuffer
- [x] 4 moving test scenes (ball/grid/tunnel/stress)
- [x] In-app FPS switch (30/60) + FIS4 ×2 toggle
- [x] Classic MC interpolator with graph HUD
- [x] PC + web preview for iteration
- [x] GitHub Actions PKG build
- [ ] ONNX model loader (`/app0/assets/fis4_nn.onnx`)
- [ ] Depth buffer capture from hooked game
- [ ] Per-game FIS4 installer (GoldHEN plugin)

Want to contribute the AI model? See `source/fis4_interpolator.cpp` — implement `interpolateAI()` and wire `loadAIModel()`. The `HYBRID` mode already blends your output with classic as a safety fallback.

---

## 📜 License

MIT — see LICENSE. OpenOrbis samples are BSD/MIT — vendored `graphics.*` retains its original notice.

---

## 🙏 Credits

- OpenOrbis team for the PS4 toolchain + Scene2D.
- PS4 homebrew community for VideoOut/Pad docs.
- You — for pushing PS4 beyond its native FPS.

**FIS4 v1.0 — “Make every frame count twice.”**
