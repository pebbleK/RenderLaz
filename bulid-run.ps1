$ErrorActionPreference = "Stop"

$ProjectRoot = Split-Path -Parent $MyInvocation.MyCommand.Path

$env:PATH = "D:\Qt\6.10.1\mingw_64\bin;D:\Qt\Tools\mingw1310_64\bin;$env:PATH"

cmake --preset windows-mingw
cmake --build --preset windows-mingw

& "$ProjectRoot\build-mingw\RenderLaz.exe"