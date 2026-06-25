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

$WindowsExecutable = Join-Path $ProjectRoot "build/$Preset/RenderLaz.exe"
$MacExecutable = Join-Path $ProjectRoot "build/$Preset/RenderLaz.app/Contents/MacOS/RenderLaz"

if (Test-Path -LiteralPath $WindowsExecutable) {
    & $WindowsExecutable
} elseif (Test-Path -LiteralPath $MacExecutable) {
    & $MacExecutable
} else {
    throw "RenderLaz executable was not found for preset '$Preset'."
}
