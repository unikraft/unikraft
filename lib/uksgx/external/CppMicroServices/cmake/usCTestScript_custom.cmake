
find_program(CTEST_COVERAGE_COMMAND NAMES gcov)
find_program(CTEST_MEMORYCHECK_COMMAND NAMES valgrind)
find_program(CTEST_GIT_COMMAND NAMES git)

set(CTEST_SITE "bigeye")
if(WIN32)
  set(CTEST_DASHBOARD_ROOT "C:/tmp/us")
else()
  set(CTEST_DASHBOARD_ROOT "/tmp/us")
  set(CTEST_BUILD_FLAGS "-j")
  #set(CTEST_COMPILER "gcc-4.5")
endif()

set(CTEST_CONFIGURATION_TYPE Release)
set(CTEST_BUILD_CONFIGURATION Release)

set(CTEST_PARALLEL_LEVEL 4)

set(US_TEST_SHARED 1)
set(US_TEST_STATIC 1)

set(US_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../")

set(US_BUILD_CONFIGURATION )
foreach(i RANGE 3)
  list(APPEND US_BUILD_CONFIGURATION ${i})
endforeach()

if(WIN32 AND NOT MINGW)
  set(US_CMAKE_GENERATOR
      "Visual Studio 12"
      )
  if (NOT ${CMAKE_VERSION} VERSION_LESS "3.1")
    list(APPEND US_CMAKE_GENERATOR
         "Visual Studio 14 2015"
        )
  endif()
endif()

include(${US_SOURCE_DIR}/cmake/usCTestScript.cmake)
