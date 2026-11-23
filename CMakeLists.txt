cmake_minimum_required(VERSION 3.28)

project(jasp
  VERSION 0.1.0
  DESCRIPTION "Jedi Academy single-player engine, ported to current platforms"
  LANGUAGES C CXX
)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
  message(FATAL_ERROR "In-source builds are not supported. Use a preset: cmake --preset <name>")
endif()

# Supported targets (docs/decisions/0003-target-platforms.md).
if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
  message(FATAL_ERROR "Only 64-bit targets are supported.")
endif()

# Toolchain baseline (docs/decisions/0004-build-system-and-toolchain.md).
set(jasp_minimum_GNU 13.0)
set(jasp_minimum_Clang 17.0)
set(jasp_minimum_AppleClang 15.0)
set(jasp_minimum_MSVC 19.38) # Visual Studio 2022 17.8
foreach(lang IN ITEMS C CXX)
  set(id "${CMAKE_${lang}_COMPILER_ID}")
  if(NOT DEFINED jasp_minimum_${id})
    message(FATAL_ERROR "Unsupported ${lang} compiler: ${id}. Supported: GCC, Clang, Apple Clang, MSVC.")
  elseif(CMAKE_${lang}_COMPILER_VERSION VERSION_LESS jasp_minimum_${id})
    message(FATAL_ERROR
      "${id} ${CMAKE_${lang}_COMPILER_VERSION} is too old; ${jasp_minimum_${id}} or newer is required.")
  endif()
endforeach()

# Defaults for every target defined below: C++20 and C11, no compiler extensions.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# No C++ modules are used; skip the dependency scan CMake would run for them.
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

# Nothing under src/ is built yet; the engine and game module targets arrive in M1
# (docs/design.md, section 8).

include(CTest)
include(cmake/StrictWarnings.cmake)

add_subdirectory(tools)
