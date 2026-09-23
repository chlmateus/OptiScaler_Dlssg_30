# FrameLock Auto Benchmark / Auto Tune

This branch is based on `OptiScaler-master-FrameLock-VRAMGuard-PseudoVRR-V1` and does **not** include OptiMXAO or OptiColor.

## Purpose

The benchmark measures real render-work time and FrameLock timing behavior for 60-180 seconds, then recommends a PseudoVRR / FrameLock profile for the current game, refresh rate and active frame-generation multiplier.

The benchmark intentionally does not benchmark the already-paced output FPS, because that would mostly measure the cap that FrameLock itself imposed. Instead it samples the time the game/driver spent rendering between the previous paced boundary and the next FrameLock wait.

## Runtime sequence

1. Save the user's current FrameLock/PseudoVRR configuration in memory.
2. Force the internal FrameLock path even if the normal profile currently uses Reflex/XeLL/no internal cap.
3. Apply a neutral measurement profile:
   - FrameLock Balanced
   - Deadline sync
   - Adaptive timing ON
   - Auto Tune ON
   - FG aware ON
   - PseudoVRR ON
   - zero PseudoVRR headroom
   - zero cadence attraction
   - exact detected/custom refresh as output ceiling
4. Warm up for 5 seconds.
5. Measure for 60-180 seconds. 90 seconds is the recommended default.
6. Restore the user's original settings automatically.
7. Calculate and display a recommendation.
8. `Apply Recommended Settings` writes the recommendation to the normal config and saves `OptiScaler.ini`.

Cancel also restores the original settings.

## Metrics

The benchmark reports:

- average render-work FPS
- percentile-derived 1% low (P99 render-work time)
- percentile-derived 0.1% low (P99.9 render-work time)
- average and P99/P99.9 render-work milliseconds
- FrameLock missed-deadline rate
- stutter rate
- average P99 pacing error
- worst observed P99 timer overshoot
- effective display refresh
- detected FG backend / multiplier
- raster availability
- VRAM budget final and peak usage
- stability score 0-100

The 1%/0.1% values are specifically **render-work** lows, not generated/output-frame lows.

## Recommendation

The tuner recommends:

- output cap
- PseudoVRR minimum and maximum base FPS
- render headroom percentage
- target hysteresis
- clean-cadence attraction
- PseudoVRR mode
- FrameLock pacing mode
- Deadline vs Hybrid Raster
- precision/spin window
- exact per-game custom refresh
- FG-aware output targeting / cadence snap / stutter guard

It does not automatically reduce render resolution. If VRAM reaches critical pressure (>=94% of the reported DXGI budget), the result warns that VRAM Guard render-ratio relief may be useful, but leaves that image-quality tradeoff to the user.

## Best test method

Use a representative demanding route/scene and keep actually playing for the full benchmark. Avoid spending the benchmark in a pause menu or staring at a wall. A 90-second run is the recommended balance between useful sampling and not turning tuning into a second job.

For FS25, drive through a farm/yard with vehicles, foliage, buildings and camera movement. If using XeFG 2x, leave XeFG enabled so the benchmark detects the real 2x multiplier.
