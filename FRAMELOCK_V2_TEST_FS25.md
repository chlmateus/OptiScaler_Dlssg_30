# FrameLock 2 - FS25 test plan

## 0. Before testing

- Remove/disable the old standalone `FrameLock.asi`.
- Keep OptiScaler as the only FrameLock implementation.
- Disable the RTSS FPS limiter. RTSS OSD/monitoring is fine.
- Start with V-Sync off while testing Raster mode so a tear boundary can be observed/calibrated.

## 1. Baseline deadline test

In OptiScaler -> Framerate:

- Limiter Backend: FrameLock
- Preset: Smooth
- FPS Limit: 72.000
- FG: off
- Sync Method: Deadline

Drive the same route for 2-3 minutes. Check:

- measured FPS
- jitter
- P99 error
- worst error
- stutters
- missed deadlines

This verifies that FrameLock 2's normal deadline backend is stable before scanline logic is involved.

## 2. Exact 143.999 Hz profile

Open **Display / Per-game Hz**:

- If Detected refresh is close to `143.999`, click **Copy detected Hz**.
- Otherwise enable **Use custom refresh for this game** and enter `143.999`.

The value is stored in this game's `OptiScaler.ini`.

## 3. 2x frame generation test

- Enable your 2x FG path.
- Keep FG-aware output cap ON.
- Click **Cap = Hz**.

Expected telemetry:

- Output target: ~143.999 FPS
- FG: 2x
- Base target: ~71.9995 FPS
- Refreshes/base frame: ~2.0000

## 4. Hybrid Raster test

Switch:

- Sync Method: Hybrid Raster (recommended)
- Target lines above bottom: 50
- Raster timeout: 0 (auto)
- Sync to VBlank: off initially

Expected:

- Raster API: available
- Raster cadence: compatible
- Raster hit rate should rise toward a high percentage after warmup
- Target scanline should be close to active height minus 50

Pan the camera horizontally and look for the tear boundary. Adjust `Target lines above bottom` in small steps, typically 20-100. The exact best number depends on driver/present overhead.

## 5. VBlank alternative

If bottom-scanline calibration is annoying or unstable, enable:

- Sync to VBlank instead of a visible scanline

This is easier to hide visually but can move the final presentation point slightly later.

## 6. When Raster should NOT be used

FrameLock automatically bypasses Raster mode when refresh/base-FPS is not close to an integer divisor. For example:

- 143.999 / 71.9995 = 2.0 -> good
- 143.999 / 47.9997 = 3.0 -> good
- 143.999 / 60 = 2.4 -> fallback to Deadline

Also prefer Deadline when the game cannot consistently render within the required slot. Scanline timing cannot compensate for a GPU-bound frame that simply finishes late.

## 7. A/B comparison

Use the same save, camera route and graphics settings:

A. Auto backend, 72 FPS, FG off
B. Legacy internal, 72 FPS, FG off
C. FrameLock Deadline, 72 FPS, FG off
D. FrameLock Hybrid Raster, 71.9995/72 FPS, FG off
E. FrameLock Deadline, 143.999 output + 2x FG
F. FrameLock Hybrid Raster, 143.999 output + 2x FG

Compare P99 error, jitter, stutters and subjective camera motion. A difference that cannot be measured or felt is not automatically an improvement merely because the menu contains more impressive nouns.
