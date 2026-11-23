---
status: accepted
date: 2026-11-23
decision-makers: Luka Dornhecker
---

# Game data location is run-time configuration

## Context and Problem Statement

The game needs the retail data files (`assets0.pk3` to `assets3.pk3`) to run. They are
copyrighted, are not part of this project, and live wherever the user installed the
game. No such path may be hardcoded in the repository. The original
engine also writes configuration, saves and screenshots into the game directory. How do
the engine and the developer tooling find the data, and where do writes go?

## Decision Drivers

* No path to game data in any tracked file (a given, not up for decision here).
* Nothing derived from game data in the repository.
* Tests must be runnable by anyone with a retail copy, and must not fail for people
  without one.
* A Steam install must not be modified by running this project.

## Considered Options

* Run-time setting only: `fs_basepath` for the engine, an environment variable for tooling
* A CMake cache variable baked into test definitions
* Auto-detection of Steam and GOG installs
* A symlink or copy at a fixed place inside the working tree

## Decision Outcome

Chosen option: "Run-time setting only", because it keeps the location entirely outside
the build and the repository, and uses a mechanism the engine already has.

* **Engine.** The existing `fs_basepath` cvar names the directory that contains `base/`.
  It is set on the command line (`+set fs_basepath <dir>`). When it is not set, the
  engine looks next to the executable.
* **Tooling.** Tests and run scripts read the environment variable `JKA_GAMEDATA_DIR`
  (the directory that contains `base/`) and pass it on as `fs_basepath`. A developer may
  set it in their shell or in `CMakeUserPresets.json`, which git ignores.
* **Tests** that need game data are reported as skipped when the variable is unset.
* **Writes.** The game data directory is treated as read-only. Configuration, saves and
  screenshots go to a per-user directory provided by the platform layer.
* **Guard.** The repository check fails if a tracked file contains a home-directory or
  Steam library path.

### Consequences

* Good, because the repository is identical for every developer and for CI.
* Good, because running the project never alters a retail install.
* Bad, because a new developer must set one variable before data-dependent tests run.
* Bad, because the original behaviour of writing next to the data changes, so existing
  retail configuration files are read but not updated.

### Confirmation

The repository check passes. With `JKA_GAMEDATA_DIR` unset, the test suite passes with
data-dependent tests skipped. After a play test, the game data directory is unchanged.

## Pros and Cons of the Options

### Run-time setting only

* Good, because nothing about the location is ever stored.
* Neutral, because one environment variable to set.

### CMake cache variable

* Good, because set once per build directory.
* Bad, because the path ends up in generated build files and test definitions, and a
  reconfigure is needed to change it.

### Auto-detection

* Good, because convenient for users.
* Bad, because it is guesswork across three platforms and several stores, and it puts
  well-known install paths into the source. It can be added later as a convenience for
  players on top of the chosen mechanism.

### Symlink or copy in the working tree

* Good, because simple.
* Bad, because one wrong `git add` puts copyrighted data, or a personal path, into
  history.
