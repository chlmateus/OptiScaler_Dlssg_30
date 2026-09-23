# VRAM Guard Final Pass - FS25 test

## Build

Build `Release | x64` exactly like the previous working VRAM Guard BuildFix.

The dependency bootstrap remains included.

## Test A - baseline

Use the old working build first and record in the same save/location:

- average FPS
- 1% low if available
- VRAM usage
- Windows VRAM budget
- frametime behavior while driving quickly

Use the same XeFG/upscaler/graphics settings for every test.

## Test B - 8 GB Balanced

Install the Final Pass build and press:

`VRAM Guard -> 8 GB Balanced`

Recommended:

- XeFG 2x
- FrameLock Deadline / Balanced
- external FPS limiter OFF
- reported VRAM Native

Check that the menu shows:

- Adaptive mode ON
- Release cached copies ON
- High 88%
- Critical 94%
- Recover 80%

Run the same scene.

Balanced should not intentionally lower render resolution.

## Test C - pressure behavior

If the game reaches Critical pressure, the menu may show:

`Adaptive emergency mode ACTIVE`

At that point optional UI/HUDless FG surfaces are temporarily refused. Depth and motion vectors remain active.

If UI artifacts appear, disable:

`Drop optional FG inputs at critical pressure`

The rest of VRAM Guard can remain enabled.

## Test D - FPS + VRAM

Press:

`8 GB Performance`

This enables a 1.5x upscale ratio. At 1080p output that is roughly 720p internal rendering.

Compare:

- average FPS
- 1% low
- GPU utilization
- VRAM usage
- image quality

If the loss of source detail is too visible, switch back to `Game/default ratio` while keeping the rest of VRAM Guard enabled.

## Reported VRAM

Only lower reported VRAM if the game itself keeps filling the Windows budget after the new memory helpers are active.

Recommended order for an 8 GB card:

1. Native
2. 7 GB
3. 6 GB only if truly necessary

Lower reported VRAM can make games choose smaller texture/streaming pools, so it is not the first-line option for avoiding blurry textures.
