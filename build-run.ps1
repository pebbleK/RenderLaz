param(
    [string]$Preset = "windows-local",
    [string[]]$PathPrefix = @()
)

$ErrorActionPreference = "Stop"

$ProjectRoot = Split-Path -Parent $MyInvocation.MyCommand.Path

foreach ($Path in $PathPrefix) {
    $env:PATH = "$Path$([IO.Path]::PathSeparator)$env:PATH"
}

cmake --preset $Preset
cmake --build --preset $Preset

# 构建目录由 preset 的 binaryDir 决定：可能带 preset 名子目录，也可能直接用 build/
$BinaryDir = Join-Path $ProjectRoot "build/$Preset"
if (-not (Test-Path -LiteralPath (Join-Path $BinaryDir "CMakeCache.txt"))) {
    $BinaryDir = Join-Path $ProjectRoot "build"
}

$WindowsExecutable = Join-Path $BinaryDir "RenderLaz.exe"
$MacExecutable = Join-Path $BinaryDir "RenderLaz.app/Contents/MacOS/RenderLaz"

if (Test-Path -LiteralPath $WindowsExecutable) {
    & $WindowsExecutable
} elseif (Test-Path -LiteralPath $MacExecutable) {
    & $MacExecutable
} else {
    throw "RenderLaz executable was not found for preset '$Preset'."
}
