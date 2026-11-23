---
status: accepted
date: 2026-11-23
decision-makers: Luka Dornhecker
---

# Use Markdown Architectural Decision Records

## Context and Problem Statement

This project will take many decisions that are expensive to reverse: build system,
platform layer, graphics API, what to delete. Work is done across many sessions, partly
by AI agents that start each session without memory. Where are decisions kept so that
later work can find them and their reasons?

## Decision Drivers

* Decisions must be findable from the repository alone.
* The reasoning and the rejected options must survive, not only the outcome.
* The format must be light enough that it is actually used.

## Considered Options

* MADR 4.0.0
* Michael Nygard's original ADR template
* Decisions kept only in the design document
* No formal record; rely on commit messages

## Decision Outcome

Chosen option: "MADR 4.0.0", because it records the considered options and their pros
and cons in a fixed place, which is the part that gets lost otherwise, and it is plain
Markdown in the repository.

Records live in `docs/decisions/`, named `NNNN-title-with-dashes.md`, and are indexed in
`docs/decisions/README.md`.

### Consequences

* Good, because every later session can read why something is the way it is.
* Good, because the design document can stay short and point to records for the reasons.
* Bad, because there is a small cost per decision, and a temptation to skip it.

### Confirmation

The work loop in `CLAUDE.md` requires an accepted record before work that depends on an
architectural decision. The decision log is checked when a milestone is closed.

## Pros and Cons of the Options

### MADR 4.0.0

* Good, because options and trade-offs are part of the template.
* Good, because the front matter carries status and date in a machine-readable way.
* Neutral, because the full template is longer than most decisions need; optional
  sections may be left out.

### Nygard template

* Good, because it is very short.
* Bad, because it has no place for the options that were not chosen.

### Design document only

* Good, because there is one place to read.
* Bad, because the document would grow with history and stop being a description of the
  current design.

### Commit messages

* Bad, because decisions taken before any code exists have no commit to live in.

## More Information

* MADR: <https://adr.github.io/madr/>
