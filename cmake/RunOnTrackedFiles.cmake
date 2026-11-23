# Runs a repository tool with the list of tracked files on its standard input:
#
#   cmake -DTOOL=<executable> [-DTOOL_ARGS=<arguments>] -P RunOnTrackedFiles.cmake
#
# Run from the repository root. The equivalent in a shell is
#
#   git ls-files -z | <executable> <arguments>

if(NOT TOOL)
  message(FATAL_ERROR "TOOL is not set")
endif()
find_program(GIT_EXECUTABLE git REQUIRED)
separate_arguments(tool_args NATIVE_COMMAND "${TOOL_ARGS}")

execute_process(
  COMMAND "${GIT_EXECUTABLE}" ls-files -z
  COMMAND "${TOOL}" ${tool_args}
  RESULTS_VARIABLE results
)
foreach(result IN LISTS results)
  if(NOT result EQUAL 0)
    message(FATAL_ERROR "failed (exit status of git and of the tool: ${results})")
  endif()
endforeach()
