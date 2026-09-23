# FrameLock Final - FS25 final test

## 1. Build and install

Build Release x64 using the same source/build workflow that already worked for FrameLock 2. The dependency bootstrap and ZIP-build fixes are retained.

Remove the old standalone `FrameLock.asi`. Install only the newly built OptiScaler files in the same way as the known-working OptiScaler installation.

Do not use the RTSS FPS limiter during comparisons.

## 2. Regression baseline

Start with:

- Limiter Backend: FrameLock
- Sync: Deadline
- FPS Limit: 72
- FG: Off
- Balanced
- Adaptive: On
- Auto-tune: On

Confirm behaviour matches the previous known-good FrameLock 2 Deadline mode.

## 3. Raster FPS-loss regression

Same FS25 scene/settings:

A. Deadline at 72 FPS
B. Hybrid Raster at 72 FPS
C. Raster Sync at 72 FPS

The final raster implementation should no longer lose several sustained FPS simply because a scanline is missed. During initial phase acquisition a handful of frames can vary slightly while phase offset converges, but the long-run FPS should remain around the configured target.

Watch:

- measured FPS
- P99 error
- phase error
- accumulated phase offset
- valid raster samples / fallbacks

Raster phase correction never waits a complete extra refresh.

## 4. XeFG final setup

For 143.999 Hz + 2x XeFG:

- custom refresh: 143.999
- FG-aware output cap: On
- press `FG Preset: Match Display`
- expected output target: 143.999
- expected base target: 71.9995
- Stutter Guard: On
- Cadence Snap: On

The live panel should report XeFG and the detected multiplier.

## 5. Optional FG headroom

Press `FG Preset: 3 Hz Headroom` to test an output target of approximately 140.999 FPS on a 143.999 Hz display. With 2x FG the base target is approximately 70.4995 FPS.

This is optional. If exact refresh matching feels/benchmarks better, keep Match Display.

## 6. Other FG backends

FSR FG and DLSSG use the same auto-target, headroom, cadence and stutter-guard system. Backend and multiplier are read from OptiScaler state rather than entered manually.

## Final recommendation

Use Deadline + FG Match Display as the stable default. Treat raster modes as optional phase-control modes, not as mandatory 'better' modes. If they do not improve the image/pacing in a specific game, Deadline is the correct choice.
