# FS25 Auto Benchmark Test

## Build

- Visual Studio
- Configuration: `Release`
- Platform: `x64`
- Rebuild the solution

This branch contains FrameLock + VRAM Guard FinalPass + PseudoVRR + Auto Benchmark. It contains no MXAO/OptiMXAO and no OptiColor.

## Before starting

1. Launch Farming Simulator 25 normally.
2. Enable the FG backend you actually intend to use, e.g. XeFG 2x.
3. Load a representative save and move to a moderately demanding area.
4. Open OptiScaler -> Framerate -> FrameLock.
5. Expand `FrameLock Auto Benchmark / Auto Tune`.

## Recommended run

- Set `Measure time` to **90 seconds**.
- Press `Start Auto Benchmark` or `90 s Recommended`.
- There is an additional fixed 5-second warm-up.
- Close/minimize the OptiScaler menu if desired and play normally for the run.
- Drive and rotate the camera. Include both lighter and heavier views.
- Do not change FrameLock/PseudoVRR settings during the run.

The benchmark temporarily changes FrameLock settings only in memory and restores the previous profile automatically at completion or cancellation.

## Result

After completion reopen the benchmark section and inspect:

- average / 1% / 0.1% render-work FPS
- missed-deadline rate
- stutter rate
- P99 pacing error
- timer overshoot
- peak VRAM budget usage
- recommended base-FPS range, headroom, hysteresis and cadence strength

Press `Apply Recommended Settings` to save the recommendation to the game's OptiScaler configuration.

## What a healthy result looks like

For a 143.999 Hz display with XeFG 2x, the maximum recommended base rate should normally be close to `71.9995 FPS`. The minimum depends on measured 1% lows. A stable workload should receive lower headroom and stronger cadence attraction; a variable workload should receive more headroom and hysteresis.

If peak VRAM is >=94%, the benchmark will warn about critical memory pressure but will not automatically lower render resolution.

## Important

The benchmark cannot remove shader-compilation stutter, CPU simulation spikes, storage stalls, or a GPU that simply cannot render the chosen settings fast enough. It tunes FrameLock around the performance the game actually delivered during the test.
