# Project memory - Yahoo! KeyKey 2 bridge

> Handoff notes for AI agents and contributors. Last updated: 2026-11-03,
> current release **v1.1.1**.
>
> Read this first, then `README.md` and `keykey2/README.md`.

## 1. What this project is

This repository contains two things:

1. The original open-source **Yahoo! KeyKey 1.1.2528** source (BSD license).
2. A new **Windows text service** (`keykey2/`) that makes the installed legacy
   KeyKey engine work in modern Windows text clients, especially WhatsApp from
   the Microsoft Store.

Why a bridge: the Store WhatsApp app is WinUI 3 / WebView2 and uses the modern
Windows text-input framework (Text Services Framework). The legacy KeyKey client
is an IMM32 IME (`KeyKey.ime`) and is not loaded by that app. The bridge is a
text service DLL (`KeyKey2.dll`) that talks to the installed legacy
`KeyKeyServer.exe` over its original local RPC protocol (`ncalrpc`, interface
`BISKeyKey`). The input engine, candidate window, status bar, databases, and
OneKey features remain the original KeyKey code.

```
WhatsApp / Notepad / other modern text client
  鈹斺攢 KeyKey2.dll                   text service (this project)
       鈹溾攢 ITfKeyEventSink          receives keys
       鈹溾攢 ITfCompositionSink       owns the composition
       鈹斺攢 local RPC (ncalrpc)
            鈻?
     KeyKeyServer.exe               legacy KeyKey engine + UI
       鈹溾攢 PlainVanilla loader
       鈹溾攢 Smart Mandarin / Cangjie / Simplex
       鈹溾攢 candidate panel + status bar
       鈹斺攢 KeyKey.db / user databases
```

## 2. Repository map

| Path | Purpose |
| --- | --- |
| `README.md` | Project overview |
| `README.txt` | Original KeyKey/OpenVanilla third-party notices |
| `LICENSE.txt` | BSD license for the original source |
| `YahooKeyKey-Source-1.1.2528/` | Original KeyKey source (do not modify for bridge work) |
| `DataSource-1.1.2528/` | Data-source placeholders; licensed corpora are not included |
| `orig/` | **Gitignored.** Original Yahoo! KeyKey 1.1.2535 x64 installer (proprietary payload) |
| `keykey2/` | Text service, installers, docs, releases |
| `keykey2/KeyKey2/` | C++ source and `build.cmd` |
| `keykey2/install.ps1` | Script installer: sign, copy, register, add input profile |
| `keykey2/uninstall.ps1` | Script uninstaller |
| `keykey2/installer/KeyKey2.iss` | Inno Setup EXE installer definition |
| `keykey2/installer/build-installer.ps1` | Extracts the MSI from `orig/` and compiles the EXE |
| `keykey2/installer/keykey64.msi` | **Gitignored.** Extracted proprietary KeyKey MSI |
| `keykey2/release/` | Release notes, INSTALL.txt, built artifacts (artifacts gitignored) |
| `keykey2/TestClient/` | Small Win32 RichEdit test client |

Tracked but generated/ignored: `keykey2/KeyKey2/build/`, `generated/`, release
EXEs/DLLs, `cert-thumbprint.txt`, `installed-path.txt`.

## 3. Key identifiers

| Item | Value |
| --- | --- |
| Text service CLSID | `{F7802624-5E24-4F6A-9BFB-5B525966B27A}` |
| Profile GUID | `{21A09CEE-663F-4481-877D-DE1373C678FA}` |
| Input tip (language list) | `0404:{F7802624-5E24-4F6A-9BFB-5B525966B27A}{21A09CEE-663F-4481-877D-DE1373C678FA}` |
| Language | `zh-Hant-TW`, LANGID `0x0404` |
| Display name | `Yahoo! KeyKey 2` |
| Inno AppId | `{205BC078-BF4D-45D3-837E-CDA0395C066C}` |
| Legacy server registry | `HKLM\SOFTWARE\Yahoo\KeyKey` value `ServerLocation` |
| Legacy server executable | `KeyKeyServer.exe` (default `C:\Program Files\Yahoo!\KeyKey\`) |
| COM registration | `HKLM\SOFTWARE\Classes\CLSID\{F7802624-...}\InprocServer32` |
| Profile registration | `HKLM\SOFTWARE\Microsoft\CTF\TIP\{F7802624-...}` |

The CLSID/profile GUIDs are defined in `keykey2/KeyKey2/Globals.cpp`. The
install scripts duplicate them; keep all copies in sync.

Environment variable `KEYKEY2_DEBUG=1` enables `%TEMP%\KeyKey2.log` for the DLL.

## 4. Build

Prerequisites:

- Windows 10/11 x64
- Visual Studio 2022 Build Tools with the C++ workload and a Windows 11 SDK
- `midl`, `cl`, `rc`, `link` on the developer-command-prompt path
- Inno Setup 6 (`ISCC.exe`) for the EXE installer
- PowerShell 5.1 or 7

Build the DLL:

```powershell
cd keykey2\KeyKey2
.\build.cmd
# output: keykey2\KeyKey2\build\KeyKey2.dll
```

`build.cmd` locates Visual Studio through `vswhere` (falling back to the
2022 Build Tools path), runs `midl` for the two RPC IDL files into
`generated/`, compiles with `/std:c++17`, and links x64.

Build the single EXE installer:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File keykey2\installer\build-installer.ps1
# output: keykey2\release\KeyKey2-<version>-x64-Setup.exe
```

`build-installer.ps1`:

1. Requires `orig/YahooKeyKey-Windows-x64-1.1.2535-zh-TW.exe`.
2. Finds the last `MSCF` CAB header at the end of the WEXTRACT EXE.
3. Extracts `keykey64.msi` with `expand.exe` into
   `keykey2/installer/keykey64.msi`.
4. Runs Inno Setup `ISCC.exe` (looked up on PATH, then
   `%LOCALAPPDATA%\Programs\Inno Setup 6\ISCC.exe`).

The build does not need the original garbled `setup64.exe`.

## 5. Install behavior

### EXE installer (recommended)

- Installs to `C:\Program Files\Yahoo!\KeyKey2` (`{autopf}\Yahoo!\KeyKey2`).
- `UsePreviousAppDir=no`, so upgrading from 1.0.x moves from the old bridge
  folder to the new one instead of reusing it.
- Requires admin (`PrivilegesRequired=admin`).
- Inno wizard is **English** (`Default.isl`); never reuse the original
  `setup64.exe` UI, whose text is garbled.
- Bundles `keykey64.msi` to `{tmp}` and deletes it after install.
- At `ssPostInstall`, runs:

  ```text
  powershell -NoProfile -ExecutionPolicy Bypass -File "{app}\install.ps1" -InstallDir "{app}"
  ```

  If `HKLM\SOFTWARE\Yahoo\KeyKey\ServerLocation\KeyKeyServer.exe` is missing, it
  also passes `-KeyKeyMsi "{tmp}\keykey64.msi"`.
- Silent flags used for testing: `/VERYSILENT /NORESTART /SUPPRESSMSGBOXES`.

### Script installer (source checkout)

- Default install dir is `C:\Program Files\Yahoo!\KeyKey2` (`-InstallDir`
  overrides).
- Expects Yahoo! KeyKey to be installed already.
- Self-elevates through UAC if not admin.

### Common install steps (`keykey2/install.ps1`)

1. Creates or reuses a local code-signing certificate
   `CN=KeyKey 2 Local Test` in `LocalMachine\My`.
2. Exports it and trusts it in `LocalMachine\Root` and
   `LocalMachine\TrustedPublisher`.
3. Signs `KeyKey2.dll` with PowerShell `Set-AuthenticodeSignature` (no
   `signtool` dependency).
4. Copies it to `<InstallDir>\KeyKey2-<yyyyMMdd-HHmmss>.dll`.
5. Writes `installed-path.txt` and `cert-thumbprint.txt` next to the script.
6. Runs `regsvr32 /s` on the new DLL.
7. Adds `zh-Hant-TW` and the input tip to the current user's language list and
   calls `Set-WinDefaultInputMethodOverride`.
8. Re-reads the language list and verifies the tip is present (retries once).

Each install uses a new timestamped DLL so an already-loaded copy does not block
an update; the CLSID always points at the newest copy.

## 6. Uninstall behavior

EXE installs appear in **Settings > Apps > Installed apps** as
**Yahoo! KeyKey 2**. The Inno uninstaller runs `uninstall.ps1` via
`[UninstallRun]`.

`keykey2/uninstall.ps1`:

1. `regsvr32 /u` the DLL from `installed-path.txt`.
2. Deletes the CTF TIP and CLSID keys directly, including `WOW6432Node`.
3. Removes the certificate from `My`, `Root`, and `TrustedPublisher`.
4. Removes the input tip from every language list entry; removes `zh-Hant-TW`
   only if it has no remaining tips.
5. Deletes the install directory (only `KeyKey2`, never the `Yahoo!` parent
   that also holds the legacy engine).
6. If files are locked by a running process, schedules their deletion with
   `MoveFileEx(..., MOVEFILE_DELAY_UNTIL_REBOOT)`.
7. Always exits `0` for best-effort cleanup.

## 7. Release checklist

Bump the version everywhere it appears:

- `keykey2/KeyKey2/KeyKey2.rc` (`FILEVERSION`, `PRODUCTVERSION`, strings)
- `keykey2/installer/KeyKey2.iss` (`AppVersion`, `AppVerName`,
  `OutputBaseFilename`)
- `keykey2/installer/build-installer.ps1` (output filename)
- `keykey2/release/RELEASE_NOTES.md`
- `keykey2/release/INSTALL.txt`
- `keykey2/README.md` (EXE filename reference)
- `README.md`, `MEMORY.md` (version/status notes)

Then:

```powershell
cd keykey2\KeyKey2; .\build.cmd; cd ..\..

pwsh -NoProfile -ExecutionPolicy Bypass -File keykey2\installer\build-installer.ps1
```

Regenerate the checksum for the Setup EXE:

```powershell
$files = @("KeyKey2-$v-x64-Setup.exe")
$lines = foreach ($f in $files) {
    "$((Get-FileHash "$rel\$f" -Algorithm SHA256).Hash.ToLower())  $f"
}
$lines | Set-Content "$rel\SHA256SUMS.txt" -Encoding ASCII
```

Test (see section 8), then commit and publish:

```powershell
git add -A
git commit -m "Release KeyKey 2 $v"
git push origin master
git tag -a "v$v" -m "KeyKey 2 $v"
git push origin "v$v"
```

Pushing the tag triggers `.github/workflows/release.yml`, which builds the DLL
and Setup EXE on `windows-latest`, writes `SHA256SUMS.txt`, and publishes the
GitHub release. The release carries only the Setup EXE and its checksum; there
is no ZIP or loose DLL asset. CI has no access to `orig/`, so the workflow
downloads the v1.0.4 Setup asset, runs it silently, and copies `keykey64.msi`
out of its temporary folder while the bundled engine install is running. The
workflow also deletes any release asset that is not the Setup EXE or the
checksum file.

Manual fallback with a local toolchain:

```powershell
gh release create "v$v" --title "Yahoo! KeyKey 2 $v (x64)" `
  --notes-file keykey2\release\RELEASE_NOTES.md `
  "keykey2\release\KeyKey2-$v-x64-Setup.exe" `
  "keykey2\release\SHA256SUMS.txt"
```

Repository: <https://github.com/able-bodied-man/yahoo_key_key>
(remote is SSH: `git@github.com:able-bodied-man/yahoo_key_key.git`).

## 8. Test checklist

Silent install:

```powershell
$setup = 'keykey2\release\KeyKey2-1.1.1-x64-Setup.exe'
$p = Start-Process $setup -ArgumentList '/VERYSILENT','/NORESTART','/SUPPRESSMSGBOXES' -Wait -PassThru
$p.ExitCode   # expect 0
```

Verify:

```powershell
Test-Path 'C:\Program Files\Yahoo!\KeyKey2'
(Get-ItemProperty 'HKLM:\SOFTWARE\Classes\CLSID\{F7802624-5E24-4F6A-9BFB-5B525966B27A}\InprocServer32').'(default)'
(Get-Item <that path>).VersionInfo.FileVersion      # expect 1.1.1.0
Get-WinUserLanguageList | Where-Object LanguageTag -eq 'zh-Hant-TW'
```

Silent uninstall:

```powershell
$u = 'C:\Program Files\Yahoo!\KeyKey2\unins000.exe'
$p = Start-Process $u -ArgumentList '/VERYSILENT','/NORESTART','/SUPPRESSMSGBOXES' -Wait -PassThru
$p.ExitCode   # expect 0
Test-Path 'C:\Program Files\Yahoo!\KeyKey2'   # expect False
Test-Path 'HKLM:\SOFTWARE\Classes\CLSID\{F7802624-5E24-4F6A-9BFB-5B525966B27A}'  # False
Test-Path 'C:\Program Files\Yahoo!\KeyKey\KeyKeyServer.exe'                      # expect True
```

Fresh-profile test (the bug that broke 1.0.3): remove the whole `zh-Hant-TW`
entry from `Get-WinUserLanguageList`, run the installer, and confirm the
language and the `{F7802624...}{21A09CEE...}` tip come back.

Upgrade test: install 1.0.4 first, then 1.1.0, and confirm the files move to
`C:\Program Files\Yahoo!\KeyKey2` and the old bridge folder is gone.

Logs:

- `%TEMP%\keykey2-install.log`
- `%TEMP%\keykey2-install-error.txt`
- `%TEMP%\keykey2-uninstall.log`
- `%TEMP%\keykey2-register.log` (registration debug, always written)
- `%TEMP%\KeyKey2.log` (only with `KEYKEY2_DEBUG=1`)

## 9. Gotchas and lessons learned

### PowerShell language-list API

- `New-WinUserLanguageList zh-TW` returns the **whole list as one pipeline
  object**. `Select-Object -First 1` keeps that List, and `List.Add` then fails
  with `Cannot find an overload for "Add" and the argument count: "1"`.
  Use `(New-WinUserLanguageList zh-TW)[0]`.
- In Windows PowerShell 5.1 `WinUserLanguage.InputMethodTips` is read-only.
  Mutate the underlying `List<string>` with `.Add()` / `.Remove()`; do not
  assign a new array to the property.
- Always re-read the language list after `Set-WinUserLanguageList` and verify
  the tip, because failures used to be swallowed as warnings.

### Uninstall script

- `uninstall.ps1` once started with `param([string])` (no variable name). That
  is a parse error, so PowerShell exited with code 1 before the script ran and
  Inno left files/registry behind. Test scripts by actually running them.
- Loaded text-service DLLs can be locked by Explorer/WhatsApp/Notepad; the
  script schedules them for reboot deletion.
- `uninstall.ps1` deletes the whole `{app}` directory, including the Inno
  uninstaller. Inno has already copied itself to `%TEMP%`, so this is safe.
- Never point `uninstall.ps1` at `C:\Program Files\Yahoo!`: that directory also
  holds the legacy KeyKey engine.

### Installer / MSI

- The original `setup64.exe` UI is garbled. Do not ship or invoke it; its inner
  `keykey64.msi` works fine.
- The v1.0.4 Setup EXE is an Inno Setup package, so it does **not** contain an
  appended CAB (`MSCF`). To get `keykey64.msi` from it, run it and copy the file
  from `%TEMP%\is-*.tmp\` while the bundled engine install is in progress. The
  release workflow does this automatically; do not reintroduce a CAB scan.
- The workflow depends on the v1.0.4 release asset staying available. If it is
  ever deleted, point the workflow at another release that ships a Setup EXE.
- `gh release create` can leave the release as an untagged draft, which is
  invisible to the public API and the releases page. The workflow reads the
  release back and runs `gh release edit --draft=false` if needed. Do not skip
  that verification.
- On a machine where KeyKey is already admin-assigned, `msiexec /i` can return
  `1625`. The EXE installer skips the MSI when `KeyKeyServer.exe` exists.
- `install.ps1`'s error trap waits for Enter when hosted in a console; under
  Inno's hidden `Exec` this can hang the installer if an error occurs. Keep
  this in mind if adding failure paths.
- The bundled MSI and `orig/` are proprietary and gitignored. Do not commit
  them.
- The self-signed certificate is for local installs only. Public distribution
  needs a real code-signing certificate.

### Composition and candidate UI

- The legacy server fills its `InputBuffer` and `CandidatePanel` windows while
  handling a key, but it does **not** show them. The TSF client must call
  `BISShowInputBuffer` and `BISShowCandidatePanel` explicitly, otherwise only
  inline composition text appears and the candidate list never shows.
- `BISHandleKey` returns `candidatePanelVisible`; the bridge now shows/hides the
  server windows from that flag and from whether the composition is empty.
- In Cangjie, the server may not report candidates until Space is pressed, so
  type the code and press Space to see the candidate page.

### Build / source debt

- x64 only. 32-bit host processes need a 32-bit build.
- `KEYKEY2_DEBUG=1` must be set per-user before the target app starts.

## 10. Current status (2026-11-09)

- Latest release: **v1.1.1** (shows the composition input buffer and candidate
  panel again).
- Release assets are intentionally minimal: `KeyKey2-<version>-x64-Setup.exe`
  and `SHA256SUMS.txt` only. The old ZIP and loose DLL assets were removed as
  redundant.
- Tags: `v1.0.0`, `v1.0.1`, `v1.0.2`, `v1.0.3`, `v1.0.4`, `v1.1.0`, `v1.1.1`, and the
  historical preview tag.
- Releases are built and published by `.github/workflows/release.yml` from the
  `v*` tag; no local toolchain is needed.
- `master` includes the EXE installer, fixed uninstall, the fresh-profile fix,
  the KeyKey 2 rename, the tag-driven release workflow, and the 1.1.1 UI fix.
- 1.0.3 is known broken for fresh installs (profile not added). Use 1.0.4+.
- 1.1.0 installs to `C:\Program Files\Yahoo!\KeyKey2` and upgrades over 1.0.x
  automatically (same Inno AppId, `UsePreviousAppDir=no`).
- 1.1.1 restores the missing legacy composition and candidate windows.

## 11. Suggested next work

- Replace the self-signed certificate with a real code-signing certificate.
- Add a 32-bit build if 32-bit host processes matter.
- Improve the UIless/touch-keyboard behavior; the current service reuses the
  legacy WinForms candidate/status windows.
- Consider adding automated install/uninstall smoke tests.

