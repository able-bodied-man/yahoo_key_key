# KeyKey TSF front end (test build)

This is the Text Services Framework front end for Yahoo! KeyKey. It lets the
installed KeyKey engine drive apps that only support TSF: the Microsoft Store
build of WhatsApp, other WinUI 3 / UWP apps, and the Windows 11 shell.

It is a front end only. KeyKey must already be installed and its
BaseIMEServer.exe must be running; the conversion engine, the dictionaries and
all the windows (status bar, the 口尸手火 input buffer, the candidate list) stay
where they are - in the KeyKey server.

## Install

1. Right-click `install.bat` and choose "Run as administrator".
   Use the file that came with this folder's KeyKeyTSF.dll (x64 or x86).
2. Settings > Time & language > Language & region
   > Chinese (Traditional, Taiwan) > ... > Add a keyboard
   > "Yahoo! KeyKey (TSF)".
3. Switch input methods with Win+Space.

If the entry does not appear in the keyboard list, sign out and back in once.

## What to check

- Notepad: type `hg`. The purple status bar should appear, `口尸手火` should show
  in the floating buffer next to the caret, and the candidate list should offer
  我 / 你 / ...
- WhatsApp from the Store: the same. This is the whole point of the port.
- Click the 中 button to switch to English: letters must come out unchanged and
  Ctrl+C must still work.
- Alt+Tab away and back: the bar and the buffer disappear and come back.

## Known gaps in this build

- Clicking a candidate in the panel does nothing yet; use the number keys.
- The composition is shown only in KeyKey's floating buffer, not inline in the
  document text.
- 64-bit apps use the x64 build, 32-bit apps use the x86 build; register both if
  you want both.
- Registration is machine-wide (HKLM), so it needs administrator.

## Uninstall

Right-click `uninstall.bat`, run as administrator.
