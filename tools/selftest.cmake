# Self-test of check-repo and fix-includes.
#
#   cmake -DCHECK_REPO=<exe> -DFIX_INCLUDES=<exe> -DWORK_DIR=<dir> [-DEMULATOR=<command>]
#         -P selftest.cmake
#
# Builds two small trees in WORK_DIR. The first has one instance of every defect
# check-repo knows; each must be reported. The second has includes that fix-includes
# can repair; after the repair, check-repo must pass and the files must read as
# expected.

foreach(required IN ITEMS CHECK_REPO FIX_INCLUDES WORK_DIR)
  if(NOT ${required})
    message(FATAL_ERROR "${required} is not set")
  endif()
endforeach()

# Writes its arguments, joined, to a file, byte for byte. file(WRITE) writes in text mode
# on Windows and turns every \n into \r\n; the trees below need the bytes they state.
# The arguments are taken one by one, because a ";" in one would split ARGN.
function(write_exact path)
  set(content "")
  math(EXPR last "${ARGC} - 1")
  foreach(i RANGE 1 ${last})
    string(APPEND content "${ARGV${i}}")
  endforeach()
  file(CONFIGURE OUTPUT "${path}" CONTENT "${content}" @ONLY NEWLINE_STYLE UNIX)
endfunction()

# Runs a tool in a tree, with the tree's file list on standard input.
function(run_tool tree out_status out_text)
  execute_process(
    COMMAND "${CMAKE_COMMAND}" -E cat "${tree}/files.txt"
    COMMAND ${EMULATOR} ${ARGN}
    WORKING_DIRECTORY "${tree}"
    RESULT_VARIABLE status
    OUTPUT_VARIABLE text
    ERROR_VARIABLE errors
  )
  set(${out_status} "${status}" PARENT_SCOPE)
  set(${out_text} "${text}${errors}" PARENT_SCOPE)
endfunction()

function(expect_contains text needle)
  string(FIND "${text}" "${needle}" at)
  if(at EQUAL -1)
    message(FATAL_ERROR "expected output to contain:\n  ${needle}\nbut the output was:\n${text}")
  endif()
endfunction()

function(write_file_list tree)
  list(JOIN ARGN "\n" lines)
  write_exact("${tree}/files.txt" "${lines}\n")
endfunction()

file(REMOVE_RECURSE "${WORK_DIR}")

# --- A tree with every defect ----------------------------------------------------

set(bad "${WORK_DIR}/defects")
# Assembled from pieces so that this file does not contain a machine-specific path.
string(CONCAT machine_path "/ho" "me/someone/games")
write_exact("${bad}/src/a/one.h" "// one\n")
write_exact("${bad}/src/b/two.h" "// two\n")
write_exact("${bad}/src/Mixed/three.h" "// three\n")
write_exact("${bad}/project.sln" "\n")
write_exact("${bad}/notes.txt" "data is in ${machine_path}\n")
# The one file with CR LF line endings, which write_exact would make LF.
file(CONFIGURE OUTPUT "${bad}/crlf.txt" CONTENT "line\n" @ONLY NEWLINE_STYLE CRLF)
write_exact("${bad}/src/a/main.cpp"
  "#include \"ONE.h\"\n"
  "#include \"..\\b\\two.h\"\n"
  "#include \"missing.h\"\n"
  "#include <../b/two.h>\n"
  "#include \"../a/one.h\"\n"
  "#include \"tolerated.h\"\n"
  "// #include \"in_a_line_comment.h\"\n"
  "/* #include \"in_a_block_comment.h\" */\n"
  "#include <stdio.h>\n"
)
write_exact("${bad}/known-missing.txt"
  "src/a/main.cpp tolerated.h  # tolerated\n"
  "src/a/main.cpp stale.h      # no longer included\n"
)
write_file_list("${bad}"
  src/a/one.h src/b/two.h src/Mixed/three.h project.sln notes.txt crlf.txt
  src/a/main.cpp known-missing.txt)

run_tool("${bad}" status text "${CHECK_REPO}" --known-missing=known-missing.txt)
if(NOT status EQUAL 1)
  message(FATAL_ERROR "check-repo should exit with 1 on the defect tree, got ${status}:\n${text}")
endif()
expect_contains("${text}" "project.sln: forbidden file type")
expect_contains("${text}" "notes.txt:1: contains a machine-specific path")
expect_contains("${text}" "crlf.txt: contains CR characters")
expect_contains("${text}" "src/Mixed/three.h: directories under src/ must have lower-case names")
expect_contains("${text}" "src/a/main.cpp:1: include has the wrong letter case: ONE.h")
expect_contains("${text}" "src/a/main.cpp:2: include uses backslashes: ..\\b\\two.h")
expect_contains("${text}" "src/a/main.cpp:3: quoted include names nothing in the repository: missing.h")
expect_contains("${text}" "src/a/main.cpp:4: header from this repository in angle brackets: ../b/two.h")
expect_contains("${text}" "src/a/main.cpp:5: include should be written \"one.h\": ../a/one.h")
expect_contains("${text}" "src/a/main.cpp: known-missing entry is no longer needed: stale.h")
expect_contains("${text}" "10 problem(s)")

# --- A tree that fix-includes can repair -----------------------------------------

set(fixable "${WORK_DIR}/fixable")
write_exact("${fixable}/src/a/one.h" "// one\n")
write_exact("${fixable}/src/b/Two.h" "// two\n")
write_exact("${fixable}/src/b/only_here.h" "// only here\n")
write_exact("${fixable}/src/a/main.cpp"
  "#include \"ONE.h\"\n"
  "\t#include \"..\\B\\two.h\"  // trailing text stays\n"
  "#include \"b/two.h\"\n"
  "#include \"only_here.h\"\n"
  "#include \"../a/one.h\"\n"
  "#include <stdio.h>\n"
  "#include \"not_in_tree.h\"\n"
  "int unchanged;\n"
)
# A last line without a newline gets none. file(WRITE) is enough for it: with no newline
# in the text, there is nothing for Windows to translate.
file(WRITE "${fixable}/src/a/last.cpp" "#include \"ONE.h\"")
write_file_list("${fixable}"
  src/a/one.h src/b/Two.h src/b/only_here.h src/a/main.cpp src/a/last.cpp)

run_tool("${fixable}" status text "${FIX_INCLUDES}")
if(NOT status EQUAL 0)
  message(FATAL_ERROR "fix-includes dry run failed with ${status}:\n${text}")
endif()
expect_contains("${text}" "would rewrite 6 include lines in 2 files")
file(READ "${fixable}/src/a/main.cpp" content)
expect_contains("${content}" "#include \"ONE.h\"")

run_tool("${fixable}" status text "${FIX_INCLUDES}" --apply)
if(NOT status EQUAL 0)
  message(FATAL_ERROR "fix-includes --apply failed with ${status}:\n${text}")
endif()
expect_contains("${text}" "rewrote 6 include lines in 2 files")

string(CONCAT expected
  "#include \"one.h\"\n"
  "\t#include \"../b/Two.h\"  // trailing text stays\n"
  "#include \"../b/Two.h\"\n"
  "#include \"../b/only_here.h\"\n"
  "#include \"one.h\"\n"
  "#include <stdio.h>\n"
  "#include \"not_in_tree.h\"\n"
  "int unchanged;\n"
)
file(READ "${fixable}/src/a/main.cpp" content)
if(NOT content STREQUAL expected)
  message(FATAL_ERROR "fix-includes produced:\n${content}\nexpected:\n${expected}")
endif()
file(READ "${fixable}/src/a/last.cpp" content)
if(NOT content STREQUAL "#include \"one.h\"")
  message(FATAL_ERROR "fix-includes produced for last.cpp:\n${content}")
endif()

write_exact("${fixable}/known-missing.txt" "src/a/main.cpp not_in_tree.h  # tolerated\n")
run_tool("${fixable}" status text "${CHECK_REPO}" --known-missing=known-missing.txt)
if(NOT status EQUAL 0)
  message(FATAL_ERROR "check-repo should pass on the repaired tree, got ${status}:\n${text}")
endif()
expect_contains("${text}" "repository checks passed (5 files)")

# --- An include that cannot be decided -------------------------------------------

set(ambiguous "${WORK_DIR}/ambiguous")
write_exact("${ambiguous}/src/a/same.h" "// a\n")
write_exact("${ambiguous}/src/b/same.h" "// b\n")
write_exact("${ambiguous}/src/c/main.cpp" "#include \"same.h\"\n")
write_file_list("${ambiguous}" src/a/same.h src/b/same.h src/c/main.cpp)

run_tool("${ambiguous}" status text "${FIX_INCLUDES}" --apply)
if(NOT status EQUAL 1)
  message(FATAL_ERROR "fix-includes should exit with 1 on an ambiguous include, got ${status}:\n${text}")
endif()
expect_contains("${text}" "ambiguous, left alone: src/c/main.cpp: same.h could be src/a/same.h or src/b/same.h")

message(STATUS "tools self-test passed")
