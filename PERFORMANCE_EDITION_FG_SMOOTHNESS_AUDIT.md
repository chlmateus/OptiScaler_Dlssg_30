# Performance Edition FG Smoothness / IQ Pass — Repository Audit

## Baseline

Base archive: `OptiScaler-master-PerformanceEdition-StabilityPass-V1-BuildFix1.zip`.
The packaged source does not contain `.git` metadata, so the archive itself was treated as the immutable baseline. No unrelated working-tree edits existed inside this copy.

## Dependency map

### Frame counters and dispatch ownership

- `OptiScaler/framegen/IFGFeature.h/.cpp`
  - Owns frame count, interpolation metadata, per-frame reset data and generic FG resources.
  - Candidate dispatch selection originates here.
  - Compatibility-facing `LastDispatchedFrame()` now means last successfully submitted frame.
- `OptiScaler/framegen/xefg/XeFG_Dx12.cpp`
- `OptiScaler/framegen/ffx/FSRFG_Dx12.cpp`
- `OptiScaler/framegen/dlssg/DLSSG_Dx12.cpp`
  - Provider-specific dispatch calls.
  - Submitted state must advance only after the provider accepts/submits the frame.
  - Present-cycle completion is published separately from candidate/submitted state.

### Present / frame limiting

- `OptiScaler/misc/FrameLimit.cpp/.h`
  - Chooses legacy internal limiter vs FrameLock.
  - Legacy path previously used a function-static wall-clock timestamp.
  - Now owns reset generation for per-thread/per-session QPC pacing.
- `OptiScaler/misc/FrameLockLimiter.cpp/.h`
  - Existing preferred QPC deadline scheduler and exact-refresh-aware timing authority.
  - Existing FrameLock behavior is preserved.
- Present and swap-chain paths feed `FrameLimit::sleep()` through the existing hooks/wrappers.

### Shared pacing state

- `OptiScaler/misc/PerformanceState.h/.cpp`
  - Existing central state from Stability Pass.
  - Extended with candidate/submitted/completed counters, validation/reset diagnostics, FSR pacing profile, and adaptive sharpening scale.

### FSR frame pacing

- `OptiScaler/framegen/ffx/FSRFG_Dx12.cpp/.h`
  - Existing FidelityFX frame-pacing tuning API preserved.
  - New profile selector resolves to the same existing API values.

### Motion vectors / HUDless / UI

- `OptiScaler/framegen/IFGFeature_Dx12.cpp/.h`
  - Common D3D12 FG resource validation.
- Provider files above consume mandatory depth/MV and optional UI/HUDless resources.
- Existing HUDfix/resource-tracking providers remain unchanged.

### Sharpening

- `OptiScaler/shaders/rcas/RCAS_Common.cpp`
  - Shared RCAS / depth-aware constants for D3D11, D3D12 and Vulkan shader backends.
  - Adaptive modifier changes only the existing sharpness scalar and is disabled by default.

### Configuration and UI

- `OptiScaler/Config.h/.cpp`
- `OptiScaler/menu/menu_common.cpp`
- `OptiScaler.ini`
- `Config.md`

## Important audit findings

1. `IFGFeature` previously performed unsigned subtraction and then tested `diff < 0`; that rollback check can never succeed for an unsigned type.
2. Candidate selection previously mutated `_lastDispatchedFrame` before provider success. A provider failure could therefore skip a frame in the logical submitted timeline.
3. `SetFrameCount()` did not establish a safe rollback/reset boundary.
4. The legacy limiter used `GetSystemTimePreciseAsFileTime()` plus a function-static previous timestamp and assumed every active FG path was exactly 2x.
5. Frame-time inputs could reach provider consumption without a second validity check if a slot was left zero/uninitialized.
6. Existing FSR FG pacing tuning already exposed AMD's tuning structure; replacing it was unnecessary. Profiles can safely wrap that API.
7. Existing `PerformanceState` is the correct telemetry aggregation point; no second pacing-state service is needed.
8. XeFG/FSR FG/DLSSG are D3D12 provider implementations. The new common RCAS sharpness modifier remains cross-API, but provider-resource validation in this pass is intentionally D3D12-specific.
9. There is no generic, provider-independent GPU callback proving each generated scanout completed. The new `completed` counter is explicitly a present-cycle completion boundary, not fabricated hardware completion telemetry.

## Compatibility policy

- Existing configuration names are preserved.
- New FSR pacing profile defaults to `Manual`, reproducing old manual tuning behavior.
- Adaptive sharpening defaults to disabled.
- Provider dispatch order is unchanged.
- Legacy limiter remains available and FrameLock remains the preferred scheduler.
- Optional UI/HUDless input rejection does not fail the mandatory FG dispatch path.
