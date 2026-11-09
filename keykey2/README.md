# Yahoo! KeyKey 2

Yahoo! KeyKey 2 is a **Windows text-service bridge** for the legacy Yahoo! KeyKey
input method. It does not reimplement the input engine. It loads the existing
KeyKey engine and data from the installed `KeyKeyServer.exe` over the same local
RPC protocol used by the legacy `KeyKey.ime` client.

That means the actual typing behavior, candidate window, status bar, OneKey
features, phrase database, and language model are still the original KeyKey
1.1.2535 code and data. Only the Windows input-client layer is new.

## Why a text service?

WhatsApp from the Microsoft Store is a 64-bit WinUI 3 / WebView2 application.
It follows the modern Windows text-input path (Text Services Framework for its
WebView2 content) and does not load legacy IMM32 IMEs such as `KeyKey.ime`.
Windows also blocks IMM32-only third-party IMEs in modern app containers.

The legacy KeyKey client is an IMM32 DLL. This project replaces only that
client with a Windows text service (`KeyKey2.dll`) and keeps the legacy server.

```
WhatsApp / Notepad / other modern text client
  +- KeyKey2.dll                      text service (this project)
  |    +- ITfKeyEventSink            receives keys
  |    +- ITfCompositionSink         owns the composition
  |    +- local RPC (ncalrpc)
  v
  KeyKeyServer.exe                    legacy KeyKey engine + UI
       +- PlainVanilla loader
       +- Smart Mandarin / Cangjie / Simplex
       +- candidate panel + status bar
       +- KeyKey.db / user databases
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
cd KeyKey2
.\build.cmd
```

The script runs `midl` for the legacy RPC IDL and then compiles
`build\KeyKey2.dll` (x64).

## Install

### EXE installer (recommended)

Run `KeyKey2-1.1.1-x64-Setup.exe` and follow the wizard. The installer:

1. checks whether Yahoo! KeyKey 1.1.2535 is already installed,
2. installs the bundled original MSI silently if it is missing,
3. installs the bridge to `C:\Program Files\Yahoo!\KeyKey2`,
4. signs and registers the DLL and adds the input profile.

The wizard uses English text, so it does not show the garbled legacy
`setup64.exe` dialogs.

### Script installer (source checkout only)

The release no longer ships a ZIP. If you are working from the repository and
already have the legacy engine installed, you can run the script from the
`keykey2` folder in a normal PowerShell window:

```powershell
.\install.ps1
```

If the window is not elevated, the script relaunches itself through UAC. The
installer:

1. creates/reuses a local code-signing certificate
   (`CN=KeyKey 2 Local Test`),
2. trusts it in `LocalMachine\Root` and `LocalMachine\TrustedPublisher`,
3. signs the DLL with PowerShell Authenticode,
4. copies it to
   `C:\Program Files\Yahoo!\KeyKey2\KeyKey2-<timestamp>.dll`,
5. registers the COM class and the language profile.

Each install uses a new timestamped DLL so an old copy that is already loaded
into Explorer/WhatsApp/Notepad does not block the update. The registry always
points to the newest copy.

## Select the input method

The service registers a Traditional Chinese profile (`zh-Hant-TW`):

- `Yahoo! KeyKey 2`

Use `Win+Space` or the input indicator in the taskbar to select it. The
installer adds `zh-Hant-TW` to the user language list if it is not already
present.

For an application that is already running, restart it after installing so it
loads the newest DLL.

## Test without WhatsApp

A small RichEdit test client is included:

```powershell
cd TestClient
cl /nologo /EHsc /std:c++17 /DUNICODE /D_UNICODE testclient.cpp /link /SUBSYSTEM:WINDOWS user32.lib gdi32.lib ole32.lib uuid.lib /OUT:TestClient.exe
.\TestClient.exe
```

Select the KeyKey 2 input method and type a Bopomofo syllable such as
`su3` followed by `Enter`. With the legacy Smart Mandarin selected, the
committed result is `浣燻 (U+4F60).

The legacy server's current primary input method is whatever is stored in
`%APPDATA%\Yahoo! KeyKey\com.yahoo.KeyKey.plist` (`PrimaryInputMethod`). It can
be `SmartMandarin`, `Generic-cj-cin`, `Generic-simplex-cin`, etc.

## Diagnostics

Set a user environment variable before starting the target application:

```powershell
[Environment]::SetEnvironmentVariable('KEYKEY2_DEBUG', '1', 'User')
```

Then restart the application. The service writes `%TEMP%\KeyKey2.log`. Useful
log lines include `ActivateServer`, `HandleServerKey`, and `ApplyKeyResult`.

If a target application does not load the service:

1. Confirm the registry has
   `HKLM\SOFTWARE\Microsoft\CTF\TIP\{F7802624-5E24-4F6A-9BFB-5B525966B27A}`.
2. Confirm `HKLM\SOFTWARE\Classes\CLSID\{F7802624-5E24-4F6A-9BFB-5B525966B27A}\InprocServer32`
   points to the newest DLL.
3. Restart `ctfmon.exe`/`TextInputHost.exe` (elevated), or sign out/in.
4. Make sure `zh-Hant-TW` with the `Yahoo! KeyKey 2` tip is selected.

## Verification

Tested on Windows 11 build 26200 with the installed Yahoo! KeyKey 1.1.2535:

- Notepad (modern text document): selected `Yahoo! KeyKey 2`, typed Cangjie
  `onf` then `Space`, and the document contained `浣燻 (U+4F60).
- WhatsApp `2.2641.101.0` x64 from the Microsoft Store (WinUI 3 + WebView2):
  selecting the profile caused `KeyKey2-<version>.dll` to load inside
  `WhatsApp.Root.exe`. The Store app exposes its composer through WebView2, so
  final composer-level typing should be checked manually in the WhatsApp window.

The legacy candidate panel/status bar are server-side windows and remain part
of the KeyKey look and behavior.

## Current limitations

- x64 only. A 32-bit build is needed for 32-bit host processes.
- Reuses the legacy out-of-process server and its WinForms candidate/status
  windows. It does not implement a UIless or touch-keyboard UI.
- Only one language profile (`zh-Hant-TW`) is registered.
- The local test certificate is not suitable for public distribution. Use a
  real code-signing certificate for that.

## Uninstall

With the EXE installer, remove **Yahoo! KeyKey 2** from
**Settings > Apps > Installed apps**.

With a script install from a source checkout:

```powershell
.\uninstall.ps1
```

Both paths unregister the language profile and COM class, remove the test
certificate, and remove the profile from the user language list. DLLs that are
still loaded by running processes are scheduled for deletion on the next
Windows restart.

