# Adaptive Fixed-Refresh Pacing Controller - Audit and Implementation Notes

## Scope

This pass audits and improves the existing OptiScaler Performance Edition pacing stack. It deliberately preserves the
working FrameLock, FG, VRAM Guard, Auto Benchmark, provider, and per-game configuration infrastructure instead of
replacing it.

The legacy `PseudoVRR` symbol/config names remain where changing them would break existing profiles. User-facing
terminology and controller behavior now describe the feature accurately: **Adaptive Fixed-Refresh Pacing**.

## Repository audit

### Existing systems reused

- `OptiScaler/misc/FrameLockLimiter.*`
  - QPC deadline scheduler
  - deadline accumulation via `nextDeadline += interval`
  - sleep/yield/spin calibration
  - legacy Hybrid Raster / Raster Sync support
  - exact/custom refresh timing
  - existing PseudoVRR state and configuration compatibility
- `OptiScaler/misc/FrameLimit.*`
  - limiter/provider integration
  - FG backend/multiplier resolution
  - Auto Benchmark
  - adaptive XeFG fallback/stability logic
- `OptiScaler/misc/PerformanceState.*`
  - shared render/pacing/FG/VRAM telemetry
  - candidate/submitted/completed FG counters
  - frame-time discontinuity and temporal-reset counters
- `OptiScaler/wrapped/wrapped_swapchain.cpp`, `OptiScaler/hooks/FG_Hooks.cpp`
  - DXGI Present hooks and final VSync/tearing flags
  - Present-block timing
- `OptiScaler/hooks/Vulkan_Hooks.cpp`
  - native Vulkan swap-chain/present interception
- existing DX12 paths
  - fence-wait telemetry already published into `PerformanceState`
- existing VRAM Guard / ResourceLedger
  - VRAM pressure and OptiScaler-owned allocation tracking
- existing FSR FG adaptive pacing profiles
  - retained as-is and fed by the shared performance state

### Problems found in the old adaptive design

1. The old PseudoVRR model correctly stated that it was not hardware VRR, but the policy still mixed arbitrary adaptive
   targets with optional attraction toward clean refresh divisors.
2. Hybrid Raster / Raster Sync remained coupled closely enough to the adaptive feature that they could be interpreted
   as a software VRR substitute.
3. Sustainable-performance estimation used a render-work interval that could contain significant Present blocking,
   allowing synchronized presentation to feed its own waiting time back into the workload estimator.
4. VSync cadence and arbitrary tearing-paced targets were not two explicit policy modes.
5. FG multiplier changes did not have a dedicated fixed-refresh cadence policy guaranteeing conservative rebasing.
6. Oscillation was counted only indirectly; there was no transitions/minute + direction-reversal guard.
7. Presentation mode itself was not published into the shared performance state, which made native Vulkan and final
   DXGI tearing behavior harder to distinguish reliably.
8. Auto Benchmark could recommend raster-assisted sync even though raster phase control is not required by the
   adaptive fixed-refresh controller.

## Implemented design

### 1. Two explicit presentation policies

A new `FrameLockAdaptivePresentationMode` config value is loaded/saved:

- `0`: VSync Cadence
- `1`: Tearing-Paced

Legacy PseudoVRR keys remain compatible.

#### VSync Cadence

Only clean **output** refresh divisors are selected. The selected output cadence is converted to base FPS through the
active FG multiplier:

`base FPS = refresh Hz / (output divisor * FG multiplier)`

For 143.999 Hz:

- no FG: 143.999 / 71.9995 / 47.9997 / 35.9998 base FPS for divisors 1/2/3/4
- 2x FG: 71.9995 base FPS gives approximately 143.999 output FPS
- 3x FG: 47.9997 base FPS gives approximately 143.999 output FPS

The controller moves downward quickly but at most one clean divisor per transition. Recovery moves upward one divisor
at a time after multiple stable windows.

#### Tearing-Paced

Arbitrary QPC targets are permitted only when the active presentation path has explicitly demonstrated tearing:

- DXGI: final Present flags contain `DXGI_PRESENT_ALLOW_TEARING`
- native Vulkan: swap chain uses `VK_PRESENT_MODE_IMMEDIATE_KHR`

If the user selects Tearing-Paced but tearing is not actually active, the controller falls back to VSync Cadence.

No code claims or attempts to change the monitor's physical scanout interval.

### 2. Raster isolation

While adaptive fixed-refresh pacing is enabled, FrameLock forces the adaptive scheduler through the deadline/QPC path.
Legacy Hybrid Raster / Raster Sync remain available only as separate compatibility/experimental FrameLock modes when
the adaptive controller is disabled.

### 3. Render-work collector

The adaptive collector now maintains a rolling 240-sample history with:

- average
- variance
- P50
- P90
- P95
- P99
- decaying short-term render spike

The interval is measured from the last actual paced boundary to the next limiter entry. Latest Present blocking is
removed from the workload sample so VSync/compositor waiting does not masquerade as rendering cost. Fence waiting is
kept in the workload cost because sustained GPU/queue pressure is a real throughput limit, while fence wait remains
published separately for diagnostics and workload classification.

Invalid, non-finite, non-positive, rollback, and severe >250 ms discontinuity samples are rejected/reset.

### 4. Sustainable FPS policy

The sustainable estimate is based on the worst of:

- bounded render EWMA
- P95
- 97% of P99

then reduced by configured headroom and VRAM-pressure headroom.

This prevents average FPS from being treated as continuously sustainable when tail render time says otherwise.

### 5. Fast drop / slow recovery

The controller evaluates target policy at 250 ms intervals.

- deterioration: 1-2 bad windows before dropping, with immediate response for critical pressure/repeated misses
- VSync Cadence: one divisor downward per transition
- Tearing-Paced: maximum 8 FPS downward step, 12 FPS under critical VRAM pressure
- recovery: 3-5 stable windows depending policy, plus extra windows under VRAM pressure or oscillation
- Tearing-Paced recovery: 1-2 FPS upward steps

### 6. Oscillation guard

Tracked state now includes:

- target changes per minute
- direction reversals
- recent reversal state

The guard activates for frequent transitions/reversals, adds hysteresis in Tearing-Paced mode, and requires longer
stable recovery windows.

### 7. FG-aware cadence and topology resets

Rendered/base FPS, multiplier, and estimated output FPS remain separate.

On FG enable/disable/multiplier changes:

- adaptive history resets
- FrameLock re-arms its QPC timeline
- the previous base target is preserved as a conservative ceiling
- VSync Cadence remaps it down to a valid clean cadence
- upward recovery waits through warm-up/stable windows

Example: a 143.999 Hz 3x target near 47.9997 base switching to 2x does not jump immediately to 71.9995. It first
remaps conservatively to 35.99975 base (71.9995 output) and can recover later if measurements support it.

### 8. Presentation-state tracking

`PerformanceState` now records:

- presentation mode known/unknown
- final tearing permission
- present sync interval semantic
- presentation generation

A change clears stale Present-block EWMA and increments the generation so adaptive/benchmark history can be invalidated.
Vulkan swap-chain recreation now publishes presentation mode and notifies the shared swap-chain generation.

### 9. VRAM-pressure integration

Existing VRAM Guard state is read by the adaptive controller:

- normal: below 88%
- high: >=88% actual or >=90% predicted -> +2 percentage points headroom and slower recovery
- critical: >=94% actual or >=96% predicted -> +5 percentage points headroom, faster downward response, slower recovery

No game texture-quality setting is automatically reduced by this controller.

### 10. Workload classification

Shared classification now distinguishes:

- Stable
- Recovering
- Heavy
- Highly Variable
- CPU Limited
- GPU Limited
- Presentation Limited
- Memory Pressured

Present blocking, fence wait, queue latency, render-tail behavior, and VRAM pressure are considered separately.

### 11. Reset/rebase behavior

Adaptive history is invalidated/rebased by:

- severe QPC/timing discontinuity
- repeated severe deadline miss
- FG multiplier/topology change
- exact refresh change
- render/output resolution generation change
- swap-chain recreation
- presentation/VSync/tearing-mode generation change
- adaptive policy/configuration change

Existing temporal-reset and resource-generation paths cover provider/device/resolution changes already present in the
fork. A 30-frame adaptive warm-up follows history resets before upward recovery is allowed.

### 12. Auto Benchmark

Auto Benchmark now:

- runs adaptive measurement using Deadline + VSync Cadence, not Hybrid Raster
- uses the sanitized adaptive render-work samples
- recommends a clean output divisor based on P95/P99 tail performance and headroom
- recommends Tearing-Paced only if tearing is already active; it never enables tearing
- reports an FG multiplier preference and may prefer 2x when an active 3x+ run is measurably unstable
- never applies/changes the FG multiplier automatically
- invalidates or reduces confidence after resolution, swap-chain, presentation mode, fullscreen, refresh, FG, or severe
  timing-discontinuity changes

### 13. UI / telemetry

The menu now exposes **Adaptive Fixed-Refresh Pacing** instead of presenting the feature as VRR emulation. Live data
includes:

- adaptive presentation mode
- exact/effective refresh
- base and output target
- selected output divisor
- FG backend and multiplier through existing telemetry
- P50/P95/P99 render work
- average/variance and short-term spike in FrameLock telemetry
- sustainable base FPS
- warm-up frames
- changes/minute and reversal count
- oscillation-guard status
- latest target-change reason
- existing Present block, fence wait, queue-depth estimate, pacing error, missed deadlines, VRAM pressure, and workload
  classification

Transitions are logged with their reason.

## Compatibility

- Existing `FrameLockPseudoVrr*` configuration names are retained.
- Existing FrameLock fallback and QPC scheduler are preserved.
- DX11/DX12 final Present flags are observed directly.
- Native Vulkan IMMEDIATE presentation can use Tearing-Paced; synchronized Vulkan modes remain cadence-oriented.
- Legacy raster modes remain available but are isolated from adaptive pacing.
- New presentation policy defaults to VSync Cadence.
- No experimental FG mode or tearing mode is enabled automatically.

## Validation performed in this environment

A full OptiScaler Windows build/runtime test cannot be completed in this Linux container because the project depends
on MSVC/Windows/DXGI/D3D headers and runtime APIs. No smoothness or latency improvement is therefore claimed from this
environment alone.

Performed instead:

1. Standalone C++20 cadence math test compiled and executed successfully with `g++`.
2. Exact 143.999 Hz divisor math verified.
3. 2x/3x FG base/output cadence math verified.
4. Conservative 3x -> 2x rebase math verified.
5. Headroom mapping verified for cadence and arbitrary tearing-paced policy.
6. Delimiter/bracket structural checks passed for every modified C/C++ source/header.
7. Config load/save symbols and Present-state publication call sites were checked.
8. User-facing menu text was checked for the obsolete “Adaptive Sync Emulator” wording.

Hardware validation should still cover DX11, DX12, native Vulkan, fullscreen/borderless, VSync on/off, DXGI tearing,
Vulkan IMMEDIATE/FIFO/MAILBOX, XeFG 2x/3x and fallback, pause/alt-tab/loading, dynamic resolution, swap-chain resize,
VRAM pressure, and exact 143.999 Hz timing while recording P95/P99 render time, P99 pacing error, misses, transitions
per minute, Present blocking, fence wait, queue depth, latency, and generated-frame stability.

## Final design statement

The system does **not** emulate hardware VRR.

It is an **Adaptive Fixed-Refresh Pacing Controller** whose job is:

`measure workload -> determine sustainable cadence -> choose a fixed-refresh-compatible target -> schedule accurately -> adapt conservatively`

VSync Cadence uses exact refresh divisors. Tearing-Paced allows arbitrary QPC-paced targets only when tearing is
explicitly active. Hardware VRR remains superior because only the display/driver link can physically vary scanout
timing.
