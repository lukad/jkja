---
status: accepted
date: 2026-11-23
decision-makers: Luka Dornhecker
---

# Single-player first; multiplayer tree leaves the working tree

## Context and Problem Statement

The release contains two forked codebases: `code/` (single-player, about 543k lines) and
`codemp/` (multiplayer, about 640k lines). They share ancestry but have diverged in every
subsystem, including the renderer (57k and 60k lines). Porting both at once doubles
every mechanical step. Which goes first, and what happens to the other meanwhile?

## Decision Drivers

* Reach a playable game and the renderer work as early as possible.
* Keep the working tree free of code that is not built.
* Do not make the later multiplayer port harder than it has to be.

## Considered Options

* Single-player first; remove `codemp/` from the working tree until its milestone
* Single-player first; leave `codemp/` in place, unbuilt
* Multiplayer first
* Both in parallel
* Merge both into one engine before porting

## Decision Outcome

Chosen option: "Single-player first; remove `codemp/` from the working tree", because
single-player has the simpler runtime (no network protocol to stay compatible with, no
bytecode virtual machine, no bot library) and is the game the renderer work is meant to
show off, and because 640k lines of unbuilt near-duplicates would pollute every search
and every mechanical change.

The multiplayer sources stay available at the import commit. How multiplayer is brought
onto the modernised engine is a separate decision at M8.

### Consequences

* Good, because every mechanical step in M0 to M4 is done once.
* Good, because searches and refactorings only see code that is built.
* Bad, because multiplayer comes late, and the multiplayer-specific parts of the
  original renderer (and anything else that only exists in `codemp/`) are not carried
  along automatically.
* Bad, because when multiplayer starts, its sources are 2013-era code again and must be
  brought up to the then-current engine.

### Confirmation

After M0, `git ls-files` shows no `codemp/` path, and `git show raven-2013:codemp/...`
still works.

## Pros and Cons of the Options

### Single-player first, `codemp/` removed

* Good, because the tree is small and everything in it is live.
* Bad, because multiplayer code is one indirection away.

### Single-player first, `codemp/` left in place

* Good, because the multiplayer code is visible for reference.
* Bad, because it rots silently and shows up in every search; this contradicts "delete,
  don't disable".

### Multiplayer first

* Good, because the multiplayer tree already has a dedicated-server and a Linux code
  path of sorts, and its game code is plain C.
* Bad, because it adds the bytecode VM with an x86 JIT, network compatibility questions
  and the bot library to the first milestone.

### Both in parallel

* Bad, because every step is done twice in slightly different code.

### Merge first

* Good, because it is the cleanest end state.
* Bad, because merging two diverged engines that do not yet compile gives no way to
  test the merge.
