---
status: accepted
date: 2026-11-23
decision-makers: Luka Dornhecker
---

# Dependencies are pinned and built by the project; nothing is taken from the system

## Context and Problem Statement

The release carries its own copies of three libraries: jpeg-6 from 1995 (`src/jpeg-6`,
18k lines), Raven's rewrite of zlib 1.1.3 with inline x86 assembly (`src/zlib32`, plus
the headers of the reference zlib in `src/zlib`), and a PNG reader and writer written on
top of that rewrite (`src/png`). None of them builds as it is on the target platforms,
and all three are to be replaced in M1. The engine uses them for exactly this:

| Use | Where |
|---|---|
| Raw inflate, streaming, for files in `.pk3` archives | `qcommon/unzip.cpp` |
| CRC-32 of names, as a hash | `qcommon/stringed_ingame.cpp`, `renderer/tr_image.cpp` |
| JPEG decode (textures) and encode (screenshots) | `renderer/tr_jpeg_interface.cpp` |
| PNG decode to 8 bits per pixel (terrain height maps) | `renderer/tr_image.cpp`, `R_LoadDataImage` |

`LoadPNG32` and `PNG_Save` have no callers. From M1 the project also has unit tests, the
platform layer needs SDL3 ([ADR-0005](0005-platform-layer-sdl3.md)), and the legacy
renderer needs the OpenGL declarations to compile. Which libraries does the project
depend on, and where does the build get each of them from?

## Decision Drivers

* Libraries installed on the system are not to be used: their versions, build options
  and package names differ from machine to machine, and finding them is a recurring
  source of build failures.
* Every machine builds against exactly the same library versions.
* The same build commands on every platform; no installation step that the build cannot
  perform itself.
* No library sources in the tree, single-header libraries excepted (a given).
* Few dependencies.
* The retail data must load: baseline JPEG textures, 8-bit PNG height maps, deflated
  `.pk3` entries.

## Considered Options

For obtaining dependencies:

* Pinned and built by the project, never searched for on the system
* Found on the system first, fetched at a pinned version otherwise
* A package manager (vcpkg or Conan)

For JPEG and PNG:

* stb_image and stb_image_write
* libjpeg-turbo and libpng

For inflate and CRC-32:

* miniz
* zlib

For unit tests:

* Catch2 version 3
* doctest
* No framework: plain executables run by `ctest`

## Decision Outcome

Chosen options: "pinned and built by the project", "stb_image and stb_image_write",
"miniz" and "Catch2 version 3".

Where each dependency comes from:

| Need | Library | Where it comes from | Used by |
|---|---|---|---|
| JPEG and PNG decode | stb_image | **committed**: one header in `external/stb/` | `src/renderer` |
| JPEG encode | stb_image_write | **committed**: one header in `external/stb/` | `src/renderer` |
| Inflate, CRC-32 | miniz | **downloaded and built** by the project | `src/qcommon`, `src/renderer` |
| Platform services | SDL3 | **downloaded and built** by the project | `src/sys` only |
| OpenGL declarations | the header inside the SDL3 download | through a header in `src/sys` | legacy renderer |
| Unit tests | Catch2, version 3 | **downloaded and built** by the project | `tests/` only |
| MP3 decode | the Xing decoder | **already in the tree** (`src/mp3code`), part of the release | `src/client` |

Rules:

* The build never calls `find_package` or `pkg-config` for a library. A library that
  happens to be installed on the machine is not looked at.
* "Downloaded and built" means: `cmake/Dependencies.cmake` declares the library with
  CMake's `FetchContent`, as the URL of an official release archive plus its SHA-256
  hash. Configuring downloads the archive, checks the hash and adds the library's own
  CMake project to the build. It is compiled with the project's compiler and flags and
  linked statically. The engine and the game module have no run-time dependency on a
  library package.
* Archives are downloaded once per working tree, into `build/_deps`, and shared by all
  presets. After that, configuring works offline.
* "Committed" is for single-header libraries only: the header is kept unmodified in
  `external/<name>/`, with its licence and the version it was taken from stated in
  `NOTICE.md`. The implementation of each is compiled in exactly one source file.
* Parts of a fetched library that the project does not use are switched off through
  that library's CMake options (SDL3: tests, examples, camera, sensors, its 2D renderer).
* The Xing decoder stays where it is; it is portable C. It is replaced only if it
  causes trouble.
* `src/jpeg-6`, `src/zlib`, `src/zlib32` and `src/png` are deleted.

### Consequences

* Good, because every developer and every CI machine builds the same library versions,
  and a build failure caused by a library is reproducible everywhere.
* Good, because the tree loses 24k lines of 1990s library code and the last inline
  assembly outside the engine proper.
* Good, because the shipped binaries depend on no library package.
* Bad, because the first configure in a working tree needs network access.
* Bad, because every preset compiles its own copy of the fetched libraries; SDL3 adds a
  few minutes to the first build of a preset.
* Bad, because one thing cannot be made independent of the system: SDL3 talks to the
  display server and the sound server of the operating system. On Windows and macOS
  their interfaces come with the compiler's platform SDK. On Linux, compiling SDL3
  needs the development headers of X11, Wayland and the audio system to be installed;
  SDL3's configure step stops with a message naming what is missing. This applies from
  M2; the headless build of M1 uses SDL3 only for timers, paths and module loading.
* Bad, because security fixes in a library arrive only when its pin is updated.
* Bad, because stb_image decodes JPEG with a different inverse DCT and chroma upsampling
  than jpeg-6: decoded textures can differ from the retail executable's by a few levels
  per channel. The image comparison of M4 and later compares the two renderers of this
  project, which use the same decoder, so it is not affected.
* Bad, because stb_image does not read arithmetic-coded or 12-bit JPEG files. The retail
  data contains none; a mod that ships one gets a load failure with a message.

### Confirmation

`CMakeLists.txt` and the files under `cmake/` contain no `find_package` for a library.
The directories named above are gone; the repository check keeps library sources out.
A unit test covers inflate, CRC-32 and both image decoders with data generated by the
test itself; the headless smoke test loads a map from the retail archives, which
exercises inflate and the JPEG decoder on real data. `ldd` on the two binaries built
by a Linux preset lists no library that this record names.

## Pros and Cons of the Options

### Pinned and built by the project

* Good, because one path, the same on every machine.
* Good, because it needs no tool beyond CMake.
* Bad, because longer first build, and network access for the first configure.

### Found on the system first, fetched otherwise

* Good, because fastest build where the packages exist.
* Bad, because two paths exist, both have to be tested, and which one a given machine
  takes depends on what is installed there. This is the failure mode to be avoided.

### A package manager

* Good, because binary caching.
* Bad, because vcpkg or Conan becomes a tool every developer has to install, which the
  build cannot do itself.

### stb_image and stb_image_write

* Good, because two headers, no build system, identical on every platform.
* Good, because they cover JPEG decode and encode and PNG decode, which is all the engine
  uses.
* Bad, because decoding is slower than libjpeg-turbo; texture loading happens at map
  load and the retail textures are small.
* Bad, because the output is not bit-identical to jpeg-6.

### libjpeg-turbo and libpng

* Good, because libjpeg-turbo descends from the bundled library and decodes closest to
  it; fastest.
* Bad, because libjpeg-turbo refuses to be built as part of another CMake project, so
  it would need a separate external build with its own toolchain handling.
* Bad, because libpng needs zlib specifically, which fixes the next choice.

### miniz

* Good, because small (one source file and one header in its release archive), with a
  CMake project that works as a sub-build without adjustment.
* Good, because it offers zlib's interface, including streaming raw inflate and CRC-32,
  which is what `unzip.cpp` was written against; CRC-32 values do not change.
* Neutral, because it is not a single header, so it is downloaded rather than committed.

### zlib

* Good, because the reference implementation.
* Bad, because its CMake project is clumsy as a sub-build: it builds a shared library
  nobody asked for, renames a header inside its own source directory and does not
  export usable include paths, all of which need workarounds.

### Catch2 version 3

* Good, because maintained, and integrates with `ctest` (one test per case).
* Bad, because it is a compiled library and adds about a minute to the first build of
  each preset.

### doctest

* Good, because a single header that could be committed: nothing to download.
* Bad, because its maintenance has been intermittent, which matters for a header that
  has to compile without warnings on three current compilers.

### No framework

* Good, because no dependency at all.
* Bad, because assertion reporting, test selection and fixtures get reinvented badly.

## More Information

The exact versions and hashes are in `cmake/Dependencies.cmake`. Revisit the JPEG
choice if a comparison against the retail executable ever needs bit-identical textures,
or if texture loading time becomes noticeable with high-resolution replacement sets
(M7).
