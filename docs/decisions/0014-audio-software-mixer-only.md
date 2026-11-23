---
status: accepted
date: 2026-11-23
decision-makers: Luka Dornhecker
---

# Audio: the engine's software mixer only; the OpenAL and EAX path is removed

## Context and Problem Statement

The sound code has two paths, chosen at run time by the cvar `s_UseOpenAL`:

* the software mixer inherited from Quake 3 (`snd_mix.cpp`, the painting half of
  `snd_dma.cpp`), which writes samples into a buffer that platform code hands to the
  sound device. It is the retail default (`s_UseOpenAL` is `0`);
* OpenAL with Creative's EAX 4 extensions for environmental reverb, about 330
  references in `snd_dma.cpp` alone, interleaved with the first path function by
  function.

The second path includes `eax/eax.h` and `eax/eaxman.h`, proprietary headers that were
removed in M0, and links against the EAX Manager, a Windows-only binary. `snd_dma.cpp`
therefore cannot compile, and M1 cannot finish without deciding what happens to that
path. The decision was planned for M2; the compile forces it now.

## Decision Drivers

* The sound code has to compile in M1 without proprietary headers.
* One audio path, the same on all three platforms.
* Retail behaviour in the default configuration is kept.
* Few dependencies.

## Considered Options

* Software mixer only, feeding an SDL3 audio stream; OpenAL and EAX removed
* Keep the OpenAL path on OpenAL Soft; remove only the EAX parts
* Postpone: make the OpenAL path compile behind stubs and decide in M2

## Decision Outcome

Chosen option: "software mixer only", because it is the path the retail game uses by
default, it needs nothing beyond the platform library the project already has, and the
other path cannot be kept as it is in any case.

* `s_UseOpenAL`, every branch that depends on it, the EAX environment code (reverb
  zones read from `.eal` files, the `s_eax*` debugging commands) and the headers in
  `src/client/openal` are deleted.
* The mixer's output goes to the device through `SNDDMA_*` in `src/sys`: a null device
  in M1, an SDL3 audio stream in M2 ([ADR-0005](0005-platform-layer-sdl3.md)).
* Game-visible sound behaviour (channels, attenuation, looping sounds, ambient sets,
  dynamic music, lip-sync data, MP3 streaming) stays with the mixer and is not changed.

### Consequences

* Good, because about a third of `snd_dma.cpp` goes away, with every `if` that picks a
  path.
* Good, because no audio library besides SDL3 is needed on any platform.
* Bad, because environmental reverb, which retail offered only on EAX-capable sound
  cards with the option switched on, is gone.
* Bad, because the software mixer's positional audio is plain stereo panning. If better
  spatialisation becomes a goal, it is a new decision; OpenAL Soft would then be
  introduced as a back end on purpose, not inherited.

### Confirmation

No file refers to OpenAL or EAX; the two EAX entries leave
`tools/known-missing-includes.txt`. In M2 the play-test checklist covers sound effects,
looping sounds, music transitions and voice with lip-sync.

## Pros and Cons of the Options

### Software mixer only

* Good, because the default retail path, already platform-neutral above `SNDDMA_*`.
* Good, because least code.
* Bad, because no reverb and no HRTF.

### OpenAL Soft without EAX

* Good, because better spatialisation, and OpenAL Soft's EFX extension could carry
  reverb later.
* Bad, because a second dependency and a second audio path to test on three platforms.
* Bad, because the existing OpenAL code is built around EAX 4 and the EAX Manager; with
  those removed, what remains still has to be reworked for EFX to offer anything the
  mixer does not.

### Postpone behind stubs

* Good, because M1 changes less.
* Bad, because it means writing stand-ins for two proprietary interfaces in order to
  compile code that the proposal for M2 already intends to delete.

## More Information

This record makes the "audio backend" decision listed for M2 in the design document.
