# Decision log

Architectural decisions for this project, recorded as
[MADR](https://adr.github.io/madr/) 4.0.0 records
([ADR-0000](0000-use-markdown-architectural-decision-records.md)).

| # | Decision | Status |
|---|---|---|
| [0000](0000-use-markdown-architectural-decision-records.md) | Use Markdown Architectural Decision Records | accepted |
| [0001](0001-start-from-the-raven-source-release.md) | Start from the Raven source release, not from OpenJK | accepted |
| [0002](0002-single-player-first.md) | Single-player first; multiplayer tree leaves the working tree | accepted |
| [0003](0003-target-platforms.md) | Target platforms: Linux, Windows, macOS; 64-bit only | accepted |
| [0004](0004-build-system-and-toolchain.md) | CMake presets with Ninja, C++20, no IDE project files | accepted |
| [0005](0005-platform-layer-sdl3.md) | SDL3 as the only platform layer | accepted |
| [0006](0006-drop-retail-savegame-compatibility.md) | Drop retail savegame compatibility | accepted |
| [0007](0007-game-data-location.md) | Game data location is run-time configuration | accepted |
| [0008](0008-renderer-feature-scope.md) | Renderer scope: rasterised HDR, shadows, PBR; no ray tracing | accepted |
| [0009](0009-graphics-api.md) | Graphics API: SDL3 GPU | accepted |
| [0010](0010-project-and-binary-names.md) | Project name `jasp`; binaries keep their original names | accepted |
| [0011](0011-repository-tooling-in-cpp.md) | Repository tooling is written in C++ and built by the project | accepted |

## How to add a decision

1. Copy [adr-template.md](adr-template.md) to `NNNN-title-with-dashes.md`, using the next
   free number.
2. Fill it in. List the options that were really considered, including the ones that
   lost, and say why they lost.
3. Set `status: proposed` and add a row to the table above.
4. The record is accepted or rejected. Only then is the status changed, and only then is
   work started that depends on it.

## Rules

- **Accepted records are not edited**, apart from the status line. A decision that
  changes gets a new record; the old one is marked `superseded by ADR-NNNN`.
- A record is `accepted` only once acceptance has been stated explicitly. A
  recommendation is `proposed`, however obvious it seems.
- One decision per record.
- Decisions still to be made are listed in [design.md, section 9](../design.md#9-decisions).
