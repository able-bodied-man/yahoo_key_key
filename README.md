# Yahoo! KeyKey

Yahoo! KeyKey (Takao) is a Traditional Chinese input method derived from
[OpenVanilla](http://openvanilla.org). This repository contains the original
Yahoo! KeyKey 1.1.2528 source plus **Yahoo! KeyKey 2**, a Windows text-service
bridge that lets the legacy KeyKey engine work in modern Windows apps such as
WhatsApp from the Microsoft Store.

## Repository layout

- `YahooKeyKey-Source-1.1.2528/` — original KeyKey/OpenVanilla source
- `keykey2/` — Yahoo! KeyKey 2 text service for x64 Windows
- `DataSource-1.1.2528/` — data-source placeholders (licensed corpora are not
  included)
- `MEMORY.md` — agent handoff notes: architecture, build, release process, and
  known pitfalls

## Yahoo! KeyKey 2

The Microsoft Store version of WhatsApp is a WinUI 3/WebView2 app and does not
load the legacy IMM32 `KeyKey.ime`. The bridge in `keykey2/` reuses the
installed `KeyKeyServer.exe` over the original local RPC protocol, so the input
engine, candidate window, status bar, databases, and OneKey features stay the
legacy KeyKey behavior.

See [`keykey2/README.md`](keykey2/README.md) for build, install, diagnostics, and
limitations.

Quick install from a release:

1. Run `KeyKey2-<version>-x64-Setup.exe` and approve UAC.
2. Select **Yahoo! KeyKey 2** with `Win+Space`.

The EXE installer bundles the original KeyKey engine and installs it silently
when needed. It installs to `C:\Program Files\Yahoo!\KeyKey2`. The installed
folder also contains `install.ps1` and `uninstall.ps1` for troubleshooting; a
source checkout can be installed manually with `install.ps1` if the legacy
engine is already present.

The bridge is currently x64 only.

## License

The original KeyKey source is distributed under the BSD license; see
`LICENSE.txt`. Third-party notices for the bridge are in
`keykey2/THIRD_PARTY_NOTICES.md`.
