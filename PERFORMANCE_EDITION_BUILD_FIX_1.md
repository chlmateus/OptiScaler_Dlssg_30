# Performance Edition Stability Pass V1 - Build Fix 1

## Build failure fixed

Visual Studio Release x64 failed in `OptiScaler/Config.cpp` while saving the Performance Edition configuration schema version.

The failing code called:

```cpp
GetIntValue(2)
```

`GetIntValue` is intentionally declared to accept `std::optional<T>`, so a plain integer literal cannot be used for template argument deduction. This caused C2672 and then a secondary/misleading `SimpleIni::SetValue` overload error (C2660).

## Fix

The configuration schema version is a fixed file-format value rather than an optional user setting, so it is now written directly:

```cpp
ini.SetValue("PerformanceEdition", "ConfigVersion", "2");
```

No FrameLock, PseudoVRR, VRAM Guard, XeFG stability, resource-lifetime, or benchmark logic was changed by this build fix.

## Validation

- Static Stability Pass validation: 106/106 passed.
- Verified no remaining `GetIntValue(<integer literal>)` calls in the OptiScaler source tree.
- Verified Performance Edition schema load remains version 2 and save emits `ConfigVersion=2`.
- Windows/MSVC runtime rebuild remains the authoritative compiler/runtime test.
