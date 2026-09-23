# Performance Edition Stability Pass: Runtime Test Matrix

This test plan is intentionally staged. Do not enable every experimental feature at once, because that converts debugging into archaeology.

## Build

- Visual Studio: `Release | x64`
- Keep the same injection/ASI setup already known to work for the previous Performance Edition branch.
- Preserve a copy of the previous working INI for A/B testing.

## Test A: Baseline regression

Settings:

- Adaptive XeFG multiplier: OFF
- Resource lifetime validation: OFF
- Existing FrameLock/PseudoVRR settings unchanged
- Existing VRAM Guard profile unchanged

Run the same game/scene used to verify the previous branch for 10-15 minutes.

Pass criteria:

- no launch regression
- no device removal
- no new freeze/crash
- FrameLock target and PseudoVRR behave like the previous build
- no obvious change in normal/FG-disabled behavior

## Test B: Resource lifetime validator

Enable:

- `PerformanceEdition -> Resource lifetime validation (debug)`

Exercise:

- alt-tab
- borderless/fullscreen transitions if supported
- resolution change
- DLAA/upscale mode change
- FG off/on
- XeFG backend/context recreation
- game resize if windowed

Watch:

- Resource validation errors
- stale-generation resources
- log messages about deferred cached-resource release

Expected:

- validation errors remain 0 in normal operation
- resources are not released before their last known fence completes
- no device-removal errors

Turn validation OFF again for normal play after testing.

## Test C: XeFG 3x baseline, controller OFF

Use XeFG 3x manually with:

- Adaptive XeFG multiplier: OFF
- FrameLock on
- exact monitor Hz configured
- PseudoVRR as normally used

Capture 5-10 minutes in a repeatable demanding route/scene.

Record:

- P99 pacing error
- missed deadlines
- stutters
- Present block
- queue/fence latency
- virtual generated late/burst events
- VRAM actual/predicted pressure

This is the baseline against which adaptive 3x is judged.

## Test D: Adaptive 3x -> 2x fallback

Enable:

- Adaptive 3x / 2x fallback: ON
- Preferred multiplier: 3x
- Stable before upgrade: 8 s
- Fallback cooldown: 12 s
- Emergency FG disable: OFF

Expected behavior:

- stable workload remains at 3x
- a single bad frame does not trigger fallback
- sustained instability for roughly three 500 ms windows can trigger 3x -> 2x
- fallback reason is logged/shown
- 2x remains active through cooldown
- upgrade to 3x only occurs after sustained stability
- no rapid 2x/3x oscillation

Failure conditions:

- repeated bouncing between multipliers
- multiplier changes every few frames
- fallback without a visible reason
- device/context recreation failure

## Test E: VRAM pressure

Use a scene/settings that approach the 8 GB budget.

Expected:

- predicted pressure may enter High before actual DXGI pressure
- predicted pressure alone never enters Critical
- optional critical behavior only occurs when actual usage reaches the configured critical threshold
- if 3x is active and memory pressure contributes to instability, adaptive XeFG may fall to 2x
- game texture quality is not directly changed by the ledger/predictor

Record:

- actual pressure
- predicted pressure
- tracked OptiScaler-owned MiB
- allocation rate
- peak tracked bytes

## Test F: Dynamic resolution / topology change

During a test run change one of:

- internal resolution
- output resolution
- DLSS/FSR/XeSS quality mode
- window/swap-chain size

Expected:

- FrameLock timing history rebases/resets
- resolution generation increments
- stale-resource diagnostic updates
- active Auto Benchmark run becomes invalid instead of producing a recommendation from mixed conditions

## Test G: Auto Benchmark confidence

Run 90 seconds in a repeatable gameplay route.

Good-run expectations:

- confidence >= 50
- no topology-change invalidation
- FG backend/multiplier stays constant
- result includes Present block, queue latency, virtual generated timeline events and VRAM pressure

Invalid-run test:

Start another benchmark and deliberately change resolution or FG mode mid-run.

Expected:

- benchmark is rejected/invalidated
- old settings restore cleanly
- no recommendation is treated as trustworthy

## Test H: Fixed 143.999 Hz cadence

For a 143.999 Hz fixed-refresh monitor compare:

1. XeFG 2x at ~71.9995 base
2. XeFG 3x at ~47.9997 base when the game can sustain it
3. PseudoVRR dynamic behavior around each multiplier

Compare:

- P99 pacing error
- missed deadlines
- Present blocking
- queue depth estimate
- virtual generated late/burst events
- subjective camera-pan consistency

Do not judge by average FPS alone.

## Test I: Normal rendering / FG disabled

Disable FG completely and run FrameLock/PseudoVRR.

Pass criteria:

- no adaptive XeFG policy interference
- no emergency FG override remains stuck
- FrameLock and PseudoVRR still work normally
- no regression in Present path or resize handling

## Acceptance target for this pass

The source changes are worth keeping only if runtime testing shows:

- no new crashes/device removals
- no unsafe resource-release validation errors
- peak tracked OptiScaler memory is no worse than baseline in equivalent conditions
- stable workloads have equal or lower P99 pacing error
- XeFG 3x has fewer sustained timing failures or safely falls to 2x
- fallback does not oscillate
- benchmark refuses mixed/invalid conditions
- FG-disabled mode has no regression
