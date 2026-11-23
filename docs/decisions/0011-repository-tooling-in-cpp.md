---
status: accepted
date: 2026-11-23
decision-makers: Luka Dornhecker
---

# Repository tooling is written in C++ and built by the project

## Context and Problem Statement

M0 introduced two repository tools: one that checks the rules every commit has to keep,
and one that rewrites `#include` lines after files move. Both were written in Python,
which made a Python interpreter a requirement for running the tests on every platform,
without that ever having been decided. In which language is repository tooling written?

## Decision Drivers

* No dependency beyond what building the game already needs: a C++ compiler, CMake,
  Ninja and git.
* The tools must run the same way on Linux, Windows and macOS, and under `ctest`.
* One language in the repository where possible.
* The tools process about 450k lines of source and must stay fast.

## Considered Options

* C++ programs built by the CMake project
* Python scripts
* POSIX shell with grep, sed and awk
* CMake script mode (`cmake -P`)

## Decision Outcome

Chosen option: "C++ programs built by the CMake project", because it is the only option
that adds no dependency and works identically on all three platforms.

* Tools live in `tools/` and are ordinary targets of the build.
* A tool does not find the repository's files itself. It reads the list of tracked files
  from its standard input, as `git ls-files -z` prints it, and is run from the
  repository root. This keeps the tools free of process-spawning and of
  platform-specific code.
* Under `ctest`, a small CMake script provides that pipe.
* Data that changes independently of the code, such as the list of tolerated missing
  headers, is kept in a text file next to the tools, not in the source.
* CMake script mode is used only for glue of this kind, never for processing source
  text.

### Consequences

* Good, because a machine that can build the game can run every check.
* Good, because the tools are compiled with the same strict warnings as other new code
  and run under the sanitizer preset.
* Bad, because a tool can only be run after a build has been configured and built. For
  the two tools this takes a few seconds.
* Bad, because text processing in C++ takes more lines than in a scripting language.

### Confirmation

The repository contains no script in an interpreted language other than CMake, and
`CMakeLists.txt` looks for no interpreter. `ctest` passes on a machine without Python.

## Pros and Cons of the Options

### C++ programs built by the project

* Good, because no new dependency and one language.
* Bad, because a build is needed first.

### Python scripts

* Good, because shortest to write and runnable without a build.
* Bad, because an interpreter becomes a requirement on every developer and CI machine.

### POSIX shell

* Good, because present on Linux and macOS.
* Bad, because on Windows it exists only inside Git for Windows, which `ctest` would
  have to locate; quoting and tool differences make it fragile.

### CMake script mode

* Good, because CMake is already required.
* Bad, because its regular expressions cannot express what the include check needs
  (comments have to be skipped correctly), and character-level processing of this much
  text is very slow.
