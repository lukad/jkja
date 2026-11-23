# Unit tests (docs/design.md, section 7).

# The engine sources under test, built as the engine builds them, together with
# stand-ins for the engine services they call.
add_library(jasp-test-subjects STATIC
  ../src/game/q_math.cpp
  ../src/game/q_shared.cpp
  ../src/qcommon/unzip.cpp
  ../src/renderer/tr_stb.cpp
  engine_stubs.cpp
)
target_compile_definitions(jasp-test-subjects PUBLIC _JK2EXE MINIZ_NO_ZLIB_COMPATIBLE_NAMES)
target_link_libraries(jasp-test-subjects PUBLIC miniz)
jasp_legacy_target(jasp-test-subjects)

# The field tables of the game module, built as the game module builds them.
add_library(jasp-test-game-tables STATIC
  ../src/game/g_savetables.cpp
)
target_include_directories(jasp-test-game-tables PRIVATE "$<TARGET_PROPERTY:jagame,BINARY_DIR>")
add_dependencies(jasp-test-game-tables jagame)
jasp_legacy_target(jasp-test-game-tables)

# The tests include headers of the legacy code, so their warnings are not errors.
add_executable(jasp-tests
  ../src/qcommon/sg_fields.cpp
  test_images.cpp
  test_savefields.cpp
  test_savetables.cpp
  test_shared.cpp
  test_zip.cpp
)
target_compile_definitions(jasp-tests PRIVATE
  "JASP_TEST_SCRATCH_DIR=\"${CMAKE_CURRENT_BINARY_DIR}\""
)
target_link_libraries(jasp-tests PRIVATE
  jasp-test-subjects jasp-test-game-tables Catch2::Catch2WithMain
)
jasp_legacy_target(jasp-tests)
if(MSVC)
  target_compile_options(jasp-tests PRIVATE /W4)
else()
  target_compile_options(jasp-tests PRIVATE -Wall -Wextra -Wpedantic -Wshadow)
endif()

# One ctest entry per test case.
FetchContent_GetProperties(Catch2 SOURCE_DIR catch2_source_dir)
include("${catch2_source_dir}/extras/Catch.cmake")
catch_discover_tests(jasp-tests)
