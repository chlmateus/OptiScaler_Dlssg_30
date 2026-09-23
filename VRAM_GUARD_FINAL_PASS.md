# VRAM Guard Final Pass

This branch is the finished OptiScaler Performance branch:

- FrameLock Final
- FG/XeFG pacing features
- VRAM Guard
- final VRAM / performance optimization pass
- no OptiColor code

## What changed in this pass

### 1. Cached FG resource cleanup

OptiScaler's DX12 frame-generation base caches full-resolution resource copies in `_resourceCopy` for paths that need a persistent copy (for example ValidNow/resource-flip cases).

The final pass adds `IFGFeature_Dx12::ReleaseCachedResources()` and calls it when XeFG, FSR FG or DLSSG backend objects are torn down/recreated.

It releases:

- cached committed D3D12 copy resources
- stale frame-resource maps that borrow those pointers
- copy command allocators/lists, which are recreated lazily only if a copy is needed again

This is a real cleanup path, not a reported-VRAM trick.

### 2. Lean XeFG / FG profile

The Lean profile disables avoidable OptiScaler copy paths:

- Depth ValidNow OFF
- Velocity ValidNow OFF
- HUDless ValidNow OFF
- Resource Flip OFF
- Make Depth Copy OFF
- Make MV Copy OFF
- Draw UI Over FG OFF
- Accept First HUDless ON
- HUDless comparison/debug path OFF

It does not delete or downscale game textures.

### 3. Adaptive VRAM pressure controller

Optional runtime controller driven by DXGI local-memory budget telemetry.

Default preset thresholds:

- High: 88%
- Critical: 94%
- Recover: 80%

Hysteresis prevents the policy from rapidly toggling around one threshold.

When `Drop optional FG inputs at critical pressure` is enabled and pressure reaches Critical, OptiScaler temporarily refuses:

- UIColor
- HudlessColor

It never drops:

- Depth
- Motion vectors / Velocity

The optional surfaces are accepted again after usage recovers below the configured recovery threshold.

This behavior is opt-in because some games need separate UI/HUDless inputs for artifact-free FG.

### 4. Per-frame monitoring without per-frame DXGI spam

`VRAMGuard::Tick()` is called from the wrapped real Present path, but the expensive DXGI budget query is internally rate-limited to once every 500 ms.

The menu now shows:

- current usage
- Windows budget
- headroom
- pressure state
- peak usage
- whether adaptive emergency mode is currently dropping optional FG inputs

### 5. 8 GB presets

#### 8 GB Balanced

Recommended quality-first preset.

- Lean FG resource path
- cached-copy release ON
- adaptive pressure mode ON
- 88 / 94 / 80 thresholds
- critical optional-input protection ON
- game/default render ratio

This preset does not intentionally reduce render resolution.

#### 8 GB Performance

Same as Balanced plus:

- Upscale Ratio Override = 1.5x

At 1920x1080 output, 1.5x corresponds to roughly 1280x720 internal rendering when the selected upscaler honors the override.

This is the part of VRAM Guard that can directly increase GPU-limited FPS as well as reduce render-target pressure. It trades source-image detail for performance.

### 6. Manual performance relief

Buttons are included for:

- 1.5x ratio
- 1.7x ratio
- Game/default ratio

These alter render resolution, not texture resolution. Some games apply the ratio immediately; others recreate the upscaler after a settings change or restart.

## What this pass deliberately does NOT do

### No half-resolution depth or motion vectors

XeFG/FSR FG/DLSSG depend on depth and motion vectors with specific geometry/resolution semantics. Arbitrarily halving those buffers can save memory but also break interpolation, disocclusion handling and motion reconstruction. The final pass therefore keeps mandatory FG inputs intact.

### No arbitrary game texture eviction

VRAM Guard does not destroy game-owned textures or resources. It only changes OptiScaler-controlled behavior and optional reported-VRAM spoofing.

### No claim that lower VRAM always means higher FPS

If the game is comfortably under budget, memory savings may mainly improve headroom. FPS gains are most likely when the system was paging/evicting resources or when the Performance render-ratio preset is used.

## Recommended RTX 3070 / 8 GB setup

Start with:

1. Press `8 GB Balanced`.
2. Leave reported VRAM at Native initially.
3. Run the same FS25 scene for several minutes.
4. Watch peak budget usage and 1% lows.
5. If usage still sits around 90-95%+, try 7 GB reported VRAM on the next game launch.
6. If you also want more GPU FPS, use `8 GB Performance` or the 1.5x ratio button.
7. Only use Aggressive FG Saver if UI/HUDless quality is acceptable.

## Expected effect

The exact VRAM saved depends on which optional/copy paths the game actually triggers. The cache-cleanup fix matters most after FG/swapchain recreation or when copy paths were used. Lean mode prevents several avoidable full-resolution copies from being requested in the first place.

The Performance preset is expected to produce the largest direct FPS improvement because it also lowers internal rendering workload.
