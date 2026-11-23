---
status: accepted
date: 2026-11-23
decision-makers: Luka Dornhecker
---

# The cgame interface is a pair of structs of typed function pointers

## Context and Problem Statement

The game module contains two halves. The server-side half talks to the engine through
`GetGameAPI`, which exchanges `game_import_t` and `game_export_t`, structs of typed
function pointers. The client-side half, cgame, still uses the Quake 3 virtual-machine
convention although it has never run in a virtual machine:

* cgame calls the engine through `int syscall(int arg, ...)`, about 120 call numbers
  (`cgameImport_t` in `cgame/cg_public.h`). The engine reads the arguments as an `int`
  array and turns them back into pointers with `#define VMA(x) ((void*)args[x])`
  (`client/cl_cgame.cpp`). Floats travel as their bit pattern in an `int`.
* The engine calls cgame through `vmMain(int command, int arg0, ...)`, 17 commands
  (`cgameExport_t` in `client/vmachine.h`); two of them pass a pointer as an `int`.

A pointer does not fit in an `int` on any supported target
([ADR-0003](0003-target-platforms.md)), and reading variadic arguments through the
address of the first parameter is not valid C++ on any of them. The design fixes the
direction (typed function pointers); the shape is open.

## Decision Drivers

* No pointer travels through an integer; the compiler checks every argument.
* Small diff against the original in the roughly 50k lines of cgame code.
* One loading mechanism for the module, in `src/sys`.
* A module built against a different interface must be rejected at load time, not crash
  later.

## Considered Options

* A second pair of structs, `cgame_import_t` and `cgame_export_t`, exchanged through a
  new entry point `GetCGameAPI`
* Extend `game_import_t` and `game_export_t` with the cgame functions
* Keep the variadic calls and widen the arguments to `intptr_t`

## Decision Outcome

Chosen option: "a second pair of structs exchanged through `GetCGameAPI`", because it
removes the untyped path completely, mirrors the interface the server-side half already
has, and keeps the two halves as separate as they are today.

* `cgame/cg_public.h` declares `cgame_import_t` (engine functions cgame may call, one
  member per former `cgameImport_t` number, with the real parameter types) and
  `cgame_export_t` (one member per former `cgameExport_t` command), and
  `CGAME_API_VERSION`.
* The module exports `cgame_export_t *GetCGameAPI( int apiVersion, cgame_import_t *import )`.
  It returns null when the version does not match, and the engine reports that as an
  error. `dllEntry` and `vmMain` are removed.
* Inside cgame, the `cgi_*` functions of `cg_syscalls.cpp` stay, with their signatures,
  and call through the import struct. No cgame call site changes.
* Inside the engine, the `switch` in `CL_CgameSystemCalls` becomes the functions that
  fill the import struct; `VM_Call( CG_X, ... )` call sites become calls through the
  export struct. `vmachine.cpp` and `vmachine.h` are removed with the `vm_t` type.
* The leftover virtual-machine plumbing of the menu code (`dllEntry`, `syscall` and
  `vmMain` in `ui/`, `CL_UISystemCalls`, `uivm`) is removed where it is dead: the menus
  are linked into the engine and already call it directly.
* Platform code loads the module once and resolves both entry points
  ([design, section 4.5](../design.md#45-platform-layer)).

### Consequences

* Good, because a mismatch between the two sides is a compile error, and a stale module
  is refused when it is loaded.
* Good, because float arguments are passed as floats; the bit-pattern helpers go away.
* Good, because cgame sources other than `cg_syscalls.cpp`, `cg_main.cpp` and
  `cg_public.h` are not touched by this change.
* Bad, because the retail game module and existing mods cannot be loaded. That is
  already ruled out by the 64-bit target ([ADR-0003](0003-target-platforms.md)).
* Bad, because the `cgi_*` wrappers are a second layer that does nothing but forward;
  they are kept for the small diff and can be dropped later without changing the
  interface.

### Confirmation

No variadic function and no cast between pointer and integer remains in
`cl_cgame.cpp` or `cg_syscalls.cpp`. The headless smoke test loads the module, which
runs the version check, initialises cgame and draws frames through the new interface.

## Pros and Cons of the Options

### A second pair of structs through `GetCGameAPI`

* Good, because it is the pattern the code base already uses and readers already know.
* Good, because the server-side interface is left as it is.
* Neutral, because the module has two entry points, as before.

### Extend the game structs

* Good, because one entry point and one version number.
* Bad, because it merges two interfaces that the engine initialises and shuts down at
  different times (server start and client connect), and it makes a later separation of
  the halves harder.
* Bad, because `game_import_t` grows from about 100 to about 220 members.

### Keep the variadic calls with `intptr_t`

* Good, because smallest change; this is what other ports of the engine family did.
* Bad, because nothing is checked: a wrong argument count or type still compiles, floats
  still travel as bit patterns, and the call into the engine still reads its arguments
  from an array that the language does not guarantee to exist.

## More Information

The member names follow `game_import_t`: the engine function's name without a prefix
(`Printf`, `Cvar_Register`, `R_RegisterModel`, `S_StartSound`).
