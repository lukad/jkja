---
status: accepted
date: 2026-11-25
decision-makers: Luka Dornhecker
---

# Continuous integration runs on GitHub Actions' hosted runners, one job per preset, with each platform's build environment set up first

Supersedes [ADR-0021](0021-continuous-integration.md).

## Context and Problem Statement

[ADR-0021](0021-continuous-integration.md) put continuous integration on GitHub
Actions' hosted runners, one job per preset, each job being the three preset commands
and nothing else, with the compilers, CMake and Ninja from the runner's image. The first
runs showed that this is not enough on two platforms:

* On Linux, SDL refuses to configure without the development files of X11 or Wayland,
  and the Ubuntu image has neither. SDL is built by the project
  ([ADR-0012](0012-dependencies.md)), but against the headers of the system's display,
  audio and input libraries, which it loads at run time. The development machine has
  them installed; nothing has said so.
* On Windows, the presets use MSVC through Ninja and need the environment that Visual
  Studio's `vcvars64.bat` sets up, which a runner's shell does not have.

The question of ADR-0021 is asked again: what does a run do?

## Decision Drivers

* All drivers of [ADR-0021](0021-continuous-integration.md) hold: the repository is
  public on GitHub, all three targets are built with their own compilers, a run is the
  presets of the developer machine, no game data, pinned machines and actions, no
  secrets, little to maintain.
* The SDL built in CI should be the SDL a player's build has, with the same video,
  audio and input backends. A build without them tests a library nobody uses.
* On Linux, the libraries those headers belong to are the player's system's in any
  case: SDL loads them at run time, so their versions are not the project's to pin.

## Considered Options

For SDL's Linux build dependencies:

* Installed from the image's distribution, Ubuntu 24.04, before the build
* SDL configured for CI with no desktop backends (`SDL_UNIX_CONSOLE_BUILD`)
* A container image of the project's own with the dependencies in it

For MSVC's environment, there is one way: the job runs `vcvars64.bat`, found by
`vswhere`, before the preset commands.

## Decision Outcome

Chosen option: "installed from the image's distribution", because it is the only one
that builds the same SDL as a player's machine without a second configuration of the
build or an image to maintain.

Everything [ADR-0021](0021-continuous-integration.md) decided stands, except what a job
does:

* **Where:** one workflow, `.github/workflows/ci.yml`, run on every push to `main` and
  on every pull request.
* **What:** one job per configure preset the runner's platform can use: on Linux
  `linux-debug`, `linux-clang-debug`, `linux-sanitize`, `linux-release` and
  `linux-sanitize-release`; on Windows and macOS their debug and release presets. A job
  sets up its platform's build environment, then runs the three preset commands. Every
  job runs to its end even when another fails.
* **Build environment:** on Linux, the development packages SDL lists for Ubuntu in its
  `docs/README-linux.md`, installed with `apt-get` from the image's distribution, less
  the build tools the image already has and the packages for SDL's own tests. The list
  is written in the workflow. On Windows, MSVC's environment from `vcvars64.bat`. On
  macOS, nothing.
* **Machines:** GitHub's runner images, named by their version (`ubuntu-24.04`,
  `windows-2025`, `macos-15` at the time of writing), never by a `-latest` label. Moving
  to a newer image is a commit of its own. Compilers, CMake and Ninja come from the
  image; the build's own checks of the minimum versions catch an image that is too old.
* **Actions:** only GitHub's own (`actions/checkout`, `actions/cache`), each pinned by
  the full hash of its commit, with the version as a comment.
* **Downloads:** the archives the build fetches are cached between runs, keyed by the
  hash of `cmake/Dependencies.cmake`.
* **Game data:** none. The smoke tests report themselves as skipped.
* **Permissions:** the workflow's token can read the repository and nothing more. No
  secrets exist.
* **Results:** CI checks each change after it lands on `main`; it is not a gate. A red
  run is fixed before other work continues.

The development machine needs the same packages, under its distribution's names. That
is a prerequisite of every Linux build of the project, like the compiler, and is stated
where building is described.

### Consequences

* Good, because the Linux jobs build SDL with X11, Wayland, the audio servers and the
  input backends, as a player's build has them.
* Good, because a Linux machine missing a package fails as CI did, with SDL's message
  naming what is missing.
* Bad, because the packages' versions are those of Ubuntu 24.04's archive at the time
  of the run, not pinned. Their headers change rarely within a release of Ubuntu, and
  the libraries are the player's system's anyway.
* Bad, because each Linux job spends some time installing packages.
* All consequences of [ADR-0021](0021-continuous-integration.md) otherwise hold.

### Confirmation

* `.github/workflows/ci.yml` exists, its jobs are the presets listed above, and each
  job's steps are its platform's set-up followed by the three preset commands.
* Every job of a run is green, with the smoke tests skipped.
* No job names a `-latest` image, and no `uses:` line has a tag in place of a commit hash.

## Pros and Cons of the Options

### Installed from the image's distribution

* Good, because SDL is built as on a player's machine.
* Good, because the list is SDL's own, for the distribution the image runs.
* Bad, because the versions are the archive's, not the project's.

### SDL with no desktop backends in CI

* Good, because nothing is installed, and the build stays the three commands.
* Bad, because it is a second configuration of SDL that only CI uses, and it hides
  every defect of the backends a player uses.
* Bad, because the switch has to be passed to the build in CI only, which is a second
  way to build.

### A container image of the project's own

* Good, because the packages would be pinned with the image.
* Bad, because the image has to be built, published and kept up to date, outside the
  repository's presets.
* Bad, because it covers Linux only.

## More Information

* SDL's list: `docs/README-linux.md` in the SDL release that `cmake/Dependencies.cmake`
  pins.
* The first CI run, of commit `a3bf4c7`, failed on Linux with SDL's message "SDL could
  not find X11 or Wayland development libraries on your system."
