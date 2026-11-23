# External libraries (docs/decisions/0012-dependencies.md).
#
# Nothing is taken from the system. Each library is a release archive pinned by URL and
# SHA-256 hash; configuring downloads it, checks the hash and adds the library's own
# CMake project to this build, where it is compiled with the project's compiler and
# linked statically. There is no find_package() for a library anywhere in this project.
#
# Archives are kept in build/_deps and shared by all presets, so each is downloaded once
# per working tree. Single-header libraries are not listed here; they are committed
# under external/.
#
# To update a library: change the version in its URL and name, and replace the hash with
# the SHA-256 of the new archive.

include(FetchContent)

set(jasp_archive_dir "${PROJECT_SOURCE_DIR}/build/_deps")

# jasp_dependency(<name> <version> <url> <sha256>)
#
# The library's targets are built only when something links to them, and its headers
# are treated as system headers, so its warnings are its own business.
function(jasp_dependency name version url sha256)
  FetchContent_Declare(${name}
    URL "${url}"
    URL_HASH "SHA256=${sha256}"
    DOWNLOAD_DIR "${jasp_archive_dir}"
    DOWNLOAD_NAME "${name}-${version}.tar.gz"
    DOWNLOAD_EXTRACT_TIMESTAMP FALSE
    EXCLUDE_FROM_ALL
    SYSTEM
  )
  FetchContent_MakeAvailable(${name})
endfunction()

# Inflate for .pk3 archives, CRC-32. Target: miniz.
# Its CMake project asks for CMake 3.5, which current CMake versions report as deprecated
# when configuring; the report is harmless.
jasp_dependency(miniz 3.1.2
  "https://github.com/richgel999/miniz/archive/refs/tags/3.1.2.tar.gz"
  98468f8924934b723276680f85238b6c78bf1f8b49b4459cc9b7214a20e2e9fb
)

# Platform services (docs/decisions/0005-platform-layer-sdl3.md). Targets: SDL3::SDL3,
# and SDL3::Headers for code that only needs declarations.
# Static, and without the parts the project does not use.
set(SDL_SHARED OFF)
set(SDL_STATIC ON)
set(SDL_TEST_LIBRARY OFF)
set(SDL_TESTS OFF)
set(SDL_EXAMPLES OFF)
set(SDL_CAMERA OFF)
set(SDL_SENSOR OFF)
set(SDL_RENDER OFF)
jasp_dependency(SDL3 3.4.20
  "https://github.com/libsdl-org/SDL/releases/download/release-3.4.20/SDL3-3.4.20.tar.gz"
  f3bb06f55bed9557227a700af2ea62e49497ebdf58037732a731a5fb3ef8581f
)

# Unit test framework, for tests/ only. Targets: Catch2::Catch2, Catch2::Catch2WithMain.
if(BUILD_TESTING)
  jasp_dependency(Catch2 3.16.1
    "https://github.com/catchorg/Catch2/archive/refs/tags/v3.16.1.tar.gz"
    d16bd1b8b2364918bd472cb216394acb93bde6784890df0d91bccfb3f28d777d
  )
endif()
