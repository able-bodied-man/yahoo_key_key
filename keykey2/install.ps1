param(
    [switch]$Elevated,
    [string]$InstallDir = 'C:\Program Files\Yahoo!\KeyKey2',
    [string]$KeyKeyMsi = ''
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
        $arguments = "-NoProfile -ExecutionPolicy Bypass -File `"$scriptPath`" -Elevated -InstallDir `"$InstallDir`""
        if ($KeyKeyMsi) {
            $arguments += " -KeyKeyMsi `"$KeyKeyMsi`""
        }
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

$logPath = Join-Path $env:TEMP 'keykey2-install.log'
$errorPath = Join-Path $env:TEMP 'keykey2-install-error.txt'
try {
    Start-Transcript -Path $logPath -Append | Out-Null
} catch {
    # A transcript is a convenience, not a requirement.
}
trap {
    $message = $_ | Out-String
    $message | Out-File $errorPath -Encoding utf8 -Append
    Write-Host ''
    Write-Host 'Installation failed:' -ForegroundColor Red
    Write-Host $message -ForegroundColor Red
    Stop-Transcript
    if ($Host.Name -eq 'ConsoleHost') {
        Read-Host 'Press Enter to close'
    }
    exit 1
}

$stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$targetDll = Join-Path $InstallDir "KeyKey2-$stamp.dll"
$subject = 'CN=KeyKey 2 Local Test'

$sourceDll = Join-Path $PSScriptRoot 'KeyKey2.dll'
if (-not (Test-Path $sourceDll)) {
    $sourceDll = Join-Path $PSScriptRoot 'KeyKey2\build\KeyKey2.dll'
}

if (-not (Test-Path $sourceDll)) {
    throw "Build the DLL first: $sourceDll not found"
}

# Ensure the legacy KeyKey engine is present. The standalone ZIP expects the
# user to install KeyKey first; the EXE installer passes -KeyKeyMsi so it can
# install the bundled original MSI silently.
$keyKeyLocation = $null
try {
    $keyKeyKey = Get-ItemProperty -Path 'HKLM:\SOFTWARE\Yahoo\KeyKey' -ErrorAction Stop
    $keyKeyLocation = $keyKeyKey.ServerLocation
} catch {
    $keyKeyLocation = $null
}

$keyKeyInstalled = $false
if ($keyKeyLocation) {
    $keyKeyServer = Join-Path $keyKeyLocation 'KeyKeyServer.exe'
    $keyKeyInstalled = Test-Path $keyKeyServer
}

if (-not $keyKeyInstalled) {
    if ($KeyKeyMsi -and (Test-Path $KeyKeyMsi)) {
        Write-Host 'Yahoo! KeyKey engine is not installed. Installing the bundled MSI...'
        $msiProcess = Start-Process -FilePath 'msiexec.exe' `
            -ArgumentList @('/i', "`"$KeyKeyMsi`"", '/qn', '/norestart', 'ALLUSERS=1') `
            -Wait -PassThru
        if ($msiProcess.ExitCode -ne 0 -and $msiProcess.ExitCode -ne 3010) {
            throw "Yahoo! KeyKey MSI installation failed with exit code $($msiProcess.ExitCode)."
        }
    }
    else {
        throw 'Yahoo! KeyKey is not installed. Install Yahoo! KeyKey 1.1.2535 first, or use the bundled EXE installer.'
    }
}

Write-Host "Installing from: $sourceDll"
New-Item -ItemType Directory -Force -Path $InstallDir | Out-Null

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
$cerPath = Join-Path $env:TEMP 'KeyKey2.cer'
Export-Certificate -Cert $cert -FilePath $cerPath -Force | Out-Null
Import-Certificate -FilePath $cerPath -CertStoreLocation Cert:\LocalMachine\Root | Out-Null
Import-Certificate -FilePath $cerPath -CertStoreLocation Cert:\LocalMachine\TrustedPublisher | Out-Null

# Sign the DLL with the locally trusted certificate. This uses the built-in
# PowerShell Authenticode support so the release does not require the Windows SDK.
Write-Host 'Signing the KeyKey 2 DLL...'
$signature = Set-AuthenticodeSignature -FilePath $sourceDll -Certificate $cert -HashAlgorithm SHA256
if ($signature.Status -ne 'Valid') {
    throw "Signing failed: $($signature.StatusMessage)"
}

Write-Host "Installing to: $targetDll"
Copy-Item -Force $sourceDll $targetDll
Set-Content -Path (Join-Path $PSScriptRoot 'installed-path.txt') -Value $targetDll -Encoding Unicode

# Register the in-proc COM server and the language profile.
Write-Host 'Registering the KeyKey 2 text service...'
$regsvr = Join-Path $env:SystemRoot 'System32\regsvr32.exe'
$process = Start-Process -FilePath $regsvr -ArgumentList "/s `"$targetDll`"" -Wait -PassThru
if ($process.ExitCode -ne 0) {
    throw "regsvr32 failed with exit code $($process.ExitCode)"
}

Write-Host "Installed: $targetDll"
Write-Host "Certificate thumbprint: $thumbprint"

# Add the input tip to the current user's Traditional Chinese profile and
# make it the default input method for new processes.
Write-Host 'Adding the Traditional Chinese input profile...'
$tip = '0404:{F7802624-5E24-4F6A-9BFB-5B525966B27A}{21A09CEE-663F-4481-877D-DE1373C678FA}'
$profileReady = $false
for ($attempt = 1; $attempt -le 2 -and -not $profileReady; $attempt++) {
    try {
        $list = Get-WinUserLanguageList
        $zh = $list | Where-Object { $_.LanguageTag -eq 'zh-Hant-TW' } | Select-Object -First 1
        if (-not $zh) {
            # New-WinUserLanguageList returns the whole list as a single object,
            # so take its first item directly. Select-Object -First 1 would keep
            # the List object and List.Add would reject it.
            $newLanguage = (New-WinUserLanguageList zh-TW)[0]
            $list.Add($newLanguage)
            Set-WinUserLanguageList $list -Force
            $list = Get-WinUserLanguageList
            $zh = $list | Where-Object { $_.LanguageTag -eq 'zh-Hant-TW' } | Select-Object -First 1
        }

        if (-not $zh) {
            throw 'The zh-Hant-TW language could not be added.'
        }

        $tips = $zh.InputMethodTips
        if (-not ($tips -contains $tip)) {
            $tips.Add($tip)
        }
        Set-WinUserLanguageList $list -Force
        Set-WinDefaultInputMethodOverride -InputTip $tip | Out-Null

        $verify = Get-WinUserLanguageList |
            Where-Object { $_.LanguageTag -eq 'zh-Hant-TW' } |
            Select-Object -First 1
        $profileReady = $verify -and (@($verify.InputMethodTips) -contains $tip)
        if (-not $profileReady) {
            throw 'The input profile did not appear in the language list.'
        }
    } catch {
        if ($attempt -ge 2) {
            Write-Warning "Could not add the input profile: $($_.Exception.Message)"
            Write-Warning 'Open Settings > Time & language > Language & region, add Traditional Chinese (Taiwan), then run the installer again.'
        }
    }
}

Write-Host ''
Write-Host 'Installation complete.' -ForegroundColor Green
Write-Host 'Restart WhatsApp or any already-running app, then select "Yahoo! KeyKey 2" with Win+Space.'
Stop-Transcript

