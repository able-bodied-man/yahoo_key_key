param(
    [string]$InstallDir = 'C:\Program Files\KeyKeyTSF'
)

$ErrorActionPreference = 'Continue'

$logPath = Join-Path $env:TEMP 'keykeytsf-uninstall.log'
try {
    Start-Transcript -Path $logPath -Append | Out-Null
} catch {
    # A transcript is a convenience, not a requirement.
}

$clsid = '{F7802624-5E24-4F6A-9BFB-5B525966B27A}'
$profile = '{21A09CEE-663F-4481-877D-DE1373C678FA}'
$tip = "0404:$clsid$profile"


if (-not ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    throw 'Run this script as Administrator.'
}

$installedPathFile = Join-Path $PSScriptRoot 'installed-path.txt'
if (Test-Path $installedPathFile) {
    $dll = (Get-Content $installedPathFile -Raw).Trim()
    if ($dll -and (Test-Path $dll)) {
        & (Join-Path $env:SystemRoot 'System32\regsvr32.exe') /s /u $dll
    }
}

# Remove the TSF profile and COM registration directly as well, in case a DLL
# was still loaded and could not unregister itself.
& reg.exe delete "HKLM\SOFTWARE\Microsoft\CTF\TIP\$clsid" /f | Out-Null
& reg.exe delete "HKLM\SOFTWARE\Classes\CLSID\$clsid" /f | Out-Null
& reg.exe delete "HKLM\SOFTWARE\WOW6432Node\Classes\CLSID\$clsid" /f | Out-Null

# Remove the local test certificate.
$thumbprintFile = Join-Path $PSScriptRoot 'cert-thumbprint.txt'
$thumbprint = $null
if (Test-Path $thumbprintFile) {
    $thumbprint = (Get-Content $thumbprintFile -Raw).Trim()
}
if (-not $thumbprint) {
    $cert = Get-ChildItem Cert:\LocalMachine\My |
        Where-Object { $_.Subject -eq 'CN=KeyKey TSF Local Test' } |
        Select-Object -First 1
    if ($cert) {
        $thumbprint = $cert.Thumbprint
    }
}
if ($thumbprint) {
    foreach ($store in @('Cert:\LocalMachine\My', 'Cert:\LocalMachine\Root', 'Cert:\LocalMachine\TrustedPublisher')) {
        $cert = Get-ChildItem $store | Where-Object { $_.Thumbprint -eq $thumbprint } | Select-Object -First 1
        if ($cert) {
            Remove-Item -Path $cert.PSPath -Force -ErrorAction SilentlyContinue
        }
    }
}

# Remove the TSF input tip from the user's language list.
try {
    $list = Get-WinUserLanguageList
    $changed = $false
    foreach ($language in $list) {
        $tips = $language.InputMethodTips
        $removed = 0
        foreach ($existingTip in @($tips)) {
            if ($existingTip -eq $tip) {
                [void]$tips.Remove($existingTip)
                $removed++
            }
        }
        if ($removed -gt 0) {
            $changed = $true
        }
    }

    # Remove a language entry that was added only for this tip.
    $emptyAddedLanguage = $list | Where-Object { $_.LanguageTag -eq 'zh-Hant-TW' -and @($_.InputMethodTips).Count -eq 0 } | Select-Object -First 1
    if ($emptyAddedLanguage) {
        [void]$list.Remove($emptyAddedLanguage)
        $changed = $true
    }

    if ($changed) {
        Set-WinUserLanguageList $list -Force
    }
} catch {
    Write-Warning "Could not update the user language list: $($_.Exception.Message)"
}

Remove-Item -Force $installedPathFile -ErrorAction SilentlyContinue
Remove-Item -Force $thumbprintFile -ErrorAction SilentlyContinue

# Best effort. Loaded copies in running processes (Explorer, WhatsApp, Notepad)
# keep the DLL locked, so fall back to a reboot-time delete.
function Register-DeleteOnReboot {
    param([string]$Path)

    if (-not ('KeyKeyTSF.NativeMethods' -as [type])) {
        Add-Type -Namespace KeyKeyTSF -Name NativeMethods -MemberDefinition @'
[System.Runtime.InteropServices.DllImport("kernel32.dll", SetLastError = true, CharSet = System.Runtime.InteropServices.CharSet.Unicode)]
public static extern bool MoveFileEx(string lpExistingFileName, string lpNewFileName, int dwFlags);
'@
    }

    # MOVEFILE_DELAY_UNTIL_REBOOT
    return [KeyKeyTSF.NativeMethods]::MoveFileEx($Path, $null, 0x4)
}

$scheduledForReboot = @()
if (Test-Path $InstallDir) {
    Remove-Item -Recurse -Force $InstallDir -ErrorAction SilentlyContinue
}

if (Test-Path $InstallDir) {
    # Schedule files first, then directories from the deepest level upwards.
    $leftovers = @(Get-ChildItem -Path $InstallDir -Recurse -File -Force -ErrorAction SilentlyContinue)
    foreach ($file in $leftovers) {
        if (Register-DeleteOnReboot $file.FullName) {
            $scheduledForReboot += $file.FullName
        }
    }

    $directories = @(Get-ChildItem -Path $InstallDir -Recurse -Directory -Force -ErrorAction SilentlyContinue |
        Sort-Object { $_.FullName.Length } -Descending)
    $directories += Get-Item $InstallDir -ErrorAction SilentlyContinue
    foreach ($directory in $directories) {
        if ($directory) {
            Register-DeleteOnReboot $directory.FullName | Out-Null
        }
    }
}

Write-Host 'KeyKey TSF unregistered. Restart Explorer/WhatsApp/Notepad to release any loaded DLL copies.'
if ($scheduledForReboot.Count -gt 0) {
    Write-Host 'Some files are in use and will be removed the next time Windows restarts.' -ForegroundColor Yellow
}

try {
    Stop-Transcript | Out-Null
} catch {
}

exit 0

