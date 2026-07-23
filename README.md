## Description
AzerothCore server mod to support EverQuest content.

In addition to this mod, EverQuest client data must also be converted by the tool in this repository: https://github.com/NathanHandley/EQWOWConverter

Requires at least this changeset: https://github.com/NathanHandley/azerothcore-wotlk/commit/f69161bcfad69ae8b07a5c9957cb8321e4700446

## In-Game Commands

| Command | Permission | Description |
|----------|------------|--------------|
| `.eqgps` | Player | Shows x y z and heading/orientation information for WoW coordinates (True), WoW coordinates before applying the world scale value in the converter configuration (Prescale), and the coordinates if it was EverQuest (EverQuest) |
| `.eqface` | Player | Changes the EQ face ID you'll see when you are form transformed into a playable EQ race |
| `.eqshowbardpulse` | Player | Enabled/Disables the spell particles for 'pulses' of bard songs on all players in a group |
| `.class` | Player | Holds many commands to change your secondary EQ class |