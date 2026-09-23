# Performance Edition — FG Smoothness and Image-Quality Pass V1

## Implemented

### 1. Counter synchronization

- Removed impossible unsigned rollback checks.
- Added comparison-safe rollback handling.
- Candidate, submitted and present-cycle-completed state are separate.
- Provider failure no longer advances submitted state.
- Counter rollback requests a temporal reset and publishes telemetry.

### 2. Frame-time sanitation

- Rejects NaN, infinity, zero and negative frame-time inputs.
- Severe absolute/relative discontinuities request temporal reset.
- Provider-side consumption revalidates Input/Opti frame-time sources.
- Dispatch-facing frame time is clamped to a conservative valid range.

### 3. Resettable QPC legacy pacing

- Removed wall-clock `FILETIME` pacing timestamp.
- Legacy limiter now uses monotonic QPC timing.
- Pacing state is thread-local/per-session and keyed by swap chain + reset generation.
- Session resets occur on limiter reset, resolution change, disabled cap, topology/session change, target change or multiplier change.
- First Present after a reset is not artificially delayed.

### 4. Interpolation-aware limiting

- Active FG multiplier is resolved from current FG provider / detected DLSSG state.
- Exact/custom display refresh is reused from FrameLock display discovery.
- Output target is capped to known refresh; base target is output target divided by actual multiplier.
- 2x/3x/4x-style interpolation no longer shares a hardcoded 2x legacy assumption.

### 5. Dispatch-state separation

- Candidate selection is side-effect free with respect to submitted state.
- Providers call `CommitDispatch()` only after their successful provider-specific submission path.
- Present-cycle completion is tracked separately and labeled honestly in telemetry.

### 6. FSR FG pacing profiles

New `FSRFG/FPTProfile`:

- `0 Manual` — previous behavior, default.
- `1 Smoothness` — larger safety margin / fence-wait preference.
- `2 Balanced` — conservative middle ground.
- `3 Low Latency` — smaller margin for stable workloads.
- `4 Adaptive` — experimental; resolves among the profiles from shared timing/queue/VRAM telemetry.

The implementation still uses the existing FidelityFX frame-pacing tuning API and does not reorder FSR provider calls.

### 7. Shared diagnostics

`PerformanceState` now exposes:

- FG candidate / submitted / present-cycle-completed counters.
- counter rollback count.
- invalid frame-time count.
- severe discontinuity count.
- temporal reset count.
- invalid motion-vector metadata count.
- rejected optional UI/HUDless resource count.
- effective FSR FG pacing profile.
- adaptive sharpness multiplier.

### 8. Motion-vector and history safety

- MV scale must be finite and within a broad sanity bound.
- D3D12 MV resource must be a valid 2D texture with valid tracked bounds.
- Invalid mandatory MV/depth inputs skip dispatch and request history reset.
- Provider/target state changes request a short temporal reset.
- Resolution change resets limiter pacing and FG temporal history.

### 9. HUDless/UI validation

Optional UI/HUDless resources are checked for:

- non-null D3D12 resource.
- 2D texture type.
- valid dimensions / format / sample count.
- tracked rectangle inside the actual resource.
- implausibly tiny optional resource relative to interpolation output.

Invalid optional resources are skipped rather than promoted into mandatory provider failure.

### 10. Adaptive sharpening

New CAS settings:

- `AdaptiveSharpnessEnabled=false` by default.
- `AdaptiveSharpnessMinScale=0.75` by default.

When explicitly enabled, the existing RCAS/depth-aware sharpness is only reduced, never increased, when telemetry indicates high FG multiplier, unstable pacing, queue pressure, or critical VRAM pressure. Motion-adaptive sharpening also receives divide-by-zero/invalid-dimension guards.

## Deliberately not changed

- No provider call-order redesign.
- No automatic image-quality reduction.
- No new sharpening pass.
- No forced adaptive FSR profile by default.
- No claim that present-cycle completion equals hardware-generated-frame scanout completion.
