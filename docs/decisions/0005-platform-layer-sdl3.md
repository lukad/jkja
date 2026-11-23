---
status: accepted
date: 2026-11-23
decision-makers: Luka Dornhecker
---

# SDL3 as the only platform layer

## Context and Problem Statement

The engine reaches the operating system through `code/win32` (35.9k lines: Win32
windowing, WGL, DirectInput, DirectSound, plus Xbox code), with stale Unix and Mac
Classic directories beside it. Three operating systems need to be supported. How does
the engine get a window, input, a sound device, timers and shared-library loading?

## Decision Drivers

* One implementation for Linux, Windows and macOS, not three.
* Platform code confined to one directory.
* Few dependencies; nothing exotic to build.
* Must not constrain the choice of graphics API.

## Considered Options

* SDL3
* SDL2
* GLFW plus separate audio and input libraries
* Native code per platform (Win32, Cocoa, Wayland/X11)

## Decision Outcome

Chosen option: "SDL3", because it covers every platform service the engine needs in one
maintained library, on all three targets, and it supports every graphics API under
consideration (OpenGL contexts for the legacy renderer, Vulkan and Metal surfaces, and
its own GPU API).

`src/sys` is the only directory that includes SDL headers, with one exception: the new
renderer's back end, which uses the SDL3 GPU API ([ADR-0009](0009-graphics-api.md)).

Scope: window and display modes, keyboard, mouse and gamepad, events, timers, threads,
shared-object loading, clipboard, per-user paths, audio device output. Whether SDL3 also
carries audio *mixing* duties or the engine's mixer is kept is decided separately before
M2.

### Consequences

* Good, because `code/win32`, `code/unix`, `code/mac` and `code/null` are replaced by one
  small directory.
* Good, because Wayland, high-DPI, raw mouse input and gamepads come without
  platform-specific code.
* Bad, because SDL3 becomes a hard dependency on every platform, and must be fetched and
  built where the system does not provide it.
* Bad, because behaviour that the Win32 code got from the OS for free (window focus
  rules, key repeat, text input) has to be re-established on SDL3's event model.

### Confirmation

After M2, no file outside `src/sys` contains an operating-system header or an `#ifdef`
for an operating system. The repository check enforces this.

## Pros and Cons of the Options

### SDL3

* Good, because one library for all platform services.
* Good, because current major version with active development.
* Good, because installed on the development machine (3.4).
* Neutral, because younger than SDL2; its API has been stable since early 2025.

### SDL2

* Good, because extremely well proven with this engine family.
* Bad, because it is in maintenance; starting a new project on it means a migration
  later, and it has no GPU API.

### GLFW plus others

* Good, because GLFW is small.
* Bad, because audio, gamepads and library loading need further libraries; more
  dependencies for the same result.

### Native per platform

* Good, because no dependency.
* Bad, because three implementations to write and keep working, two of which cannot be
  run on the development machine.
