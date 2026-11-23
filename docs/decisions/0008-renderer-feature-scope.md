---
status: accepted
date: 2026-11-23
decision-makers: Luka Dornhecker
---

# Renderer scope: rasterised HDR, shadows, PBR; no ray tracing

## Context and Problem Statement

The project replaces the original fixed-function OpenGL renderer with "a modern one with
modern features". That phrase covers everything from bloom to full path tracing, and the
answer determines the graphics API, the hardware floor and the amount of new art data
needed. What is the new renderer for?

## Decision Drivers

* The result should run on ordinary GPUs on all three platforms, including integrated
  and Apple GPUs.
* The retail assets have diffuse textures and baked lightmaps only.
* The scope must be finishable.

## Considered Options

* Rasterised pipeline: HDR, dynamic shadows, physically based shading
* Hybrid: rasterised with ray-traced shadows or reflections
* Full path tracing
* Parity only: the original image on a modern API

## Decision Outcome

Chosen option: "Rasterised pipeline: HDR, dynamic shadows, physically based shading",
because that is what the project is meant to deliver, and it is achievable on all
three platforms without specialised hardware.

Ray tracing is out of scope. Nothing in the design has to keep the door open for it.

### Consequences

* Good, because the graphics API only needs a mainstream feature set, which keeps
  portable options open ([ADR-0009](0009-graphics-api.md)).
* Good, because the hardware floor stays low.
* Bad, because believable lighting has to be built from baked lightmaps plus dynamic
  lights, which needs care to avoid counting light twice (see `docs/design.md`,
  section 6.6).
* Bad, because full PBR needs material data the retail assets do not have; it has to
  come from derived defaults and optional replacement sets.

### Confirmation

Milestones M6 and M7 in `docs/design.md` define the features and their exit criteria.

## Pros and Cons of the Options

### Rasterised HDR, shadows, PBR

* Good, because runs everywhere and is well understood.
* Bad, because lighting quality is bounded by the baked data.

### Hybrid ray tracing

* Good, because better shadows and reflections.
* Bad, because it forces an API and hardware with ray-tracing support and adds a second
  scene representation.

### Path tracing

* Good, because it replaces the baked lighting outright.
* Bad, because it needs high-end GPUs, complete material data for every surface, and
  rules out the portable API options.

### Parity only

* Good, because smallest scope.
* Bad, because it does not deliver what the project is for. It is, however, the first
  renderer milestone (M5).
