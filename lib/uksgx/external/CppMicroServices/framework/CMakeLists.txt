# sources and headers
include(${CMAKE_CURRENT_SOURCE_DIR}/src/CMakeLists.txt)
include(${CMAKE_CURRENT_SOURCE_DIR}/include/CMakeLists.txt)

set(_framework_srcs )
foreach(_src ${_srcs})
  list(APPEND _framework_srcs ${CMAKE_CURRENT_SOURCE_DIR}/src/${_src})
endforeach()

set(_framework_private_headers)
foreach(_header ${_private_headers})
  list(APPEND _framework_private_headers ${CMAKE_CURRENT_SOURCE_DIR}/src/${_header})
endforeach()

set(_framework_public_headers )
foreach(_header ${_public_headers})
  list(APPEND _framework_public_headers ${CMAKE_CURRENT_SOURCE_DIR}/include/${_header})
endforeach()

# link libraries for the CppMicroServices lib
set(_link_libraries )
if(UNIX)
  list(APPEND _link_libraries dl)
endif()
if(WIN32)
  list(APPEND _link_libraries shlwapi.lib)
endif()
if(CMAKE_THREAD_LIBS_INIT)
  list(APPEND _link_libraries ${CMAKE_THREAD_LIBS_INIT})
endif()

# Configure the bundles manifest.json file
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/resources/manifest.json.in
               ${CMAKE_CURRENT_BINARY_DIR}/resources/manifest.json)

usMacroCreateBundle(Framework
  VERSION "${CppMicroServices_VERSION}"
  TARGET CppMicroServices
  SYMBOLIC_NAME system_bundle
  SKIP_INIT # we do it manually
  PRIVATE_INCLUDE_DIRS src/util src/service src/bundle ../third_party
  LINK_LIBRARIES ${_link_libraries}
  SOURCES ${_framework_srcs}
  PUBLIC_HEADERS ${_framework_public_headers}
  PRIVATE_HEADERS ${_framework_private_headers}
  BINARY_RESOURCES manifest.json
)

set_property(
  TARGET ${PROJECT_TARGET} APPEND PROPERTY
  COMPILE_DEFINITIONS "MINIZ_NO_ARCHIVE_WRITING_API;MINIZ_NO_ZLIB_COMPATIBLE_NAMES"
  )
