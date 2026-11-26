---
status: accepted
date: 2026-11-26
decision-makers: Luka Dornhecker
---

# Game data location is run-time configuration; without it, the engine asks once and remembers

Supersedes [ADR-0007](0007-game-data-location.md).

## Context and Problem Statement

[ADR-0007](0007-game-data-location.md) made the game data's location a run-time
setting: `fs_basepath` on the engine's command line, and next to the executable when it
is not set. That serves developers and tests. A player who starts the game from the
macOS Finder, from a desktop entry or by a double click on `jasp.exe` outside the game's
folder has no command line, and next to the executable there is no `base/`. On macOS,
"next to the executable" is even inside the application bundle
([ADR-0024](0024-macos-application-bundle.md)). The game then stops with an error about
missing files. How does the engine find the data when nobody tells it?

## Decision Drivers

* All drivers of [ADR-0007](0007-game-data-location.md) hold: no path to game data in
  any tracked file, nothing derived from game data in the repository, tests runnable by
  anyone with a retail copy and skipped for anyone without one, a retail install never
  modified.
* A player must be able to start the game without a command line, on every platform.
* Tests, scripted runs and anything without a display must never wait on a dialog.
* Platform code stays in `src/sys`, and through SDL where SDL offers it.

## Considered Options

* Ask once with a folder dialog, and remember the answer in the per-user directory
* Look next to the application: beside the executable, and on macOS beside the bundle
* Search the usual install places of Steam, GOG and the Mac App Store
* Unchanged: the command line, else next to the executable

## Decision Outcome

Chosen option: "Ask once with a folder dialog, and remember the answer", because it
finds the data wherever the player installed it, without the project knowing any store's
paths, and asks only the first time.

* **Engine.** The directory that contains `base/` is the first of:
  1. `fs_basepath`, when set on the command line;
  2. the directory remembered in the per-user directory, if it still contains `base/`;
  3. the directory of the executable as SDL names it (`SDL_GetBasePath`), if it contains
     `base/`, as before;
  4. a directory the player chooses in the platform's folder dialog
     (`SDL_ShowOpenFolderDialog`). A choice without `base/` in it is refused, and the
     dialog asks again. The accepted choice is written to the per-user directory, to
     `gamedata.txt`, and used.
* **Cancelling** the dialog ends the engine with a message window that says the game's
  data is needed and how `fs_basepath` passes it.
* **No dialog without a player.** With `sys_headless`, or when no window can be opened,
  step 4 is left out, and the engine ends with the error it gives today. Tests always
  pass `fs_basepath`.
* **Forgetting** the choice is deleting `gamedata.txt`; a directory that no longer has
  `base/` is forgotten by itself and asked for again. `fs_basepath` on the command line
  wins without changing what is remembered.
* **Tooling.** Tests and run scripts read the environment variable `JKA_GAMEDATA_DIR`
  (the directory that contains `base/`) and pass it on as `fs_basepath`. A developer may
  set it in their shell or in `CMakeUserPresets.json`, which git ignores.
* **Tests** that need game data are reported as skipped when the variable is unset.
* **Writes.** The game data directory is treated as read-only. Configuration, saves,
  screenshots and `gamedata.txt` go to the per-user directory the platform layer
  provides.
* **Guard.** The repository check fails if a tracked file contains a home-directory or
  Steam library path.

### Consequences

* Good, because a player starts the game from the Finder, a desktop entry or a double
  click, on every platform, once the folder is chosen.
* Good, because the project knows no install paths of any store, and the repository is
  identical for everyone.
* Good, because developers and tests see no change.
* Bad, because the release never asked; the dialog is new behaviour, outside "port,
  don't improve", for a case the release did not have (it was installed into its data's
  folder).
* Bad, because the dialog's behaviour is the platform's and differs between them, and
  on Linux it depends on the desktop's portal. It is checked on Linux here, and on
  Windows and macOS only on real machines.
* Bad, because a player who moves the data is asked again, which is the intent, but
  may surprise.

### Confirmation

* The engine started without `fs_basepath` and with no `base/` beside it opens the
  dialog; a folder with `base/` is accepted, written to `gamedata.txt`, and used on the
  next start without a dialog. Checked on Linux with scripted runs; on Windows and
  macOS by a person, as part of M3's exit criteria.
* With `+set sys_headless 1` and no data, the engine ends with its error and opens
  nothing. The smoke tests keep passing `fs_basepath` and see no dialog.
* No tracked file contains a path to game data.

## Pros and Cons of the Options

### Ask once and remember

* Good, because it works for any install, on every platform, with one SDL call.
* Bad, because it is new behaviour, and a dialog to test on three platforms.

### Next to the application

* Good, because simple, and the rule players of the release know.
* Bad, because the player has to move the application to the data, and on macOS put the
  bundle into a folder with the game's files.

### Search the stores' install places

* Good, because nothing to choose when the game is found.
* Bad, because the project would carry the stores' paths, which change, in code, and
  the check that tracked files contain no such paths would need exceptions.
* Bad, because a copy installed elsewhere is not found, so a dialog is needed anyway.

### Unchanged

* Good, because nothing to build.
* Bad, because a player without a command line cannot start the game.

## More Information

* `SDL_ShowOpenFolderDialog` is asynchronous: its callback runs on a later pass of SDL's
  event loop, which the engine has to run until the choice is made.
* To be revisited if the project is packaged for a store that installs data and engine
  together.
