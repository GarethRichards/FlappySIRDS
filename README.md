# FlappySIRDS

A small Direct3D 11 demo that renders a simple "Flappy-bird" game for stereoscopic single-image random dot stereogram (SIRDS) generation. The app renders a left and right depth buffer, converts those into a SIRDS image and displays it. Intended for Windows (desktop) development and experimentation.

---

## Features

- Direct3D 11 rendering of a simple 3D scene
- Simple input to start/play/flap and toggle stereo eye

---

## Requirements

- Windows 10 (or later)
- Visual Studio 2019 / 2022 (MSVC)
- Windows 10 SDK
- Direct3D 11 runtime (part of Windows)
- Project uses portions of DirectX Tool Kit / DirectXTex (included or bundled in the repo)

---

## Quick start

1. Open the solution in Visual Studio: open the `src` folder and load the `.vcxproj` under `src`.
2. Build via __Build → Build Solution__.
3. Run from Visual Studio (F5) or use __Debug → Start Without Debugging__.

Debug and runtime output appears in the Visual Studio __Output__ pane and via `DebugOut()` logging.

---

## Project layout (important files)

- `src/Game.h` / `src/Game.cpp` — main application, rendering loop, device setup, resize handling.
- `src/FlappyData.h` — gameplay state and the `ViewingParameters` class.
- `src/Source.cpp` — SIRDS generator (`InitStatics`, `ZBuffersToDrawer`) / CPU drawer.
- `src/3DText.*` — font / text rendering helpers.
- `src/*` — other helpers and resource files.

All code is under `src/`.

---

## How it works (high level)

1. Scene is drawn twice (left and right eye) into depth-enabled render targets.
2. Depth images are captured/read back to CPU (`CaptureTexture`) and provided to the `SirdsDrawer`.
3. `SirdsDrawer` runs a CPU stereogram algorithm to produce an RGBA image.
4. The RGBA image is uploaded to the GPU as a shader resource and drawn as a fullscreen sprite.

`ViewingParameters` centralizes eye separation, viewing distance, DPI-derived `pmm`, and near/far values.

---

## Controls

- Key press (any assigned key per `WM_KEYDOWN`) to start the game and flap.
  - When not playing, a keypress initiates Play mode.
  - When playing, a keypress triggers the flap (see `flapEffect` in `FlappyData`).
- Left mouse button toggles left/right eye.
- Right mouse button sets center-eye view.

Adjust gameplay values in `src/FlappyData.h` (e.g. `gravity`, `flapEffect`, `gapHeight`).

---

## Viewing parameters

`ViewingParameters` (in `src/FlappyData.h`) contains:

- `viewDistance` — viewing distance used by the SIRDS algorithm
- `offsetDistance` — offset distance used for depth composition
- `eyeSeparation` — inter-eye separation in mm
- `pmm` — pixels-per-millimeter (set in `Game::InitGame()`)
- `zNear`, `zFar` — computed per-frame viewing frustum near/far
- `offset` — additional offset tuning

Use `flappyData.view.<member>` in code or edit defaults for different behavior.

---

## Debugging tips

- Enable the D3D debug layer for better diagnostics (already conditionally enabled under `_DEBUG`).
- Use Visual Studio Graphics Diagnostics to inspect textures, render targets and shader resources.
- Name resources using `SetPrivateData` when developing to make frame capture inspection easier.

---

## Contributing

- Fork the repo, create feature branches, open PRs against `main`.
- Keep changes scoped per PR and include a brief explanation + perf/regression notes.
- Mention platform or SDK assumptions for any environment-specific changes.
