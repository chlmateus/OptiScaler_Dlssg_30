# VRAM Guard build fix

Fixes the Release x64 compile error in `OptiScaler/menu/menu_common.cpp`:

`error C2065: 'menuResScale': undeclared identifier`

`RenderVramGuardSettings(RenderMenuContext& ctx)` now binds `ctx.menuResScale` in the same way as other OptiScaler menu rendering functions:

```cpp
auto& menuResScale = ctx.menuResScale;
```

No limiter, FrameLock, XeFG, or VRAM Guard behavior was otherwise changed.
