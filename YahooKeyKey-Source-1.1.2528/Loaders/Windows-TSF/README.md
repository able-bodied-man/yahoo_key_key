# KeyKeyTSF - a standalone CangJie input method for Windows

A Text Services Framework (TSF) text service that types Traditional Chinese with
CangJie. It is self-contained: the CangJie table (`DataTables/cj-ext.cin`, ~70k
characters, HKSCS extended) is embedded in the DLL as a resource, the candidate
window and the status bar are drawn by the DLL itself, and nothing from Yahoo!
KeyKey needs to be installed or running.

## Why it exists

Yahoo! KeyKey's Windows IME is an IMM32 loader, so packaged apps (UWP, WinUI 3,
the Microsoft Store build of WhatsApp) never load it: no status bar, no
candidates, no Chinese at all. The obvious port - a TSF loader that keeps using
KeyKey's engine over RPC - was built first (`src/KeyKeyRpc.*`, kept for
reference) and hit a wall: every call into the server's WinForms UI throws a
managed exception (`0xE0434F4D`) when it comes from a TSF callback, and the
engine's candidate list is only produced through that UI. So the input method
became its own program.

## Layout

| file | what it does |
| --- | --- |
| `src/CinTable.*` | parses the embedded `%gen_inp` table: codes, radicals, punctuation |
| `src/CangjieEngine.*` | the input method state machine: code buffer, candidates, selection, commit |
| `src/KeyTranslation.*` | Win32 key event -> OpenVanilla key code/mask (same encoding the KeyKey loader used) |
| `src/UiWindows.*` | the candidate window (composition + candidates, follows the caret) and the status bar |
| `src/CangjieService.*` | the TSF text service wiring: activation, key sink, edit sessions, commit |
| `src/Module.cpp` | DLL plumbing, COM class factory, registration |

## Typing with it

- CangJie letters build the code; the composition shows as radicals (日尸木...).
- `1`-`9` or Space/Enter commit the highlighted candidate, `Esc` cancels, Backspace edits the code.
- A single Shift toggles Chinese/English (the 中 button on the bar does the same).
- Punctuation keys (`,` `.` `;` `'` `[` `]` `/` ...) insert full-width punctuation when no code is being typed.

## Build and install

Visual Studio 2022 (Desktop development with C++) + CMake:

```
cmake -S . -B build -A x64
cmake --build build --config Release
regsvr32 build\Release\KeyKeyTSF.dll     (elevated)
```

`build.ps1` does the same and stages a package folder with `install.bat`.
CI (`.github/workflows/build-tsf.yml`) builds x64 and x86 on `windows-latest`
and publishes both zips to the `tsf-preview` release.

Then add "Yahoo! KeyKey (TSF)" under 中文(繁體，台灣) in Windows language settings
and switch with Win+Space. Diagnostics: `%LOCALAPPDATA%\KeyKeyTSF\keykeytsf.log`
and `diagnose.ps1` in the package.

## Not done yet

- No phrase database / 詞庫, no 繁簡 conversion, no Bopomofo or Simplex tables
  (the table-driven design takes them, the UI and the engine are CangJie-shaped).
- ASCII/English mode is a straight pass-through, not an alphanumeric input module.
- Per-application settings and an installer.
