# OptiScaler VRAM Guard fork

This fork is based on the working FrameLock Final tree and adds a conservative VRAM-pressure toolkit.

## What it does

- Live DXGI local-memory budget telemetry (budget, current usage, headroom, pressure %).
- Per-game reported-VRAM cap using OptiScaler's existing DXGI/Vulkan VRAM spoofing.
- Safe FG memory saver preset: disables Depth/Velocity/HUDless `ValidNow` paths that OptiScaler itself marks as using extra VRAM.
- Aggressive FG memory saver preset: additionally disables separate UI/HUDless FG inputs when available. This can reduce full-resolution auxiliary resources but may cause artifacts.

## What it does NOT do

- It does not compress arbitrary game textures.
- It does not delete game-owned GPU resources.
- It does not guarantee lower VRAM use in games that ignore reported VRAM.

## RTX 3070 8 GB starting profile

1. Open OptiScaler menu -> VRAM Guard.
2. Start with Reported VRAM = 7 GB.
3. Use Safe FG memory saver.
4. Restart the game after changing reported VRAM.
5. Watch Windows budget usage. If sustained pressure remains above ~85-90%, try 6 GB.
6. Only use Aggressive FG saver if needed and verify UI/FG quality afterward.

The reported-VRAM cap is per-game because OptiScaler.ini is installed per game.
