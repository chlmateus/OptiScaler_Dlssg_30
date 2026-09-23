# FrameLock Final for OptiScaler

This is the final FrameLock integration branch based on the user's working OptiScaler master build.
FrameLock remains an internal framerate backend behind OptiScaler's existing presentation path. It does not add a second DXGI/Present hook chain.

## Final changes

### Raster / scanline pacing fix

FrameLock 2 used a blocking scanline wait. If the requested line was missed, it could wait until the same line on the next refresh. At ~144 Hz that can cost roughly 6.94 ms for that frame and, when repeated, reduce average FPS.

The final implementation removes that behaviour completely.

Hybrid Raster and Raster Sync now use a phase-assisted QPC model:

1. The persistent QPC deadline remains the authoritative frame cadence.
2. FrameLock samples the current display scanout phase with D3DKMT.
3. It predicts where scanout will be at the deadline.
4. A bounded microsecond correction is added to a persistent phase offset.
5. The normal timer/yield/spin path waits for the adjusted QPC deadline.
6. The underlying deadline interval is never lengthened by an extra refresh.

The phase offset converges over several frames while average FPS remains tied to the original deadline cadence.

- Hybrid Raster uses a smaller/slower correction.
- Raster Sync uses a stronger/faster correction.
- Missing/unavailable scanline information falls back safely to the QPC timeline.
- Raster modes still require an integer-ish refresh/base-FPS cadence.

## Frame-generation features

FrameLock's FG features work with XeFG, FSR FG, DLSSG and generic FG paths. XeFG is detected directly from OptiScaler state.

### FG-aware output cap

`FramerateLimit` is treated as desired output FPS. OptiScaler's actual interpolation count is used to derive the base/render FPS target.

Example at 143.999 Hz:

- 2x FG -> 71.9995 base FPS
- 3x FG -> 47.9996667 base FPS
- 4x FG -> 35.99975 base FPS

### FG auto-target display refresh

When enabled, the active per-game detected/custom display refresh becomes the output target automatically.

Example:

- custom game refresh: 143.999 Hz
- XeFG: 2x
- auto-target: enabled
- headroom: 0 Hz
- effective output target: 143.999 FPS
- base target: 71.9995 FPS

### FG output headroom

An optional headroom value is subtracted from the per-game refresh when FG auto-target is enabled.

Example:

- refresh: 143.999 Hz
- headroom: 3 Hz
- output target: 140.999 FPS
- 2x base target: 70.4995 FPS

Headroom is optional and defaults to 0.

### FG cadence snap

When the requested cap is already very close to an exact display/base cadence, FrameLock snaps internally to the exact rational cadence. This removes tiny long-term mismatch drift such as requesting 144.000 on a 143.999 Hz display.

An intentional output-headroom margin is not snapped away.

### FG base-frame stutter guard

Generated frames can make alternating slow/fast base-frame cadence especially noticeable. When this option is enabled, a meaningfully late base frame causes FrameLock to start a fresh interval from the real boundary instead of scheduling a short catch-up frame immediately afterward.

## Menu controls

Normal OptiScaler menu -> Framerate -> FrameLock.

Main controls:

- Limiter backend: Auto / FrameLock / Legacy internal
- Pacing: Balanced / Precision / Low CPU
- Sync: Deadline / Hybrid Raster / Raster Sync
- Adaptive timer compensation
- Auto-tune precision stage
- FG-aware output cap
- Per-game detected/custom refresh

FG panel:

- detected FG backend
- auto-target display refresh
- output headroom in Hz
- near-exact cadence snap
- base-frame stutter guard
- Match Display preset
- 3 Hz Headroom preset

Raster panel:

- scanline target / VBlank target
- maximum per-frame phase correction
- scanline query time
- per-frame phase correction
- measured phase error
- accumulated phase offset
- valid sample/fallback counters

Live telemetry adds:

- requested output FPS
- effective output FPS
- base target FPS
- detected multiplier/backend
- estimated generated-output FPS
- output frame interval
- applied FG headroom

## Recommended XeFG setup

For the user's 143.999 Hz display, start with:

- Limiter Backend: FrameLock
- Pacing Mode: Balanced
- Sync: Deadline
- FG-aware output cap: On
- Custom refresh: 143.999 Hz
- FG Preset: Match Display
- Cadence snap: On
- Stutter guard: On

At 2x XeFG this gives 71.9995 base / 143.999 output.

Only enable Hybrid Raster or Raster Sync if the scanout-phase behaviour is actually useful in that game. Deadline remains the best compatibility baseline.

## New INI keys

```ini
[Framerate]
FrameLockFGAutoTarget=auto
FrameLockFGHeadroomHz=auto
FrameLockFGCadenceSnap=auto
FrameLockFGStutterGuard=auto
```

Existing `FrameLockRasterTimeoutUs` is retained for config compatibility, but in the final build it controls the maximum per-frame raster phase correction rather than a blocking scanline timeout.
