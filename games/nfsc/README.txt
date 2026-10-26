FRSFastTest
===========

For Need for Speed Carbon: the game boots straight into a race, through the
developers' own quick start (SkipFE) still inside the game. No intro, no
menus, no save loaded. Meant for modders who restart the game often.


INSTALL
-------

Copy everything in this folder into the game folder (the one with NFSC.exe):

    dinput8.dll                  next to NFSC.exe
    scripts\FRSFastTest.asi      into the game's "scripts" folder
    scripts\FRSFastTest.ini

dinput8.dll is Ultimate ASI Loader, by ThirteenAG. If you already have an ASI
loader in the game folder, keep yours and skip this file.


SETTINGS  (scripts\FRSFastTest.ini)
--------

    [FastTest]
    Enabled=1        0 = the game starts as usual
    Car=             your car, a folder name from CARS (empty = viper)
    OpponentCar=     the opponents' car, as Car (empty = viper)
    AICars=-1        number of opponents (-1 = the default, none)
    Laps=0           laps (0 = the default, 2)
    SkipSMS=1        no text messages (the free-roam SMS at the start)
    [Window]
    Windowed=0       1 = run in a window
    Monitor=0        0 = where the game puts it, 1 = primary, 2.. = others
    Width=0          resolution for this session only (0 = game setting)
    Height=0
    Background=0     1 = never take the focus, keep running without it


GAME VERSION
------------

Made for NFSC.exe v1.4:

    size    7,217,152 bytes
    MD5     53708939d0dee3a3c58d046cb73eb80d

On any other executable FRSFastTest does nothing.

UNINSTALL
---------

Delete scripts\FRSFastTest.asi and its .ini. Leave dinput8.dll if other mods
use it.


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
