# ZIP build dependency fix

The GitHub source ZIP contains empty directories for OptiScaler's git submodules.
OptiScaler's Visual Studio project expects those submodules to be populated, so a
plain ZIP build fails first at `spdlog/spdlog.h` and would then encounter other
missing SDK/header dependencies.

This package adds:

- `bootstrap_build_dependencies.ps1`
- `bootstrap_build_dependencies.bat`
- automatic dependency bootstrap from the Visual Studio pre-build event

The bootstrap downloads the exact submodule commits referenced by the OptiScaler
master revision used for this FrameLock integration:

- simpleini `6048871ea9ee0ec24be5bd099d161a10567d7dc2`
- unordered_dense `73f3cbb237e84d483afafc743f1f14ec53e12314`
- XeSS `8fe81bdbbaf00b3c1b733fd0d830c333dc84e6f0`
- Vulkan-Headers `d64e9e156ac818c19b722ca142230b68e3daafe3`
- spdlog `faa0a7a9c5a3550ed5461fab7d8e31c37fd1a2ef`
- FidelityFX-SDK `c6efa6bf7f2027b3ec94f28578bb5965eabb9e55`
- magic_enum `a733a2ea665ca5d72b7270f0334bf2e7b82bd0cc`
- FidelityFX-SDK-v2 `60f4ea81909200d8542eca14dccb2628b763a9a3`
- NVAPI `9b181ea572f680327fe01a14a0f1f41c78034104`

Existing populated dependency directories are left alone. This means a normal
recursive Git clone still works without redundant downloads.

After the first successful dependency bootstrap, subsequent builds skip the
already-populated directories.
