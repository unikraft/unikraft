
if(WIN32)
  string(REPLACE "/" "\\\\" CMAKE_LIBRARY_OUTPUT_DIRECTORY_NATIVE ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
  string(REPLACE "/" "\\\\" CMAKE_RUNTIME_OUTPUT_DIRECTORY_NATIVE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
else()
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_NATIVE ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_NATIVE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
endif()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}DriverConfig.h.in
               ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}DriverConfig.h)

include_directories(${CMAKE_CURRENT_BINARY_DIR})

if(NOT US_BUILD_SHARED_LIBS)
  # Using static bundles requires that they be linked into an EXE or shared library and that
  # any resources inside the static bundles are merged into that EXE or shared library.

  # _target_zip_archives contains the list of static bundles and is used to both link with the executable
  # and merge their resources into the executable.
  set(_target_zip_archives
    Tutorial-dictionaryclient
    Tutorial-dictionaryclient2
    Tutorial-dictionaryclient3
    Tutorial-dictionaryservice
    Tutorial-frenchdictionary
    Tutorial-eventlistener
    Tutorial-spellcheckclient
    Tutorial-spellcheckservice
    ${CppMicroServices_LIBRARIES})

  set(_srcs main.cpp)
  usFunctionGetResourceSource(TARGET usTutorialDriver OUT _srcs)

  add_executable(usTutorialDriver ${_srcs})
  target_link_libraries(usTutorialDriver ${_target_zip_archives})

  # Merge all resources from the static bundles and embed them into the executable.
  usFunctionAddResources(TARGET usTutorialDriver
                         BUNDLE_NAME main
                         ZIP_ARCHIVES ${_target_zip_archives})
  usFunctionEmbedResources(TARGET usTutorialDriver)
else()
  add_executable(usTutorialDriver main.cpp)
  target_link_libraries(usTutorialDriver ${CppMicroServices_LIBRARIES})
endif()
