---
status: accepted
date: 2026-11-23
decision-makers: Luka Dornhecker
---

# Project name `jasp`; binaries keep their original names

## Context and Problem Statement

The build needs a project name, and the two binaries it will produce need file names.
The retail game ships the engine as `jasp.exe` and the game module as `jagamex86.dll`,
the latter hardcoded in the engine. The titles of the game are trademarks and should not
appear in the name of an unofficial project.

## Decision Drivers

* No trademark term in the project name.
* Recognisable to people who know the game and its files.
* The game module name must work for more than one CPU architecture.

## Considered Options

* Project `jasp`; binaries `jasp` and `jagame` plus an architecture suffix
* Project `academy`, binaries named after it
* Project `jediacademy`, matching the repository directory

## Decision Outcome

Chosen option: "Project `jasp`; binaries `jasp` and `jagame` plus an architecture
suffix", because the name is the one the engine has always had, carries no trademark,
and keeps every file where people who know the game expect it.

* CMake project name: `jasp`.
* Engine executable: `jasp` (`jasp.exe` on Windows).
* Game module: `jagame` followed by the architecture, with the platform's usual suffix
  for loadable modules: `jagamex86_64` on x86-64, `jagamearm64` on arm64. The engine
  derives the name at build time; it is not hardcoded per platform in the source.

### Consequences

* Good, because existing documentation, habits and scripts that say `jasp` stay valid.
* Good, because the module name states its architecture, so a wrong-architecture module
  is recognisable by name.
* Bad, because `jasp` means "single-player"; a later multiplayer port needs its own
  binary name (the retail one is `jamp`) and perhaps a broader project name. That is
  part of the multiplayer decision at M8.
* Bad, because the built executable has the same name as the retail one and must not be
  placed into a retail install directory by accident. The game data directory is
  treated as read-only ([ADR-0007](0007-game-data-location.md)), which already rules
  that out for the project's own tooling.

### Confirmation

The top-level `CMakeLists.txt` declares `project(jasp ...)`. From M1, the build produces
exactly the two binaries named above.

## Pros and Cons of the Options

### `jasp`, original binary names

* Good, because familiar and neutral.
* Bad, because narrow once multiplayer exists.

### `academy`

* Good, because it covers single-player and multiplayer.
* Bad, because it is a generic word, hard to search for, and nobody who knows the game
  would look for its files under that name.

### `jediacademy`

* Good, because it says what it is.
* Bad, because it contains a trademark term.
