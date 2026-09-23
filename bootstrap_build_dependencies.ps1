param(
    [switch]$Force
)

$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'

$Root = Split-Path -Parent $MyInvocation.MyCommand.Path

$Dependencies = @(
    @{ Path = 'external/simpleini';          Repo = 'brofield/simpleini';                              Sha = '6048871ea9ee0ec24be5bd099d161a10567d7dc2' },
    @{ Path = 'external/unordered_dense';    Repo = 'martinus/unordered_dense';                        Sha = '73f3cbb237e84d483afafc743f1f14ec53e12314' },
    @{ Path = 'external/xess';               Repo = 'intel/xess';                                     Sha = '8fe81bdbbaf00b3c1b733fd0d830c333dc84e6f0' },
    @{ Path = 'external/vulkan';             Repo = 'KhronosGroup/Vulkan-Headers';                     Sha = 'd64e9e156ac818c19b722ca142230b68e3daafe3' },
    @{ Path = 'external/spdlog';             Repo = 'gabime/spdlog';                                  Sha = 'faa0a7a9c5a3550ed5461fab7d8e31c37fd1a2ef' },
    @{ Path = 'external/FidelityFX-SDK';     Repo = 'GPUOpen-LibrariesAndSDKs/FidelityFX-SDK';         Sha = 'c6efa6bf7f2027b3ec94f28578bb5965eabb9e55' },
    @{ Path = 'external/magic_enum';         Repo = 'Neargye/magic_enum';                              Sha = 'a733a2ea665ca5d72b7270f0334bf2e7b82bd0cc' },
    @{ Path = 'external/FidelityFX-SDK-v2';  Repo = 'GPUOpen-LibrariesAndSDKs/FidelityFX-SDK';         Sha = '60f4ea81909200d8542eca14dccb2628b763a9a3' },
    @{ Path = 'external/nvapi';              Repo = 'NVIDIA/nvapi';                                   Sha = '9b181ea572f680327fe01a14a0f1f41c78034104' }
)

function Test-PopulatedDirectory([string]$Path) {
    if (-not (Test-Path -LiteralPath $Path -PathType Container)) {
        return $false
    }

    $firstFile = Get-ChildItem -LiteralPath $Path -File -Recurse -ErrorAction SilentlyContinue | Select-Object -First 1
    return $null -ne $firstFile
}

function Install-Dependency($Dependency) {
    $destination = Join-Path $Root $Dependency.Path

    if (-not $Force -and (Test-PopulatedDirectory $destination)) {
        Write-Host "[deps] $($Dependency.Path) already populated - skipping."
        return
    }

    Write-Host "[deps] Fetching $($Dependency.Repo) @ $($Dependency.Sha)..."

    $tempRoot = Join-Path ([System.IO.Path]::GetTempPath()) ("OptiScalerDeps_" + [Guid]::NewGuid().ToString('N'))
    $archive = Join-Path $tempRoot 'dependency.zip'
    $extract = Join-Path $tempRoot 'extract'

    try {
        New-Item -ItemType Directory -Path $tempRoot -Force | Out-Null
        New-Item -ItemType Directory -Path $extract -Force | Out-Null

        $uri = "https://codeload.github.com/$($Dependency.Repo)/zip/$($Dependency.Sha)"
        Invoke-WebRequest -UseBasicParsing -Uri $uri -OutFile $archive
        Expand-Archive -LiteralPath $archive -DestinationPath $extract -Force

        $sourceRoot = Get-ChildItem -LiteralPath $extract -Directory | Select-Object -First 1
        if ($null -eq $sourceRoot) {
            throw "Downloaded archive for $($Dependency.Path) did not contain a root directory."
        }

        if (Test-Path -LiteralPath $destination) {
            Remove-Item -LiteralPath $destination -Recurse -Force
        }
        New-Item -ItemType Directory -Path $destination -Force | Out-Null

        Get-ChildItem -LiteralPath $sourceRoot.FullName -Force | ForEach-Object {
            Copy-Item -LiteralPath $_.FullName -Destination $destination -Recurse -Force
        }

        if (-not (Test-PopulatedDirectory $destination)) {
            throw "Dependency $($Dependency.Path) was downloaded but the destination is still empty."
        }

        Set-Content -LiteralPath (Join-Path $destination '.optiscaler_submodule_commit') -Value $Dependency.Sha -Encoding Ascii
        Write-Host "[deps] Installed $($Dependency.Path)."
    }
    finally {
        if (Test-Path -LiteralPath $tempRoot) {
            Remove-Item -LiteralPath $tempRoot -Recurse -Force -ErrorAction SilentlyContinue
        }
    }
}

Write-Host '[deps] Checking OptiScaler build dependencies...'

# A proper git clone with populated submodules needs no network bootstrap.
foreach ($dependency in $Dependencies) {
    Install-Dependency $dependency
}

$spdlogHeader = Join-Path $Root 'external/spdlog/include/spdlog/spdlog.h'
if (-not (Test-Path -LiteralPath $spdlogHeader -PathType Leaf)) {
    throw "Dependency bootstrap completed but spdlog header is still missing: $spdlogHeader"
}

Write-Host '[deps] All required OptiScaler submodule directories are populated.'
