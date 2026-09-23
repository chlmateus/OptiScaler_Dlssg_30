# Adaptive Fixed-Refresh Pacing - FS25 Runtime Test Guide

> Compatibility filename: older builds called this feature PseudoVRR.

Build `Release | x64` on Windows/MSVC, then test one repeatable Farming Simulator 25 route/scene.

## Baseline

Record with adaptive pacing disabled:

- average/base FPS
- 1% / 0.1% low
- P95/P99 render work
- P99 pacing error
- missed deadlines
- Present blocking / fence wait
- subjective camera-pan smoothness and input latency

## 143.999 Hz + XeFG 2x: VSync Cadence

1. Select `Limiter Backend = FrameLock`.
2. Enable `Adaptive Fixed-Refresh Pacing`.
3. Select `VSync Cadence (clean divisors)`.
4. Use exact/custom refresh `143.999` Hz.
5. Set the base range around `48.0 - 71.9995` FPS and 6-8% headroom.
6. Use `Deadline` sync. Do not enable Hybrid Raster/Raster Sync for this controller test.
7. Make sure XeFG is actually active and detected as 2x.
8. Drive through a workload that moves between roughly 50 and 72 sustainable base FPS.

Expected behavior:

- light load can recover toward 71.9995 base / about 143.999 output
- if 71.9995 base is not sustainable, the target moves to the next clean output divisor, not an arbitrary 60-65 FPS base target
- recovery is slower than deterioration
- rapid up/down target hunting should trigger the oscillation guard
- P50/P95/P99 render-work telemetry should exclude most VSync Present blocking

## XeFG 3x and 3x -> 2x fallback

At 143.999 Hz, full-refresh 3x output corresponds to about 47.9997 base FPS. If the existing XeFG stability logic falls back to 2x, verify:

- adaptive history resets
- FrameLock rebases instead of catching up
- the target does not immediately jump to the new 71.9995 maximum base cadence
- it starts conservatively on a legal 2x cadence and recovers only after stable windows

## Tearing-Paced mode

Test separately. Use it only with an actually tearing-enabled path:

- DXGI: final Present uses `DXGI_PRESENT_ALLOW_TEARING`
- native Vulkan: `VK_PRESENT_MODE_IMMEDIATE_KHR`

Expected behavior:

- arbitrary targets inside the configured range are allowed
- QPC deadline pacing remains active
- if tearing is not actually available, telemetry/menu should show cadence fallback

## Reset tests

Repeat with:

- alt-tab / pause / loading stall
- fullscreen <-> borderless
- swap-chain resize/recreation
- resolution or dynamic-resolution change
- refresh change
- FG enable/disable and multiplier change
- VRAM pressure above the high and critical thresholds

History should reset/re-warm rather than treating the transition as valid steady-state workload.

## Pass criteria

Do not call the controller better based only on average FPS. Compare baseline and adaptive runs using P95/P99 render work, P99 pacing error, missed deadlines, target transitions/minute, Present blocking, fence wait, latency, and generated-frame stability. A valid improvement is a measurable reduction in pacing instability without an unacceptable latency regression.
