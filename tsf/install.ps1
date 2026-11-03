param(
    [switch]$Elevated
)

$ErrorActionPreference = 'Stop'

function Test-Administrator {
    $identity = [Security.Principal.WindowsIdentity]::GetCurrent()
    $principal = New-Object Security.Principal.WindowsPrincipal($identity)
    return $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
}

if (-not (Test-Administrator)) {
    $scriptPath = $PSCommandPath
    if (-not $scriptPath) {
        $scriptPath = $MyInvocation.MyCommand.Path
    }

    Write-Host 'Administrator rights are required. Launching an elevated PowerShell...' -ForegroundColor Yellow
    try {
        $arguments = "-NoProfile -ExecutionPolicy Bypass -File `"$scriptPath`" -Elevated"
        Start-Process -FilePath (Join-Path $env:SystemRoot 'System32\WindowsPowerShell\v1.0\powershell.exe') `
            -Verb RunAs `
            -ArgumentList $arguments | Out-Null
    }
    catch {
        Write-Host "Could not start the elevated installer: $($_.Exception.Message)" -ForegroundColor Red
        Write-Host 'Open PowerShell as Administrator and run install.ps1 again.'
    }
    exit
}

Start-Transcript -Path 'C:\Users\Able\AppData\Local\Temp\opencode\keykeytsf-install.log' -Append
trap {
    $message = $_ | Out-String
    $message | Out-File 'C:\Users\Able\AppData\Local\Temp\opencode\keykeytsf-install-error.txt' -Encoding utf8 -Append
    Write-Host ''
    Write-Host 'Installation failed:' -ForegroundColor Red
    Write-Host $message -ForegroundColor Red
    Stop-Transcript
    if ($Host.Name -eq 'ConsoleHost') {
        Read-Host 'Press Enter to close'
    }
    exit 1
}

$installDir = 'C:\Program Files\KeyKeyTSF'
$stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$targetDll = Join-Path $installDir "KeyKeyTSF-$stamp.dll"
$subject = 'CN=KeyKey TSF Local Test'

$sourceDll = Join-Path $PSScriptRoot 'KeyKeyTSF.dll'
if (-not (Test-Path $sourceDll)) {
    $sourceDll = Join-Path $PSScriptRoot 'KeyKeyTSF\build\KeyKeyTSF.dll'
}

if (-not (Test-Path $sourceDll)) {
    throw "Build the DLL first: $sourceDll not found"
}

Write-Host "Installing from: $sourceDll"
New-Item -ItemType Directory -Force -Path $installDir | Out-Null

# Create or reuse a local code-signing certificate.
Write-Host 'Preparing the local code-signing certificate...'
$cert = Get-ChildItem Cert:\LocalMachine\My | Where-Object { $_.Subject -eq $subject } | Select-Object -First 1
if (-not $cert) {
    $cert = New-SelfSignedCertificate `
        -Type CodeSigningCert `
        -Subject $subject `
        -KeyUsage DigitalSignature `
        -TextExtension @('2.5.29.37={text}1.3.6.1.5.5.7.3.3') `
        -CertStoreLocation Cert:\LocalMachine\My `
        -NotAfter (Get-Date).AddYears(5)
}

$thumbprint = $cert.Thumbprint
Set-Content -Path (Join-Path $PSScriptRoot 'cert-thumbprint.txt') -Value $thumbprint -Encoding ASCII

# Trust the certificate for local code signing.
$cerPath = Join-Path $env:TEMP 'KeyKeyTSF.cer'
Export-Certificate -Cert $cert -FilePath $cerPath -Force | Out-Null
Import-Certificate -FilePath $cerPath -CertStoreLocation Cert:\LocalMachine\Root | Out-Null
Import-Certificate -FilePath $cerPath -CertStoreLocation Cert:\LocalMachine\TrustedPublisher | Out-Null

# Sign the DLL with the locally trusted certificate. This uses the built-in
# PowerShell Authenticode support so the release does not require the Windows SDK.
Write-Host 'Signing the TSF DLL...'
$signature = Set-AuthenticodeSignature -FilePath $sourceDll -Certificate $cert -HashAlgorithm SHA256
if ($signature.Status -ne 'Valid') {
    throw "Signing failed: $($signature.StatusMessage)"
}

Write-Host "Installing to: $targetDll"
Copy-Item -Force $sourceDll $targetDll
Set-Content -Path (Join-Path $PSScriptRoot 'installed-path.txt') -Value $targetDll -Encoding Unicode

# Register the in-proc COM server and the TSF language profile.
Write-Host 'Registering the TSF text service...'
$regsvr = Join-Path $env:SystemRoot 'System32\regsvr32.exe'
$process = Start-Process -FilePath $regsvr -ArgumentList "/s `"$targetDll`"" -Wait -PassThru
if ($process.ExitCode -ne 0) {
    throw "regsvr32 failed with exit code $($process.ExitCode)"
}

Write-Host "Installed: $targetDll"
Write-Host "Certificate thumbprint: $thumbprint"

# Add the TSF input tip to the current user's Traditional Chinese profile and
# make it the default input method for new processes.
Write-Host 'Adding the Traditional Chinese input profile...'
$tip = '0404:{F7802624-5E24-4F6A-9BFB-5B525966B27A}{21A09CEE-663F-4481-877D-DE1373C678FA}'
try {
    $list = Get-WinUserLanguageList
    $zh = $list | Where-Object { $_.LanguageTag -eq 'zh-Hant-TW' } | Select-Object -First 1
    if (-not $zh) {
        $newLanguage = (New-WinUserLanguageList zh-TW | Select-Object -First 1)
        $list.Add($newLanguage.PSObject.BaseObject)
        Set-WinUserLanguageList $list -Force
        $list = Get-WinUserLanguageList
        $zh = $list | Where-Object { $_.LanguageTag -eq 'zh-Hant-TW' } | Select-Object -First 1
    }

    if ($zh) {
        $tips = $zh.InputMethodTips
        if (-not ($tips -contains $tip)) {
            $tips.Add($tip)
        }
        Set-WinUserLanguageList $list -Force
        Set-WinDefaultInputMethodOverride -InputTip $tip
    }
} catch {
    Write-Warning "Could not update the user language list: $($_.Exception.Message)"
}

Write-Host ''
Write-Host 'Installation complete.' -ForegroundColor Green
Write-Host 'Restart WhatsApp or any already-running app, then select "Yahoo! KeyKey (TSF)" with Win+Space.'
Stop-Transcript
