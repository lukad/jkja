---
status: superseded by ADR-0023
date: 2026-11-24
decision-makers: Luka Dornhecker
---

# Continuous integration runs on GitHub Actions' hosted runners, one job per preset

## Context and Problem Statement

The project is developed and checked on one Linux x86-64 machine. Windows x86-64 and
macOS arm64 are targets as well ([ADR-0003](0003-target-platforms.md)), and nothing
builds there today: the Windows and macOS presets exist but have never been run. The
design document requires that continuous integration builds all three platforms and
runs everything that does not need game data (section 7), and lists the provider as a
decision for M3 (section 9). Which service runs the builds, on which machines, and what
does a run do?

## Decision Drivers

* The repository is to be hosted publicly on GitHub.
* All three targets need a real build with their own compiler: MSVC on Windows, Apple
  Clang on macOS arm64, GCC and Clang on Linux. No Windows or macOS machine is
  available to the project.
* A run is the same three commands as on the developer machine (`cmake --preset`,
  `cmake --build --preset`, `ctest --preset`); CI adds no second way to build
  ([ADR-0004](0004-build-system-and-toolchain.md)).
* No game data ever reaches CI ([ADR-0007](0007-game-data-location.md)); the tests that
  need it are skipped there, as they are on any machine without `JKA_GAMEDATA_DIR`.
* What a run uses is pinned, as the libraries are ([ADR-0012](0012-dependencies.md)): a
  run of an old commit should see the same machine, and a change of machine is a
  commit.
* The project's code and its configuration are public; nothing in CI may need a secret.
* Little to maintain.

## Considered Options

* GitHub Actions with GitHub's hosted runners
* GitHub Actions with runners of the project's own
* Linux only, with Windows cross-compiled and macOS left out
* No CI; Windows and macOS built by hand on borrowed machines

## Decision Outcome

Chosen option: "GitHub Actions with GitHub's hosted runners", because it is the only one
that builds all three targets with their native compilers on every change without a
machine of the project's own, and for a public repository its runners cost nothing.

* **Where:** one workflow, `.github/workflows/ci.yml`. It runs on every push to `main`
  and on every pull request.
* **What:** one job per configure preset the runner's platform can use: on Linux
  `linux-debug`, `linux-clang-debug`, `linux-sanitize`, `linux-release` and
  `linux-sanitize-release`; on Windows and macOS their debug and release presets. A job
  is the three preset commands and nothing else. Every job runs to its end even when
  another fails.
* **Machines:** GitHub's runner images, named by their version (`ubuntu-24.04`,
  `windows-2025`, `macos-15` at the time of writing), never by a `-latest` label. Moving
  to a newer image is a commit of its own. Compilers, CMake and Ninja come from the
  image; the build's own checks of the minimum versions
  ([ADR-0004](0004-build-system-and-toolchain.md)) catch an image that is too old. The
  Linux image's compilers are older than the developer machine's and near those
  minimums, so CI is also where the minimums are tested.
* **Actions:** only GitHub's own (`actions/checkout`, `actions/cache`), each pinned by
  the full hash of its commit, with the version as a comment.
* **Downloads:** the archives the build fetches are cached between runs, keyed by the
  hash of `cmake/Dependencies.cmake`. A changed pin is a new key, so the cache never
  serves a library that is no longer pinned.
* **Game data:** none. The smoke tests report themselves as skipped, which is not a
  failure. They run on the developer machine, as before.
* **Permissions:** the workflow's token can read the repository and nothing more. No
  secrets exist.
* **Results:** CI checks each change after it lands on `main`; it is not a gate that
  commits wait on. A red run is fixed before other work continues, as a failing local
  check is.

### Consequences

* Good, because every change is built by MSVC and Apple Clang without a Windows or Mac
  machine in the project.
* Good, because a run is the same presets as on the developer machine. A failure in CI
  is reproduced by running the same preset locally, when it is a Linux one.
* Good, because the Linux jobs cover the declared minimum compiler versions, which the
  developer machine does not.
* Neutral, because the debug and release presets of the three platforms, five on Linux,
  make nine jobs on every push. For a public repository these cost nothing, and they run
  in parallel.
* Bad, because a defect that only Windows or macOS shows at run time is not found:
  without game data, CI runs only the repository checks, the tools' self-test and the
  unit tests there. Running the game on those platforms stays a manual exit criterion
  ([design document](../design.md), section 10).
* Bad, because the project depends on GitHub's runner images. An image that is retired
  forces a move to its successor.
* Bad, because a failure on Windows or macOS can only be investigated through CI runs,
  one push per attempt.

### Confirmation

* `.github/workflows/ci.yml` exists, and its jobs are the presets listed above.
* The first run after the workflow lands has every job green, with the smoke tests
  skipped. The Windows and macOS jobs are expected to fail at first, since those presets
  have never been run; making them pass is the M3 work they are there to show.
* No job names a `-latest` image, and no `uses:` line has a tag in place of a commit hash.

## Pros and Cons of the Options

### GitHub Actions with GitHub's hosted runners

* Good, because Linux x86-64, Windows x86-64 and macOS arm64 runners are all hosted.
* Good, because public repositories use them without charge.
* Good, because nothing has to be set up or kept running outside the repository.
* Bad, because the images change under their version label as GitHub updates them;
  pinning the version keeps the operating system, not every tool on it.

### GitHub Actions with runners of the project's own

* Good, because the machines are fully under the project's control, and game data could
  be present for the smoke tests.
* Bad, because it needs a Windows and a Mac machine, which the project does not have.
* Bad, because a runner of its own for a public repository runs code from any pull
  request on that machine.
* Bad, because game data on a CI machine invites reports and artifacts that contain its
  path or what was rendered from it.

### Linux only, with Windows cross-compiled

* Good, because one kind of machine, the same as the developer's.
* Bad, because a cross build with MinGW or with clang-cl against downloaded Windows
  headers is not the MSVC build that ADR-0004 names, and needs a toolchain set up that
  no developer uses.
* Bad, because macOS arm64 is not built at all, and Apple's toolchain does not run on
  Linux.

### No CI

* Good, because nothing to maintain.
* Bad, because Windows and macOS break silently between the occasions someone builds
  there, which is the risk section 10 of the design document names.

## More Information

* GitHub-hosted runners and their labels:
  <https://docs.github.com/en/actions/reference/runners/github-hosted-runners>;
  the contents of each image: <https://github.com/actions/runner-images>.
* To be revisited if the repository moves away from GitHub, becomes private (hosted
  runners are then charged by the minute, macOS ten times over), or if tests with game
  data are to run in CI.
