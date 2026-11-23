---
status: accepted
date: 2026-11-23
decision-makers: Luka Dornhecker
---

# Target platforms: Linux, Windows, macOS; 64-bit only

## Context and Problem Statement

Linux, macOS and Windows support is required. The original code targets 32-bit
x86 Windows and Xbox, and assumes in many places that a pointer fits in an `int`. Which
operating system and CPU combinations are supported, and is 32-bit one of them?

## Decision Drivers

* Linux, macOS and Windows are required (a given, not up for decision here).
* Current Macs are arm64.
* Every supported combination costs testing effort for the whole life of the project.
* The work must be verifiable on the development machine (Linux x86-64).

## Considered Options

* 64-bit only: Linux x86-64, Windows x86-64, macOS arm64
* The above plus 32-bit x86 as a permanent target
* The above plus 32-bit x86 as a temporary first milestone
* The above plus macOS x86-64 and Linux/Windows arm64

## Decision Outcome

Chosen option: "64-bit only: Linux x86-64, Windows x86-64, macOS arm64", because these
three are what the three required operating systems run on today, and a 32-bit build
would keep alive exactly the assumptions the port has to remove.

Other 64-bit little-endian combinations (Linux arm64, macOS x86-64) are expected to work
because nothing in the code may depend on the architecture, but they are not tested and
not promised.

### Consequences

* Good, because the untyped pointer-through-`int` interfaces must be fixed properly;
  there is no configuration in which they still work and can be left alone.
* Good, because the test matrix is three entries.
* Bad, because the port cannot be brought up in a 32-bit configuration first to
  separate compiler-modernisation defects from pointer-size defects. The compiler
  rejects truncating pointer casts in C++, which finds most of them at build time; the
  sanitizer builds are the net for the rest.
* Bad, because retail 32-bit game modules and mods cannot be loaded. This is accepted.

### Confirmation

CMake presets exist for exactly these three targets. Continuous integration builds all
three from M3.

## Pros and Cons of the Options

### 64-bit only

* Good, because smallest matrix and no legacy assumptions kept alive.
* Bad, because no intermediate "known-good" 32-bit build.

### 32-bit as a permanent target

* Good, because retail mods could in principle be loaded.
* Bad, because no current platform needs it and macOS cannot run it at all.

### 32-bit as a temporary first milestone

* Good, because it would give a running game sooner and isolate 64-bit defects.
* Bad, because it needs 32-bit builds of SDL3 on the development machine, which are not
  installed, and the retail executable under Wine already serves as the behavioural
  reference.

### More architectures

* Neutral, because they probably work for free.
* Bad, because promising them means testing them.

## More Information

Byte order: all targets are little-endian; no big-endian work is done.
