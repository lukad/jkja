---
status: accepted
date: 2026-11-23
decision-makers: Luka Dornhecker
---

# CMake presets with Ninja, C++20, no IDE project files

## Context and Problem Statement

The release builds only through Visual Studio 2003 project files, with settings repeated
per file and per configuration (the single-player engine project sets precompiled-header
options roughly 500 times). The project needs one build description that works the same
on Linux, macOS and Windows, and that nobody has to open an IDE to change.

## Decision Drivers

* One build description for all platforms; the same commands everywhere.
* No generated or hand-maintained IDE project files in the repository.
* Good support for the dependencies (SDL3 ships CMake packages) and for CI images.
* Readable by people and agents; editable as text.
* A single language standard, recent enough to be pleasant, old enough to be supported
  by all three compilers.

## Considered Options

* CMake with presets, Ninja as the only generator
* Meson
* xmake or premake
* Plain Makefiles plus a separate Windows build

## Decision Outcome

Chosen option: "CMake with presets, Ninja as the only generator", because it is the one
build system that every dependency, every CI image and every IDE on all three platforms
already understands, and presets give exactly one named way to build each configuration.

Rules that come with it:

* `CMakePresets.json` defines every supported configuration. Building is
  `cmake --preset <name>`, `cmake --build --preset <name>`, `ctest --preset <name>`.
* Ninja is the only generator. On Windows, MSVC and clang-cl are used through Ninja.
  Visual Studio users open the folder; no `.sln` or `.vcxproj` exists in the tree.
* `CMakeUserPresets.json` is for local settings and is ignored by git.
* Target-based CMake only: properties on targets, no directory-wide flags, no globbing.
* No precompiled headers until the code builds cleanly without them; every file includes
  what it uses.
* C++20 for all C++ code; C11 for the remaining C. No compiler extensions.
* Compilers: current stable GCC, Clang, MSVC and Apple Clang. Exact minimum versions are
  written into the top-level `CMakeLists.txt` when it lands.

### Consequences

* Good, because the build is the same three commands on every platform.
* Good, because the build description is a handful of readable text files.
* Bad, because CMake's language is unlovely; discipline (target-based, no globbing) is
  what keeps it clean.
* Bad, because compiling 2003-era C++ as C++20 surfaces more errors at once (`register`,
  old `for` scoping, pre-standard headers). They are mechanical and are fixed in M1.

### Confirmation

A fresh clone builds on each platform with the three commands above and nothing else.
The repository check rejects `.sln`, `.vcproj`, `.vcxproj`, `.dsp` and `.dsw` files.

## Pros and Cons of the Options

### CMake with presets and Ninja

* Good, because universal tool and dependency support.
* Good, because presets remove "how do I configure this" as a question.
* Bad, because the language invites mess if not held to a style.

### Meson

* Good, because the cleanest build language of the candidates; fast; presets are not
  needed because there are fewer knobs.
* Bad, because weaker support for macOS application bundles and for consuming
  dependencies that only ship CMake packages, and less familiar to contributors.

### xmake or premake

* Good, because concise.
* Bad, because small ecosystems; premake's purpose is generating IDE projects, which is
  what this project is avoiding.

### Makefiles plus a Windows build

* Bad, because two build descriptions is the problem being solved.

## More Information

Revisit the C++ standard when all three compilers have complete support for a newer one
and there is a concrete feature the code wants.
