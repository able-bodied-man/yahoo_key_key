# Yahoo! KeyKey

Yahoo! KeyKey (Takao) is a Traditional Chinese input method derived from
[OpenVanilla](http://openvanilla.org). This repository contains the original
Yahoo! KeyKey 1.1.2528 source plus a TSF bridge that lets the legacy KeyKey
engine work in modern Windows apps such as WhatsApp from the Microsoft Store.

## Repository layout

- `YahooKeyKey-Source-1.1.2528/` — original KeyKey/OpenVanilla source
- `tsf/` — TSF text service for x64 Windows
- `DataSource-1.1.2528/` — data-source placeholders (licensed corpora are not
  included)
- `MEMORY.md` — agent handoff notes: architecture, build, release process, and
  known pitfalls

## TSF bridge

The Microsoft Store version of WhatsApp is a WinUI 3/WebView2 app and does not
load the legacy IMM32 `KeyKey.ime`. The TSF bridge in `tsf/` reuses the
installed `KeyKeyServer.exe` over the original local RPC protocol, so the input
engine, candidate window, status bar, databases, and OneKey features stay the
legacy KeyKey behavior.

See [`tsf/README.md`](tsf/README.md) for build, install, diagnostics, and
limitations.

Quick install from a release:

1. Run `KeyKeyTSF-<version>-x64-Setup.exe` and approve UAC.
2. Select **Yahoo! KeyKey (TSF)** with `Win+Space`.

The EXE installer bundles the original KeyKey engine and installs it silently
when needed. The ZIP release can also be installed from an elevated PowerShell:

```powershell
.\install.ps1
```

The ZIP/script path requires an already installed Yahoo! KeyKey 1.1.2535. The
bridge is currently x64 only.

## License

The original KeyKey source is distributed under the BSD license; see
`LICENSE.txt`. Third-party notices for the TSF bridge are in
`tsf/THIRD_PARTY_NOTICES.md`.
