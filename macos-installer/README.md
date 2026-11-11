# Yahoo! KeyKey 2 macOS native installer

This directory contains a **modern/native macOS installer build** for Yahoo! KeyKey 2 (macOS package) using `pkgbuild` and `productbuild`, replacing the legacy PackageMaker/DMG workflow. It is aligned with the Windows KeyKey 2 naming/versioning where applicable (installer title/branding and default version `1.1.2`, output `KeyKey2.pkg`).

It is intended to preserve the original installer behavior:

- Installs payload to `/Library/Input Methods/Yahoo! KeyKey.app`
- Runs `preinstall` / `postinstall` scripts (postinstall opens `InstallerHelp.app`, matching the original package)
- Shows the original license text
- Installs on the system volume only and requires restart on conclusion (as in the original distribution)

## Build

Provide a payload root containing:

`<payload-root>/Library/Input Methods/Yahoo! KeyKey.app`

Then run:

```sh
./build-pkg.sh /path/to/payload-root ./out
```

Output: `./out/KeyKey2.pkg` (and `./out/KeyKey2App.pkg` component package). Set `KEYKEY2_VERSION` to override the default version.

## Native Apple Silicon support (important)

Rewriting the **installer** is not enough for Apple-Silicon-only macOS versions that cannot run Intel binaries. The app bundle itself must contain **arm64 (or universal)** Mach-O binaries for:

- `Yahoo! KeyKey.app/Contents/MacOS/Yahoo! KeyKey`
- Bundled frameworks (e.g. `OpenVanilla.framework`, `LFExtensions.framework`)
- Bundled helper apps under `Contents/SharedSupport/` (Preferences, PhraseEditor, InstallerHelp, DownloadUpdate, etc.)
- Any loadable modules/plug-ins shipped inside the app

The original `orig/YahooKeyKey.pkg` payload inspected in this repo contains x86_64/i386/PPC binaries (no arm64). To keep functionality identical on Apple Silicon without Intel app support, the legacy Mac sources in `YahooKeyKey-Source-1.1.2528/` (e.g. `Loaders/OSX-IMK`, `Takao.xcodeproj`, preference/helper apps, frameworks/modules) must be ported and rebuilt with a modern Xcode/toolchain for `arm64`/universal, then packaged with this installer.

## Original package reference

From `orig/YahooKeyKey.pkg`:
- Product/component id: `com.yahoo.keykey.installerpackage` (version 7.0)
- Install target: `/Library/Input Methods`
- `require-scripts=true`, `rootVolumeOnly=true`, restart on conclusion
- Postinstall: `open "/Library/Input Methods/Yahoo! KeyKey.app/Contents/SharedSupport/InstallerHelp.app"`
