---
status: proposed
date: 2026-11-24
decision-makers: Luka Dornhecker
---

# The window is titled "Jedi Academy" and has no icon of its own until the platforms are packaged

## Context and Problem Statement

The release's window had the title "Jedi Knight®: Jedi Academy" and the icon of its
executable, `starwars.ico` (`raven-2013:code/win32/starwars.ico`): 16 and 32 pixels, the
word "JEDI" in red under a blue light. The SDL window of the port is titled "Jedi
Academy" and has no icon, so the window system shows its default. The roadmap of M2
lists the icon and the title among what the Windows code did for the player and
`src/sys` does not yet do ([design document](../design.md), section 4.5). What does
the window call itself, and which icon does it show?

## Decision Drivers

* "Star Wars" and "Jedi Knight" are trademarks of their owners, and this project must
  not suggest an official product ([design document](../design.md), section 11;
  [ADR-0010](0010-project-and-binary-names.md)).
* No binaries in the repository; the release's icon is one, and it is the game's
  artwork.
* A player should recognise the game among other windows and in the task switcher.
* Where a window's icon comes from differs per platform. Windows takes the taskbar icon
  from the window and the Explorer icon from a resource of the executable. macOS takes
  it from the application bundle. On Wayland, where the port is played, most compositors
  take it from the desktop entry that matches the program's identifier, which is
  installed alongside it, and ignore what a program sets at run time. Only compositors
  that support the toplevel icon protocol use the latter.
* Packaging (an application bundle, an executable with resources, a desktop entry) is
  M3 work.

## Considered Options

* "Jedi Academy", no icon of its own until the platforms are packaged
* As the release: "Jedi Knight®: Jedi Academy" and the release's icon
* The project's name, `jasp`, and no icon of its own
* "Jedi Academy", and the release's icon read at run time from the retail `jasp.exe`

## Decision Outcome

Chosen option: "'Jedi Academy', no icon of its own until the platforms are packaged",
because it names the game the player's own data makes up without the registered
title and mark of the official product, puts nothing into the repository that is not
the project's, and leaves the icon to the work that can make it count on every
platform.

* The title of the window, and of the window that shows a fatal error, is "Jedi
  Academy". That is what it is today; the decision records it.
* The window sets no icon. The window system shows its default.
* The icon is decided again together with the packaging of M3, as part of the macOS
  application bundle, the Windows executable and a Linux desktop entry, all of which
  need one. That decision chooses between a new icon made for the project, and the
  icon of the player's retail copy installed by a tool.

### Consequences

* Good, because nothing in the window claims to be the official product.
* Good, because the repository gains no artwork and no binary.
* Good, because it is no work in M2.
* Bad, because the window shows a generic icon until M3, where the release showed its
  own.
* Bad, because the title differs from the release's.

### Confirmation

* The title is the constant `sysWindowTitle` in `src/sys/sys_local.h`, used by the
  window and by the error window.
* No call to `SDL_SetWindowIcon` exists.
* The M3 roadmap names the icon as part of packaging.

## Pros and Cons of the Options

### "Jedi Academy", no icon of its own until packaging

* Good, because the title says which game runs, as the name of a file or a folder
  would, without the registered title and its mark.
* Good, because the icon is settled once for all three platforms, where it takes
  effect.
* Bad, because of the generic icon until then.

### As the release

* Good, because it is what the release did.
* Bad, because a window with the official title, its ® and the game's own artwork is
  the official product's window, which an unofficial build must not appear to be.
* Bad, because the icon would have to be committed, as a binary or as its pixels
  written out in source, which is a binary in another form.
* Bad, because on most Wayland compositors an icon set at run time is not shown at
  all.

### The project's name, `jasp`

* Good, because no trademark appears anywhere.
* Bad, because the name says nothing to a player looking for the game among windows.
  It is the executable's name, not a title.

### "Jedi Academy", icon read from the retail `jasp.exe`

* Good, because the icon is the release's own and comes from the player's copy, like
  the rest of the game data.
* Bad, because it needs a reader for the resources of Windows executables, for an icon
  that most Wayland compositors would not show.
* Bad, because the executable is not part of the game data the engine reads
  ([ADR-0007](0007-game-data-location.md)): it lies beside `base/` in copies for
  Windows, and the macOS release has none.

## More Information

* `raven-2013:code/win32/win_local.h`, `WINDOW_CLASS_NAME`;
  `raven-2013:code/win32/game.rc`.
* To be revisited with the packaging of M3.
