# starcraft1bot

StarCraft: Brood War AI bot project. Zerg bot that tracks visible enemy army value
and plays a ling rush. Developed on Linux (OpenBW), submitted to BASIL via SSCAIT.

## Layout

- `valuebot/` — bot source (shared by Linux and Windows builds)
  - `ValueBot.h`, `ValueBot.cpp`
  - `CMakeLists.txt` — Linux / OpenBW build
  - `vs/` — Visual Studio 2017 project for the Windows tournament build
- `basil-upload/` — submission folder: build the Windows DLL on your PC,
  package it with the script inside, and upload the ZIP to SSCAIT.
  Start with `basil-upload/README.md`.

## Local test result (Linux, OpenBW headless)

ValueBot (Zerg) vs ExampleAIModule (Terran) on Fighting Spirit:
first game found and fixed a scouting bug, second game won at frame 6978.
Replays from that run are not stored here (see `basil-upload` for the ladder path).
