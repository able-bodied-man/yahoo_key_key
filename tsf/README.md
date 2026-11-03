# KeyKey TSF text service

This is a **Text Services Framework (TSF) bridge** for the legacy Yahoo! KeyKey
input method. It does not reimplement the input engine. It loads the existing
KeyKey engine and data from the installed `KeyKeyServer.exe` over the same local
RPC protocol used by the legacy `KeyKey.ime` client.

That means the actual typing behavior, candidate window, status bar, OneKey
features, phrase database, and language model are still the original KeyKey
1.1.2535 code and data. Only the Windows input-client layer is new.

## Why TSF?

WhatsApp from the Microsoft Store is a 64-bit WinUI 3 / WebView2 application.
It follows the modern Windows text-input path (TSF1 for its WebView2 content)
and does not load legacy IMM32 IMEs such as `KeyKey.ime`. Windows also blocks
IMM32-only third-party IMEs in modern app containers.

The legacy KeyKey client is an IMM32 DLL. This project replaces only that
client with a TSF text service (`KeyKeyTSF.dll`) and keeps the legacy server.

```
WhatsApp / Notepad / other TSF client
  鈹斺攢 KeyKeyTSF.dll                 TSF text service (this project)
       鈹溾攢 ITfKeyEventSink          receives keys
       鈹溾攢 ITfCompositionSink       owns the TSF composition
       鈹斺攢 local RPC (ncalrpc)
            鈻?     KeyKeyServer.exe               legacy KeyKey engine + UI
       鈹溾攢 PlainVanilla loader
       鈹溾攢 Smart Mandarin / Cangjie / Simplex
       鈹溾攢 candidate panel + status bar
       鈹斺攢 KeyKey.db / user databases
```

## Requirements

- Windows 10/11 x64
- Legacy Yahoo! KeyKey installed (tested with 1.1.2535 at
  `C:\Program Files\Yahoo!\KeyKey`)
- `KeyKeyServer.exe` available and registered under
  `HKLM\SOFTWARE\Yahoo\KeyKey\ServerLocation`
- Visual Studio 2022 Build Tools with the C++ workload and a Windows 11 SDK,
  to build

## Build

From this directory:

```powershell
cd KeyKeyTSF
.\build.cmd
```

The script runs `midl` for the legacy RPC IDL and then compiles
`build\KeyKeyTSF.dll` (x64).

## Install

### EXE installer (recommended)

Run `KeyKeyTSF-1.0.4-x64-Setup.exe` and follow the wizard. The installer:

1. checks whether Yahoo! KeyKey 1.1.2535 is already installed,
2. installs the bundled original MSI silently if it is missing,
3. installs the bridge to `C:\Program Files\Yahoo! KeyKey TSF`,
4. signs and registers the DLL and adds the input profile.

The wizard uses English text, so it does not show the garbled legacy
`setup64.exe` dialogs.

### ZIP / script installer

Run from a normal PowerShell window:

```powershell
.\install.ps1
```

If the window is not elevated, the script relaunches itself through UAC. The
installer:

1. creates/reuses a local code-signing certificate
   (`CN=KeyKey TSF Local Test`),
2. trusts it in `LocalMachine\Root` and `LocalMachine\TrustedPublisher`,
3. signs the DLL with PowerShell Authenticode,
4. copies it to `C:\Program Files\KeyKeyTSF\KeyKeyTSF-<timestamp>.dll`,
5. registers the COM class and TSF language profile.

Each install uses a new timestamped DLL so an old copy that is already loaded
into Explorer/WhatsApp/Notepad does not block the update. The registry always
points to the newest copy.

## Select the input method

The service registers a Traditional Chinese profile (`zh-Hant-TW`):

- `Yahoo! KeyKey (TSF)`

Use `Win+Space` or the input indicator in the taskbar to select it. The
installer adds `zh-Hant-TW` to the user language list if it is not already
present.

For an application that is already running, restart it after installing so it
loads the newest TSF DLL.

## Test without WhatsApp

A small RichEdit test client is included:

```powershell
cd TestClient
cl /nologo /EHsc /std:c++17 /DUNICODE /D_UNICODE testclient.cpp /link /SUBSYSTEM:WINDOWS user32.lib gdi32.lib ole32.lib uuid.lib /OUT:TestClient.exe
.\TestClient.exe
```

Select the KeyKey TSF input method and type a Bopomofo syllable such as
`su3` followed by `Enter`. With the legacy Smart Mandarin selected, the
committed result is `浣燻 (U+4F60).

The legacy server's current primary input method is whatever is stored in
`%APPDATA%\Yahoo! KeyKey\com.yahoo.KeyKey.plist` (`PrimaryInputMethod`). It can
be `SmartMandarin`, `Generic-cj-cin`, `Generic-simplex-cin`, etc.

## Diagnostics

Set a user environment variable before starting the target application:

```powershell
[Environment]::SetEnvironmentVariable('KEYKEY_TSF_DEBUG', '1', 'User')
```

Then restart the application. The service writes `%TEMP%\KeyKeyTSF.log`.
Useful log lines include `ActivateServer`, `HandleServerKey`, and
`ApplyKeyResult`.

If a target application does not load the service:

1. Confirm the registry has
   `HKLM\SOFTWARE\Microsoft\CTF\TIP\{F7802624-5E24-4F6A-9BFB-5B525966B27A}`.
2. Confirm `HKLM\SOFTWARE\Classes\CLSID\{F7802624-5E24-4F6A-9BFB-5B525966B27A}\InprocServer32`
   points to the newest DLL.
3. Restart `ctfmon.exe`/`TextInputHost.exe` (elevated), or sign out/in.
4. Make sure `zh-Hant-TW` with the `Yahoo! KeyKey (TSF)` tip is selected.

## Verification

Tested on Windows 11 build 26200 with the installed Yahoo! KeyKey 1.1.2535:

- Notepad (RichEdit/TSF document): selected `Yahoo! KeyKey (TSF)`, typed Cangjie
  `onf` then `Space`, and the document contained `浣燻 (U+4F60).
- WhatsApp `2.2641.101.0` x64 from the Microsoft Store (WinUI 3 + WebView2):
  selecting the profile caused `KeyKeyTSF-<version>.dll` to load inside
  `WhatsApp.Root.exe`. The Store app exposes its composer through WebView2, so
  final composer-level typing should be checked manually in the WhatsApp window.

The legacy candidate panel/status bar are server-side windows and remain part
of the KeyKey look and behavior.

## Current limitations

- x64 only. A 32-bit build is needed for 32-bit host processes.
- Reuses the legacy out-of-process server and its WinForms candidate/status
  windows. It does not implement a TSF UIless or touch-keyboard UI.
- Only one language profile (`zh-Hant-TW`) is registered.
- The local test certificate is not suitable for public distribution. Use a
  real code-signing certificate for that.

## Uninstall

With the EXE installer, remove **Yahoo! KeyKey TSF** from
**Settings > Apps > Installed apps**.

With the ZIP installer:

```powershell
.\uninstall.ps1
```

Both paths unregister the TSF profile and COM class, remove the test
certificate, and remove the profile from the user language list. DLLs that are
still loaded by running processes are scheduled for deletion on the next
Windows restart.

