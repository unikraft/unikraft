
macro(_us_create_test_bundle_helper)

  add_library(${name} ${_srcs} $<TARGET_OBJECTS:util>)
  if(US_BUILD_SHARED_LIBS AND US_TEST_LIBRARY_EXTENSION)
    set_target_properties(${name} PROPERTIES SUFFIX ${US_TEST_LIBRARY_EXTENSION})
  endif()
  set_property(TARGET ${name}
               APPEND PROPERTY COMPILE_DEFINITIONS US_BUNDLE_NAME=${_bundle_symbolic_name})
  set_property(TARGET ${name} PROPERTY US_BUNDLE_NAME ${_bundle_symbolic_name})
  # Clear a possible debug postfix
  set_property(TARGET ${name} PROPERTY DEBUG_POSTFIX "")
  if(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
    get_property(_compile_flags TARGET ${name} PROPERTY COMPILE_FLAGS)
    set_property(TARGET ${name} PROPERTY COMPILE_FLAGS "${_compile_flags} -fPIC")
  endif()

  target_include_directories(${name} PRIVATE $<TARGET_PROPERTY:util,INCLUDE_DIRECTORIES>)
  target_link_libraries(${name} ${${PROJECT_NAME}_TARGET} ${US_TEST_LINK_LIBRARIES} CppMicroServices)

  if(_res_files OR US_TEST_LINK_LIBRARIES)
    usFunctionAddResources(TARGET ${name} WORKING_DIRECTORY ${_res_root}
                           FILES ${_res_files}
                           ZIP_ARCHIVES ${US_TEST_LINK_LIBRARIES})
  endif()
  if(_bin_res_files)
    usFunctionAddResources(TARGET ${name} WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/resources
                           FILES ${_bin_res_files})
  endif()

  usFunctionEmbedResources(TARGET ${name} ${_mode})

  if(NOT US_TEST_SKIP_BUNDLE_LIST)
    set(_us_test_bundle_libs "${_us_test_bundle_libs};${name}" CACHE INTERNAL "" FORCE)
  endif()

endmacro()

function(usFunctionCreateTestBundle name)
  set(_srcs ${ARGN})
  set(_res_files )
  set(_bin_res_files )
  set(_bundle_symbolic_name ${name})
  usFunctionGenerateBundleInit(TARGET ${name} OUT _srcs)
  _us_create_test_bundle_helper()
endfunction()

function(usFunctionCreateTestBundleWithResources name)
  cmake_parse_arguments(US_TEST "SKIP_BUNDLE_LIST;LINK_RESOURCES;APPEND_RESOURCES" "RESOURCES_ROOT;LIBRARY_EXTENSION;BUNDLE_SYMBOLIC_NAME" "SOURCES;RESOURCES;BINARY_RESOURCES;LINK_LIBRARIES" "" ${ARGN})

  if(US_TEST_BUNDLE_SYMBOLIC_NAME)
    set(_bundle_symbolic_name ${US_TEST_BUNDLE_SYMBOLIC_NAME})
  else()
    set(_bundle_symbolic_name ${name})
  endif()
  
  set(_mode )
  if(US_TEST_LINK_RESOURCES)
    set(_mode LINK)
  elseif(US_TEST_APPEND_RESOURCES)
    set(_mode APPEND)
  endif()

  set(_srcs ${US_TEST_SOURCES})
  usFunctionGetResourceSource(TARGET ${name} OUT _srcs ${_mode})
  set(_res_files ${US_TEST_RESOURCES})
  set(_bin_res_files ${US_TEST_BINARY_RESOURCES})
  if(US_TEST_RESOURCES_ROOT)
    set(_res_root ${US_TEST_RESOURCES_ROOT})
  else()
    set(_res_root ${CMAKE_CURRENT_SOURCE_DIR}/resources)
  endif()
  usFunctionGenerateBundleInit(TARGET ${name} OUT _srcs)
  _us_create_test_bundle_helper()
endfunction()
