---
status: accepted
date: 2026-11-25
decision-makers: Luka Dornhecker
---

# The Windows build is also made on Linux, with clang-cl and MSVC's libraries, and run under Wine

## Context and Problem Statement

M3 needs a Windows x86-64 build with MSVC and with clang-cl, through the same presets
([design document](../design.md), section 8). The project has no Windows machine.
Continuous integration builds the Windows presets on GitHub's runners
([ADR-0021](0021-continuous-integration.md)). There, each attempt is a push and a wait,
nothing but the tests without game data runs, and a failure can only be looked at
through its log. The 2003 code has never been compiled by a current MSVC, so many
attempts are to be expected. How are Windows builds made and tried on the Linux
development machine?

## Decision Drivers

* The two Windows compilers are MSVC and clang-cl
  ([ADR-0004](0004-build-system-and-toolchain.md)). A local build should break where
  theirs break: the same C runtime, standard library and Windows headers.
* Run-time behaviour on Windows has so far not been checkable at all. Anything that
  runs the Windows build here, with the game data, narrows that gap.
* The development machine has clang-cl, lld-link and Wine already. The retail game
  runs under that Wine and is the reference for the release's behaviour.
* MSVC's C runtime, its standard library and the Windows SDK are Microsoft's, under
  Microsoft's licence. They may not be committed, and their licence has to be accepted
  by the person who downloads them.
* One build system and presets as the only way to build
  ([ADR-0004](0004-build-system-and-toolchain.md)). No scripts in other languages
  ([ADR-0011](0011-repository-tooling-in-cpp.md)).
* What a build uses is pinned ([ADR-0012](0012-dependencies.md)).

## Considered Options

* clang-cl on Linux, against MSVC's libraries and the Windows SDK fetched with xwin,
  run under Wine
* MinGW-w64's GCC on Linux, run under Wine
* Continuous integration only
* A Windows virtual machine with Visual Studio's build tools

## Decision Outcome

Chosen option: "clang-cl on Linux, against MSVC's libraries and the Windows SDK fetched
with xwin, run under Wine", because it builds with one of the two Windows compilers,
against the same runtime, standard library and headers as the MSVC build, on the
machine where development happens, and lets that build run, the game included.

* **Libraries:** [xwin](https://github.com/Jake-Shadle/xwin) downloads MSVC's C
  runtime and standard library and the Windows SDK from Microsoft and lays them out as
  a directory that clang-cl reads with `/winsysroot`. xwin names the architecture's
  library directories `x86_64` where `/winsysroot` expects `x64`, so the linker is
  given those directories one by one. xwin's version, and the MSVC and SDK versions it
  fetches, are pinned and written at the top of the toolchain file below, with the one
  command that installs xwin and the one that fetches the libraries. The second names a
  download cache outside the working tree; xwin's default is the current directory.
  The developer runs them and accepts Microsoft's licence with the second.
* **Where:** outside the working tree, in the directory named by the environment
  variable `JASP_WINSYSROOT`, as the game data's is by `JKA_GAMEDATA_DIR`
  ([ADR-0007](0007-game-data-location.md)). Nothing of it is committed, and its location
  is never written into a tracked file.
* **Preset:** `windows-cross-release`, available on Linux hosts only, with a toolchain
  file `cmake/toolchains/windows-cross-clang-cl.cmake`: target Windows x86-64, clang-cl
  and lld-link from the host, `/winsysroot` from `JASP_WINSYSROOT`. Configuring stops
  with a message when the variable is unset. The release configuration only: it uses
  the runtime DLLs that Wine provides itself, whereas MSVC's debug runtime DLLs are not
  redistributable and Wine has no equivalent of them. The debug configuration is built
  by CI's `windows-debug` job.
* **Tests:** `ctest --preset windows-cross-release` runs every test under Wine, through
  CMake's cross-compiling emulator. With `JKA_GAMEDATA_DIR` set, that includes the smoke
  tests. Wine runs in a prefix of the build directory's own, not the developer's, with
  no display, so that nothing opens on the desktop, and without its dialog for a
  program that crashes. The test scripts take the emulator as an argument and run their
  program through it. The unit tests are one ctest entry there, not one per test case:
  Catch2 for Windows reads an argument that starts with a slash as an option, and its
  test discovery passes the host's absolute path of a file.
* **Not in CI.** CI builds on Windows itself, with MSVC; the cross preset is for the
  development machine.
* **Host tools:** clang-cl, lld-link and Wine come from the host, as GCC and Clang do
  for the Linux presets. Their minimum versions are the build's
  ([ADR-0004](0004-build-system-and-toolchain.md)).

### Consequences

* Good, because most of what breaks the Windows build is seen in seconds, locally:
  MSVC's headers and standard library, the Windows SDK, `long` of 32 bits, the linker's
  view of the game module.
* Good, because the Windows build runs here, tests and game, with the game data, which
  no other option allows.
* Good, because it builds the clang-cl half of the M3 item. CI builds the MSVC half.
* Neutral, because Wine is not Windows. A defect that shows only under Wine is not
  necessarily one of the build, and a clean run under Wine does not replace a run on a
  Windows machine, which remains the exit criterion of M3.
* Bad, because the debug configuration does not run here.
* Bad, because Microsoft may withdraw a pinned version of the libraries from its
  download manifest. The pin then has to move, which is a commit of its own.
* Bad, because the libraries take of the order of a gigabyte on the development
  machine.
* Bad, because clang-cl and MSVC still differ: some MSVC errors, warnings and
  optimiser behaviour show only in CI.

### Confirmation

A spike confirms the record before it is accepted. It passes when:

* xwin, at the pinned version, fetches the libraries into `JASP_WINSYSROOT`;
* `cmake --preset windows-cross-release` and `cmake --build --preset
  windows-cross-release` build the engine, the game module, the tools and the tests;
* `ctest --preset windows-cross-release` runs them under Wine, the smoke tests with the
  game data, and reports the same as on Linux, or lists what differs and why.

Fixes to the code that the spike shows to be needed are not part of the spike. They are
the M3 work that follows.

## Pros and Cons of the Options

### clang-cl with MSVC's libraries, under Wine

* Good, because the runtime, standard library and headers are the shipped build's.
* Good, because the tools are already installed, apart from xwin and the libraries.
* Bad, because of the licence, the download and the size.

### MinGW-w64 under Wine

* Good, because it is installed and needs nothing downloaded or accepted.
* Bad, because GCC's standard library and MinGW's runtime are not the shipped build's.
  The breakage that matters most, MSVC's, is not seen.
* Bad, because it is a fifth compiler to keep the code building with, for no platform
  that is shipped.

### Continuous integration only

* Good, because nothing more to set up.
* Bad, because every attempt is a push and a wait of minutes, for the many attempts the
  first build needs.
* Bad, because the Windows build never runs with the game data before a real machine
  is found.

### A Windows virtual machine

* Good, because it is real Windows with real MSVC.
* Bad, because it needs a Windows licence and an installation of tens of gigabytes to
  keep up to date, outside the project.
* Bad, because OpenGL in a virtual machine is unreliable, so the game would hardly run
  there.

## More Information

The spike was run on 2026-11-24, with xwin 0.10.0, MSVC 14.44.17.14 (Visual Studio
2022 17.14, the version on CI's Windows runner), the Windows SDK 10.0.26100, clang-cl
and lld-link 23.1.4 and Wine 11.21. Engine, game module, tools and tests built without
an error. Under Wine, the repository checks, the tools' self-test, the unit tests and
the smoke test that loads `yavin1b` without a window passed. The smoke test that
renders was skipped, as on any machine without an OpenGL context for a program without
a display. The first run found a defect of the Windows build that CI could not show:
`setvbuf` with `_IOLBF` and a size of 0, which Microsoft's C runtime refuses, ended the
engine at its first statement.

* xwin: <https://github.com/Jake-Shadle/xwin>; clang-cl's `/winsysroot`:
  <https://clang.llvm.org/docs/UsersManual.html#clang-cl>.
* To be revisited if a Windows machine becomes available, or if clang-cl and MSVC
  diverge enough that the cross build hides more than it finds.
