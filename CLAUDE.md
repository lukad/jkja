# CLAUDE.md

Port of Raven's 2013 *Jedi Academy* source release to current toolchains on Linux, macOS
and Windows, followed by a new renderer (rasterised HDR, shadows, PBR).

## Sources of truth

Read these before doing anything; they outrank assumptions and prior knowledge.

1. [docs/design.md](docs/design.md): what is being built, in what order, and how it is
   verified. Section 8 is the roadmap and shows the current milestone.
2. [docs/decisions/](docs/decisions/README.md): the decision log (MADR). An accepted
   record overrides the design document.

The unmodified original is the import commit `44e8ea1`, tagged `raven-2013`. Use
`git show raven-2013:<path>` to see what any file looked like, including trees that
have since been removed.

## Work loop

Every piece of work goes through these steps, in order.

1. **Orient.** Open the roadmap in `docs/design.md`, section 8. Find the current
   milestone and the next unticked item. Read the decision records that touch it.
2. **Check for a missing decision.** If the work needs an architectural choice that no
   *accepted* record covers (a dependency, an interface, a file format, a directory
   move, deleting a subsystem), stop. Write a record from
   `docs/decisions/adr-template.md` with `status: proposed`, add it to the index, and
   ask for it to be accepted or rejected. Do not build on a proposed record. The one exception
   is a spike whose stated purpose is to confirm a proposed record.
3. **Plan the smallest step.** One logical change. If it contains a mechanical
   transformation (scripted rename, line endings, stripping `#ifdef` blocks), that
   transformation is a step of its own.
4. **Change.** Follow the rules below.
5. **Verify.** Run every check in "Verification" that exists at the current milestone.
   Fix what fails before going on.
6. **Record.** Tick the roadmap item if it is finished. If the work showed that the
   design document is wrong, fix the document in the same change; if that alters a
   decision, it needs a new record first (step 2).
7. **Report.** Say what changed, what was verified and how, and what was *not* verified.
   Run-time behaviour on macOS and Windows cannot be checked from this machine; say so
   whenever it applies.
8. **Commit** only when the go-ahead has been given. Never push unasked.

## Rules

### Repository hygiene

- **No game data, and no trace of it.** Never write the location of the retail game data
  into a tracked file. Never commit anything extracted from or rendered with it:
  no assets, no screenshots, no reference images, no logs containing its path.
- Tooling gets the location from the environment variable `JKA_GAMEDATA_DIR` (the
  directory that contains `base/`). Tests that need it are skipped when it is unset.
  Treat that directory as read-only.
- No binaries, no IDE project files (`.sln`, `.vcproj`, `.vcxproj`, `.dsp`, `.dsw`), no
  copied-in library sources other than single-header libraries.
- One build system: CMake presets with Ninja. Do not add a second way to build.
- Repository tooling is C++ under `tools/`, built by the project
  ([ADR-0011](docs/decisions/0011-repository-tooling-in-cpp.md)). No scripts in other
  languages; CMake script mode only as glue.

### Code

- **Delete, don't disable.** Out-of-scope code is removed, not wrapped in `#if 0` or a
  new `#ifdef`. The original is always available at the import commit.
- **Platform code lives in `src/sys` only.** No operating-system headers and no
  operating-system `#ifdef`s anywhere else.
- **Graphics API calls live in the renderer's back end only.**
- **Port, don't improve.** Game logic keeps its behaviour. Do not refactor, rename or
  reformat legacy code beyond what the task needs; small diffs against the original are
  a feature.
- **Fix pointer-size errors by fixing the type**, never with a wider cast that silences
  the compiler.
- New code is C++20, formatted with the repository's `.clang-format`, and builds without
  warnings.
- A header that lives in this repository is included in quotes, by its path relative to
  the including file, with forward slashes and the exact letter case of the file on disk.
  After moving or renaming sources, run
  `git ls-files -z | build/<preset>/tools/fix-includes --apply`.

### Commits

- One logical change per commit.
- A mechanical commit contains only the output of its script or command, and its message
  contains that script or command, so the commit can be checked by re-running it.
- Subject line in the imperative, starting in lower case, under about 70 characters;
  the body says why.

### Decisions

- Accepted records are not edited apart from their status line. A changed decision is a
  new record that supersedes the old one.
- A record is `accepted` only once acceptance has been stated explicitly.
- Documents are written impersonally. Requirements and wishes are stated in the passive
  ("support for X is required"), never attributed to a person ("the owner wants X").

## Verification

Checks come into existence milestone by milestone; run all that exist.

| Check | Exists from | Command |
|---|---|---|
| Repository checks (includes, forbidden files, no data paths) | M0 | part of `ctest`; the tool is `build/<preset>/tools/check-repo` |
| Configure and build | M0 | `cmake --preset <name>` then `cmake --build --preset <name>` |
| Tests | M0 | `ctest --preset <name>`; runs the repository checks and the tools' self-test, and from M1 the unit tests |
| Sanitizer build and tests | M1 | the `linux-sanitize` preset, same commands |
| Headless smoke test (needs `JKA_GAMEDATA_DIR`) | M1 | part of `ctest` |
| Play-test checklist | M2 | manual; listed in `docs/design.md`, section 7 |
| Image comparison, new renderer against legacy | M4 | part of `ctest`, needs game data |

Presets on Linux: `linux-debug` (GCC), `linux-clang-debug`, `linux-sanitize`,
`linux-release`; `cmake --list-presets` shows what the current machine can use. The
Windows and macOS presets have not been run yet.

From M1 the build produces the engine (`jasp`), the game module (`jagame` plus the
architecture), the unit tests (`tests/`, Catch2) and the repository tools. The smoke
test is `smoke-load-map`; `ctest` reports it as skipped unless `JKA_GAMEDATA_DIR` is set
in the environment. If a script or preset name changes, update this table in the same
change.

"Verified" means the check was run and passed in this session. Anything else is
reported as not verified.

## Reference points

- The retail executable runs under Wine and answers "what did the original do?".
- OpenJK may be consulted to learn what breaks on modern systems. Its code is not
  imported ([ADR-0001](docs/decisions/0001-start-from-the-raven-source-release.md)).
