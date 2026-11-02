# Builds KeyKeyTSF.dll locally with Visual Studio + CMake and stages a folder
# you can copy anywhere and register with install.bat.
#
#   powershell -ExecutionPolicy Bypass -File build.ps1            (x64)
#   powershell -ExecutionPolicy Bypass -File build.ps1 -Arch Win32
#
param(
    [ValidateSet('x64', 'Win32')]
    [string]$Arch = 'x64'
)

$ErrorActionPreference = 'Stop'
$here = Split-Path -Parent $MyInvocation.MyCommand.Path
$build = Join-Path $here "build-$Arch"

if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
    throw "cmake not found. Install Visual Studio 2022 (Desktop development with C++) and run this from a Developer PowerShell, or add CMake to PATH."
}

cmake -S $here -B $build -A $Arch
if ($LASTEXITCODE -ne 0) { throw "cmake configure failed" }

cmake --build $build --config Release
if ($LASTEXITCODE -ne 0) { throw "build failed" }

$dll = Get-ChildItem $build -Recurse -Filter KeyKeyTSF.dll |
       Sort-Object LastWriteTime -Descending | Select-Object -First 1
if (-not $dll) { throw "KeyKeyTSF.dll was not produced" }

$stage = Join-Path $here "package-$Arch"
New-Item -ItemType Directory -Force $stage | Out-Null
Copy-Item $dll.FullName (Join-Path $stage 'KeyKeyTSF.dll') -Force

$installer = if ($Arch -eq 'x64') { 'install_x64.bat' } else { 'install_x86.bat' }
Copy-Item (Join-Path $here "packaging\$installer") (Join-Path $stage 'install.bat') -Force
Copy-Item (Join-Path $here 'packaging\uninstall.bat') $stage -Force
Copy-Item (Join-Path $here 'packaging\README.txt') $stage -Force

Write-Host ""
Write-Host "Package ready: $stage"
Write-Host "Run $stage\install.bat as administrator, then add 'Yahoo! KeyKey (TSF)'"
Write-Host "under Chinese (Traditional, Taiwan) in the language settings."
