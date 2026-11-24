# Windows x86-64 built on Linux with clang-cl and lld-link, against MSVC's C runtime,
# standard library and the Windows SDK; the programs run under Wine
# (docs/decisions/0022-windows-cross-build-on-linux.md). Used by the preset
# windows-cross-release.
#
# The libraries are Microsoft's and are fetched by each developer, who accepts
# Microsoft's licence in doing so, into a directory outside the working tree that the
# environment variable JASP_WINSYSROOT names:
#
#   cargo install xwin --version 0.10.0 --locked
#   xwin --accept-license --cache-dir <a directory outside the working tree> \
#     --crt-version 14.44.17.14 --sdk-version 10.0.26100 \
#     splat --use-winsysroot-style --output "$JASP_WINSYSROOT"
#
# MSVC 14.44 is Visual Studio 2022 17.14, the version on CI's Windows runner.

if(NOT DEFINED ENV{JASP_WINSYSROOT} OR "$ENV{JASP_WINSYSROOT}" STREQUAL "")
  message(FATAL_ERROR
    "JASP_WINSYSROOT is not set. It names the directory with MSVC's libraries and the "
    "Windows SDK; cmake/toolchains/windows-cross-clang-cl.cmake says how to fetch them.")
endif()
set(jasp_winsysroot "$ENV{JASP_WINSYSROOT}")
if(NOT IS_DIRECTORY "${jasp_winsysroot}/VC/Tools/MSVC" OR
   NOT IS_DIRECTORY "${jasp_winsysroot}/Windows Kits/10")
  message(FATAL_ERROR "JASP_WINSYSROOT does not hold MSVC's libraries and the Windows SDK")
endif()

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_VERSION 10.0)
set(CMAKE_SYSTEM_PROCESSOR AMD64)

set(CMAKE_C_COMPILER clang-cl)
set(CMAKE_CXX_COMPILER clang-cl)
set(CMAKE_LINKER lld-link)
set(CMAKE_RC_COMPILER llvm-rc)
set(CMAKE_MT llvm-mt)
set(CMAKE_AR llvm-lib)

set(jasp_target_flags "--target=x86_64-pc-windows-msvc -winsysroot \"${jasp_winsysroot}\"")
set(CMAKE_C_FLAGS_INIT "${jasp_target_flags}")
set(CMAKE_CXX_FLAGS_INIT "${jasp_target_flags}")

# xwin names the architecture's library directories x86_64 where Microsoft's layout,
# which /winsysroot expects, says x64; the linker is given them one by one.
file(GLOB jasp_msvc_lib LIST_DIRECTORIES true "${jasp_winsysroot}/VC/Tools/MSVC/*/lib/x86_64")
file(GLOB jasp_sdk_lib LIST_DIRECTORIES true "${jasp_winsysroot}/Windows Kits/10/Lib/*")
list(LENGTH jasp_msvc_lib jasp_msvc_count)
list(LENGTH jasp_sdk_lib jasp_sdk_count)
if(NOT jasp_msvc_count EQUAL 1 OR NOT jasp_sdk_count EQUAL 1)
  message(FATAL_ERROR "JASP_WINSYSROOT must hold exactly one MSVC and one SDK version")
endif()
set(jasp_link_flags
  "/libpath:\"${jasp_msvc_lib}\" /libpath:\"${jasp_sdk_lib}/um/x86_64\" /libpath:\"${jasp_sdk_lib}/ucrt/x86_64\"")
foreach(kind IN ITEMS EXE SHARED MODULE)
  set(CMAKE_${kind}_LINKER_FLAGS_INIT "${jasp_link_flags}")
endforeach()

# Only the release runtime is fetched; CMake's checks of the compiler build in Debug
# otherwise and would need the debug one.
set(CMAKE_TRY_COMPILE_CONFIGURATION RelWithDebInfo)

# Nothing of the Linux host is looked at for libraries or headers.
set(CMAKE_FIND_ROOT_PATH "${jasp_winsysroot}")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# Tests run the Windows programs under Wine, in a Wine prefix of the build's own, without
# the desktop's display (nothing the tests do needs a window), without Wine's dialog for
# a program that crashes (the crash is the test's result), and without Wine's own
# messages.
find_program(jasp_wine wine REQUIRED)
set(CMAKE_CROSSCOMPILING_EMULATOR
  "${CMAKE_COMMAND}" -E env
    "WINEPREFIX=${CMAKE_BINARY_DIR}/wine"
    "WINEDEBUG=-all"
    "WINEDLLOVERRIDES=winedbg.exe=d;mscoree=d;mshtml=d"
    --unset=DISPLAY --unset=WAYLAND_DISPLAY
    "${jasp_wine}"
)
