# Warning level for new code: everything on, warnings are errors
# (docs/design.md, section 4.3). Legacy code does not use this.
function(jasp_strict_warnings target)
  if(MSVC)
    target_compile_options(${target} PRIVATE /W4 /WX /permissive-)
  else()
    target_compile_options(${target} PRIVATE
      -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Werror)
  endif()
endfunction()
