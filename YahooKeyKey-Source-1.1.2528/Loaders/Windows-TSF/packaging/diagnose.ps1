# KeyKey TSF front end - diagnostics
# Run: right-click -> Run with PowerShell   (no administrator needed)
# Writes keykeytsf-diag.txt next to this script and prints it.

$ErrorActionPreference = 'SilentlyContinue'
$here = Split-Path -Parent $MyInvocation.MyCommand.Path
$report = Join-Path $here 'keykeytsf-diag.txt'
$out = New-Object System.Collections.Generic.List[string]

function Add($text) { $out.Add([string]$text) | Out-Null }

Add "KeyKey TSF diagnostics  $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')"
Add ("OS: " + (Get-CimInstance Win32_OperatingSystem).Caption + " build " + [Environment]::OSVersion.Version)
Add ("User: " + $env:USERNAME)
Add ""

Add "== DLL next to this script =="
$dll = Join-Path $here 'KeyKeyTSF.dll'
if (Test-Path $dll) {
    $item = Get-Item $dll
    Add ("path:   " + $item.FullName)
    Add ("size:   " + $item.Length)
    Add ("built:  " + $item.LastWriteTime)
    $bytes = [System.IO.File]::ReadAllBytes($dll)
    $peOffset = [BitConverter]::ToInt32($bytes, 0x3C)
    $machine = [BitConverter]::ToUInt16($bytes, $peOffset + 4)
    $arch = switch ($machine) { 0x8664 { 'x64' } 0x14c { 'x86' } 0xAA64 { 'arm64' } default { "0x{0:x}" -f $machine } }
    Add ("arch:   " + $arch)
    $motw = Get-Item $dll -Stream * | Where-Object Stream -eq 'Zone.Identifier'
    if ($motw) {
        Add "mark-of-the-web: PRESENT - run  Unblock-File .\KeyKeyTSF.dll  and reinstall"
    } else {
        Add "mark-of-the-web: none"
    }
} else {
    Add "KeyKeyTSF.dll NOT FOUND here"
}
Add ""

Add "== COM registration =="
$clsid = '{B7E1B1E4-2C4B-4C57-9B4D-6C2E7A6E1F01}'
foreach ($view in @(@{ Path = "HKLM:\SOFTWARE\Classes\CLSID\$clsid\InprocServer32"; Name = '64-bit' },
                   @{ Path = "HKLM:\SOFTWARE\WOW6432Node\Classes\CLSID\$clsid\InprocServer32"; Name = '32-bit' })) {
    $key = Get-ItemProperty $view.Path
    if ($key) {
        Add ("{0}: {1}" -f $view.Name, $key.'(default)')
        Add ("          ThreadingModel = " + $key.ThreadingModel)
    } else {
        Add ("{0}: not registered" -f $view.Name)
    }
}
Add ""

Add "== TSF profile (CTF) =="
$tip = "HKLM:\SOFTWARE\Microsoft\CTF\TIP\$clsid\LanguageProfile"
if (Test-Path $tip) {
    Get-ChildItem $tip -Recurse | ForEach-Object {
        $props = Get-ItemProperty $_.PSPath
        $desc = $props.'(default)'
        if (-not $desc -and $props.Description) { $desc = $props.Description }
        if ($_.PSChildName -like '0x*' -or $desc) {
            Add ("  " + $_.Name.Replace('HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\CTF\TIP\' + $clsid + '\LanguageProfile\', ''))
        }
    }
    $enabled = Get-ChildItem $tip -Recurse | Where-Object { $_.PSChildName -eq 'Enable' }
    if ($enabled) { Add "  Enable bit set" } else { Add "  WARNING: no Enable value found under the profile" }
} else {
    Add "  no CTF profile registered for this CLSID"
}
Add ""

Add "== Is the DLL loaded in any application? =="
$loadedAny = $false
foreach ($proc in Get-Process) {
    try {
        $module = $proc.Modules | Where-Object { $_.ModuleName -eq 'KeyKeyTSF.dll' } | Select-Object -First 1
        if ($module) {
            Add ("  " + $proc.ProcessName + " (pid " + $proc.Id + ") <- " + $module.FileName)
            $loadedAny = $true
        }
    } catch { }
}
if (-not $loadedAny) {
    Add "  not loaded anywhere - Windows never pulled the text service into an application."
    Add "  Close and reopen the app (or reboot) after installing: a process keeps the"
    Add "  DLL it started with, and Smart App Control can refuse an unblocked-but-unsigned DLL."
}
Add ""

Add "== KeyKey processes =="
$procs = Get-Process | Where-Object { $_.ProcessName -match 'KeyKey|BaseIME|Takao|ctfmon' }
if ($procs) {
    $procs | ForEach-Object { Add ("  " + $_.ProcessName + " (pid " + $_.Id + ")") }
} else {
    Add "  none running - KeyKey's server is not up, so the front end has nothing to talk to"
}
Add ""

Add "== Log =="
$log = Join-Path $env:LOCALAPPDATA 'KeyKeyTSF\keykeytsf.log'
if (Test-Path $log) {
    Add ("file: " + $log + "  (" + (Get-Item $log).Length + " bytes, written " + (Get-Item $log).LastWriteTime + ")")
    Add "--- last 40 lines ---"
    Get-Content $log -Tail 40 | ForEach-Object { Add ($_) }
} else {
    Add ("no log at " + $log)
    Add "  (means the text service was never loaded into any application)"
}

$out -join "`r`n" | Set-Content -Path $report -Encoding UTF8
Write-Host ($out -join "`n")
Write-Host ""
Write-Host "Saved to $report - paste that file here."
