param(
    [string]$Preset = "",
    [string[]]$PathPrefix = @()
)

if (-not $Preset) {
    $Preset = if ($IsWindows -or $env:OS -eq 'Windows_NT') { "windows-local" }
              elseif ($IsMacOS) { "macos-local" }
              else { "default" }
}

$presetList = cmake --list-presets 2>$null
if ($presetList -notmatch """$Preset""") {
    Write-Warning "Preset '$Preset' not found, falling back to 'default'"
    $Preset = "default"
}

& (Join-Path $PSScriptRoot "bulid-run.ps1") -Preset $Preset -PathPrefix $PathPrefix
