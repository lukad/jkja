# Design

Status: basis for work. Decisions 0000 to 0017 accepted on 2026-11-23.
This document is the basis for all work in this repository. It describes where the code
starts, where it is going, and in what order. Decisions that shape it are recorded as ADRs
in [decisions/](decisions/README.md); where this document and an accepted ADR disagree,
the ADR wins and this document gets fixed.

## 1. Purpose

Take Raven Software's 2013 source release of *Star Wars Jedi Knight: Jedi Academy* and turn
it into a codebase that

1. builds with current toolchains on Linux, macOS and Windows, and
2. renders through a new renderer with a modern rasterised pipeline: HDR, dynamic shadows
   and physically based shading.

### Goals

- A playable single-player game on all three platforms, built from this repository plus a
  retail copy of the game data.
- One build system, one way to build, no IDE project files, no binary blobs.
- A renderer that can reproduce the original look and, on top of that, offers HDR,
  shadowed dynamic lights and PBR materials.
- A codebase a newcomer can read: clear module boundaries, platform code in one place,
  dead code deleted rather than disabled.

### Non-goals

- Ray tracing or path tracing ([ADR-0008](decisions/0008-renderer-feature-scope.md)).
- Compatibility with retail save files ([ADR-0006](decisions/0006-drop-retail-savegame-compatibility.md)).
- Binary compatibility with retail game modules or 32-bit mods.
- Console platforms (the Xbox and GameCube code paths are removed).
- 32-bit builds ([ADR-0003](decisions/0003-target-platforms.md)).
- The original content tools (ModView and friends, which are MFC applications).
- Multiplayer, until single-player is done ([ADR-0002](decisions/0002-single-player-first.md)).

## 2. Starting point

Measured on the import commit `44e8ea1`.

| Fact | Value |
|---|---|
| Trees | `code/` single-player, about 543k lines; `codemp/` multiplayer, about 640k lines. Forks of each other, diverged. |
| Build files | Visual Studio 2003 `.vcproj` only. `code/unix/Makefile` is a Quake 3 leftover from 1998. |
| SP binaries | `jasp.exe` (engine) and `jagamex86.dll` (game + cgame + ICARUS), per `starwars.vcproj` and `game/game.vcproj`. |
| Completeness | All sources referenced by the SP PC projects exist. About 40 references differ only by letter case. |
| Line endings | 1416 of 1421 source files are CRLF. |
| Includes | 150 `#include` lines use backslashes; many use the wrong letter case. |
| Console code | About 2,800 lines mention `_XBOX` or `_GAMECUBE`; whole files are console-only (`*_console.*`, `*_xbox.*`, `*_dx8.*`). |
| Proprietary pieces | SmartHeap, Immersion force feedback (`code/ff`, `_IMMERSION`), EAX, Bink, OpenAL binaries. |
| Bundled libraries | jpeg-6 (1995), zlib (1998, plus an x86-assembly variant), libpng, Xing MP3 decoder. |
| Inline assembly | 13 files in the SP tree. |
| Platform code | `code/win32`, 35.9k lines, of which about 7k are an OpenGL-on-DirectX-8 shim for Xbox. |
| Licence | No licence file in the tree. See [section 11](#11-legal). |

After M0 the working tree is `src/`: 518 source files, about 445k lines. The paths below
refer to the import commit.

Things that matter for the port:

- **cgame calls are untyped.** The engine and the cgame half of the game module talk
  through `int syscall(int arg, ...)`, and the engine turns arguments back into pointers
  with `#define VMA(x) ((void*)args[x])` (`code/client/cl_cgame.cpp:433`). That cannot
  work when pointers are 64 bits wide.
- **The server-side game interface is typed.** `GetGameAPI` exchanges structs of function
  pointers (`game_import_t` / `game_export_t`). That part survives 64-bit as it is.
- **Savegames are memory dumps.** 65 call sites write and read structs with `sizeof`,
  patching pointers through field tables (`code/game/g_savegame.cpp`).
- **`long` is used 667 times** outside third-party and platform code. It is 64 bits on
  Linux and macOS and 32 bits on Windows.
- **The default sound path is the software mixer.** `s_UseOpenAL` defaults to `0`
  (`code/client/snd_dma.cpp:475`); OpenAL with EAX is the optional path.
- **The renderer has an interface, but it leaks.** See [section 6](#6-renderer).
- **The single-player tree is, in places, the console port.** The release's `code/`
  tree does not hold the PC game as shipped: in a number of files the Xbox version
  replaced the PC code instead of sitting beside it behind `_XBOX`. Found in M1:
  the renderer's image code read only DDS and uploaded through a call of the console's
  OpenGL shim; the memory tag list was the trimmed console one; mouse input was the
  thumbstick code; game, cgame and menus called engine functions and each other's
  variables directly, which only works in the console's single statically linked
  executable; the savegame writer signed and stored saves the console way; the menus
  carried the console's storage and controller dialogs. The PC projects of the release
  therefore never built as they stand. Where the PC needs the overwritten behaviour, it
  is restored from the multiplayer tree of the import commit (`codemp/`, where the two
  variants are mostly still separated), or from commented-out PC lines next to the
  console code; console-only features are removed. Where both trees hold the console
  version and the PC code is in neither, it comes from Raven's Jedi Outcast release
  ([ADR-0017](decisions/0017-missing-pc-code-from-jedi-outcast.md)). Taken from there
  so far:

  | What | Where | Decision |
  |---|---|---|
  | The menus' mouse handling: `Item_CorrectedTextRect`, `Menu_HandleMouseMove`, `Menus_HandleOOBClick`, `Item_Slider_HandleKey`, the mouse and edit-field parts of `Menu_HandleKey` | `src/ui/ui_shared.cpp` | [ADR-0016](decisions/0016-menu-mouse-handling.md) |
  | The loading bar: `CG_LoadBar` and the registration of its three images | `src/cgame/cg_info.cpp`, `src/cgame/cg_main.cpp` | ADR-0017 | More such places may surface when the game
  first runs with a window (M2).

## 3. Principles

1. **One way to do each thing.** One build system, one platform layer, one renderer at
   the end.
2. **Delete, don't disable.** Out-of-scope code is removed from the tree. The original is
   always available at the import commit.
3. **Mechanical changes are separate from thinking changes.** A scripted rename or a
   line-ending conversion is its own commit, with the command in the commit message, so
   it can be reviewed by re-running it.
4. **Platform differences live in `src/sys`.** No new `#ifdef _WIN32` anywhere else.
5. **Behaviour stays the same unless a decision says otherwise.** Gameplay code is ported,
   not improved.
6. **Nothing from the retail game goes into the repository.** Not the data, not paths to
   it, not screenshots made from it ([ADR-0007](decisions/0007-game-data-location.md)).
7. **Every architectural choice has an ADR before it has code.**

## 4. Target structure

### 4.1 Repository layout

```
CMakeLists.txt
CMakePresets.json
cmake/                 build helpers
docs/
  design.md            this document
  decisions/           ADRs
external/              single-header libraries, unmodified
shaders/               shader sources for the new renderer
src/
  qcommon/             common, cvars, commands, filesystem, collision, memory
  client/              client, input mapping, cinematics, sound mixer
  server/              server
  ui/                  menu system
  ghoul2/              Ghoul2 model data, skeletons, bolts, collision
  rmg/                 random mission generator
  renderer_legacy/     original OpenGL renderer (transitional)
  renderer/            new renderer
  sys/                 platform layer (the only place that knows the OS)
  game/                server-side game logic         } built into
  cgame/               client-side game logic         } the game
  icarus/              scripting                      } module
  ratl/ ravl/ ragl/ rufl/   Raven template libraries
tests/
tools/                 repository tools, built with the project
```

The Quake 3 module names are kept because anyone who knows the engine family knows them.
Directory names are lower case. `codemp/`, `tools/`, `bin/`, `base/`, `ui/` (top level)
and all console and Mac Classic directories are removed from the working tree in
milestone M0. The bundled libraries stay until they are replaced in M1, and the Win32
platform code stays in `src/win32` until `src/sys` replaces it in M2.

### 4.2 Binaries

Two binaries, as shipped:

- the **engine executable**, and
- the **game module**, a shared library loaded at start-up, containing game, cgame and
  ICARUS.

They stay separate because both compile the shared `q_shared`/`q_math` sources with
different settings and define functions of the same name. The engine is `jasp`; the module
is `jagame` plus the architecture (`jagamex86_64`, `jagamearm64`), derived at build time
instead of being hardcoded to `jagamex86.dll`
([ADR-0010](decisions/0010-project-and-binary-names.md)).

### 4.3 Build

CMake with presets and Ninja; C++20 for all C++ code
([ADR-0004](decisions/0004-build-system-and-toolchain.md)). One preset per platform and
configuration, plus a Clang preset and a sanitizer preset on Linux; `cmake --list-presets`
shows the ones usable on the current machine. Visual Studio users open the folder; no
`.sln` or `.vcxproj` is ever generated into or committed to the tree.

Warnings: new code (`src/sys`, `src/renderer`, tests) builds with a strict warning set
treated as errors. Legacy code builds with a smaller set where the dangerous classes
(pointer truncation, missing return, format mismatches) are errors and the rest is
tightened over time.

### 4.4 Dependencies

Decided in [ADR-0012](decisions/0012-dependencies.md):

| Need | In the release | Now | Where it comes from |
|---|---|---|---|
| Window, input, timers, threads, library loading, audio device, GPU | `src/win32` | SDL3 ([ADR-0005](decisions/0005-platform-layer-sdl3.md)) | downloaded and built by the project |
| Inflate for `.pk3`, CRC-32 | Raven's rewrite of zlib 1.1.3, with assembly | miniz | downloaded and built by the project |
| JPEG and PNG decode; screenshot encode | jpeg-6, Raven's PNG code | stb_image, stb_image_write | committed, `external/stb/` |
| TGA decode | the renderer's own loader | unchanged | in the tree |
| MP3 decode | Xing decoder (portable C, GPL) | unchanged; replaced only if it causes trouble | in the tree |
| Unit tests | none | Catch2 | downloaded and built by the project |
| Memory debugging | SmartHeap | AddressSanitizer | compiler |
| Force feedback | Immersion | removed; SDL haptics if wanted later | |

Nothing is taken from the system: the build never searches for an installed library.
A dependency is either a release archive pinned by URL and hash, downloaded at configure
time and compiled as part of the project, or a single-header library committed under
`external/`. No other library source is copied into the tree.

### 4.5 Platform layer

`src/sys` replaces `src/win32`. The surface the
engine needs, after console and proprietary functions are dropped, is small:

| Group | Functions (today's names) | Backed by |
|---|---|---|
| Process | `Sys_Init`, `Sys_Quit`, `Sys_Error`, `Sys_Print`, `Sys_Milliseconds`, `Sys_GetEvent`, `Sys_QueEvent` | SDL3 events and timers |
| Files | `Sys_ListFiles`, `Sys_FreeFileList`, `Sys_Mkdir`, `Sys_Cwd`, `Sys_DefaultBasePath`, `Sys_DefaultHomePath` | C++ `<filesystem>`, SDL3 paths |
| Modules | `Sys_GetGameFunction`, `Sys_UnloadGame` | SDL3 shared-object loading |
| Window | `GLimp_Init`, `GLimp_Shutdown`, `GLimp_EndFrame`, `GLimp_SetGamma`, the `qgl*` function pointers | SDL3 window; a GL context for the legacy renderer |
| Input | `IN_Init`, `IN_Shutdown`, `IN_Frame` | SDL3 keyboard, mouse, gamepad |
| Sound device | `SNDDMA_Init`, `SNDDMA_Shutdown`, `SNDDMA_GetDMAPos`, `SNDDMA_BeginPainting`, `SNDDMA_Submit` | SDL3 audio stream |
| Misc | `Sys_GetClipboardData`, system console | SDL3 clipboard; stdin/stdout |

CD checks (`Sys_CheckCD`), CD paths, streaming-file helpers for consoles, file codes and
the rumble scripting layer are removed.

As of M2 the layer provides all of these groups through SDL3 and the standard library,
gamepads excepted, and keeps the null variants of window, input and sound, which are
chosen at start-up with `+set sys_headless 1`. Points that differ from the Windows
code of the release:

- A full-screen window takes the size of the desktop; the display mode is not changed,
  and `r_mode` sizes a window on the desktop only. The menu's list of resolutions, which
  is part of the game data and ends at 2048 by 1536, therefore has no effect in full
  screen. Menus and HUD are laid out for 4:3 and are stretched to the shape of the
  display, as the release stretches them in a wide custom mode.
- SDL3 offers no gamma ramp. The renderer therefore takes the path it has for devices
  without one: textures are brightened when they are loaded and there are no overbright
  bits, so the picture is darker in the highlights than the release's on hardware with
  gamma control. Whether and how that is made up for is open.
- The console reads lines from standard input on a thread of its own.
- The game module is unloaded at the top of the main loop, not at the moment the engine
  lets go of it, because an error can be on its way up a stack that has functions of
  the module on it.
- `sys_key`, `sys_char` and `sys_mousemove` queue the events a keyboard or mouse would
  have produced. Tests use them to operate the menus and the game without a person.

The null sound device is a device, not the absence of one: the mixer is given a buffer
and a play position that follows the clock, and what it mixes is discarded. Without a
device the engine loads no sounds, and a debug build then refuses maps with ambient
sound sets; with the null device a headless run loads, decodes and mixes sounds as a
run with a device does.

A **null** variant of window, input and sound exists from the first day, so the engine
can start, mount the game data, load a map and quit without a display. That is the
backbone of automated testing.

Until M4 there is no null renderer: game logic needs the model loading and the Ghoul2
skeleton code that still live inside the legacy renderer. The null window therefore
gives the legacy renderer an OpenGL binding whose functions do nothing. Maps and models
load for real, the front end runs, and nothing is drawn.

### 4.6 Filesystem and game data

- The engine finds the game data through the existing `fs_basepath` cvar, which names the
  directory that contains `base/`. It is given at run time only.
- The game data directory is treated as **read-only**. Configuration, saves and
  screenshots go to the directory named by the cvar `fs_homepath`, which defaults to a
  per-user directory given by the platform layer and is searched before the game data,
  so the engine never writes into a Steam install.
- Developer tooling (tests, run scripts) reads the location from the environment variable
  `JKA_GAMEDATA_DIR`. It is never written into a tracked file. Tests that need game data
  are skipped, not failed, when it is unset.

Details in [ADR-0007](decisions/0007-game-data-location.md).

### 4.7 Audio

The engine's software mixer is kept and feeds an SDL3 audio stream; the OpenAL and EAX
path is removed ([ADR-0014](decisions/0014-audio-software-mixer-only.md)). The software
mixer is the retail default, so this loses only the optional EAX reverb. The removal
happens in M1, because that path cannot compile without the proprietary EAX headers;
the SDL3 audio stream follows in M2.

The mixer runs on the main thread, once a frame, and the engine empties the sound
buffer before every file it reads ("stop sounds from repeating", `FS_ReadFile`), so
that a slow load does not loop the last fraction of a second. The sound therefore drops
out briefly whenever something is loaded while the game runs, for example the models of
the character menu. That is the release's code and behaviour, kept as it is.

### 4.8 64-bit and portability rules

Targets are 64-bit only: x86-64 and arm64.

- **Typed module interface.** The variadic cgame syscalls are replaced by a struct of
  typed function pointers, the same pattern the server-side game interface already uses.
  This removes the pointer-through-`int` path completely
  ([ADR-0013](decisions/0013-typed-cgame-interface.md)).
- **Fixed-width integers at boundaries.** Anything read from or written to a file uses
  `int32_t`-style types. `long` is removed from non-third-party code.
- **Pointer/integer casts.** Compilers reject truncating pointer casts in C++; every such
  error is fixed by changing the type, never by a wider cast that hides it.
- **No assembly.** The 13 files with inline x86 assembly get portable C++.
- **Alignment and aliasing.** File formats are read through `memcpy` or packed accessors
  where the original cast pointers into unaligned data. arm64 tolerates unaligned loads
  but the sanitizer builds do not, and they are the gate.
- **Speed.** The levels rely on timing that every machine of the time produced. One
  such place is known: a frame that loads files while a level comes up always exceeded
  the engine's clamp of 200 ms, so the player entered a level a fixed number of server
  frames after its start, and level scripts count on the characters spawned in those
  frames. Such frames are counted as 200 ms whatever they took. The optimised build
  shows this kind of defect and the debug build hides it, so a fix is checked in the
  `linux-release` preset as well.
- **Byte order.** All targets are little-endian. The byte-swap macros stay as no-ops at
  the file-format boundary; no big-endian work is done.

### 4.9 Savegames

Retail compatibility is dropped ([ADR-0006](decisions/0006-drop-retail-savegame-compatibility.md)).
Every saved structure is written field by field from a table that names all of its
members, in fixed-width encodings, and the tables a file was written with are stored in
the file, so a save does not depend on the build that wrote it and survives members
being added to or removed from a structure
([ADR-0015](decisions/0015-savegame-format.md)). The chunk structure is kept and the
existing pointer tables become part of the new ones, so a retail-format reader could be
added later.

Two limits are known. The test that compares a table with its structure cannot see a
missing member that is small enough to fit where alignment would have left padding
anyway. And a few chunks that are not structures (timer names, script variable names,
ICARUS state) are still read into buffers of a fixed size with the length the file
gives; a damaged file is caught by its checksum only after the copy.

## 5. Order of work

Single-player goes first, with the original renderer kept alive as a reference:

1. Make the tree clean (M0).
2. Make it compile, 64-bit, without a display (M1).
3. Make it run on Linux with the **original** renderer (M2).
4. Make it run on Windows and macOS (M3).
5. Put the renderer behind a hard boundary (M4).
6. Build the new renderer to parity next to the old one (M5).
7. Add the modern pipeline (M6, M7).
8. Decide what to do about multiplayer (M8).

The original renderer is brought up first because it turns "does the port work?" and
"does the new renderer work?" into two separate questions with two separate answers.
It is deleted once the new renderer reaches parity.

A 32-bit intermediate build was considered and dropped: it needs 32-bit SDL3 on the
development machine, and the retail `jasp.exe` under Wine already serves as the
behavioural reference.

## 6. Renderer

### 6.1 What exists

The client talks to the renderer through `refexport_t` (`src/renderer/tr_public.h`),
about 50 function pointers. Inside, the renderer follows the Quake 3 design:

```
client -> re.* -> front end                       -> command list -> back end
                  RE_RenderScene                                     RB_ExecuteRenderCommands
                  R_RenderView                                       RB_RenderDrawSurfList
                  R_AddWorldSurfaces (BSP, PVS)                      RB_BeginSurface
                  R_AddEntitySurfaces (MD3, Ghoul2)                  ...fill tess arrays...
                  R_AddDrawSurf, R_SortDrawSurfs                     RB_StageIteratorGeneric
```

The front end decides *what* is visible and in which order. The back end draws it one
shader stage at a time with fixed-function OpenGL: about 840 `qgl*` call sites, immediate
mode, the matrix stack, NV register combiners and ARB assembly programs for the glow
effect.

Content it must handle:

- Raven BSP (`RBSP` version 1) with up to four lightmap styles per surface, vertex
  colours and a light grid for entities.
- Quake 3 shader scripts, extended by Raven: about 110 keywords, including `glow`,
  `surfaceSprites`, light styles and material hints.
- Surface types: world faces, curved patches, triangle soups, MD3 meshes, Ghoul2
  skinned models, terrain, flares, sprites and beams, client-submitted polygons.
- Effects: dynamic glow, surface sprites (grass, foliage), weather, marks, projected and
  stencil shadows, fog volumes, sky, portals, screen dissolves, the binocular views.
- 2D: menus, HUD, fonts including Asian glyph sets, cinematic frames, save-game
  thumbnails.

### 6.2 The boundary (M4)

The interface exists but is not respected:

- `tr_local.h`, the renderer's private header, is included by the precompiled header
  `src/server/exe_headers.h` and therefore by the whole engine.
- **Ghoul2 is split across the boundary the wrong way.** The skeleton, bone-cache and
  bolt code that game logic, collision and ragdoll depend on lives in
  `renderer/tr_ghoul2.cpp` (4,217 lines) next to the drawing code, and `ghoul2/` reaches
  back into renderer internals (`R_GetModelByHandle`, `model_t`, the `mdxm`/`mdxa`
  headers).

M4 fixes this: `tr_public.h` and `tr_types.h` become the only headers the engine sees;
Ghoul2 model data, skeleton evaluation and bolts move to `src/ghoul2` and the renderer
keeps only skinned-surface drawing. The direct OpenGL calls the engine made outside the
renderer were console code and went away in M0; the repository check keeps it that way
from M4. The renderer becomes a library selected at start-up, and a null renderer is added for tests.

### 6.3 The new renderer

It implements the same `refexport_t`, so the client does not change.

**Keep** the front end's ideas: PVS and frustum culling, the scene API, shader-script
parsing, sort order. JKA scenes are small by today's standards; none of this is a
bottleneck.

**Replace** the back end and the data that feeds it:

| Area | Original | New |
|---|---|---|
| World geometry | copied into CPU arrays every frame | uploaded once at map load; drawn as index ranges batched by material |
| Dynamic geometry (effects, sprites, marks, 2D) | CPU arrays | per-frame streaming buffers |
| Ghoul2 models | CPU skinning | GPU skinning from the existing bone cache |
| MD3 models | CPU frame interpolation | interpolation in the vertex shader |
| Shader scripts | interpreted per stage, one pass per stage | compiled at load into material descriptions that select generated shader variants; stages collapsed into one pass where blending allows |
| Render state | global GL state machine | pipeline objects cached by state key |
| Glow | separate scene pass plus blur | bloom from the HDR target, driven by the same `glow` stages |
| Frame | straight to the back buffer | fixed pass sequence: shadows, opaque, sky, translucent, post-processing, 2D |

The new renderer starts as a copy of the legacy front end and diverges. The two do not
share code beyond what M4 moves out of the renderer; the duplication is temporary and
ends when the legacy renderer is deleted.

GPU calls are confined to the back-end directory of the new renderer. Nothing outside
it includes a graphics API header.

### 6.4 Graphics API

The SDL3 GPU API ([ADR-0009](decisions/0009-graphics-api.md)). It runs on
Vulkan on Linux, Metal on macOS, and Direct3D 12 or Vulkan on Windows, and it comes with
the platform library the project already uses. Its feature set covers everything in
[section 6.5](#65-feature-stages); its stated limit, no bleeding-edge features such as
ray tracing, matches the scope. Direct Vulkan is the fallback.

The decision is confirmed by a spike at the start of M5, with pass criteria written in
the ADR.

### 6.5 Feature stages

**Parity (M5).** The new renderer produces the original image. It runs in *legacy colour
mode*: 8-bit targets and gamma-space blending, exactly as the original, because that is
the only way to compare images meaningfully.

**HDR pipeline (M6).** Linear-light rendering into a floating-point target, tone
mapping, bloom replacing the glow passes, anti-aliasing, 2D drawn after tone mapping so
menus and HUD are unchanged. Dynamic lights become per-pixel and cast shadows.

**PBR (M7).** Physically based shading for surfaces that have material data, image-based
lighting from the sky and captured probes, and an override system for high-resolution
material sets.

Modern features are options. The original look must stay reachable by configuration.

### 6.6 What the original assets allow

The retail data was authored for a 2003 pipeline. This constrains every modern feature
and is the main design problem of M6 and M7.

- **Lighting is baked.** Static light, including sunlight, is in the lightmaps and
  vertex colours. Adding a physically correct sun on top would count it twice.
  Approach: baked light remains the diffuse base. Dynamic objects cast sun shadows that
  *attenuate* the baked light (subtractive mixed lighting), using the sun direction and
  colour the sky shader already provides (`tr.sunDirection`, `tr.sunLight`).
- **Dynamic lights are few and simple.** At most 32 per scene, projected as additive
  blobs. They become real lights: clustered, per-pixel, with shadow maps for the most
  important ones. Lightsabers are the showcase.
- **Blending was authored in gamma space.** Additive and multiplicative effects look
  different when blended in linear light. This is the largest risk to visual fidelity
  in M6 and needs per-category handling and visual sign-off, not a global switch.
- **There is no material data.** Textures are diffuse only. Defaults are derived
  (non-metallic, roughness from the surface's material class where the shader script
  gives one); real PBR needs replacement material sets supplied through overrides.
- **Specular light needs a direction.** For baked light it comes from the light grid,
  which stores ambient colour, directed colour and direction, plus image-based lighting.

## 7. Verification

| Layer | What | When |
|---|---|---|
| Repo checks | Includes of repository headers are quoted, relative to the including file, with exact case and forward slashes; no tracked file contains a machine-specific path; no binaries or project files; LF line endings | every change |
| Build | All presets the machine can build, warnings as configured | every change |
| Sanitizers | AddressSanitizer and UBSan preset on Linux | every change from M1 |
| Unit tests | New code with logic worth testing (serialisation, shader-script compiler, math replacements for assembly) | with the code |
| Headless smoke test | Start with null window and sound and the renderer bound to a no-op OpenGL, mount game data, load a map, run N frames, quit cleanly | from M1, needs game data |
| Render smoke test | The same with a real OpenGL context on a surface nobody sees (SDL's offscreen video driver) and the sound sent to SDL's dummy device, so that texture upload and drawing run for real | from M2, needs game data and such a context |
| Scripted input | `sys_key`, `sys_char` and `sys_mousemove` on the command line operate menus and game; with `screenshot` the result can be looked at | on demand, needs game data |
| Play test | A fixed checklist on the opening level: movement, combat, cutscene, save, load. Played with the `linux-release` preset: the retail game is a build without assertions, and the debug presets stop on assertions of the release that the game's own data trips | milestone exits |
| Image comparison | Screenshots at fixed camera positions on a fixed set of maps, new renderer against legacy renderer, perceptual difference under a threshold | M4 onwards |
| Reference | Retail `jasp.exe` under Wine, for "what did the original do?" | on demand |

Reference images are made from retail data and are therefore never committed; they are
generated into the build directory.

Continuous integration builds all three platforms and runs everything that does not need
game data. Tests that need it run on a developer machine.

What cannot be verified from the Linux development machine, and must be said so plainly
when it applies: run-time behaviour on macOS and Windows.

## 8. Roadmap

Each milestone ends when its exit criteria hold. Boxes are ticked as work lands.

### M0. Foundation

- [x] Tag the import commit `raven-2013`
- [x] Add licence text and third-party notices ([section 11](#11-legal))
- [x] Remove out-of-scope trees: `codemp/`, `tools/`, `bin/`, `base/`, top-level `ui/`
- [x] Remove binary blobs, CVS and SourceSafe metadata, project files, duplicate project copies
- [x] Strip conditional blocks mechanically: console (`_XBOX`, `_GAMECUBE`, `VV_LIGHTING`, `XBOX_DEMO`), force feedback (`_IMMERSION`, `_FF`), multiplayer (`_JK2`, `_JK2MP`), SmartHeap (`MEM_DEBUG`)
- [x] Remove console-only files and every other source the PC single-player projects never built
- [x] Remove `mac/`, `unix/`, `null/`, `ff/`, `smartheap/`, `SHDebug/`, `x_*`, `goblib/`, `bspthing/`
- [x] Convert line endings to LF; add `.gitattributes`, `.gitignore`, `.editorconfig`, `.clang-format`
- [x] Move `code/` to `src/` with lower-case directories; fix every `#include` to exact case and forward slashes
- [x] Add the repo check script
- [x] CMake skeleton with presets

Exit: the tree contains only in-scope single-player sources; the repo checks pass.

### M1. Compiles

- [x] Decide and record: dependencies, game module interface, audio
- [x] Engine and game module targets in CMake
- [x] Remove precompiled-header assumptions; every file includes what it uses
- [x] Fix compiler-dialect errors (MSVC keywords, `for` scope, non-standard library calls, pre-standard headers)
- [x] Replace bundled jpeg, png and zlib
- [x] Restore the PC texture loading and upload path of the legacy renderer
- [x] Remove the OpenAL and EAX sound path
- [x] Replace inline assembly
- [x] Replace the variadic cgame syscalls with a typed interface
- [x] Fix pointer-size compile errors; remove `long`
- [x] Null platform layer; legacy renderer bound to a no-op OpenGL when there is no window
- [x] Headless smoke test
- [x] Fix the run-time defects that keep the smoke test from passing under the sanitizers

Exit: GCC and Clang build engine and game module for Linux x86-64; the headless smoke
test mounts the game data, loads a map and quits cleanly under the sanitizers.

The exit criteria hold as of 2026-11-23. What M1 leaves unverified: everything that
needs a window or a sound device. The restored PC texture path compiles and runs
against the no-op OpenGL, which says nothing about what it draws; the smoke test covers
one map, `yavin1b`, for 120 frames.

### M2. Runs on Linux

- [x] Decide and record: savegame format
- [x] Decide and record: where the menus' mouse handling comes from. It is missing from
      both trees of the release: `Menu_HandleMouseMove` has only its console body, and
      mouse clicks on edit fields and slider bars are not handled
- [ ] Menu mouse handling from the Jedi Outcast release
      ([ADR-0016](decisions/0016-menu-mouse-handling.md))
- [x] SDL3 window, input and events
- [ ] System console input and the clipboard
- [x] Legacy renderer running in an SDL3 OpenGL context
- [ ] Sound and music
- [x] Cinematics
- [x] Per-user write directory; read-only game data
- [x] Savegames, new format ([ADR-0015](decisions/0015-savegame-format.md))
- [ ] Savegame leftovers of the console version: the picture stored with a save is
      commented out, and respawning loads a save named "Checkpoint" that nothing writes
- [ ] Gamepads
- [ ] Decide and record: brightness without a gamma ramp (section 4.5)
- [ ] Remove `src/win32` once nothing more has to be restored from it
- [ ] Fix run-time 64-bit defects found by play testing

Exit: the opening level is playable from start to finish including cutscenes; save and
load round-trip; no sanitizer reports during the play-test checklist.

State on 2026-11-23, after a first play test at a real window. Window, keyboard and
mouse work, the menus were used as far as starting a new game, sound is heard,
configuration and saves go to the per-user directory, and the game was played from the
opening cutscene through the first levels to a mission of the first tier. What the play
test found, and what became of it:

| Finding | Cause | State |
|---|---|---|
| No cutscene on the first level, the companion does nothing | The script reader took the length of a script's 4-byte header from a pointer: no script was loaded on 64-bit | fixed |
| Full screen drawn into a corner | The window's size was read before Wayland had applied the full-screen state | fixed |
| Walk and run animations at about half speed | The console version's halved limit on playback speed | fixed, PC line restored |
| Saber upright in the saber menu | The code that lays it on its side was commented out, as on the console | fixed, lines restored |
| "Load checkpoint" does nothing | Respawning loaded the console's "Checkpoint" save, which nothing writes | fixed, PC logic restored |
| Garbage text on the console when a cutscene is skipped | A format without its argument | fixed |
| "glGetError() = 0x501" at level changes | Cloud layers drawn for skies that have none | fixed |
| In the release preset the companion does nothing after a change of level | Level-load frames shorter than the engine's 200 ms clamp let the player into the level before the characters its start script addresses are spawned (section 4.8) | fixed |
| Loading bar is a blank upright box | `CG_LoadBar` is the console version; the PC one is not in the release | fixed, taken from the Jedi Outcast release ([ADR-0017](decisions/0017-missing-pc-code-from-jedi-outcast.md)) |
| Debug build aborts when a droid loses a limb | An assertion of the release that the game's own data trips; the retail build has assertions off | kept; play tests use the `linux-release` preset |
| Sound drops out while models load in the menus | The release empties the sound buffer before every file read (section 4.7) | kept; listed under "Not scheduled" |
| Resolutions of today missing from the menu | The list is game data (section 4.5) | listed under "Not scheduled" |

Checked without a person, under the sanitizers: menus, levels and the opening cutscene
render in a real OpenGL context, the first level changes to the second, videos play, a
save made on a level with NPCs loads again in the same and in a new process, damaged
saves are refused, and dying and "load *respawn" restarts from the automatic save.

Still open before the exit criteria can hold: the opening level has not been played to
its end by a person since these fixes; music, the clipboard, edit fields, sliders and
list boxes of the menus have not been confirmed; and the unticked items above.

### M3. Runs on Windows and macOS

- [ ] Decide and record: continuous integration
- [ ] Windows x86-64 build (MSVC and clang-cl, through the same presets)
- [ ] macOS arm64 build and application bundle
- [ ] CI for all three platforms

Exit: the M2 exit criteria hold on Windows and macOS, verified on real machines.

### M4. Renderer boundary

- [ ] `tr_local.h` no longer visible outside the renderer
- [ ] Ghoul2 model data, skeleton and bolt code moved out of the renderer
- [ ] No graphics API calls outside the renderer
- [ ] Renderer selected at start-up; legacy renderer is one implementation
- [ ] Screenshot comparison harness and camera position sets

Exit: the engine links against the null renderer alone; the legacy renderer passes the
play test unchanged; the harness produces stable images run to run.

### M5. New renderer, parity

- [ ] Spike to confirm [ADR-0009](decisions/0009-graphics-api.md); decide and record the shader toolchain
- [ ] Device, swap chain, frame loop, 2D and fonts (menus render)
- [ ] World: BSP faces, patches, lightmaps and light styles, vertex light, sky, fog
- [ ] Shader-script compiler and pipeline cache
- [ ] Models: MD3, Ghoul2 with GPU skinning
- [ ] Dynamic geometry: effects, sprites, marks, beams, surface sprites, weather
- [ ] Dynamic lights, shadows as in the original, flares, portals
- [ ] Glow, dissolves, screenshots, cinematic frames, save-game thumbnails
- [ ] Decide and record: removal of the legacy renderer

Exit: image comparison passes on the agreed map set; the full campaign is playable with
the new renderer on all three platforms.

### M6. HDR pipeline

- [ ] Decide and record: lighting model
- [ ] Linear-light rendering, floating-point target, tone mapping
- [ ] Bloom replacing glow passes
- [ ] Anti-aliasing
- [ ] Per-pixel dynamic lights with shadow maps
- [ ] Sun shadows from dynamic objects
- [ ] Configuration presets, including "original look"

Exit: visual sign-off on the agreed map set in both the original-look and the modern
preset.

### M7. PBR

- [ ] Decide and record: material override format
- [ ] PBR shading with derived defaults
- [ ] Material overrides and high-resolution texture replacement
- [ ] Image-based lighting

Exit: a replacement material set for one map demonstrates the full pipeline.

### M8. Multiplayer

- [ ] Decide and record how multiplayer is brought onto this engine

### Not scheduled

Limits of the release that the port keeps for now. Each needs a decision before it is
given a milestone.

- Wide displays: menus and HUD are stretched from 4:3, and the field of view is fixed
  horizontally, so a wide picture shows less at top and bottom.
- Resolution in full screen: always that of the desktop (section 4.5). A lower
  resolution scaled up to the display, and current resolutions in the menu's list, are
  not offered.
- Sound drops out while files are read during play (section 4.7). Mixing on a thread of
  its own would remove that.

## 9. Decisions

Recorded: see the [decision log](decisions/README.md).

Still to be made, each before the milestone that needs it:

| Decision | Needed by |
|---|---|
| Continuous integration provider | M3 |
| Shader language and toolchain | M5 |
| Removal of the legacy renderer | end of M5 |
| Lighting model for mixed baked and dynamic light | M6 |
| Material override format | M7 |
| Multiplayer approach | M8 |

## 10. Risks

| Risk | Effect | Response |
|---|---|---|
| The graphics API choice turns out too limiting | Rework of the renderer back end | GPU calls confined to one directory; spike before commitment; Vulkan as fallback |
| Linear-light blending changes the look of effects | The modern pipeline looks wrong | Parity first in legacy colour mode; per-category handling; visual sign-off |
| Ghoul2 separation is harder than estimated | M4 slips | It is isolated in its own milestone; the legacy renderer keeps working throughout |
| Hidden 64-bit defects in game logic | Crashes far into the campaign | Sanitizer builds as the default for play testing |
| macOS and Windows cannot be run on the development machine | Defects found late | CI builds from M3; run-time checks on real machines are an explicit exit criterion |
| The licence statement was taken from mirrors of the release, not from the original SourceForge page | Cannot publish if the mirrors are wrong | Check the original release page before the repository is made public |
| Scope | The project never reaches the renderer | Multiplayer and tools are out; each milestone is playable or testable on its own |

## 11. Legal

- Raven's release is under the GNU General Public License, version 2. The tree as
  imported contains no licence file; the readme of the release, as preserved by mirrors,
  states "GNU GPLv2" without "or later" wording. The licence text is in `LICENSE`, and
  the provenance and third-party notices are in `NOTICE.md`.
- The Xing MP3 decoder in `mp3code/` carries a GPL notice in its headers. jpeg-6, zlib
  and libpng carry their own permissive notices and are being replaced.
- Proprietary binaries and headers (SmartHeap, Immersion, EAX, Bink, the OpenAL
  redistributables) are removed in M0.
- The game data is not part of this project and is never redistributed. A retail copy
  is required to run the game.
- "Star Wars" and "Jedi Knight" are trademarks of their owners. The project name must
  not suggest an official product.
