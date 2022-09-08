#===============================================================================
# Copyright 2017-2021 Intel Corporation
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#===============================================================================

#
# Intel® Integrated Performance Primitives Cryptography (Intel® IPP Cryptography)
#

cmake_minimum_required(VERSION 3.12)

include("${CMAKE_CURRENT_SOURCE_DIR}/sources/cmake/ippcp-utils.cmake")
ippcp_getlibversion("${CMAKE_CURRENT_SOURCE_DIR}/include/ippversion.h")
if ((NOT DEFINED IPPCP_VERSION_MAJOR) OR
    (NOT DEFINED IPPCP_VERSION_MINOR) OR
    (NOT DEFINED IPPCP_VERSION_UPDATE) OR
    (NOT DEFINED IPPCP_INTERFACE_VERSION_MAJOR) OR
    (NOT DEFINED IPPCP_INTERFACE_VERSION_MINOR))
    message(WARNING "Cannot parse version from ippversion.h file. The project might be corrupted.")
endif()

set(PROJECT_FULL_NAME "Intel® Integrated Performance Primitives Cryptography (Intel® IPP Cryptography)")
set(PROJECT_NAME "Intel(R) IPP Crypto")
set(PROJECT_VERSION ${IPPCP_VERSION})

set(LIB_NAME ippcp)

set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)

cmake_policy(SET CMP0042 NEW)
cmake_policy(SET CMP0054 NEW)
cmake_policy(SET CMP0068 NEW)

if("${CMAKE_GENERATOR}" STREQUAL "NMake Makefiles")
  if(NOT(C_COMPILER  STREQUAL ""))
    set(CMAKE_C_COMPILER ${C_COMPILER})
  endif()
  if(NOT(CXX_COMPILER STREQUAL ""))
    set(CMAKE_CXX_COMPILER ${CXX_COMPILER})
  endif()
endif()

project(${PROJECT_NAME}
        VERSION ${PROJECT_VERSION}
        LANGUAGES C CXX)

if("${CMAKE_BUILD_TYPE}" STREQUAL "")
  message(STATUS "CMAKE_BUILD_TYPE is unset, defaulting to Release")
  set(CMAKE_BUILD_TYPE "Release")
endif()

find_package(Python REQUIRED)

if(WIN32 AND (${CMAKE_GENERATOR} MATCHES "Visual Studio"))
  if(CMAKE_GENERATOR_PLATFORM) # VS 2019+ -A param
    if(${CMAKE_GENERATOR_PLATFORM} MATCHES "x64")
      set(ARCH intel64)
    else()
      set(ARCH ia32)
    endif()
  else()
    if(${CMAKE_GENERATOR} MATCHES "Win64") # old way of platform setting for VS
      set(ARCH intel64)
    else()
      set(ARCH ia32)
    endif()
  endif()
else()
  if (NOT "${ARCH}" STREQUAL "intel64" AND NOT "${ARCH}" STREQUAL "ia32")
    message(FATAL_ERROR "Please, set the ARCH parameter to ia32 or intel64")
  endif()
endif(WIN32 AND (${CMAKE_GENERATOR} MATCHES "Visual Studio"))

if ((NOT NONPIC_LIB) AND (NOT CODE_COVERAGE))
  set(DYNAMIC_LIB ON)
else()
  set(DYNAMIC_LIB OFF)
endif()

if("${MERGED_BLD}" STREQUAL "")
  set(MERGED_BLD ON)
endif()

# Set default installation directories
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  if (UNIX)
    set(CMAKE_INSTALL_PREFIX "$ENV{HOME}/intel/ippcp_${PROJECT_VERSION}" CACHE PATH "..." FORCE)
  else()
    set(CMAKE_INSTALL_PREFIX "c:/Program Files (x86)/IntelSWTools/ippcp_${PROJECT_VERSION}" CACHE PATH "..." FORCE)
  endif()
endif()

set(IPP_CRYPTO_DIR                  "${CMAKE_CURRENT_SOURCE_DIR}")
set(IPP_CRYPTO_INCLUDE_DIR          "${IPP_CRYPTO_DIR}/include")
set(IPP_CRYPTO_SOURCES_INCLUDE_DIR  "${IPP_CRYPTO_DIR}/sources/include")
set(IPP_CRYPTO_SOURCES_DIR          "${IPP_CRYPTO_DIR}/sources/ippcp")
set(IPP_CRYPTO_DISPATCHER_DIR       "${IPP_CRYPTO_DIR}/sources/dispatcher")
set(TOOLS_DIR                       "${IPP_CRYPTO_DIR}/tools")

if(NOT CMAKE_OUTPUT_DIR)
  set(CMAKE_OUTPUT_DIR              "${CMAKE_BINARY_DIR}/.build")
endif()

message (STATUS "CMAKE_VERSION ......................... " ${CMAKE_VERSION})

if(UNIX AND NOT APPLE)
  if(NONPIC_LIB)
    message (STATUS "NONPIC_LIB ............................ on")
  else()
    message (STATUS "NONPIC_LIB ............................ off")
  endif()
else()
  set(${NONPIC_LIB} false)
endif(UNIX AND NOT APPLE)

set(NONPIC_SUBDIRECTORY "")
if(NONPIC_LIB)
  set(NONPIC_SUBDIRECTORY "/nonpic")
endif()

foreach( OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES} )
  string( TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG )
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} "${CMAKE_OUTPUT_DIR}/${OUTPUTCONFIG}/lib${NONPIC_SUBDIRECTORY}")
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} "${CMAKE_OUTPUT_DIR}/${OUTPUTCONFIG}/lib${NONPIC_SUBDIRECTORY}")
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} "${CMAKE_OUTPUT_DIR}/${OUTPUTCONFIG}/lib${NONPIC_SUBDIRECTORY}")
endforeach( OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES )

if(${CMAKE_BUILD_TYPE} STREQUAL "Release")
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE}")
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE}")
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE}")
endif()

if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG}")
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG}")
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG}")
endif()

message (STATUS "PROJECT ............................... " ${PROJECT_FULL_NAME})
message (STATUS "CMAKE_BINARY_DIR ...................... " ${CMAKE_BINARY_DIR})
message (STATUS "CMAKE_OUTPUT_DIR ...................... " ${CMAKE_OUTPUT_DIR})
message (STATUS "CMAKE_SOURCE_DIR ...................... " ${CMAKE_SOURCE_DIR})
message (STATUS "IPP_CRYPTO_DIR ........................ " ${IPP_CRYPTO_DIR})
message (STATUS "CMAKE_GENERATOR ....................... " ${CMAKE_GENERATOR})
message (STATUS "CMAKE_C_COMPILER_ID ................... " ${CMAKE_C_COMPILER_ID})
message (STATUS "CMAKE_CXX_COMPILER_ID ................. " ${CMAKE_CXX_COMPILER_ID})
message (STATUS "IPP_CRYPTO_INCLUDE_DIR ................ " ${IPP_CRYPTO_INCLUDE_DIR})
message (STATUS "IPP_CRYPTO_SOURCES_INCLUDE_DIR ........ " ${IPP_CRYPTO_SOURCES_INCLUDE_DIR})
message (STATUS "IPP_CRYPTO_SOURCES_DIR ................ " ${IPP_CRYPTO_SOURCES_DIR})
message (STATUS "ARCH .................................. " ${ARCH})
message (STATUS "DYNAMIC_LIB ........................... " ${DYNAMIC_LIB})
message (STATUS "CMAKE_INSTALL_PREFIX .................. " ${CMAKE_INSTALL_PREFIX})

if(Python_Interpreter_FOUND)
  message (STATUS "PYTHON_VERSION_STRING ................. " ${Python_VERSION})
else()
  message (STATUS "PYTHON_VERSION_STRING ................. Python not found" )
endif()

if(MERGED_BLD)
  message (STATUS "MERGED_BLD ............................ on")
else()
  message (STATUS "MERGED_BLD ............................ off")
endif()

option(BUILD_EXAMPLES "Build examples" OFF)
if(BUILD_EXAMPLES)
  message (STATUS "BUILD_EXAMPLES ........................ on")
else()
  message (STATUS "BUILD_EXAMPLES ........................ off")
endif()

if((UNIX) AND (NOT APPLE))
  set(LINUX ON)
else()
  set(LINUX OFF)
endif()

add_subdirectory(sources/ippcp)
if(EXISTS "${IPP_CRYPTO_DIR}/tests/")
  add_subdirectory(tests)
endif()
if(EXISTS "${IPP_CRYPTO_DIR}/perf_tests/")
  add_subdirectory(perf_tests)
endif()

if(BUILD_EXAMPLES)
  # This helps to organize examples projects structure in IDE by folders
  set_property(GLOBAL PROPERTY USE_FOLDERS ON)
  set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "CMakeTargets")

  add_subdirectory(examples)
endif()
