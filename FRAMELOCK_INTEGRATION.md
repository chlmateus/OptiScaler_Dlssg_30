# FrameLock integration for OptiScaler master

> **Update:** This package now contains **FrameLock 2**. See `FRAMELOCK_V2.md` for the current refresh-aware/raster-sync controls and test procedure.

This tree integrates the FrameLock deadline scheduler directly into OptiScaler's existing framerate limiting path.
It does **not** add another DXGI factory, swap-chain, Present, Present1, Vulkan, or ASI hook.

## Why this architecture

OptiScaler already calls `FrameLimit::sleep()` from its wrapped DXGI swap-chain / FG / Vulkan presentation paths.
FrameLock is therefore implemented as another limiter backend behind that function rather than as an independent injector.

When FrameLock is selected, Reflex/XeLL/Anti-Lag may remain active for low-latency behavior, but their FPS interval is cleared so only one system owns frame pacing.

## Menu

Open the normal OptiScaler menu and go to **Framerate**.

`Limiter Backend` now contains:

- **Auto (existing OptiScaler behavior)** - upstream behavior; Reflex/XeLL/Anti-Lag owns the cap when available, otherwise legacy fallback.
- **FrameLock** - QPC persistent-deadline limiter integrated into OptiScaler.
- **Legacy internal** - forces OptiScaler's old timer + fixed busy-wait limiter for A/B testing.

When FrameLock is selected the menu exposes:

- Balanced / Precision / Low CPU pacing mode
- adaptive timer compensation
- FG-aware output cap
- precision window
- maximum spin
- timer overshoot calibration ceiling
- live target/output/base FPS
- actual FPS and average frame time
- jitter and P95/P99 deadline error
- timer overshoot mean/P99
- sleep/yield/spin time
- missed deadlines

Decimal FPS input is supported directly in the menu.

## Frame Generation behavior

With `FrameLockFGAware=true`, `FramerateLimit` means desired **output FPS**.
OptiScaler's existing FG state is used to derive the rendered/base cap.

Example:

- output target = 143.999 FPS
- detected FG = 2x
- base target = 71.9995 FPS

For MFG, OptiScaler's detected interpolation count is used when available.

## Configuration

New `[Framerate]` keys:

```ini
FramerateLimit=143.999
LimiterBackend=1
FrameLockMode=0
FrameLockAdaptiveTiming=true
FrameLockFGAware=true
FrameLockSpinThresholdUs=300
FrameLockMaxSpinThresholdUs=1000
FrameLockOvershootCeilingMs=2.0
```

Values:

- `LimiterBackend`: 0 Auto, 1 FrameLock, 2 Legacy internal
- `FrameLockMode`: 0 Balanced, 1 Precision, 2 Low CPU

All keys default to `auto`, preserving upstream behavior.

## Scheduler design

FrameLock uses:

1. `QueryPerformanceCounter` / `QueryPerformanceFrequency`
2. a persistent fractional QPC deadline timeline
3. high-resolution waitable timer for the long wait
4. learned timer P99 overshoot as an adaptive wake guard
5. cooperative `SwitchToThread` / `Sleep(0)` before the final precision window
6. a hard-bounded `_mm_pause` / `YieldProcessor` final spin
7. deadline resynchronization after large stalls

No disk I/O or heap allocation occurs on the normal wait path.

## First FS25 test

1. Build OptiScaler as you normally build this master.
2. Install the resulting OptiScaler build exactly as your working OptiScaler installation is installed.
3. Start FS25 with `LimiterBackend=0` first and verify normal OptiScaler behavior.
4. Open the OptiScaler menu -> Framerate.
5. Set FPS Limit to `72.000`.
6. Change Limiter Backend to `FrameLock`.
7. Use `Balanced`, adaptive ON, FG-aware ON.
8. Verify the game remains stable and the live telemetry begins updating.
9. Then test `143.999` output with your preferred FG mode.
10. Compare against Auto and Legacy internal using the same scene/settings.

For the first FrameLock test, disable any RTSS FPS cap. RTSS monitoring/overlay can remain if stable, but two simultaneous FPS limiters invalidate pacing comparisons.

## Build validation note

This environment does not contain the Windows SDK, so the final Windows OptiScaler DLL cannot be compiled here. The uploaded upstream tree itself also contains project references to generated/external headers that are absent from the ZIP; the modified project adds no new missing references beyond that baseline.
