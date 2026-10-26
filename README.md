# FRSFastTest

ASI plugins for the Black Box Need for Speed games that boot the game straight
into play: no intro movies, no "press Enter", no menus. One plugin per game,
released separately.

| Game | Folder | Executable | What it boots into | Status |
|---|---|---|---|---|
| Underground | [`games/nfsu1`](games/nfsu1) | speed.exe v1.4 | a race on a chosen track (SkipFE) | beta |
| Underground 2 | [`games/nfsu2`](games/nfsu2) | SPEED2.EXE v1.2 | the career's free roam from your save, or any track (SkipFE) | released |
| Most Wanted | [`games/nfsmw`](games/nfsmw) | speed.exe v1.3 | a race with a chosen car and track (SkipFE) | beta |
| ProStreet | [`games/nfsps`](games/nfsps) | nfs.exe v1.1 | an event with a chosen car preset (SkipFE) | beta |
| Carbon | [`games/nfsc`](games/nfsc) | NFSC.exe v1.4 | any event with chosen cars, opponents and laps, or free roam (SkipFE) | beta |

Each plugin checks the code it patches and does nothing on any other
executable. All but the Underground 2 plugin also have a
`[Window]` section for development: windowed mode, a fixed resolution for the
session, the monitor to open on, and running in the background without focus.

## Install

1. An ASI loader in the game folder (any Ultimate ASI Loader build).
2. Copy `FRSFastTest.asi` and the game's files (`FRSFastTest.ini`, and the
   track or event list where there is one) into the game's `scripts` folder.

Each release package has the loader, the files and a README for its game.

## Configuration

`FRSFastTest.ini` sits beside the plugin in `scripts`. These are the defaults each package ships with.

### Underground

Every track, with its family, is listed in `FRSFastTest-Tracks.txt`. The race is always a lap race.

```ini
[FastTest]
; 0 = the game starts as usual
Enabled = 1
; Track: a track number (0 = the default, the first track of the list).
; Every track: FRSFastTest-Tracks.txt, next to this file. The race is always a lap race.
Track = 0
; AICars: opponents (-1 = the default, none)
AICars = -1
; Laps (0 = the default, 2)
Laps = 0
; Car: a car folder name, e.g. SKYLINE, CIVIC, SUPRA, RX7, 350Z, IMPREZA, LANCER, S2000 (empty = the first garage car)
Car =

[Window]
; A window comes from NFSUnderground.WidescreenFix (WindowedMode in its ini); Monitor applies to a window only.
; Monitor: 0 = leave the window where the game puts it, 1 = primary, 2.. = the others
Monitor = 0
; Width, Height: resolution for this session only (0 = the game setting)
Width = 0
Height = 0
; Background: 1 = never take the focus, and keep running without it
Background = 0
```

### Underground 2

Every track, with its race type, is listed in `FRSFastTest-Tracks.txt`.

```ini
[FastTest]
; Boot straight into free roam, no screens to click through (0 = normal start).
Enabled=1
; career = the career's free roam, with your save (default).
; dev    = the developers' SkipFE: faster, straight into the city, but no save loaded.
Mode=career

; career: leave the intro movies (EA, THX, PSA, intro) out of the boot.
SkipIntro=1
; career: leave the 'press Enter' screen out of the boot.
SkipSplash=1
; career: answer the profile's 'Continue' by itself.
AutoContinue=1
; career: extra wait, in ms, once the career is loaded.
DelayMs=0

; dev: the track number (-1 = the race type's own; 4000, the whole city, for free roam).
; Every track, with its race type: FRSFastTest-Tracks.txt, next to this file.
DevTrack=-1
; dev: the race type -- 0 free roam / circuit / speedway / SUV, 1 sprint, 2 street X, 4 drag, 5 drift.
DevRaceType=0

; Quit the game when the pause menu opens, in a session started this way (0 = off).
QuitOnPause=0
```

### Most Wanted

```ini
[FastTest]
; 0 = the game starts as usual
Enabled = 1
; Car: a car model, e.g. bmwm3gtre46, 911turbo, gti, 997s, murcielago (empty = bmwm3gtre46)
Car =
; Track: a track number (0 = the default, 2000)
Track = 0
; Laps (0 = the default, 2)
Laps = 0
; AICars: number of opponents (-1 = the default)
AICars = -1
; AIControl: 1 = the AI drives the player's car, from 1 s into the race
AIControl = 0

[Window]
; Windowed: 1 = run in a window (the game's own windowed mode)
Windowed = 0
; Monitor: 0 = leave the window where the game puts it, 1 = primary, 2.. = the others
Monitor = 0
; Width, Height: resolution for this session only (0 = the game setting)
Width = 0
Height = 0
; Background: 1 = never take the focus, and keep running without it
Background = 0
```

### Carbon

Every event id is listed in `FRSFastTest-Events.txt`.

```ini
[FastTest]
; 0 = the game starts as usual
Enabled = 1
; Event: an event id such as sf.1.1; all ids are in FRSFastTest-Events.txt (empty = free roam)
Event = sf.1.1
; Car: the player's car, a folder name from CARS, e.g. viper, bmwm3gtre46, rx7, gallardo, 997tt (empty = viper)
Car =
; OpponentCar: the opponents' car, as Car (empty = viper)
OpponentCar =
; AICars: number of opponents (-1 = the default, none)
AICars = -1
; Laps: laps of a circuit event (0 = the default, 2)
Laps = 0
; SkipSMS: 1 = no text messages (the SMS that holds the screen when free roam starts)
SkipSMS = 1

[Window]
; Windowed: 1 = run in a window (the game's own windowed mode)
Windowed = 0
; Monitor: 0 = leave the window where the game puts it, 1 = primary, 2.. = the others
Monitor = 0
; Width, Height: resolution for this session only (0 = the game setting)
Width = 0
Height = 0
; Background: 1 = never take the focus, and keep running without it
Background = 0
```

### ProStreet

Every event id, with its track, type and layout, is listed in `FRSFastTest-Events.txt`.

```ini
[FastTest]
; 0 = the game starts as usual
Enabled = 1
; Event: an event id such as 75.gr.1; all ids, with track, type and layout, are in FRSFastTest-Events.txt
; (empty = the game's default)
Event = 75.gr.1
; Car: a car preset, e.g. 350z_track, bmwm3e92_track, camaro_track, rx7_track, zonda_track (empty = 350z_track)
Car = 350z_track
; SkipMatchup: 1 = no pre-race matchup screen
SkipMatchup = 1
; SkipIntroNIS: 1 = skip the race intro cut-scenes
SkipIntroNIS = 1

[Window]
; Monitor: 0 = leave the window where the game puts it, 1 = primary, 2.. = the others
Monitor = 0
; Width, Height: resolution for this session only; the saved setting is kept (0 = the game setting)
Width = 0
Height = 0
; Background: 1 = never take the focus, and keep running without it
Background = 0
```
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

Free to use, study, modify and redistribute, with credit to Chrystian Farias
and a link back to this repository; it may not be sold, in whole or in part.
See [LICENSE](LICENSE) for the full terms.
