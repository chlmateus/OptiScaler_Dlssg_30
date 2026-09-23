# FrameLock 2 for OptiScaler

FrameLock 2 is an experimental internal framerate/presentation pacing backend for OptiScaler. It reuses OptiScaler's existing present path and does not install a second DXGI/Present hook.

## What's new over FrameLock 1

- Persistent fractional QPC deadlines remain the safe baseline.
- Auto-tuned final precision stage based on measured timer behavior.
- Exact display refresh detection through Windows display configuration timing rationals.
- Per-game custom refresh override (stored in that game's `OptiScaler.ini`).
- `Deadline`, `Hybrid Raster`, and `Raster Sync` synchronization methods.
- Raster phase lock using `D3DKMTGetScanLine` when supported.
- Automatic fallback to deadline pacing when raster timing is unavailable or the FPS/refresh cadence is incompatible.
- FG-aware output FPS: OptiScaler's current interpolation count derives the real/base target.
- Cap presets: Hz, Hz/2, Hz/3, Hz-3, Hz-0.01.
- Smooth / Low CPU / Raster Match presets.
- Live raster hit rate, scanline, timeout count, stutter count, worst/P95/P99 timing error, timer overshoot and FPS/P99 history graphs.

## Per-game refresh profiles

Every OptiScaler installation/config is already game-local. Enable **Use custom refresh for this game** and enter the exact timing, for example:

```ini
FrameLockUseCustomRefresh=true
FrameLockCustomRefreshHz=143.999
```

This value is used for FrameLock's refresh-divisor checks and raster timing model. Windows' detected refresh remains visible in the menu for comparison.

## Synchronization methods

### Deadline

The safest mode. Uses a persistent fractional QPC timeline, high-resolution waitable timer, cooperative yielding and a bounded final precision stage.

### Hybrid Raster

Recommended experimental mode. Deadline pacing does the coarse wait. Near presentation, FrameLock queries the display scanout and phase-locks to the configured bottom scanline (or VBlank). A successful raster hit becomes the next QPC phase anchor.

### Raster Sync

More aggressive raster alignment with an earlier raster handoff. It still has a timeout and automatic deadline fallback.

Raster modes are only armed when:

- scanline querying is available for the game's monitor, and
- `refresh Hz / base FPS` is close to an integer between 1 and 8.

Examples at 143.999 Hz:

- 143.999 base FPS -> ratio 1 -> compatible
- 71.9995 base FPS -> ratio 2 -> compatible
- 47.9997 base FPS -> ratio 3 -> compatible
- 60 base FPS -> ratio ~2.4 -> raster bypassed, Deadline fallback

## Scanline controls

`FrameLockRasterOffsetLines` is measured upward from the bottom of the active image:

- 0 = bottom visible scanline
- 50 = 50 lines above bottom (good starting point)
- 100 = 100 lines above bottom

`FrameLockRasterWaitForVBlank=true` targets VBlank instead of a visible scanline.

`FrameLockRasterTimeoutUs=0` uses an automatic timeout based on one refresh period. Non-zero values override it.

## Recommended first test for 143.999 Hz + 2x FG

1. Disable RTSS's framerate limiter. OSD/monitoring can stay enabled.
2. Select `Limiter Backend = FrameLock`.
3. Start with **Preset: Smooth** at 72 FPS and FG off to verify baseline stability.
4. Enable 2x FG.
5. In Display / Per-game Hz, use detected Hz or set custom `143.999`.
6. Click **Cap = Hz** so output target is 143.999.
7. Keep FG-aware output cap enabled. Base target should show about 71.9995 FPS.
8. Test Deadline first.
9. Then select **Hybrid Raster** with `Target lines above bottom = 50`.
10. Confirm Raster API = available and Raster cadence = compatible.
11. Drive/pan horizontally and watch raster hit rate, P99 error, stutters and frametime behavior.
12. Try 20-100 scanline offsets in small steps if you can still see a stable tear boundary.

Do not combine Raster mode with another active scanline/fps limiter while comparing results.

## New `[Framerate]` keys

```ini
LimiterBackend=1
FramerateLimit=143.999
FrameLockMode=0
FrameLockSyncMode=1
FrameLockAdaptiveTiming=true
FrameLockAutoTune=true
FrameLockFGAware=true
FrameLockUseCustomRefresh=true
FrameLockCustomRefreshHz=143.999
FrameLockRasterOffsetLines=50
FrameLockRasterTimeoutUs=0
FrameLockRasterWaitForVBlank=false
FrameLockSpinThresholdUs=300
FrameLockMaxSpinThresholdUs=1000
FrameLockOvershootCeilingMs=2.0
```

Enums:

- `FrameLockMode`: 0 Balanced, 1 Precision, 2 Low CPU
- `FrameLockSyncMode`: 0 Deadline, 1 Hybrid Raster, 2 Raster Sync

## Important limitations

- Raster/scanline synchronization requires GPU/render headroom. If a frame frequently takes longer than its refresh slot, no scanline strategy can manufacture the missing render time.
- D3DKMT scanline querying can be unavailable on some display/driver paths. The implementation falls back to Deadline mode.
- With VRR/G-SYNC/FreeSync, Deadline or a normal cap just below refresh is usually the cleaner first choice. Raster Sync is aimed primarily at fixed-refresh / VSYNC-off style pacing experiments.
- FrameLock 2 is not RTSS Scanline Sync code and does not attempt to duplicate RTSS internals. It uses the same general raster-phase concept through Windows scanline APIs.
