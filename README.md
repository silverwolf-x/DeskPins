# DeskPins

DeskPins makes any application window topmost, i.e. keeps it above all other windows.
Grab a pin from the DeskPins icon in the notification area and click on any window.
Features: unlimited pins, wildcard-based autopin rules, global hotkeys (default
<kbd>Ctrl</kbd>+<kbd>F11</kbd> to enter pin mode, <kbd>Ctrl</kbd>+<kbd>F12</kbd> to toggle the active window).

The original author is [Elias Fotinis](https://efotinis.neocities.org/index.html);
the source was retrieved from the [DeskPins webpage](https://efotinis.neocities.org/deskpins/index.html)
(development stopped in 2015 at v1.32). This fork makes it buildable again:

* no third-party dependencies: the author's unpublished `eflib` and Boost were replaced with plain Win32 code
* native x64 build with a statically linked CRT (a single exe, no VC++ redistributable needed)
* several memory-safety fixes (buffer sizes passed in bytes instead of characters, pointer truncation on x64, uninitialized structs)
* user interface in English and Simplified Chinese (follows the Windows display language, switchable in Options > Language)
* CMake build and NSIS installer (English/Chinese, optional desktop shortcut), built by GitHub Actions

## Download

Installers and portable zips are produced by the [build workflow](../../actions/workflows/build.yml);
tagged versions (`v*`) are published on the Releases page.

Requires 64-bit Windows 7 or later.

## Building

Builds run on GitHub Actions (`.github/workflows/build.yml`). For reference, the steps are:

```
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release   (from an x64 MSVC dev prompt)
cmake --build build
makensis /DVERSION=1.33.0 /DBINDIR=<abs path to build> installer\DeskPins.nsi
```

To publish a release, push a tag like `v1.33.0`.

## Layout

```
src/         application sources and resources (lang_zh_cn.rc: Simplified Chinese)
help/        HTML Help sources (English, German, Greek) and the prebuilt DeskPins.chm
installer/   NSIS installer script
```

## License

MIT, see [license.txt](license.txt).
