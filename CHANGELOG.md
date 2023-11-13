# Changelog

## 4.0.14
- Fix the description of a release created by the build pipeline.

## 4.0.13
- Addressed to-do in Autobuy: adhere to Ammo Limits specified in .ini files.

## 4.0.12
- Fix the bugs reported in SonarCloud. This is mainly throwing a proper exception rather than just a string.

## 4.0.11
- Added Loot Tables plugin that allows you greater control over the loot dropped by NPCs.

## 4.0.10
- Fixed bug in CI that prevented Sonar running if there hasn't been a commit for a while.

## 4.0.9
- Fixed bug in Stats plugin where data was not being converted to UTF-8 causing issues with non-standard characters.

## 4.0.8
- Fix bugs with NPC Control Plugin.
- Fix bug with the chase admin command.

## 4.0.7
- Fix code smells shown in SonarLint.

## 4.0.6

- Moved multikill tracking to KillTracker plugin & removed corresponding functionality
from other places in the code.

## 4.0.5

- Changed CargoDrop plugin to accept more than two commodities, amount of Hull drops now calculcated by mass rather than ship hold size.

## 4.0.4

- Update autobuy to cover miscellaneous ammo types if they are present (HP_GUN).

## 4.0.3

- Fixed exceptions on JumpInComplete hook due to improper parameter count.

## 4.0.2

- Added functionality to warehouse to list all bases a player has stored items and the quantity. 
- Fixed comparison operator with withdrawing items from storage to allow user to fill cargo to full.

## 4.0.1

- Fix commands being ignored if they begin with s, g, or l.

## 4.0.0

--- TODO: Write comprehensive list of changes for 4.0.0