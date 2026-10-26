FRSFastTest
===========

For Need for Speed Underground: the game boots straight into a race, through
the developers' own quick start (SkipFE) still inside the game. No intro, no
menus, no save loaded. Meant for modders who restart the game often.


INSTALL
-------

Copy everything in this folder into the game folder (the one with speed.exe):

    dinput8.dll                  next to speed.exe
    scripts\FRSFastTest.asi      into the game's "scripts" folder
    scripts\FRSFastTest.ini
    scripts\FRSFastTest-Tracks.txt

dinput8.dll is Ultimate ASI Loader, by ThirteenAG. If you already have an ASI
loader in the game folder, keep yours and skip this file.


SETTINGS  (scripts\FRSFastTest.ini)
--------

    [FastTest]
    Enabled=1        0 = the game starts as usual
    Track=0          track number, from FRSFastTest-Tracks.txt
                     (0 = the default, the first track)
    AICars=-1        opponents (-1 = the default, none)
    Laps=0           laps (0 = the default, 2)
    Car=             car folder name, e.g. SKYLINE, SUPRA, RX7, CIVIC
                     (empty = the first garage car)
    [Window]
    Monitor=0        0 = where the game puts it, 1 = primary, 2.. = others
                     (a window only: set WindowedMode in the WidescreenFix ini)
    Width=0          resolution for this session only (0 = game setting)
    Height=0
    Background=0     1 = never take the focus, keep running without it


The race is always a lap race: the quick start cannot set the race type, so
drag and drift tracks load without their drag or drift gameplay.


GAME VERSION
------------

Made for speed.exe v1.4:

    size    3,178,496 bytes
    MD5     684b760e077e2b51158b20bb42e71278

On any other executable FRSFastTest does nothing.

UNINSTALL
---------

Delete scripts\FRSFastTest.asi, its .ini and FRSFastTest-Tracks.txt. Leave
dinput8.dll if other mods use it.


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
