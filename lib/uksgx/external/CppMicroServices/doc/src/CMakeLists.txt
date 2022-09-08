# [prj-start]
project(CppMicroServicesExamples)

set(CMAKE_CXX_STANDARD_REQUIRED 1)
set(CMAKE_CXX_STANDARD 11)

find_package(CppMicroServices NO_MODULE REQUIRED)

cmake_minimum_required(VERSION ${US_CMAKE_MINIMUM_REQUIRED_VERSION})
cmake_policy(VERSION ${US_CMAKE_MINIMUM_REQUIRED_VERSION})
# [prj-end]


#-----------------------------------------------------------------------------
# Init output directories
#-----------------------------------------------------------------------------

set(CppMicroServicesExamples_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib")
set(CppMicroServicesExamples_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib")
set(CppMicroServicesExamples_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin")

foreach(_type ARCHIVE LIBRARY RUNTIME)
  if(NOT CMAKE_${_type}_OUTPUT_DIRECTORY)
    set(CMAKE_${_type}_OUTPUT_DIRECTORY ${CppMicroServicesExamples_${_type}_OUTPUT_DIRECTORY})
  endif()
endforeach()


function(CreateTutorial _name)
  set(_srcs ${ARGN})

  usFunctionGetResourceSource(TARGET Tutorial-${_name} OUT _srcs)
  usFunctionGenerateBundleInit(TARGET Tutorial-${_name} OUT _srcs)

  add_library(Tutorial-${_name} ${_srcs})
  set_property(TARGET Tutorial-${_name} APPEND PROPERTY COMPILE_DEFINITIONS US_BUNDLE_NAME=${_name})
  set_property(TARGET Tutorial-${_name} PROPERTY DEBUG_POSTFIX "")

  if(${_name}_DEPENDS)
    foreach(_dep ${${_name}_DEPENDS})
      include_directories(${PROJECT_SOURCE_DIR}/tutorial/${_dep})
      target_link_libraries(Tutorial-${_name} Tutorial-${_dep})
    endforeach()
  endif()
  target_link_libraries(Tutorial-${_name} ${CppMicroServices_LIBRARIES})
  set_target_properties(Tutorial-${_name} PROPERTIES
    LABELS Tutorial
    OUTPUT_NAME ${_name}
  )

  usFunctionAddResources(TARGET Tutorial-${_name} BUNDLE_NAME ${_name} WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/tutorial/${_name}/resources FILES manifest.json)
  usFunctionEmbedResources(TARGET Tutorial-${_name})

endfunction()

# [fnc-end]

set (_tutorial_names
  eventlistener
  dictionaryservice
  frenchdictionary
  dictionaryclient
  dictionaryclient2
  dictionaryclient3
  spellcheckservice
  spellcheckclient
  )

foreach(_tutorial ${_tutorial_names})
  add_subdirectory(tutorial/${_tutorial})
endforeach()

add_subdirectory(tutorial/driver)

foreach(_tutorial ${_tutorial_names})
  add_dependencies(usTutorialDriver Tutorial-${_tutorial})
endforeach()

add_subdirectory(examples/getting_started)

#-----------------------------------------------------------------------------
# Test if the doc code compiles against an install tree and if the
# Makefile example compiles
#-----------------------------------------------------------------------------

if(US_BUILD_TESTING)
  enable_testing()

  set(_doc_code_tests )

  if(WIN32)
    string(REGEX REPLACE "^.:" "" _install_prefix ${CMAKE_INSTALL_PREFIX})
  else()
    set(_install_prefix ${CMAKE_INSTALL_PREFIX})
  endif()
  set(_install_dir "${CppMicroServices_BINARY_DIR}/install_test/${_install_prefix}")

  add_test(NAME InstallCleanTest
           COMMAND ${CMAKE_COMMAND} -E remove_directory "${_install_dir}")

  add_test(NAME InstallTest
           WORKING_DIRECTORY ${CppMicroServices_BINARY_DIR}
           COMMAND ${CMAKE_COMMAND} --build ${CppMicroServices_BINARY_DIR} --config $<CONFIGURATION> --target install)
  set_tests_properties(InstallTest PROPERTIES
                       ENVIRONMENT "DESTDIR=${CppMicroServices_BINARY_DIR}/install_test"
                       DEPENDS InstallCleanTest)

  set(_doc_code_binary_dir "${CppMicroServices_BINARY_DIR}/doc_code_build")

  add_test(NAME DocCodeCleanTest
           COMMAND ${CMAKE_COMMAND} -E remove_directory "${_doc_code_binary_dir}")

  add_test(NAME DocCodeCreateDirTest
           COMMAND ${CMAKE_COMMAND} -E make_directory "${_doc_code_binary_dir}")
  set_tests_properties(DocCodeCreateDirTest PROPERTIES
                       DEPENDS DocCodeCleanTest)

  list(APPEND _doc_code_tests InstallCleanTest InstallTest DocCodeCleanTest
                             DocCodeCreateDirTest)

  if(CMAKE_CONFIGURATION_TYPES)
    foreach(config ${CMAKE_CONFIGURATION_TYPES})
      add_test(NAME DocCodeConfigureTest-${config} CONFIGURATIONS ${config}
               WORKING_DIRECTORY ${_doc_code_binary_dir}
               COMMAND ${CMAKE_COMMAND}
                       -D CMAKE_C_COMPILER:STRING=${CMAKE_C_COMPILER}
                       -D CMAKE_CXX_COMPILER:STRING=${CMAKE_CXX_COMPILER}
                       "-DCMAKE_CXX_FLAGS:STRING=${CMAKE_CXX_FLAGS}"
                       -D US_BUILD_SHARED_LIBS:BOOL=${US_BUILD_SHARED_LIBS}
                       -G ${CMAKE_GENERATOR}
                       "-DCppMicroServices_DIR:PATH=${_install_dir}/${AUXILIARY_CMAKE_INSTALL_DIR}"
                       "${CMAKE_CURRENT_LIST_DIR}")
      set_tests_properties(DocCodeConfigureTest-${config} PROPERTIES
                           DEPENDS "InstallTest;DocCodeCreateDirTest")

      add_test(NAME DocCodeBuildTest-${config} CONFIGURATIONS ${config}
               WORKING_DIRECTORY ${_doc_code_binary_dir}
               COMMAND ${CMAKE_COMMAND} --build . --config ${config})
      set_tests_properties(DocCodeBuildTest-${config} PROPERTIES
                           DEPENDS DocCodeConfigureTest-${config})

      list(APPEND _doc_code_tests DocCodeConfigureTest-${config} DocCodeBuildTest-${config})
    endforeach()
  else()
    add_test(NAME DocCodeConfigureTest-${CMAKE_BUILD_TYPE}
             WORKING_DIRECTORY ${_doc_code_binary_dir}
             COMMAND ${CMAKE_COMMAND}
                     -D CMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
                     -D CMAKE_C_COMPILER:STRING=${CMAKE_C_COMPILER}
                     "-DCMAKE_CXX_FLAGS:STRING=${CMAKE_CXX_FLAGS}"
                     -D CMAKE_CXX_COMPILER:STRING=${CMAKE_CXX_COMPILER}
                     -D US_BUILD_SHARED_LIBS:BOOL=${US_BUILD_SHARED_LIBS}
                     -G ${CMAKE_GENERATOR}
                     "-DCppMicroServices_DIR:PATH=${_install_dir}/${AUXILIARY_CMAKE_INSTALL_DIR}"
                     "${CMAKE_CURRENT_LIST_DIR}")
    set_tests_properties(DocCodeConfigureTest-${CMAKE_BUILD_TYPE} PROPERTIES
                         DEPENDS "InstallTest;DocCodeCreateDirTest")

    add_test(NAME DocCodeBuildTest-${CMAKE_BUILD_TYPE}
             WORKING_DIRECTORY ${_doc_code_binary_dir}
             COMMAND ${CMAKE_COMMAND} --build . --config ${CMAKE_BUILD_TYPE})
    set_tests_properties(DocCodeBuildTest-${CMAKE_BUILD_TYPE} PROPERTIES
                         DEPENDS DocCodeConfigureTest-${CMAKE_BUILD_TYPE})

    list(APPEND _doc_code_tests DocCodeConfigureTest-${CMAKE_BUILD_TYPE} DocCodeBuildTest-${CMAKE_BUILD_TYPE})
  endif()

  # The makefile is Linux specific, so only try to build the Makefile example
  # if we are on a proper system
  if(UNIX AND NOT APPLE AND US_BUILD_SHARED_LIBS)

    # We need to supply the correct language flag to the Makefile
    set(US_CXX11_FLAGS -std=c++11)
    usFunctionCheckCompilerFlags(${US_CXX11_FLAGS} _have_cxx11)
    if(_have_cxx11)
      set(US_CXX_FLAGS "${US_CXX_FLAGS} ${US_CXX11_FLAGS}")
    else()
      set(US_CXX11_FLAGS -std=c++0x)
      usFunctionCheckCompilerFlags(${US_CXX11_FLAGS} _have_cxx0x)
      if(_have_cxx0x)
        set(US_CXX_FLAGS "${US_CXX_FLAGS} ${US_CXX11_FLAGS}")
      else()
        set(US_CXX11_FLAGS )
      endif()
    endif()

    find_program(MAKE_COMMAND NAMES make gmake)
    find_program(CXX_COMMAND NAMES g++)
    mark_as_advanced(MAKE_COMMAND CXX_COMMAND)
    if(MAKE_COMMAND AND CXX_COMMAND)
      add_test(NAME MakefileExampleCleanTest
               WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/examples/makefile
               COMMAND ${MAKE_COMMAND} clean)
      add_test(NAME MakefileExampleTest
               WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/examples/makefile
               COMMAND ${MAKE_COMMAND})
      set_tests_properties(MakefileExampleTest PROPERTIES
                           DEPENDS "MakefileExampleCleanTest;InstallTest"
                           ENVIRONMENT "CppMicroServices_ROOT=${CppMicroServices_BINARY_DIR}/install_test${CMAKE_INSTALL_PREFIX};US_CXX_FLAGS=${US_CXX_FLAGS};US_CPPMICROSERVICES_TARGET=$<TARGET_FILE:CppMicroServices>")
      list(APPEND _doc_code_tests MakefileExampleCleanTest MakefileExampleTest)
    endif()
  endif()

  set_tests_properties(${_doc_code_tests} PROPERTIES
                       LABELS regular)

endif()
