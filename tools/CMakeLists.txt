# Repository tools (docs/decisions/0011-repository-tooling-in-cpp.md).

add_executable(check-repo check_repo.cpp repo_text.h)
jasp_strict_warnings(check-repo)

add_executable(fix-includes fix_includes.cpp repo_text.h)
jasp_strict_warnings(fix-includes)

if(BUILD_TESTING)
  # The rules every commit has to keep, applied to this repository.
  add_test(
    NAME check-repo
    COMMAND "${CMAKE_COMMAND}"
      "-DTOOL=$<TARGET_FILE:check-repo>"
      "-DTOOL_ARGS=--known-missing=tools/known-missing-includes.txt"
      -P "${PROJECT_SOURCE_DIR}/cmake/RunOnTrackedFiles.cmake"
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
  )

  # The tools themselves, applied to small trees with known defects.
  add_test(
    NAME tools-selftest
    COMMAND "${CMAKE_COMMAND}"
      "-DCHECK_REPO=$<TARGET_FILE:check-repo>"
      "-DFIX_INCLUDES=$<TARGET_FILE:fix-includes>"
      "-DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/selftest"
      -P "${CMAKE_CURRENT_SOURCE_DIR}/selftest.cmake"
  )
endif()
