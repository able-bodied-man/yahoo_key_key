$ErrorActionPreference = 'Continue'
Start-Transcript -Path 'C:\Users\Able\AppData\Local\Temp\opencode\keykeytsf-uninstall.log' -Append

$clsid = '{F7802624-5E24-4F6A-9BFB-5B525966B27A}'
$profile = '{21A09CEE-663F-4481-877D-DE1373C678FA}'
$tip = "0404:$clsid$profile"
$installDir = 'C:\Program Files\KeyKeyTSF'

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
        $before = @($language.InputMethodTips).Count
        $language.InputMethodTips = @($language.InputMethodTips | Where-Object { $_ -ne $tip })
        if (@($language.InputMethodTips).Count -ne $before) {
            $changed = $true
        }
    }

    # Remove a language entry that was added only for this tip.
    $emptyAddedLanguage = $list | Where-Object { $_.LanguageTag -eq 'zh-Hant-TW' -and @($_.InputMethodTips).Count -eq 0 }
    if ($emptyAddedLanguage) {
        $list = @($list | Where-Object { $_.LanguageTag -ne 'zh-Hant-TW' })
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

# Best effort. Loaded copies in running processes keep the files locked.
Remove-Item -Recurse -Force $installDir -ErrorAction SilentlyContinue

Write-Host 'KeyKey TSF unregistered. Restart Explorer/WhatsApp/Notepad to release any loaded DLL copies.'
Stop-Transcript
