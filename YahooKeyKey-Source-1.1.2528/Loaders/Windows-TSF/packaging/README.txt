# KeyKeyTSF - CangJie input method (TSF)

Traditional Chinese input with CangJie, as a modern Windows text service.
Self-contained: the CangJie table is compiled into KeyKeyTSF.dll and the
candidate window and status bar are drawn by the DLL. Yahoo! KeyKey does not need
to be installed.

## Install

1. Right-click install.bat and choose "Run as administrator".
   (Use the one that came with this folder's KeyKeyTSF.dll: x64 or x86.)
2. Settings > Time & language > Language & region
   > Chinese (Traditional, Taiwan) > ... > Add a keyboard
   > "Yahoo! KeyKey (TSF)".
3. Switch input methods with Win+Space.

## Typing

- CangJie letters build a code; radicals show in the floating window next to the
  caret, candidates below them.
- 1-9 or Space/Enter commits, Esc cancels, Backspace edits the code.
- A single Shift toggles Chinese/English; so does clicking 中 on the purple bar.
- , . ; ' [ ] / etc. type full-width punctuation when no code is being typed.

## If something is wrong

Run diagnose.ps1 (right-click -> Run with PowerShell) and send keykeytsf-diag.txt.
The log is at %LOCALAPPDATA%\KeyKeyTSF\keykeytsf.log.

If Windows blocks the file (Smart App Control), first run
    powershell -Command "Unblock-File .\KeyKeyTSF.dll"
and reinstall. Otherwise build it locally - a locally produced binary is usually
accepted: see build.ps1 in the repository.

## Uninstall

Right-click uninstall.bat, run as administrator.
