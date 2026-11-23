---
status: accepted
date: 2026-11-23
decision-makers: Luka Dornhecker
---

# Start from the Raven source release, not from OpenJK

## Context and Problem Statement

The project needs a starting codebase. Raven's 2013 release is the original: Visual
Studio 2003 projects, 32-bit Windows and Xbox only. The community fork OpenJK has
already ported that code to CMake, SDL2, 64-bit, Linux and macOS. Which one does this
project build on?

## Decision Drivers

* The result should be clean and small, with a structure chosen on purpose.
* The Visual Studio-centred build and layout of OpenJK are to be avoided.
* Time to a running game.

## Considered Options

* Start from the Raven release
* Fork OpenJK
* Start from the Raven release and import OpenJK subsystems piecemeal

## Decision Outcome

Chosen option: "Start from the Raven release", because every structural choice in the
project is to be made deliberately, and inheriting OpenJK means inheriting a decade of
its build and layout decisions.

OpenJK remains a reference for *what* breaks on modern systems. Fixes are written for
this codebase's structure; OpenJK code is not imported wholesale.

### Consequences

* Good, because the tree contains nothing that was not put there on purpose.
* Good, because the git history starts at the unmodified original, so every change is
  attributable.
* Bad, because the porting work OpenJK already did (compiler dialect, 64-bit, platform
  layer) is done again. This is the bulk of milestones M0 to M3.
* Bad, because defects OpenJK has already found and fixed will be found again unless
  someone looks.

### Confirmation

The import commit `44e8ea1` is the unmodified release and is tagged `raven-2013` in M0.

## Pros and Cons of the Options

### Start from the Raven release

* Good, because full control over structure, build and dependencies.
* Bad, because slowest route to a running game.

### Fork OpenJK

* Good, because it runs on all three platforms today; work could begin at the renderer.
* Bad, because it brings its build system, directory layout, compatibility layers and
  conventions, which are explicitly not wanted.

### Import OpenJK subsystems piecemeal

* Good, because it saves time on the dullest parts.
* Bad, because the result is a mixture of two structures, which is the opposite of the
  goal.

## More Information

Both codebases descend from the same release and, as far as is known, share its licence,
so consulting OpenJK is unproblematic. See `docs/design.md`, section 11, for the open
point on confirming the licence text.
