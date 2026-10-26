FRSFastTest
===========

For Need for Speed ProStreet: the game boots straight into an event, through
the developers' own quick start (SkipFE) still inside the game. No intro, no
menus, no matchup screen, no race intro. Meant for modders who restart the
game often.


INSTALL
-------

Copy everything in this folder into the game folder (the one with nfs.exe):

    dinput8.dll                  next to nfs.exe
    scripts\FRSFastTest.asi      into the game's "scripts" folder
    scripts\FRSFastTest.ini
    scripts\FRSFastTest-Events.txt

dinput8.dll is Ultimate ASI Loader, by ThirteenAG. If you already have an ASI
loader in the game folder, keep yours and skip this file.


SETTINGS  (scripts\FRSFastTest.ini)
--------

    [FastTest]
    Enabled=1        0 = the game starts as usual
    Event=75.gr.1    event id; every id is in FRSFastTest-Events.txt
    Car=350z_track   car preset, e.g. bmwm3e92_track, camaro_track, zonda_track
    SkipMatchup=1    no pre-race matchup screen
    SkipIntroNIS=1   skip the race intro cut-scenes

    [Window]
    Monitor=0        0 = where the game puts it, 1 = primary, 2.. = others
    Width=0          resolution for this session only; the saved setting
    Height=0         is kept (0 = game setting)
    Background=0     1 = never take the focus, keep running without it


GAME VERSION
------------

Made for nfs.exe v1.1:

    size    28,739,656 bytes
    MD5     0b2f2ff8e8c334ad33f13ab789fcfde2

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
