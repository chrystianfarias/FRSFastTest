FRSFastTest
===========

For Need for Speed Underground 2: the game boots straight into free roam --
no intro movies, no "press Enter", no profile screen, no menus.


INSTALL
-------

Copy everything in this folder into the game folder (the one with SPEED2.EXE):

    dinput8.dll                  next to SPEED2.EXE
    scripts\FRSFastTest.asi      into the game's "scripts" folder
    scripts\FRSFastTest.ini
    scripts\FRSFastTest-Tracks.txt

dinput8.dll is Ultimate ASI Loader, by ThirteenAG: it is what loads the .asi
when the game starts. If you already have an ASI loader in the game folder
(dinput8.dll, dsound.dll, winmm.dll, ...), keep yours and skip this file.

No other mod is needed. It works alongside NFSU2ExtraOptions.


MODES
-----

Mode=career  (default)
    Your own career: your save, your car, your money, the shops. The game
    loads your profile and goes straight to the street.

Mode=dev
    The developers' own quick start, still inside the game: faster, but no
    save is loaded (a default car, no money). Any track can be started this
    way: set DevTrack and DevRaceType -- FRSFastTest-Tracks.txt lists every
    track with its race type.


SETTINGS  (scripts\FRSFastTest.ini)
--------

    Enabled=1        0 = the game starts as usual
    Mode=career      career or dev
    SkipIntro=1      career: no intro movies
    SkipSplash=1     career: no "press Enter" screen
    AutoContinue=1   career: answers the profile's "Continue"
    DelayMs=0        career: extra wait once the career is loaded
    DevTrack=-1      dev: the track number (-1 = free roam)
    DevRaceType=0    dev: 0 free roam / circuit / speedway / SUV,
                     1 sprint, 2 street X, 4 drag, 5 drift
    QuitOnPause=0    1 = the game closes when the pause menu opens

Holding Shift while the career loads keeps you in the menus for that session.


GAME VERSION
------------

Made for this SPEED2.EXE (the build usually called v1.2):

    size    4,800,512 bytes
    MD5     6445adbf4e8d64be51e96a69bba168a4

On any other executable FRSFastTest does nothing. To check yours, open
PowerShell in the game folder and run:

    Get-FileHash .\SPEED2.EXE -Algorithm MD5


UNINSTALL
---------

Delete scripts\FRSFastTest.asi (and its .ini and .txt). Leave dinput8.dll if
other mods use it.


SUPPORT
-------

If FRSFastTest saves you some clicks, you can buy me a coffee:
https://buymeacoffee.com/chrystianfarias


LICENSE
-------

FRSFastTest is free to use, share and modify, with credit to Chrystian
Farias and a link to https://github.com/chrystianfarias/FRSFastTest. It may
not be sold, in whole or in part. Ultimate ASI Loader keeps its own license
(MIT). See LICENSE.txt for both texts.
