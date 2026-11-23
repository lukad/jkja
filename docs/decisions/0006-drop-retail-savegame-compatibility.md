---
status: accepted
date: 2026-11-23
decision-makers: Luka Dornhecker
---

# Drop retail savegame compatibility

## Context and Problem Statement

Single-player savegames are written as raw copies of in-memory structs: 65 call sites
use `sizeof` on game structures, and pointers inside them are converted through field
tables (`code/game/g_savegame.cpp`). The layout of those structs changes on 64-bit, so a
64-bit build cannot read retail saves, or even its own, without work. Must saves written
by the retail game load in this project?

## Decision Drivers

* Savegames must work reliably on 64-bit.
* Early milestones should not carry work that does not serve getting the game running.
* The option to add retail compatibility later should not be destroyed.

## Considered Options

* Drop retail compatibility; new format
* Keep retail compatibility from the start
* No savegames until later

## Decision Outcome

Chosen option: "Drop retail compatibility; new format", because reproducing the exact
32-bit memory layout of every saved struct is a substantial project of its own and
serves nobody until the game runs. If it is wanted later, it can be brought back.

The new format serialises field by field, driven by the existing field tables, with a
new version number so that retail saves are rejected cleanly rather than misread. The
chunk structure and the field tables are kept, which is what a future retail-format
reader would need.

### Consequences

* Good, because savegame work in M2 is limited to making saves correct on 64-bit.
* Good, because struct layouts in game code are free to change.
* Bad, because players cannot continue a retail campaign; they start over.

### Confirmation

A save written and loaded on each platform restores the play-test state. A retail save
file is rejected with a clear message and no crash.

## Pros and Cons of the Options

### New format

* Good, because least work and most freedom.
* Bad, because no continuity with retail saves.

### Retail compatibility from the start

* Good, because existing saves keep working.
* Bad, because it freezes the 32-bit layout of every saved struct into the code before
  the game even runs.

### No savegames until later

* Good, because no work now.
* Bad, because save and load is part of the play test that defines "the game runs".

## More Information

The exact new format is its own decision, due before M2 (see `docs/design.md`,
section 9).
