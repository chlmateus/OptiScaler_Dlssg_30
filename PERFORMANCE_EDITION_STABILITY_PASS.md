# OptiScaler Performance Edition - Stability Pass V1

This branch improves the existing Performance Edition without replacing its working systems.

Base features preserved:

- FrameLock
- Deadline / Precision / Balanced pacing
- Hybrid Raster
- Exact refresh-rate support
- XeFG / FSR FG / DLSSG detection
- FG-aware targeting
- PseudoVRR
- VRAM Guard
- lazy resource allocation / cleanup
- Auto Benchmark
- per-game configuration

OptiColor and MXAO are not included.

## Added in this pass

### Shared performance state

A central fact model now exposes refresh, render work, pacing error, Present/fence blocking, approximate queue depth, FG multiplier/backend, virtual generated-frame timing events, resolution generations, VRAM pressure and fallback reasons.

FrameLock remains the only final pacing authority.

### XeFG 3x stability controller

Experimental and OFF by default.

- stability score
- 500 ms evaluation windows
- three-window sustained-instability fallback
- 3x -> 2x safe fallback
- cooldown and stable-window recovery
- pending-request/context-recreation handling
- explicit fallback reasons
- highly gated experimental 4x upgrade
- optional emergency FG disable for sustained critical VRAM only

### FrameLock / PseudoVRR

- P95/P99 render-work tracking
- percentile-aware sustainable FPS
- cadence attraction only for stable workloads
- repeated missed-deadline rebase
- Smoothness / Balanced / Low Latency policies

### VRAM Guard / resource lifetime

- OptiScaler-owned DX12 FG resource ledger
- ownership/lifetime/format/resolution-generation metadata
- allocation rate and short-horizon pressure prediction
- actual pressure required for Critical state
- queue/fence last-use tracking
- production release-before-fence safety
- debug reuse/generation validator
- resize/swap-chain generation tracking

### Auto Benchmark

- run-local counter baselines
- confidence score separate from stability score
- Present blocking / queue latency telemetry
- generated timeline late/burst estimates
- resolution/backend/multiplier/refresh invalidation
- refuses low-confidence mixed-condition recommendations

## Important limitations

- Generated-frame timeline values are estimates. XeFG does not expose every generated scanout through a normal Present hook.
- Resource-ledger coverage is currently strongest for DX12 FG-owned committed resources, not every allocation in the process.
- Multi-pass automatic native/upscale/2x/3x/4x benchmark orchestration is deferred until this core pass is runtime-proven.
- No generic system claims to detect visual FG artifacts.
- Vulkan keeps the existing limiter path but does not receive the full DX12 XeFG resource controller.

## Recommended first test

1. Build `Release | x64`.
2. Leave Adaptive XeFG and Resource Validation OFF.
3. Verify the previous Performance Edition behavior has no regression.
4. Enable Resource Validation temporarily and exercise resize/window/FG transitions.
5. Test manual XeFG 3x.
6. Enable Adaptive 3x / 2x fallback only after the baseline is stable.

See `PERFORMANCE_EDITION_STABILITY_TEST.md` for the full staged matrix.
