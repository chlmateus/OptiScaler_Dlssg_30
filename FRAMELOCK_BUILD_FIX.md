# FrameLock OptiScaler ZIP Build Fix

This build can be compiled from an extracted source ZIP without a `.git` directory.

The OptiScaler pre-build event previously ran `git rev-parse --short HEAD` unconditionally while PowerShell used `$ErrorActionPreference='Stop'`. On a ZIP extraction, Git printed `fatal: not a git repository`, which terminated the pre-build event before C++ compilation began.

The pre-build event now:

1. Defaults `VER_BUILD_COMMIT` to `unknown`.
2. Checks that `$(SolutionDir)\.git` exists.
3. Checks that `git.exe` is available.
4. Calls `git rev-parse` only when both checks pass.
5. Falls back to `unknown` if Git still fails.

No limiter or graphics code is changed by this build-system fix.
