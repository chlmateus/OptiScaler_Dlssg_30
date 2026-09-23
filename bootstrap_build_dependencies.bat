@echo off
setlocal
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0bootstrap_build_dependencies.ps1" %*
if errorlevel 1 (
    echo.
    echo [deps] Failed to prepare OptiScaler build dependencies.
    exit /b 1
)
exit /b 0
