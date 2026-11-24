# Settings for the code inherited from the release (docs/design.md, section 4.3).
#
# Behaviour the code was written against is kept: plain char is signed, and objects are
# freely accessed through pointers of another type. The dangerous classes of warnings
# are errors; the rest is tightened over time.
function(jasp_legacy_target target)
  # What the version string calls the platform, as in the release: "win-x86",
  # "win-x86-debug".
  string(TOLOWER "${CMAKE_SYSTEM_NAME}" system)
  if(system STREQUAL "darwin")
    set(system macos)
  endif()
  target_compile_definitions(${target} PRIVATE
    "CPUSTRING=\"${system}-${JASP_ARCHITECTURE}$<$<CONFIG:Debug>:-debug>\""
    $<$<CONFIG:Debug>:_DEBUG>
    # What the release called its shipping configuration.
    $<$<NOT:$<CONFIG:Debug>>:FINAL_BUILD>
  )
  if(MSVC)
    target_compile_options(${target} PRIVATE
      /W3
      /we4311 /we4312 # pointer truncation
      /we4715 /we4716 # missing return
      /we4473 /we4477 # format string mismatches
      # String literals passed as char *, everywhere in the release. C++20 makes MSVC
      # refuse them; GCC and Clang warn, as MSVC did for the release.
      /Zc:strictStrings-
    )
  else()
    target_compile_options(${target} PRIVATE
      -fsigned-char
      -fno-strict-aliasing
      -Werror=return-type
      -Werror=format
      -Werror=int-to-pointer-cast
    )
  endif()
endfunction()
