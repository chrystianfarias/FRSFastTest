# FRSFastTest

ASI plugins for the Black Box Need for Speed games that boot the game straight
into play: no intro movies, no "press Enter", no menus. One plugin per game,
released separately.

| Game | Folder | Executable | What it boots into | Status |
|---|---|---|---|---|
| Underground | [`games/nfsu1`](games/nfsu1) | speed.exe v1.4 | a race on a chosen track (SkipFE) | in progress |
| Underground 2 | [`games/nfsu2`](games/nfsu2) | SPEED2.EXE v1.2 | the career's free roam from your save, or any track (SkipFE) | released |
| Most Wanted | [`games/nfsmw`](games/nfsmw) | speed.exe v1.3 | a race with a chosen car and track (SkipFE) | beta |
| ProStreet | [`games/nfsps`](games/nfsps) | nfs.exe v1.1 | an event with a chosen car preset (SkipFE) | beta |
| Carbon | [`games/nfsc`](games/nfsc) | NFSC.exe v1.4 | a race with a chosen car and track (SkipFE) | in progress |

Each plugin checks the code it patches and does nothing on any other
executable. The Most Wanted, ProStreet and Carbon plugins also have a
`[Window]` section for development: windowed mode, a fixed resolution for the
session, the monitor to open on, and running in the background without focus.

## Install

1. An ASI loader in the game folder (any Ultimate ASI Loader build).
2. Copy `FRSFastTest.asi` and the game's files (`FRSFastTest.ini`, and the
   track or event list where there is one) into the game's `scripts` folder.

Each release package has the loader, the files and a README for its game.

## Layout

```
common/fasttest.h     logging, patching, import hooks, [Window] options
games/<game>/         FastTest.cpp, FRSFastTest.ini, data files, README.txt (the package's), VERSION
tools/package.ps1     builds and packages one game
tools/dev/            test loop: build, install, run and capture a game
```

The addresses each plugin relies on, and why, are documented at the top of
its `FastTest.cpp`.

## Build

Visual Studio 2022 with the C++ workload, and CMake 3.21 or newer. The games
are 32-bit, so the plugins are built for Win32:

```
cmake -S . -B build -G "Visual Studio 17 2022" -A Win32
cmake --build build --config Release                 # every game
cmake --build build --config Release --target nfsps  # one game
```

The output is `build\<game>\FRSFastTest.asi`, with the runtime linked
statically.

## Release

Every game is versioned on its own (`games/<game>/VERSION`) and tagged as
`<game>-v<version>`:

```
.\tools\package.ps1 -Game nfsps
git tag nfsps-v0.1.0
gh release create nfsps-v0.1.0 dist\FRSFastTest-NFSPS-0.1.0.zip --title "FRSFastTest for ProStreet 0.1.0"
```

## Development

`tools/dev/run.ps1` builds one game's plugin, installs it, starts the game,
and prints `FRSFastTest.log` after a while. Game folders are listed in
`tools/dev/games.local.psd1`, which is not committed:

```powershell
@{
    nfsu2 = "C:\Games\NFSU2"
    nfsps = "C:\Games\Need for Speed ProStreet"
}
```

```
.\tools\dev\run.ps1 -Game nfsc -Seconds 40 -Shot D:\tmp\carbon.png
```

## Support

If FRSFastTest saves you some clicks, you can buy me a coffee:
https://buymeacoffee.com/chrystianfarias

## License

Licensed under Creative Commons Attribution-NonCommercial 4.0 International
(CC BY-NC 4.0). You may share and adapt this work with attribution, for
non-commercial purposes only. See [LICENSE](LICENSE) for the full text.
