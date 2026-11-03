# KeyKey TSF 1.0.3

A TSF text-service bridge that makes the legacy Yahoo! KeyKey input method work
in modern Windows text clients, including WhatsApp from the Microsoft Store.

## New in 1.0.3

- A single `KeyKeyTSF-1.0.3-x64-Setup.exe` installer replaces the manual ZIP
  workflow.
- The installer bundles the original Yahoo! KeyKey 1.1.2535 MSI and installs it
  silently when KeyKey is not already present.
- The wizard is built with Inno Setup and uses English text, so the garbled
  legacy `setup64.exe` dialogs no longer appear.
- The TSF bridge installs to `C:\Program Files\Yahoo! KeyKey TSF`.
- Uninstall now removes the TSF DLLs, the TSF/COM registration, the local
  certificate, and the input profile. Files that are still loaded by a running
  app are scheduled for deletion on the next restart.
- The uninstaller no longer reports failure after a successful cleanup.

## Installer fixes in 1.0.1

- The installer now relaunches itself elevated when started from a normal
  PowerShell window.
- Signing no longer requires `signtool.exe` or the Windows SDK; it uses the
  built-in PowerShell Authenticode support.
- Installation progress and failures are printed instead of exiting silently.
- Registration waits for `regsvr32` correctly, including paths containing
  spaces.
- Input-language setup now works from PowerShell 7 as well as Windows
  PowerShell 5.1.

## What is included

- `KeyKeyTSF-1.0.3-x64-Setup.exe` - one-step installer (recommended)
- `KeyKeyTSF-1.0.3-x64.zip` - manual installation for advanced users
- `KeyKeyTSF.dll` (x64) - TSF text service
- `install.ps1` - local code signing, installation, COM/TSF registration, and
  input-profile setup
- `uninstall.ps1` - removes the TSF profile, COM registration, certificate, and
  input-language entry
- `INSTALL.txt` - quick installation and troubleshooting instructions

## Why a bridge

The original KeyKey client is an IMM32 IME. The Store WhatsApp app is a
WinUI 3 / WebView2 application and uses TSF. This release keeps the original
KeyKey engine and data in `KeyKeyServer.exe`, and replaces only the Windows
input-client layer with a TSF text service.

## Requirements

- Windows 10/11 x64
- Administrator access for installation
- If KeyKey is not installed, the EXE installer supplies Yahoo! KeyKey 1.1.2535

## Verification

- Notepad/RichEdit: Cangjie `onf` + `Space` committed `你` (U+4F60).
- WhatsApp Store x64: the TSF service loaded in `WhatsApp.Root.exe`.
- Silent install and silent uninstall both exit with code 0 and leave no TSF
  files or registry keys behind.

## Notes

- The included installer uses a locally generated code-signing certificate.
  For public distribution, replace it with a real code-signing certificate.
- x64 only.
- The legacy candidate window, status bar, databases, and OneKey features are
  supplied by the installed KeyKey server.

## SHA-256

See `SHA256SUMS.txt` in the release assets.
