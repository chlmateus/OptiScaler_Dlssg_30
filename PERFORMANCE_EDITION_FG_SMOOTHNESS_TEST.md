# Performance Edition FG Smoothness V1 — Runtime Test Matrix

Build `Release | x64` first. Preserve the previous known-good DLL so A/B testing is possible.

## Phase A — baseline compatibility

1. Start with all new experimental settings at defaults:
   - `FSRFG/FPTProfile=0` (Manual)
   - `CAS/AdaptiveSharpnessEnabled=false`
2. Test FG disabled.
3. Test the same game / scene with the previous BuildFix1 DLL and this DLL.
4. Confirm no change in ordinary upscaling, UI, swap-chain creation or normal Present behavior.

## Phase B — counter / reset behavior

Test:

- FG enable -> disable -> enable.
- pause / resume.
- loading transition.
- alt-tab out and back.
- resize / borderless transition.
- internal or output resolution change.

Watch telemetry:

- candidate should not permanently run behind submitted.
- submitted must not advance on provider failure.
- completed/present-cycle should trail or equal submitted, never run ahead.
- rollback count should normally remain zero.
- expected state changes may increment temporal-reset telemetry.

## Phase C — XeFG 2x / 3x

At exact display refresh (for example 143.999 Hz):

- XeFG 2x: verify limiter base/output targets are consistent with multiplier.
- XeFG 3x: verify legacy limiter (when selected) no longer assumes 2x.
- Trigger a multiplier change and verify pacing session rebase does not cause a long first-frame wait.
- If adaptive XeFG fallback from the prior Stability Pass is enabled, verify 3x -> 2x still recreates cleanly.

Record P99 pacing error, missed deadlines, present blocking and queue depth before/after.

## Phase D — frame-time discontinuity

Natural test cases:

- loading screen.
- pause for several seconds.
- alt-tab.
- temporary breakpoint only in a developer build.

Expected:

- invalid/severe frame-time telemetry increases.
- a short temporal reset is requested.
- provider does not receive NaN/infinite/zero frame time.
- normal history recovers after the discontinuity.

## Phase E — FSR FG pacing profiles

Keep `FramePacingTuning=true`.

Test separately:

1. Manual — should reproduce prior behavior.
2. Smoothness.
3. Balanced.
4. Low Latency.
5. Adaptive (experimental).

Compare:

- output FPS.
- P99 pacing error.
- present blocking.
- queue depth.
- missed deadlines.
- visible FG cadence.

Adaptive should switch profiles only when shared telemetry warrants it. If it oscillates, leave it disabled and capture logs.

## Phase F — MV / UI / HUDless validation

Use a title/path that provides UI and HUDless resources.

- Confirm valid UI composition remains unchanged.
- Confirm HUDless compare remains unchanged with valid resources.
- Inspect log for unexpected optional-resource rejection.
- If a resource is rejected, verify FG continues without crashing and telemetry identifies the rejection.

## Phase G — adaptive sharpening

A/B with the exact same scene:

- Disabled: must match prior sharpening behavior.
- Enabled with min scale 0.75.

Test static camera and fast motion under XeFG 2x/3x. Confirm the feature never becomes sharper than the configured base sharpness. Watch the live adaptive scale in telemetry.

## Device-reset / recreation paths

Where the game exposes them, test:

- swap-chain recreation.
- fullscreen/borderless transition.
- HDR state transition.
- device-reset/recovery path.

No stale FG resources, device removal, counter runaway or permanent paused state is acceptable.
