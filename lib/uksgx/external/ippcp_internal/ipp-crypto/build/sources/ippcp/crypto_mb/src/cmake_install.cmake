# Install script for directory: /home/xiangyi/code/unikraft/unikraft/lib/uksgx/external/ippcp_internal/ipp-crypto/sources/ippcp/crypto_mb/src

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/home/xiangyi/intel/ippcp_2021.3.0")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "1")
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

# Set default install directory permissions.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/usr/bin/objdump")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  foreach(file
      "$ENV{DESTDIR}/home/xiangyi/intel/ippcp_2021.3.0/lib/libcrypto_mb.so.11.1"
      "$ENV{DESTDIR}/home/xiangyi/intel/ippcp_2021.3.0/lib/libcrypto_mb.so.11"
      )
    if(EXISTS "${file}" AND
       NOT IS_SYMLINK "${file}")
      file(RPATH_CHECK
           FILE "${file}"
           RPATH "")
    endif()
  endforeach()
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/home/xiangyi/intel/ippcp_2021.3.0/lib/libcrypto_mb.so.11.1;/home/xiangyi/intel/ippcp_2021.3.0/lib/libcrypto_mb.so.11")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  file(INSTALL DESTINATION "/home/xiangyi/intel/ippcp_2021.3.0/lib" TYPE SHARED_LIBRARY FILES
    "/home/xiangyi/code/unikraft/unikraft/lib/uksgx/external/ippcp_internal/ipp-crypto/build/.build/RELEASE/lib/libcrypto_mb.so.11.1"
    "/home/xiangyi/code/unikraft/unikraft/lib/uksgx/external/ippcp_internal/ipp-crypto/build/.build/RELEASE/lib/libcrypto_mb.so.11"
    )
  foreach(file
      "$ENV{DESTDIR}/home/xiangyi/intel/ippcp_2021.3.0/lib/libcrypto_mb.so.11.1"
      "$ENV{DESTDIR}/home/xiangyi/intel/ippcp_2021.3.0/lib/libcrypto_mb.so.11"
      )
    if(EXISTS "${file}" AND
       NOT IS_SYMLINK "${file}")
      if(CMAKE_INSTALL_DO_STRIP)
        execute_process(COMMAND "/usr/bin/strip" "${file}")
      endif()
    endif()
  endforeach()
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}/home/xiangyi/intel/ippcp_2021.3.0/lib/libcrypto_mb.so" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}/home/xiangyi/intel/ippcp_2021.3.0/lib/libcrypto_mb.so")
    file(RPATH_CHECK
         FILE "$ENV{DESTDIR}/home/xiangyi/intel/ippcp_2021.3.0/lib/libcrypto_mb.so"
         RPATH "")
  endif()
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/home/xiangyi/intel/ippcp_2021.3.0/lib/libcrypto_mb.so")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  file(INSTALL DESTINATION "/home/xiangyi/intel/ippcp_2021.3.0/lib" TYPE SHARED_LIBRARY FILES "/home/xiangyi/code/unikraft/unikraft/lib/uksgx/external/ippcp_internal/ipp-crypto/build/.build/RELEASE/lib/libcrypto_mb.so")
  if(EXISTS "$ENV{DESTDIR}/home/xiangyi/intel/ippcp_2021.3.0/lib/libcrypto_mb.so" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}/home/xiangyi/intel/ippcp_2021.3.0/lib/libcrypto_mb.so")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/usr/bin/strip" "$ENV{DESTDIR}/home/xiangyi/intel/ippcp_2021.3.0/lib/libcrypto_mb.so")
    endif()
  endif()
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/home/xiangyi/intel/ippcp_2021.3.0/include/crypto_mb/")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  file(INSTALL DESTINATION "/home/xiangyi/intel/ippcp_2021.3.0/include/crypto_mb" TYPE DIRECTORY FILES "" FILES_MATCHING REGEX "/[^/]*\\.h[^/]*$")
endif()

