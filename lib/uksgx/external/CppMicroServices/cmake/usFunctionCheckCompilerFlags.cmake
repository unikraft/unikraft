#
# Helper macro allowing to check if the given flags are supported
# by the underlying build tool
#
# If the flag(s) is/are supported, they will be appended to the string identified by RESULT_VAR
#
# Usage:
#   usFunctionCheckCompilerFlags(FLAGS_TO_CHECK VALID_FLAGS_VAR)
#
# Example:
#
#   set(myflags)
#   usFunctionCheckCompilerFlags("-fprofile-arcs" myflags)
#   message(1-myflags:${myflags})
#   usFunctionCheckCompilerFlags("-fauto-bugfix" myflags)
#   message(2-myflags:${myflags})
#   usFunctionCheckCompilerFlags("-Wall" myflags)
#   message(1-myflags:${myflags})
#
#   The output will be:
#    1-myflags: -fprofile-arcs
#    2-myflags: -fprofile-arcs
#    3-myflags: -fprofile-arcs -Wall

include(CheckCXXCompilerFlag)

function(usFunctionCheckCompilerFlags FLAG_TO_TEST RESULT_VAR)

  if(FLAG_TO_TEST STREQUAL "")
    message(FATAL_ERROR "FLAG_TO_TEST shouldn't be empty")
  endif()

  # Save the contents of RESULT_VAR temporarily.
  # This is needed in case ${RESULT_VAR} is one of the CMAKE_<LANG>_FLAGS_* variables.
  set(_saved_result_var ${${RESULT_VAR}})

  # Clear all flags. If not, existing flags triggering warnings might lead to
  # false-negatives when checking for certain compiler flags.
  set(CMAKE_C_FLAGS )
  set(CMAKE_C_FLAGS_DEBUG )
  set(CMAKE_C_FLAGS_MINSIZEREL )
  set(CMAKE_C_FLAGS_RELEASE )
  set(CMAKE_C_FLAGS_RELWITHDEBINFO )
  set(CMAKE_CXX_FLAGS )
  set(CMAKE_CXX_FLAGS_DEBUG )
  set(CMAKE_CXX_FLAGS_MINSIZEREL )
  set(CMAKE_CXX_FLAGS_RELEASE )
  set(CMAKE_CXX_FLAGS_RELWITHDEBINFO )

  # Internally, the macro CMAKE_CXX_COMPILER_FLAG calls TRY_COMPILE. To avoid
  # the cost of compiling the test each time the project is configured, the variable set by
  # the macro is added to the cache so that following invocation of the macro with
  # the same variable name skip the compilation step.
  # For that same reason, the usFunctionCheckCompilerFlags function appends a unique suffix to
  # the HAS_CXX_FLAG variable. This suffix is created using a 'clean version' of the
  # flag to test. The value of HAS_CXX_FLAG_${suffix} additonally needs to be a valid
  # pre-processor token because CHECK_CXX_COMPILER_FLAG adds it as a definition to the compiler
  # arguments. An invalid token triggers compiler warnings, which in case of the "-Werror" flag
  # leads to false-negative checks.
  string(REGEX REPLACE "[=/-]" "_" suffix ${FLAG_TO_TEST})
  string(REGEX REPLACE "[, \\$\\+\\*\\{\\}\\(\\)\\#]" "" suffix ${suffix})
  CHECK_CXX_COMPILER_FLAG(${FLAG_TO_TEST} HAS_CXX_FLAG_${suffix})

  if(HAS_CXX_FLAG_${suffix})
    set(${RESULT_VAR} "${_saved_result_var} ${FLAG_TO_TEST}" PARENT_SCOPE)
  endif()

endfunction()
