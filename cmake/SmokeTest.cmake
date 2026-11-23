# Headless smoke test (docs/design.md, section 7): starts the engine without a window,
# mounts the retail game data, loads a map, runs frames and quits.
#
#   cmake -DJASP=<engine executable> -DWORK_DIR=<scratch directory> -DMAP=<map name>
#         -DFRAMES=<number> -P SmokeTest.cmake
#
# The game data is found through the environment variable JKA_GAMEDATA_DIR, the
# directory that contains base/ (docs/decisions/0007-game-data-location.md). Without it
# the test prints a line starting with SKIPPED, which ctest reports as "skipped".
# Nothing is written to the game data: the engine's write directory is in WORK_DIR.

if(NOT DEFINED ENV{JKA_GAMEDATA_DIR} OR "$ENV{JKA_GAMEDATA_DIR}" STREQUAL "")
  message("SKIPPED: JKA_GAMEDATA_DIR is not set")
  return()
endif()
set(gamedata "$ENV{JKA_GAMEDATA_DIR}")
if(NOT IS_DIRECTORY "${gamedata}/base")
  message(FATAL_ERROR "JKA_GAMEDATA_DIR does not name a directory that contains base/")
endif()

set(home "${WORK_DIR}/home")
file(REMOVE_RECURSE "${home}")
file(MAKE_DIRECTORY "${home}")

# The engine keeps its memory until the process ends, which a leak checker reports as
# leaks; every other finding of the sanitizers ends the run.
set(ENV{ASAN_OPTIONS} "detect_leaks=0:abort_on_error=0")
set(ENV{UBSAN_OPTIONS} "halt_on_error=1:print_stacktrace=1")

execute_process(
  COMMAND "${JASP}"
    +set fs_basepath "${gamedata}"
    +set fs_homepath "${home}"
    +map "${MAP}"
    +wait "${FRAMES}"
    +quit
  WORKING_DIRECTORY "${WORK_DIR}"
  RESULT_VARIABLE result
  OUTPUT_VARIABLE output
  ERROR_VARIABLE output
  TIMEOUT 600
)
message("${output}")

if(NOT result EQUAL 0)
  message(FATAL_ERROR "the engine ended with status ${result}")
endif()
