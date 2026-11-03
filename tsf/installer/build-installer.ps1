$ErrorActionPreference = 'Stop'

$tsfRoot = Split-Path -Parent $PSScriptRoot
$repoRoot = Split-Path -Parent $tsfRoot
$origExe = Join-Path $repoRoot 'orig\YahooKeyKey-Windows-x64-1.1.2535-zh-TW.exe'
$msiPath = Join-Path $PSScriptRoot 'keykey64.msi'
$dllPath = Join-Path $tsfRoot 'KeyKeyTSF\build\KeyKeyTSF.dll'

if (-not (Test-Path $origExe)) {
    throw "Original KeyKey installer not found: $origExe"
}

if (-not (Test-Path $dllPath)) {
    Write-Host 'Building KeyKeyTSF.dll...'
    Push-Location (Join-Path $tsfRoot 'KeyKeyTSF')
    try {
        cmd /c build.cmd
        if ($LASTEXITCODE -ne 0) { throw "build.cmd failed with exit code $LASTEXITCODE" }
    }
    finally {
        Pop-Location
    }
}

# Extract keykey64.msi from the original WEXTRACT self-extracting EXE. The CAB
# payload is appended after the PE image; find the last MSCF header.
Write-Host 'Extracting the original KeyKey MSI...'
$bytes = [IO.File]::ReadAllBytes($origExe)
$cabOffset = -1
for ($i = $bytes.Length - 4; $i -ge 0; $i--) {
    if ($bytes[$i] -eq 0x4D -and $bytes[$i + 1] -eq 0x53 -and
        $bytes[$i + 2] -eq 0x43 -and $bytes[$i + 3] -eq 0x46) {
        $cabOffset = $i
        break
    }
}
if ($cabOffset -lt 0) {
    throw 'Cannot find the embedded cabinet in the original KeyKey installer.'
}

$tempCab = Join-Path $env:TEMP 'keykey-orig.cab'
$tempDir = Join-Path $env:TEMP 'keykey-orig-extract'
Remove-Item -Recurse -Force $tempDir -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force $tempDir | Out-Null

$stream = [IO.File]::Create($tempCab)
try {
    $stream.Write($bytes, $cabOffset, $bytes.Length - $cabOffset)
}
finally {
    $stream.Close()
}

& "$env:SystemRoot\System32\expand.exe" -F:* $tempCab $tempDir | Out-Null
$extractedMsi = Join-Path $tempDir 'keykey64.msi'
if (-not (Test-Path $extractedMsi)) {
    throw 'keykey64.msi was not found in the extracted cabinet.'
}
Copy-Item -Force $extractedMsi $msiPath

$iscc = Get-Command iscc.exe -ErrorAction SilentlyContinue
if (-not $iscc) {
    $candidate = Join-Path $env:LOCALAPPDATA 'Programs\Inno Setup 6\ISCC.exe'
    if (Test-Path $candidate) {
        $iscc = Get-Item $candidate
    }
}
if (-not $iscc) {
    throw 'Inno Setup ISCC.exe was not found. Install Inno Setup 6.'
}

Write-Host 'Compiling the installer...'
& $iscc.FullName (Join-Path $PSScriptRoot 'KeyKeyTSF.iss')
if ($LASTEXITCODE -ne 0) {
    throw "Inno Setup failed with exit code $LASTEXITCODE"
}

$output = Join-Path $tsfRoot 'release\KeyKeyTSF-1.0.4-x64-Setup.exe'
if (-not (Test-Path $output)) {
    throw "Installer output not found: $output"
}
Write-Host "Built: $output"

