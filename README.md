# FRSFastBoot

An ASI plugin for **Need for Speed Underground 2** that boots the game straight
into free roam -- no intro movies, no "press Enter", no profile screen, no
menus.

Two modes, chosen in `FRSFastBoot.ini`:

- **`Mode=career`** (default) -- the career's free roam, from your own save:
  your car, your money, the shops. The front end still runs underneath,
  because that is where the game reads the save, but every screen that waits
  for the player is left out or answered.
- **`Mode=dev`** -- the developers' own switch, *SkipFE*, still in the
  executable: the front end never runs and the game loads the city at once.
  Faster, but no save is loaded (a default car, no money). Any track can be
  started this way, not only free roam (`DevTrack`, `DevRaceType`).

Optionally (`QuitOnPause=1`) a session started this way ends at the pause
menu: the game quits when the pause menu opens, however it was opened -- the
menu itself is detected, not a key, so mods that use Esc are not affected.

## Install

1. An ASI loader in the game folder (any Ultimate ASI Loader build).
2. Copy `FRSFastBoot.asi`, `FRSFastBoot.ini` and `FRSFastBoot-Tracks.txt`
   into the game's `scripts` folder.

It needs no other mod. It can sit beside NFSU2ExtraOptions: a boot screen is
only removed while it still names the game's own screen, so when another mod
has already dealt with it, the two do not fight.

## Game version

Written and tested against this `SPEED2.EXE`:

| | |
|---|---|
| Size | 4,800,512 bytes |
| MD5 | `6445adbf4e8d64be51e96a69bba168a4` |
| SHA-256 | `f9dd86c054878ce6276beb07c1fd61874f7a1e4bf1f241b084c65b73e24168a7` |

(the executable carries no version resource; this is the build commonly called
v1.2). The plugin checks the start of the code it relies on and does nothing
on any other executable. To check yours, in PowerShell in the game folder:

```
Get-FileHash .\SPEED2.EXE -Algorithm MD5
```

## Settings

| Key | Default | |
|---|---|---|
| `Enabled` | 1 | 0 leaves the game alone |
| `Mode` | career | `career` or `dev` |
| `SkipIntro` | 1 | career: no intro movies |
| `SkipSplash` | 1 | career: no "press Enter" screen |
| `AutoContinue` | 1 | career: answers the profile's "Continue" |
| `DelayMs` | 0 | career: extra wait once the career is loaded |
| `DevTrack` | -1 | dev: track number, see `FRSFastBoot-Tracks.txt` |
| `DevRaceType` | 0 | dev: 0 free roam / circuit / speedway / SUV, 1 sprint, 2 street X, 4 drag, 5 drift |
| `QuitOnPause` | 0 | 1 quits the game when the pause menu opens |

In career mode, holding **Shift** when the career has loaded stays in the menus
for that session. A career that has not started yet (its intro is a front-end
screen) also stays in the menus.

In dev mode, races start with the developers' defaults for opponents and laps,
which cannot be set yet; only free roam (track 4000) has been tried.

## Build

Visual Studio 2022 with the C++ workload, and CMake 3.21 or newer. The game is
32-bit, so the plugin is built for Win32:

```
cmake -S . -B build -G "Visual Studio 17 2022" -A Win32
cmake --build build --config Release
```

The plugin is `build\Release\FRSFastBoot.asi`. It links the runtime statically:
nothing else has to be shipped with it.

## How it works

Everything is written up where it is used, at the top of `src/FastBoot.cpp`:
the career's resume path the plugin runs without its screens, the table of
boot screens the intro and "press Enter" are taken out of, the memory-card
dialog whose "Continue" is answered, the controller mask a key press would have
set, the SkipFE switch and its track and race-type globals, and how the pause
menu is recognised.

## Support

If FRSFastBoot saves you some clicks, you can buy me a coffee:
https://buymeacoffee.com/chrystianfarias

## License

Licensed under Creative Commons Attribution-NonCommercial 4.0 International
(CC BY-NC 4.0). You may share and adapt this work with attribution, for
non-commercial purposes only. See [LICENSE](LICENSE) for the full text.
