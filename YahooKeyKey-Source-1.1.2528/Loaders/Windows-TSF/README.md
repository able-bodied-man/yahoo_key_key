# KeyKey TSF loader

A Text Services Framework (TSF) text service for Yahoo! KeyKey, so the input method works in
modern packaged apps (WhatsApp from the Store, WinUI/UWP apps, Win11 shell surfaces) where the
legacy IMM32 loader in `Loaders/Windows-IMM/BaseIME` cannot be loaded.

## Why a new loader and not a patch

`BaseIME.ime` is an IMM32 IME DLL (`ImeInquire` / `ImeProcessKey` in `BaseIME.def`). Windows only
calls those entry points inside processes that host an IMM context. Packaged apps (UWP, WinUI 3)
use TSF only, so the DLL is never loaded there: no `WM_IME_SETCONTEXT` / `WM_IME_SELECT` /
`IMN_OPENSTATUSWINDOW`, therefore no status bar, no input buffer, no candidates, no Chinese input.

The good news: in this codebase the IME DLL is only a *front end*. All conversion happens in
`BaseIMEServer.exe`, which hosts the OpenVanilla modules, the dictionaries, and the whole C# UI
(status bar, floating input buffer, candidate panel). The front end talks to it over an RPC
interface, `BaseIMERPC/BIServerRPCInterface.idl`.

So the port is: **write a new front end that speaks the same RPC**. The engine, the databases, the
preferences application and the UI stay exactly as they are — that is what keeps this tractable.

```
old:  app process -> BaseIME.ime (IMM32)  --ncalrpc-->  BaseIMEServer.exe (engine + UI)
new:  app process -> KeyKeyTSF.dll (TSF)  --ncalrpc-->  BaseIMEServer.exe (engine + UI)
```

## RPC contract (already implemented in the server)

| call | used for |
| --- | --- |
| `BISOpenConnection` / `BISCloseConnection` | per-process connection |
| `BISActivate` / `BISDeactivate` | focus gained/lost; also returns the current IM name |
| `BISHandleKey(keyCode, modifiers, ...)` | the entire key pipeline |
| `BISSetInputBufferPosition` / `BISShowInputBuffer` / `BISHideInputBuffer` | floating composition window |
| `BISSetCandidatePanelPosition` / `BISShowCandidatePanel` / `BISHideCandidatePanel` | candidate list |
| `BISShowStatusBar` / `BISHideStatusBar` | the purple toolbar |

`BISHandleKey` returns, per keystroke: committed text, the combined composition string plus a
per-character attribute string (`r`/`h`/`n`), the candidate page (separated by `0xffff`), the
highlighted index, and two flags saying whether the candidate panel should be visible/updated.

Key encoding is OpenVanilla's: `keyCode` is the ASCII character for printable keys (Shift folded
in, lower-cased unless Shift), or a special code (`Up=30, Down=31, Left=28, Right=29, Home=1,
End=4, PageUp=11, PageDown=12, Tab=9, Esc=27, Space=32, Return=13, Backspace=8, Delete=127`);
`keyModifiers` uses `Alt=1, Ctrl=4, Shift=8, NumLock=0x20, CapsLock=0x40`.

## The server endpoint is distribution specific

`BASEIME_SERVER_IDENTIFIER` is `"BIS" BASEIME_PRODUCT_NAME`, and the product name is a build define:
the vanilla tree uses `"BaseIME"`, the Yahoo! KeyKey distribution uses `"KeyKey"`
(`Loaders/Windows-IMM/BaseIME/BaseIME.vcproj`). KeyKey therefore listens on **`BISKeyKey`**, not
`BISBaseIME`. The loader tries the known names in order and logs which one answered; guessing a
single name produces a front end that loads, registers, and silently does nothing.

Diagnostics land in `%LOCALAPPDATA%\KeyKeyTSF\keykeytsf.log` (activation, connect attempts, the
first 40 keystrokes with their engine answers).

## Status

- [x] M1 — TSF text service: registers, activates, forwards keys to the server, commits text,
      shows the server-side status bar, input buffer and candidate panel at the caret.
- [ ] M2 — client RPC endpoint (`BICRequestSendFakeKey`) so clicking a candidate in the panel works;
      then `BISSetCurrentClientRPCEndpoint` / `BISEndCurrentClientRPCEndpoint`.
- [ ] M3 — inline composition in the document (ITfComposition + display attributes) instead of /
      in addition to the floating buffer window.
- [ ] M4 — per-application config, hotkeys (`ITfPreservedKeyNotifySink`), 32-bit build, installer.

## Build

Needs Visual Studio (MSVC + Windows SDK, for `midl`) and CMake ≥ 3.20:

```
cmake -S . -B build -A x64
cmake --build build --config Release
regsvr32 build\Release\KeyKeyTSF.dll      (elevated)
```

Then add the "Yahoo! KeyKey (TSF)" input method under 中文(繁體，台灣) in Windows language
settings. The installed KeyKey server (`BaseIMEServer.exe`) must be running — the TSF loader is
only a front end and has no dictionaries of its own.

Unregister with `regsvr32 /u KeyKeyTSF.dll`.

CI: `.github/workflows/build-tsf.yml` builds the DLL on `windows-latest` (midl + MSVC).
This repository is a GitHub *fork*, so Actions are switched off until the owner enables
them once in the Actions tab ("I understand my workflows, go ahead and enable them").

### What to test after registering

1. Settings → Time & language → Language & region → 中文(繁體，台灣) → Add a keyboard →
   "Yahoo! KeyKey (TSF)". Switch to it with Win+Space.
2. Notepad: type `hg` — the purple status bar appears, `口尸手火` shows in the floating
   input buffer next to the caret, and the candidate panel lists 我, 你, ...
3. Repeat in the Microsoft Store build of WhatsApp — this is the whole point of the port.
4. English mode (click 中 in the bar): letters must come out untouched, Ctrl+C must work.
5. Alt+Tab away: status bar and input buffer must disappear; back: they return at the caret.

## Notes / known limits

- The floating composition window and candidate list are drawn by `BaseIMEServer.exe`, i.e. by a
  different process. They therefore float above any app, packaged or not — the look is unchanged.
- Keys the engine does not consume are returned to the app (`OnKeyDown` returns `FALSE`), so
  English mode, Ctrl+C and friends keep working without a passthrough hack.
- `Connection point` for the server UI: the server's status bar is not actually topmost in the
  legacy build (`SetWindowPos(..., HWND_TOP)`), which is unrelated to TSF but worth fixing in
  `BaseIMEUI/Win32FunctionHelper.cpp` if the bar still hides behind some windows.
