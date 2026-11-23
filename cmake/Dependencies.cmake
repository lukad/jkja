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
