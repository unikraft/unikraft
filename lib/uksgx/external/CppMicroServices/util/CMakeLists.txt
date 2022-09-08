add_library(util OBJECT
  include/cppmicroservices/util/Error.h
  include/cppmicroservices/util/FileSystem.h
  include/cppmicroservices/util/String.h

  src/Error.cpp
  src/FileSystem.cpp
  src/String.cpp
)

set_property(TARGET util PROPERTY POSITION_INDEPENDENT_CODE 1)

target_include_directories(util
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
  PRIVATE
    ${CMAKE_SOURCE_DIR}/third_party
    ${CMAKE_BINARY_DIR}/include
)
