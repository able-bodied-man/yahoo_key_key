# Yahoo! KeyKey 2 1.1.2

Yahoo! KeyKey 2 is a Windows text-service bridge that makes the legacy Yahoo!
KeyKey input method work in modern Windows text clients, including WhatsApp
from the Microsoft Store.

## New in 1.1.2

- Candidate lists are now clickable: click a candidate to commit it.
- Clicking a candidate can open the **associated phrase** (關聯詞) panel, just
  like the legacy input method.
- The client RPC endpoint is now registered after the server context is
  activated. Previously the server ignored it, so clicks had nowhere to send
  the selection and it logged `No client endpoint is available`.

## New in 1.1.1

- Restored the legacy composition box that shows the typed Cangjie strokes
  (for example `人大口`).
- Restored the candidate list when more than one candidate matches.
- The bridge now tells the legacy server to show and hide its input-buffer and
  candidate windows, matching the original IMM client behavior.
- The windows are hidden again when the composition ends or is cancelled.

## New in 1.1.0

- Renamed the product to **Yahoo! KeyKey 2**; the old name is no longer used
  anywhere in the product, DLL, installer, logs, or documentation.
- The default install folder is now `C:\Program Files\Yahoo!\KeyKey2`.
  Upgrading from 1.0.x migrates to the new folder automatically.
- The release now ships only the Setup EXE and its checksum; the ZIP and loose
  DLL assets were redundant.
- The service DLL is now `KeyKey2.dll` and release files use the `KeyKey2-`
  prefix.
- Diagnostics use `KEYKEY2_DEBUG` and write `%TEMP%\KeyKey2.log`.
- The registration debug log now uses `%TEMP%` instead of a hard-coded
  user-specific path.

## New in 1.0.4

- Fixed the input profile not being added on a fresh install. The installer
  previously passed a whole language list to `List.Add`, which failed with
  "Cannot find an overload for Add and the argument count 1", so the profile
  never appeared in Win+Space.
- The installer now reads the language list back after setting it and retries
  once if the profile is missing.
- If the profile still cannot be added, the installer prints an actionable
  message instead of failing silently.

## New in 1.0.3

- A single `KeyKey2-1.1.0-x64-Setup.exe` installer replaces the manual ZIP
  workflow.
- The installer bundles the original Yahoo! KeyKey 1.1.2535 MSI and installs it
  silently when KeyKey is not already present.
- The wizard is built with Inno Setup and uses English text, so the garbled
  legacy `setup64.exe` dialogs no longer appear.
- Uninstall removes the DLLs, the COM registration, the local certificate, and
  the input profile. Files that are still loaded by a running app are scheduled
  for deletion on the next restart.
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

- `KeyKey2-1.1.0-x64-Setup.exe` - one-step installer (recommended)
- `SHA256SUMS.txt` - SHA-256 checksum for the installer

The installer deploys `KeyKey2.dll`, `install.ps1`, `uninstall.ps1`,
`INSTALL.txt`, `README.md`, and the license/notice files into
`C:\Program Files\Yahoo!\KeyKey2`.

## Why a bridge

The original KeyKey client is an IMM32 IME. The Store WhatsApp app is a
WinUI 3 / WebView2 application and uses the modern Windows text-input
framework. This release keeps the original KeyKey engine and data in
`KeyKeyServer.exe`, and replaces only the Windows input-client layer with a
text service.

## Requirements

- Windows 10/11 x64
- Administrator access for installation
- If KeyKey is not installed, the EXE installer supplies Yahoo! KeyKey 1.1.2535

## Verification

- Fresh install with no `zh-Hant-TW` entry: the installer added the language and
  the **Yahoo! KeyKey 2** profile, and recorded it in the language list.
- Silent install and silent uninstall both exit with code 0 and leave no files
  or registry keys behind.
- Notepad: Cangjie `onf` + `Space` committed `你` (U+4F60).
- WhatsApp Store x64: the text service loaded in `WhatsApp.Root.exe`.

## Notes

- The included installer uses a locally generated code-signing certificate.
  For public distribution, replace it with a real code-signing certificate.
- x64 only.
- The legacy candidate window, status bar, databases, and OneKey features are
  supplied by the installed KeyKey server.

## SHA-256

See `SHA256SUMS.txt` in the release assets.
