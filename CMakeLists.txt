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

# The architecture as it appears in the game module's file name
# (docs/decisions/0010-project-and-binary-names.md).
string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" jasp_processor)
if(CMAKE_OSX_ARCHITECTURES)
  string(TOLOWER "${CMAKE_OSX_ARCHITECTURES}" jasp_processor)
endif()
if(jasp_processor MATCHES "^(x86_64|amd64)$")
  set(JASP_ARCHITECTURE x86_64)
elseif(jasp_processor MATCHES "^(aarch64|arm64)$")
  set(JASP_ARCHITECTURE arm64)
else()
  message(FATAL_ERROR "Unsupported processor: ${CMAKE_SYSTEM_PROCESSOR}. Supported: x86-64, arm64.")
endif()

include(CTest)
include(cmake/StrictWarnings.cmake)
include(cmake/LegacyTarget.cmake)
include(cmake/Dependencies.cmake)

add_subdirectory(tools)
add_subdirectory(src)
if(BUILD_TESTING)
  add_subdirectory(tests)
endif()
