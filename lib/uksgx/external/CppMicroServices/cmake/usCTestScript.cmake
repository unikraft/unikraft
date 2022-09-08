
macro(build_and_test)

  set(CTEST_SOURCE_DIRECTORY ${US_SOURCE_DIR})
  set(CTEST_BINARY_DIRECTORY "${CTEST_DASHBOARD_ROOT}/${CTEST_PROJECT_NAME}_${CTEST_DASHBOARD_NAME}")

  #if(NOT CTEST_BUILD_NAME)
  #  set(CTEST_BUILD_NAME "${CMAKE_SYSTEM}_${CTEST_COMPILER}_${CTEST_DASHBOARD_NAME}")
  #endif()

  ctest_empty_binary_directory(${CTEST_BINARY_DIRECTORY})

  ctest_start("Experimental")

  if(NOT EXISTS "${CTEST_BINARY_DIRECTORY}/CMakeCache.txt")
    file(WRITE "${CTEST_BINARY_DIRECTORY}/CMakeCache.txt" "${CTEST_INITIAL_CACHE}")
  endif()

  ctest_configure(RETURN_VALUE res)
  if (res)
    message(FATAL_ERROR "CMake configure error")
  endif()
  ctest_build(RETURN_VALUE res)
  if (res)
    message(FATAL_ERROR "CMake build error")
  endif()

  ctest_test(RETURN_VALUE res PARALLEL_LEVEL ${CTEST_PARALLEL_LEVEL})
  if (res)
   message(FATAL_ERROR "CMake test error")
  endif()


  if(WITH_MEMCHECK AND CTEST_MEMORYCHECK_COMMAND)
    ctest_memcheck()
  endif()

  if(WITH_COVERAGE)
    if(CTEST_COVERAGE_COMMAND)
      ctest_coverage()
    else()
      message(FATAL_ERROR "CMake could not find coverage tool")
    endif()
  endif()

  #ctest_submit()

endmacro()

function(create_initial_cache var _shared _threading)

  set(_initial_cache "
      US_BUILD_TESTING:BOOL=ON
      US_ENABLE_COVERAGE:BOOL=$ENV{WITH_COVERAGE}
      US_BUILD_SHARED_LIBS:BOOL=${_shared}
      US_ENABLE_THREADING_SUPPORT:BOOL=${_threading}
      US_BUILD_EXAMPLES:BOOL=ON
      ")

  set(${var} ${_initial_cache} PARENT_SCOPE)

  if(_shared)
    set(CTEST_DASHBOARD_NAME "shared")
  else()
    set(CTEST_DASHBOARD_NAME "static")
  endif()

  if(_threading)
    set(CTEST_DASHBOARD_NAME "${CTEST_DASHBOARD_NAME}-threading")
  endif()

  set(CTEST_DASHBOARD_NAME "${CTEST_DASHBOARD_NAME} (${_generator})" PARENT_SCOPE)

endfunction()

#=========================================================

set(CTEST_PROJECT_NAME CppMicroServices)

if(NOT CTEST_PARALLEL_LEVEL)
  set(CTEST_PARALLEL_LEVEL 1)
endif()


#            SHARED THREADING

set(config0     1       1     )
set(config1     0       1     )
set(config2     1       0     )
set(config3     0       0     )

if(NOT US_CMAKE_GENERATOR)
  set(US_CMAKE_GENERATOR "Unix Makefiles")
endif()

foreach (_generator ${US_CMAKE_GENERATOR})
  set(CTEST_CMAKE_GENERATOR ${_generator})
  foreach(i ${US_BUILD_CONFIGURATION})
    create_initial_cache(CTEST_INITIAL_CACHE ${config${i}})
    message("Testing build configuration: ${CTEST_DASHBOARD_NAME}")
    build_and_test()
  endforeach()
endforeach()
