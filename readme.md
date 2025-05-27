# Meritous Gaiden
Original game by Lancer-X/ASCEAI

Item randomizer and Archipelago integration by KewlioMZX

## Story

Five generations after the Orcus Dome incident, a new facility called the Atlas Dome is built below the surface of the moon. There, unknown research takes place, led by a man known only as "Berserker".

However, as of recently, strange events have been occurring, which seem to threaten the fabric of reality itself. Thus, PSI user VIRTUE travels to the secretive Atlas Dome in search of answers.

## Installation

Run Meritous-ap.exe after extraction and everything should work correctly.

If the game is too difficult for you, you can play in 'Training mode' for a far more sombre gaming experience.

## Basic Controls

Key|Description
--|--
Arrow keys|Move around. To walk through doors, simply walk up to them and push against them.
Space|Charge your PSI circuit for attacking.
C|A certain artifact will be enabled while this key is held.
Tab|View the map (you can then use the arrow keys to scroll around the map).
Enter|Activate a trigger tile that you are standing on. Enter is also used for various other things, such as for reading in-game dialogue.
P|Pause the game.
H|View the help file.
Escape|Exit the game.

Some controllers may be supported, but there is currently no guarantee that this support will work.

## Item Randomizer & Archipelago Multiworld

This game comes equipped with an item randomizer which is also compatible with Archipelago Multiworld. For players who have played the original ("vanilla") version of MERITOUS, you will notice that PSI Enhancement tiles have been replaced with item cache tiles. Each one of these may contain any sort of item, including artifacts, PSI enhancements, crystals, or, in the case of a multiworld, items for other players.

### meritous-ap.json

Currently, in order to configure the randomizer, meritous-ap.json must be modified to suit your playing environment. The values are as follows:

Key|Definition
--|--
`ap-enable`|Enables the Archipelago multiworld client. If it is not enabled, or if this file is missing, the local randomizer will be used. *Default: `false`*
`server`|The server to which the Archipelago client will connect. May be a domain name or IP address. *Default: archipelago.gg*
`port`|The port to which the Archipelago client will connect. *Default: 38281*
`password`|If applicable, the password to use to connect to the Archpelago instance. If one is used, provide it within quotation marks. If not, omit this value or provide `null` without quotation marks. *Default: `null` (no password will be used)*
`slotname`|The slot name to connect to. This would be your player name, as provided in the configuration file (YAML) for this Archipelago session. *No default (must be provided if AP is enabled)*

In a future update, for ease of use, this process will be moved into in-game menus.

### YAML configuration file

The player configuration file (also known as YAML) will determine the generation parameters for the game. To obtain one, refer to [the official Archipelago options page for this game](https://archipelago.gg/games/Meritous/player-options).

## Configuring playlist

The file `dat/m/manifest.json` contains the playlist currently used for the game. This file can be changed if different songs are desired. In general, any tracker format songs supported under libxmp can be used. Formats such as mp3, ogg, etc. might also be used, but these are untested.

Track name|Description
--|--
`title`|Used on the title screen/menu.
`cave1-4`| Used in the Atlas Dome. Which one is played depends on how far from the Entrance the player is.
`ambush`|Used in active Artifact rooms, which contain enemy ambushes and chests.
`boss1-3`| Used during standard bosses. `boss1` is also used during the final form of the true final boss.
`chase`|Used after collecting the Cursed Seal, as the player returns to the Entrance.
`finale`|Used during the final boss.
`truefinal`|Used during the second form of the true final boss.
`ending`|Used during the ending sequence.

## Building Meritous

Meritous Gaiden depends on the following libraries:

- SDL 2
  - SDL2-image
  - SDL2-mixer
- zlib
- libpng
- asio <= 1.32

In addition, under Windows, the Winsock library is required for the Archipelago client component to work, and `windres` is recommended to compile the RC file.

**PLEASE NOTE**: Although it has been previously recommended to build Meritous in 32-bit, the ASIO library required by `apclientpp` is no longer available in 32-bit. Also note that 32-bit and 64-bit saves may not be cross-compatible, but they should be so between Windows and Linux.

## Acknowledgements

Thanks to Lancer-X's beta testers: Quasar, Terryn and Wervyn, for their hard work in assisting him in keeping Meritous fairly stable.

Thanks to Kewlio's beta testers: MazukiTskiven, alwaysontreble, and the Archipelago community in general, for continuing support in this endeavor.

Thanks to code contributors: @jknorr1992, @Rnd-Guy

Special thanks to Frankomatic for streaming Meritous at just the right time to give Kewlio the idea to do this in the first place.
