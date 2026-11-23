---
status: accepted
date: 2026-11-23
decision-makers: Luka Dornhecker
---

# Savegame format: every saved structure is written field by field, with its schema in the file

## Context and Problem Statement

[ADR-0006](0006-drop-retail-savegame-compatibility.md) dropped compatibility with retail
saves and left the new format open. A survey of the save code in M2 gives the facts the
format has to be chosen from.

A save file is a sequence of chunks (identifier, length, data, checksum; optionally
run-length compressed). ICARUS state, script variables, timers, cvars and config
strings are already written value by value. The game structures are not: `gentity_t`,
`gclient_t`, `gNPC_t`, `Vehicle_t`, `level_locals_t`, `parms_t`, the objectives and the
Ghoul2 bone, bolt and surface lists are written as they lie in memory, with `sizeof`.
Before a structure is written, the pointer members named in five field tables
(63 entries) are replaced by indexes; after it is read they are turned back into
pointers. The tables name the pointer members only. The structures hold well over a
thousand other members between them.

On a 64-bit build this code is wrong in several places, all for one reason: an index is
stored as a 4-byte `int` in the low half of an 8-byte pointer and the member is then
still used as a pointer.

* A full save that contains an NPC cannot be loaded. The writer marks an NPC's client
  with the index -2 and then compares the whole pointer with -2, which is never equal,
  so the client is not written; the reader expects it and stops with an error.
* The high halves of all converted pointers go to disk as they were, and so do ten
  pointers of the two sabers, the client array pointer of the level and four pointers
  in every Ghoul2 bone, which no table names.
* The comment chunk stores a `time_t`, and one Ghoul2 chunk is written with a literal
  length of 4 taken from a pointer.

What does the game write instead?

## Decision Drivers

* Save and load have to round-trip on 64-bit; that is part of the exit of M2.
* No saved byte may depend on an address, on the compiler's padding or on the width of
  a pointer.
* A save should outlive the build that wrote it: the same file is to load on Linux,
  Windows and macOS, and after a member has been added to or removed from a saved
  structure.
* A file that cannot be read is refused with a message instead of being misread.
* Nothing may be done that blocks a reader for retail saves later (ADR-0006).

## Considered Options

* Field-by-field serialisation of every saved structure
* Memory image with correct pointer conversion and a layout record
* Field-by-field for the common structures, memory image for the rest

## Decision Outcome

Chosen option: "field-by-field serialisation of every saved structure", because it is
the only option under which a save does not belong to one build, and because it is
what ADR-0006 and the design document announced.

**Field tables.** Every structure that is saved has a table that names every one of
its members. The five existing tables become part of these: a pointer member's entry
says what it points into (entity, client, item, group, vehicle type, string), and a
pointer that is rebuilt after loading is entered as such and not written. A member
that is itself a structure refers to that structure's table; an array is written
element by element. The encoding of a member is derived from its C++ type by the
compiler, not written into the table by hand; a pointer member without a stated
meaning does not compile.

**Encodings.** Values are little-endian and of fixed width whatever the build:
integers and enumerations in their declared width (8, 16 or 32 bits), `float` as
32 bits, `bool` as one byte, character arrays as their bytes, pointers as 32-bit
indexes or as strings according to their kind. Padding is not written.

**Schema in the file.** The file contains, once per structure, the table it was
written with: member names, encodings and counts. A reader matches the file's members
to its own by name. A member the file does not have keeps the value it has in a
freshly initialised structure; a member the reading build does not have is skipped.
The data itself carries no names. A change of meaning, as opposed to an added or
removed member, is what the version number in the `_VER` chunk is for.

**Around it.**

* The chunk structure and the order of chunks stay, as ADR-0006 requires. The version
  number changes, so retail saves are refused by the existing version check.
* The Ghoul2 instance data (`CGhoul2Info`, bones, bolts, surfaces) is written the same
  way.
* What is already written value by value (ICARUS, script variables, timers, cvars,
  config strings, arrays of integers) keeps its form; values that have no fixed width
  get one (the time of the save becomes a 64-bit integer).
* A chunk that is missing, too long or damaged ends the load with an error instead of
  being skipped silently, and a save is written under a temporary name and renamed
  when it is complete, so a failed save does not destroy the one it would have replaced.

### Consequences

* Good, because a save is independent of compiler, platform, pointer width and padding.
* Good, because adding or removing a member of a saved structure does not invalidate
  existing saves.
* Good, because the defects found by the survey disappear with the memory image.
* Good, because the tables are what a reader for retail saves would be built on.
* Bad, because the tables have to be written: well over a thousand members in
  `gentity_t`, `gclient_t` with `playerState_t` and `saberInfo_t`, `gNPC_t`,
  `Vehicle_t`, `level_locals_t` and the Ghoul2 types.
* Bad, because every later change to a saved structure needs its table changed with
  it. A member that is forgotten is not saved; the check below exists to catch that.
* Bad, because this is a larger piece of work than the rest of M2's save handling
  would otherwise be.

### Confirmation

* A unit test checks each table against its structure: the members named must cover
  the structure without gaps other than alignment padding and without overlaps, so a
  member that is added to a structure and not to its table fails the test.
* Unit tests round-trip structures filled with known values through the writer and the
  reader, including a file whose schema lacks a member and one that has an extra one.
* The play-test checklist: on the opening level and on a level with NPCs, save, quit,
  start, load; the state is the one that was saved, under the sanitizers. A retail save
  is refused with a message and no crash.

## Pros and Cons of the Options

### Field-by-field serialisation of every saved structure

* Good, because files are independent of the build and survive changes of the
  structures.
* Good, because it is the base a reader for retail saves would be built on.
* Bad, because the tables do not exist and have to be kept in step with the structures.
* Bad, because an error in a table that the checks do not catch shows only as wrong
  behaviour after a load.

### Memory image with correct pointer conversion and a layout record

The structures are written as they lie in memory, as now, with the pointer conversion
made correct for 8-byte pointers and a record of the structure sizes in the file, so
that a file from a different build is refused.

* Good, because least code, a few hundred lines, and the game structures and the save
  code keep their shape.
* Bad, because a save belongs to the build that wrote it: a change to any saved
  structure invalidates existing saves, and a save made on one platform is not promised
  to load on another.
* Bad, because the files still contain uninitialised padding.

### Field-by-field for the common structures, memory image for the rest

* Good, because the structures most likely to change get the robust treatment.
* Bad, because it has the costs of both: the large tables for the large structures,
  and files that are still tied to the build through the rest.

## More Information

The survey this record rests on was made at commit `8ef4641`.
