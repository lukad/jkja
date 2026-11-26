# Checks the macOS application bundle (docs/decisions/0024-macos-application-bundle.md):
# its layout, its signature, and the minimum macOS version of the code in it.
#
#   cmake -DBUNDLE=<path of the .app> -DEXECUTABLE=<name> -DMODULE=<file name>
#         -DMINIMUM=<version> -P CheckBundle.cmake

foreach(variable IN ITEMS BUNDLE EXECUTABLE MODULE MINIMUM)
  if(NOT DEFINED ${variable})
    message(FATAL_ERROR "${variable} is not set")
  endif()
endforeach()

set(code "${BUNDLE}/Contents/MacOS/${EXECUTABLE}" "${BUNDLE}/Contents/MacOS/${MODULE}")
foreach(file IN ITEMS "${BUNDLE}/Contents/Info.plist" ${code})
  if(NOT EXISTS "${file}")
    message(FATAL_ERROR "The bundle has no ${file}")
  endif()
endforeach()

execute_process(
  COMMAND codesign --verify --strict --deep --verbose=2 "${BUNDLE}"
  RESULT_VARIABLE result
  OUTPUT_VARIABLE output
  ERROR_VARIABLE output
)
if(NOT result EQUAL 0)
  message(FATAL_ERROR "codesign does not accept the bundle:\n${output}")
endif()

foreach(file IN LISTS code)
  execute_process(
    COMMAND otool -l "${file}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE output
  )
  if(NOT result EQUAL 0 OR NOT output MATCHES "minos ([0-9.]+)")
    message(FATAL_ERROR "otool names no minimum macOS version for ${file}")
  endif()
  if(NOT CMAKE_MATCH_1 VERSION_EQUAL MINIMUM)
    message(FATAL_ERROR "${file} needs macOS ${CMAKE_MATCH_1}, not ${MINIMUM}")
  endif()
endforeach()

message(STATUS "bundle checked: layout, signature, macOS ${MINIMUM}")
