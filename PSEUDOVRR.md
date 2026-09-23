# Adaptive Fixed-Refresh Pacing Controller

> Compatibility note: older OptiScaler Performance Edition builds called this feature **PseudoVRR**. The legacy
> `FrameLockPseudoVrr*` INI keys remain supported so existing per-game profiles do not break. The implementation does
> **not** emulate VRR and does not change the monitor's physical refresh interval.

## What the controller actually controls

A fixed-refresh display remains fixed-refresh. The controller can only influence software presentation behavior:

- base/rendered frame cadence
- QPC deadline scheduling
- Present timing
- VSync/tearing-compatible target selection
- frame-generation-aware base/output cadence

Real Adaptive-Sync, FreeSync, G-SYNC, or other hardware VRR requires support and negotiation across the display,
driver, GPU, and link. Software pacing cannot manufacture that capability.

## Presentation modes

### VSync Cadence

Use this for synchronized fixed-refresh presentation. The adaptive target is restricted to clean **output refresh
divisors**. For a measured 143.999 Hz display, clean output cadences include:

- 143.999 FPS
- 71.9995 FPS
- 47.9997 FPS
- 35.9998 FPS

Frame generation is accounted for before converting the output cadence back to a rendered/base target. For example:

- 143.999 Hz + 2x FG -> 71.9995 rendered FPS for full-refresh output
- 143.999 Hz + 3x FG -> about 47.9997 rendered FPS for full-refresh output

This mode never invents an arbitrary 61.7 FPS target and calls it synchronized. If the current divisor is not
sustainable, it moves conservatively to a lower clean cadence and requires stable windows before recovering upward.

### Tearing-Paced

Use this only when the active presentation path explicitly permits tearing. DXGI reports this through the final
`DXGI_PRESENT_ALLOW_TEARING` Present flags; native Vulkan uses `VK_PRESENT_MODE_IMMEDIATE_KHR`.

Arbitrary targets are legal in this mode and are paced by the QPC deadline scheduler. The display still scans out at
its fixed refresh rate, so tearing can occur. If explicit tearing permission is not observed, the controller falls
back to VSync Cadence.

## Workload measurement

The controller does not use capped Present FPS as its sustainable-performance estimate. It measures time from one
paced boundary to the next FrameLock entry, subtracts the latest known Present/fence blocking contribution, sanitizes
invalid samples, and maintains rolling statistics:

- average and variance
- P50, P90, P95, and P99 render work
- a decaying short-term spike metric
- pacing error and missed deadlines

P95/P99 tail behavior is weighted more heavily than the average. Long pauses, loading/alt-tab stalls, clock
rollbacks, swap-chain recreation, resolution changes, refresh changes, and FG topology changes invalidate the
relevant history and trigger a warm-up period.

## Adaptation policy

The useful behavior from the legacy controller remains:

- configurable headroom
- fast drop
- slow recovery
- hysteresis
- target-change/reversal tracking
- oscillation guard
- minimum/maximum target preferences

VRAM pressure increases headroom and slows recovery. It does not reduce game texture quality. OptiScaler's existing
VRAM Guard remains responsible for trimming OptiScaler-owned resources where appropriate.

## Frame-generation behavior

The controller tracks rendered/base FPS, FG multiplier, and estimated output FPS separately. When FG is enabled,
disabled, or changes multiplier, cadence history is reset and FrameLock is rebased. The old target is preserved as a
conservative starting point and is remapped to a legal cadence before upward recovery.

Auto Benchmark may recommend a lower FG multiplier when 3x+ output is measurably unstable, but applying a benchmark
recommendation does **not** change or enable an FG mode automatically.

## Raster/scanline behavior

Adaptive fixed-refresh pacing always uses FrameLock's QPC deadline path. Legacy Hybrid Raster and Raster Sync remain
available when the adaptive controller is disabled, but they are isolated from adaptive pacing and are not presented
as substitutes for VRR.

## Recommended 143.999 Hz + XeFG 2x starting profile

Use VSync Cadence with:

- exact/custom refresh: 143.999 Hz
- output ceiling: 143.999 FPS
- max base FPS: 71.9995
- min base FPS: 48
- render headroom: 6-8%
- hysteresis: about 1 FPS
- controller policy: FG Optimized
- FrameLock sync: Deadline
- FG cadence snap/stutter guard: enabled

Use Tearing-Paced instead only if you intentionally run a tearing-capable presentation mode and want arbitrary base
targets inside the configured range.

## Final design statement

The system does not emulate hardware VRR. It is an **Adaptive Fixed-Refresh Pacing Controller**:

measure workload -> determine sustainable rendering cadence -> choose a fixed-refresh-compatible target -> schedule
frames accurately -> adapt conservatively when workload changes.

In synchronized mode it prefers exact refresh divisors. In explicitly tearing-enabled mode it may use arbitrary
QPC-paced targets. Hardware VRR remains superior because it can physically vary display scanout timing.
