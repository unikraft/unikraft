
#-----------------------------------------------------------------------------
# Configure files, include dirs, etc.
#-----------------------------------------------------------------------------

# Make sure that the correct paths separators are used on each platform
if(WIN32)
  set(DIR_SEP "\\\\")
  string(REPLACE "/" "\\\\" CMAKE_LIBRARY_OUTPUT_DIRECTORY_NATIVE ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
  string(REPLACE "/" "\\\\" CMAKE_RUNTIME_OUTPUT_DIRECTORY_NATIVE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
else()
  set(DIR_SEP "/")
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_NATIVE ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_NATIVE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
endif()

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/TestingConfig.h.in" "${PROJECT_BINARY_DIR}/include/TestingConfig.h")

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/../util
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../third_party
  )


#-----------------------------------------------------------------------------
# Add unit tests
#-----------------------------------------------------------------------------

set(_tests
  AnyTest
  AnyMapTest
  BundleRegistryPerformanceTest
  FrameworkEventTest
  FrameworkListenerTest
  FrameworkFactoryTest
  HelgrindTest
  LDAPFilterTest
  LDAPQueryTest
  LogTest
  BundleActivatorTest
  BundleEventTest
  BundleHooksTest
  BundleManifestTest
  BundleTest
  BundleResourceTest
  MemcheckTest
  MultipleListenersTest
  ResourceCompilerTest
  ShrinkableMapTest
  ShrinkableVectorTest
  ServiceFactoryTest
  ServiceHooksTest
  ServiceRegistryPerformanceTest
  ServiceRegistryTest
  ServiceTemplateTest
  ServiceTrackerTest
  StaticBundleResourceTest
  StaticBundleTest
)

if(US_BUILD_SHARED_LIBS)
  list(APPEND _tests
       ServiceListenerTest
       SharedLibraryTest
      )
endif()

if(US_ENABLE_THREADING_SUPPORT)
  list(APPEND _tests ConcurrencyTest)
endif()

set(_additional_srcs
  TestDriverActivator.cpp
  TestManager.cpp
  ../util/TestUtilBundleListener.cpp
  ../util/TestUtilFrameworkListener.cpp
  ../util/TestUtils.cpp
  ../util/ImportTestBundles.cpp
  $<TARGET_OBJECTS:util>
)

#-----------------------------------------------------------------------------
# Build the main test driver executable
#-----------------------------------------------------------------------------

set(_test_driver us${PROJECT_NAME}TestDriver)
set(_test_sourcelist_extra_args )
create_test_sourcelist(_srcs ${_test_driver}.cpp ${_tests} ${_test_sourcelist_extra_args})
set(_third_party_srcs 
  ../../../third_party/miniz.c
  ../../../third_party/jsoncpp.cpp
)


# Generate a custom "bundle init" file for the test driver executable
usFunctionGenerateBundleInit(TARGET ${_test_driver} OUT _srcs)

usFunctionGetResourceSource(TARGET ${_test_driver} OUT _srcs)
add_executable(${_test_driver} ${_srcs} ${_additional_srcs} ${_third_party_srcs})
# Needed for ResourceCompilerTest
add_dependencies(${_test_driver} ${US_RCC_EXECUTABLE_TARGET})
set_property(TARGET ${_test_driver} APPEND PROPERTY COMPILE_DEFINITIONS US_BUNDLE_NAME=main)
set_property(TARGET ${_test_driver} PROPERTY US_BUNDLE_NAME main)

target_include_directories(${_test_driver} PRIVATE $<TARGET_PROPERTY:util,INCLUDE_DIRECTORIES>)

# Disable deprecation warnings.
if (("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang") OR
    ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang") OR
    ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU"))
  set_source_files_properties(${_srcs} PROPERTIES COMPILE_FLAGS -Wno-error=deprecated-declarations)
endif()

if(NOT US_BUILD_SHARED_LIBS)
  target_link_libraries(${_test_driver} ${_us_test_bundle_libs})
endif()

target_link_libraries(${_test_driver} ${Framework_TARGET})

# Needed for clock_gettime with glibc < 2.17
if(UNIX AND NOT APPLE)
  target_link_libraries(${_test_driver} rt)
endif()

# Add resources
usFunctionEmbedResources(TARGET ${_test_driver}
                       FILES TestResource.txt manifest.json
                       ZIP_ARCHIVES ${Framework_TARGET} ${_us_test_bundle_libs})

# Register tests
us_add_tests(${_test_driver} ${_tests})

if(US_MEMCHECK_COMMAND)
  set_tests_properties(memcheck_MemcheckTest PROPERTIES WILL_FAIL 1)
  if(US_ENABLE_HELGRIND)
    set_tests_properties(helgrind_HelgrindTest PROPERTIES WILL_FAIL 1)
  endif()
endif()

if(US_ENABLE_TSAN)
  set_tests_properties(HelgrindTest PROPERTIES WILL_FAIL 1)
endif()

# Add a Preprocessor flag if code coverage is turned ON
if(US_ENABLE_COVERAGE)
  target_compile_definitions(${_test_driver} PUBLIC US_COVERAGE_ENABLED=1)
endif()

#-----------------------------------------------------------------------------
# Add dependencies for shared libraries
#-----------------------------------------------------------------------------

if(US_BUILD_SHARED_LIBS)
  foreach(_test_bundle ${_us_test_bundle_libs})
    add_dependencies(${_test_driver} ${_test_bundle})
  endforeach()
endif()


#-----------------------------------------------------------------------------
# Add preprocessor macro to indicate C++11 unicode literal support
#-----------------------------------------------------------------------------

target_compile_definitions(${_test_driver} PUBLIC
                           $<$<COMPILE_FEATURES:cxx_unicode_literals>:US_CXX_UNICODE_LITERALS=1>) 

