# OptiScaler Performance Edition: Audit, Status and Implementation Plan

Base audited: **FrameLock + VRAM Guard FinalPass + PseudoVRR + Auto Benchmark**, with OptiColor and MXAO excluded.

## 1. Repository map and subsystem status

| Subsystem | Primary files / entry points | Status before this pass | Status after this pass |
|---|---|---|---|
| FrameLock | `OptiScaler/misc/FrameLockLimiter.h/.cpp`, `OptiScaler/misc/FrameLimit.h/.cpp` | Implemented. Deadline scheduler, timer calibration, pacing modes, Hybrid Raster, exact refresh, FG awareness. Core branch was previously user-tested. | Improved. P95/P99 render-work use, repeated-miss rebase, latency policies, central-state publication. Runtime validation of the new changes is still required. |
| PseudoVRR | `misc/FrameLockLimiter.*`, `misc/FrameLimit.*`, `menu/menu_common.cpp` | Implemented and previously reported working by the user. | Improved. Sustainable FPS is now percentile-aware; cadence attraction is gated by workload stability; existing fast-drop/slow-recovery and hysteresis are preserved. |
| VRAM Guard | `misc/VRAMGuard.h/.cpp`, FG resource paths | Implemented. DXGI budget pressure, safe/optional FG input dropping, cache release and presets. | Improved. Predictive pressure, tracked OptiScaler-owned resource telemetry, allocation rate and validation counters. Prediction can enter High early, but cannot enter Critical by itself. |
| Frame Generation common | `framegen/IFGFeature.h/.cpp`, `framegen/IFGFeature_Dx12.h/.cpp` | Implemented. Common resource/cache and interpolation state. | Improved. Resource ledger integration, queue/fence ownership tracking, fence-aware teardown and wait telemetry. |
| XeFG | `framegen/xefg/XeFG_Dx12.h/.cpp`, `hooks/FG_Hooks.cpp`, `misc/FrameLimit.cpp` | Implemented multi-frame capability and interpolation-count configuration. | Experimental adaptive multiplier controller added. 3x is preferred when configured, 2x is the reliable fallback, 4x remains highly gated/experimental. Default is OFF. |
| FSR FG | `framegen/ffx/FSRFG_Dx12.*` | Implemented existing backend. | Benefits from shared resource lifetime/telemetry changes, but adaptive multiplier policy is not applied to FSR FG in this pass. |
| DLSSG | `framegen/dlssg/DLSSG_Dx12.*` | Implemented existing backend/mod compatibility. | Benefits from shared resource lifetime/telemetry changes, but adaptive XeFG multiplier policy is not applied. |
| Auto Benchmark | `misc/FrameLimit.*`, `menu/menu_common.cpp` | Implemented single current-mode benchmark with recommendation/apply path. | Improved confidence scoring, run-local counters, topology invalidation, queue/Present metrics and virtual generated-timeline events. Multi-pass native/upscale/2x/3x/4x/stress suite is **not yet implemented**. |
| Config | `Config.h/.cpp` | Existing per-game INI system and Performance Edition settings. | Schema version 2 marker, safe migration, value validation, new adaptive XeFG/VRAM/latency flags. Old config remains compatible. |
| Swap-chain handling | `wrapped/wrapped_swapchain.cpp`, `hooks/FG_Hooks.cpp` | Existing DXGI Present/resize hooks. | Present blocking measured; resize/swap-chain recreation increments shared generations and invalidates stale timing/benchmark state. |
| DX12 synchronization | `IFGFeature_Dx12.*`, XeFG/FSRFG/DLSSG DX12 implementations | Existing command queues, command lists, fences. | Fence wait timing and resource last-use fence ownership are tracked; teardown validation added. |
| DX11 | `with_dx12/dx11_with_dx12_sc.cpp`, wrapped DXGI paths | Existing DX11 and DX11-to-DX12 FG bridge. | Fence waits through the DX11-to-DX12 bridge are instrumented. Full allocation-ledger coverage of every DX11 allocation is not implemented. |
| Vulkan | `hooks/Vulkan_Hooks.cpp` and existing Vulkan upscaler paths | FrameLock limiter hook exists. | Shared adaptive XeFG/resource-ledger work is DX12-centric. Vulkan does not receive the full XeFG adaptive-resource controller in this pass. |
| Resource lifetime validation | New `misc/ResourceLedger.h/.cpp`, `IFGFeature_Dx12.*` | Not implemented as a central validator. | Partially implemented for OptiScaler-owned DX12 FG committed copy/scratch resources. Broad whole-project validation remains a future milestone. |
| Shared performance state | New `misc/PerformanceState.h/.cpp` | Facts were distributed across FrameLock, State, VRAM Guard and benchmark code. | Implemented as a shared read-mostly fact model. FrameLock remains the single pacing-policy authority. |

## 2. Existing architecture and overlap found in audit

Before this pass, three systems observed related facts independently:

- FrameLock owned pacing, render-work estimates, FG multiplier awareness and PseudoVRR decisions.
- VRAM Guard independently owned memory pressure state.
- Auto Benchmark sampled FrameLock and VRAM data after the fact.

That architecture worked, but made 3x fallback difficult to reason about because there was no single snapshot of render timing, queue blocking, generated-output estimate and memory pressure.

The new `PerformanceState` is intentionally **not another controller**. It stores facts. FrameLock/FrameLimit remains the one place that decides final pacing policy.

## 3. Shared PerformanceState

New files:

- `OptiScaler/misc/PerformanceState.h`
- `OptiScaler/misc/PerformanceState.cpp`

Tracked state includes:

- detected and effective exact refresh rate
- render work time
- sustainable base FPS
- current base/output targets
- output FPS estimate
- pacing error and P99 pacing error
- timer overshoot
- Present blocking EWMA
- fence wait EWMA
- queue-latency estimate
- approximate queue depth in generated-frame intervals
- generated-output interval
- virtual generated-timeline lateness
- VRAM usage, budget, current pressure and predicted pressure
- tracked OptiScaler-owned bytes and peak
- missed deadlines and stutters
- virtual generated late/burst events
- resolution and swap-chain generations
- active/requested FG multiplier and backend
- render/output dimensions
- workload class
- fallback reason
- FG stability score
- HDR/output and fullscreen state

### Important limitation

The generated-frame timeline is **virtual/estimated**. XeFG does not expose every generated scanout as a normal game `Present` that this fork can reliably timestamp. The estimator uses observed base-frame cadence, active multiplier, target output interval, Present blocking and fence waits. UI/logging must not describe it as exact hardware-generated-frame latency.

## 4. XeFG 3x stability controller

New experimental configuration:

```ini
[XeFG]
AdaptiveMultiplier=false
PreferredMultiplier=3
StableSecondsBeforeUpgrade=8
FallbackCooldownSeconds=12
EmergencyDisable=false
```

Default is OFF so old behavior is preserved.

### Stability inputs

A 0–100 stability score considers:

- P99 pacing error relative to base interval
- missed-deadline deltas
- stutter deltas
- virtual generated-late events
- virtual generated-burst events
- Present blocking
- queue/fence latency
- actual VRAM pressure
- predicted VRAM pressure

### Hysteresis policy

- Evaluation happens in 500 ms windows, not every Present.
- Three unstable windows are required before 3x falls to 2x.
- Normal recovery path is always **2x first**.
- 2x upgrades back to 3x only after the configured stable duration and fallback cooldown.
- 4x requires backend capability, a long stable period, score >= 95, no new late/burst events, and comfortable VRAM headroom.
- Optional emergency FG disable is OFF by default and only applies to severe sustained critical-memory failure while already at 2x.

### Timing vs visual instability

This controller detects **timing instability**. It does not claim to detect visual artifacts such as ghosting/disocclusion because no reliable generic visual-quality signal exists in this path. Visual instability remains a manual/backend-specific concern.

## 5. FrameLock improvements

Existing absolute QPC deadline scheduling is preserved.

Added/improved:

- P95/P99 render-work sample window for sustainable FPS estimation.
- Rebase after repeated significant missed deadlines instead of repeatedly chasing a stale deadline.
- Existing adaptive timer overshoot calibration remains authoritative.
- Present and fence blocking are published into shared state so timing stalls can be distinguished from limiter timing.
- Latency policies:
  - Smoothness
  - Balanced
  - Low Latency

Latency policies only adjust conservative headroom/spin behavior. They do not bypass safety checks or resource synchronization.

## 6. PseudoVRR improvements

PseudoVRR now uses a robust render-work estimate:

- EWMA
- P95
- P99

The sustainable-work estimate uses the worst useful robust statistic rather than a simple average.

Cadence attraction is permitted only when the render window is stable enough (`P99` reasonably close to `P95`). Fast drop, slow recovery and hysteresis remain intact.

Current workload classes:

- Stable
- Recovering
- Heavy
- Highly variable
- GPU/queue limited
- Memory pressured
- CPU limited (enum exists, but this pass deliberately does not auto-classify it because current generic signals cannot reliably separate a CPU stall from unsaturated GPU work)

## 7. VRAM allocation ledger and pressure prediction

New files:

- `OptiScaler/misc/ResourceLedger.h`
- `OptiScaler/misc/ResourceLedger.cpp`

Tracked record fields include:

- pointer
- owner subsystem/type label
- allocation byte estimate
- dimensions
- format/flags
- resolution generation
- last queue ID
- last fence value
- ownership class

Ownership classes:

- OptiScaler-owned
- shared/wrapped
- observed game-owned
- unknown

Current direct instrumentation covers OptiScaler-owned DX12 FG committed copy/scratch resources. It does **not** claim total process/GPU allocation visibility.

### Prediction

The ledger tracks recent allocation rate and computes a short-horizon, capped forecast. This may transition VRAM Guard to **High** early. Prediction alone is never allowed to enter **Critical** or perform destructive/aggressive policy changes.

## 8. Resource lifetime validator

Debug-only validation (`PerformanceEdition.ResourceValidation`) checks currently instrumented resources for:

- duplicate live ownership registration
- stale resolution generation
- incompatible size/format/flags reuse
- wrong queue ownership
- release before last known fence completion

Teardown submits pending work and waits for relevant slot fences before releasing cached FG resources.

If a fence cannot be proven complete, release is deferred and the pointer remains tracked for a later safe retry. Safety wins over use-after-free.

### Scope limitation

This validator is **partial**. It is not yet a universal validator for every D3D11/D3D12/Vulkan allocation in OptiScaler. Broadening coverage is intentionally deferred until the instrumented DX12 FG path is runtime-proven.

## 9. Dynamic resolution and swap-chain coordination

Resolution/swap-chain generations are published centrally.

On internal/output resolution change or successful swap-chain recreation:

- shared generation increments
- FrameLock timing history is reset/rebased
- ResourceLedger marks stale-generation resources for diagnostics
- an active Auto Benchmark run is invalidated because workload topology changed

FG and non-FG resize hooks are conditioned to avoid double-generation increments.

## 10. Auto Benchmark correctness changes

Fixed a previous correctness issue where some counter baselines were static locals and could survive between benchmark runs.

New result data includes:

- average Present block time
- average queue/fence latency
- virtual generated late events
- virtual generated burst events
- resolution generation
- workload class
- confidence score separate from stability score

A benchmark is invalidated if:

- FG backend changes
- FG multiplier changes unexpectedly
- effective refresh changes materially
- resolution generation changes
- insufficient/low-confidence data is collected

Confidence and stability are different:

- **Stability score**: how good the measured pacing/workload was.
- **Confidence score**: whether the run was sufficiently representative to trust the recommendation.

### Deferred benchmark milestone

The requested automated suite with separate native, upscale-no-FG, 2x, 3x, 4x and VRAM-stress passes is **not implemented in this pass**. Arbitrarily hot-switching upscaler/FG contexts across games is higher-risk and needs runtime proof of the central state/adaptive controller first. It should be a later experimental feature flag, not quietly forced into the stable path.

## 11. Configuration safety

Schema marker:

```ini
[PerformanceEdition]
ConfigVersion=2
ResourceValidation=false
```

New user values are clamped/validated on load. Existing configs remain valid because missing keys use conservative defaults.

Experimental behavior is opt-in:

- adaptive XeFG multiplier: OFF
- emergency FG disable: OFF
- resource lifetime validation: OFF

## 12. API coverage and compatibility notes

### DirectX 12
Primary/fullest coverage in this pass. XeFG controller, resource ledger, fence ownership and copy/scratch teardown are implemented here.

### DirectX 11
Existing DX11-to-DX12 bridge remains intact. Bridge fence waits are instrumented. Whole-DX11 allocation-ledger coverage is not yet implemented.

### Vulkan
Existing FrameLock limiter hook is preserved. The DX12-specific XeFG resource controller/ledger does not pretend to cover Vulkan resources. No Vulkan behavior was removed or replaced.

## 13. Known assumptions and risks

1. Virtual generated-frame events are estimates, not exact generated scanout timestamps.
2. Present blocking may include driver/VSync/window-compositor behavior, not only GPU queue pressure.
3. Queue depth is an estimate based on observed queue latency divided by generated-output interval.
4. ResourceLedger visibility is partial and currently strongest for DX12 FG-owned committed resources.
5. Relaxed texture size-class pooling is deliberately deferred because copy-compatible D3D12 textures require exact semantics; unsafe aliasing would risk corruption/device removal.
6. Visual FG instability cannot be generically inferred from timing telemetry.
7. CPU-limited classification is not automatically asserted without a reliable generic CPU-vs-GPU saturation signal.
8. Auto Benchmark remains single-mode per run in this milestone.
9. Windows/MSVC/DX12 runtime behavior has not been executed in the Linux build environment used for this source pass.

## 14. Implementation order

Completed/in progress in this Stability Pass:

1. Audit and status report
2. Shared performance state
3. Partial resource lifetime validator for DX12 FG-owned resources
4. Virtual XeFG generated timeline and burst/late estimation
5. Adaptive XeFG 3x -> 2x fallback with hysteresis, default OFF
6. VRAM allocation ledger and pressure prediction
7. Dynamic-resolution/swap-chain generation coordination
8. Latency policy and repeated-deadline rebase
9. Auto Benchmark confidence/topology validation
10. Config schema/migration and UI diagnostics

Deferred until runtime validation:

- whole-project allocation ledger
- generic texture size-class pooling
- automated multi-mode benchmark suite
- automatic visual-instability detection
- full Vulkan resource-ledger/adaptive-FG path
- broader DX11 native allocation validation

## 15. Release philosophy

This pass preserves the working old paths and hides risky policy behind feature flags. The intended release progression is:

1. Baseline test with all new experimental toggles OFF.
2. Enable resource validator and exercise resize/window/backend transitions.
3. Test XeFG 3x manually with adaptive controller OFF.
4. Enable adaptive 3x/2x policy and verify fallback/hysteresis.
5. Stress VRAM and verify prediction/critical separation.
6. Validate benchmark confidence and invalidation rules.
7. Only then consider enabling adaptive XeFG by default in a later release.

The priority remains correct pacing and safe resource lifetime over a larger FPS number.
