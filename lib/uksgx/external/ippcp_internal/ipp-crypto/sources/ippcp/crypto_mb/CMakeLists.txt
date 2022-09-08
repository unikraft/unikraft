#===============================================================================
# Copyright 2019-2021 Intel Corporation
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

cmake_minimum_required(VERSION 3.10)

project("crypto_mb" C)

include("${CMAKE_CURRENT_SOURCE_DIR}/src/cmake/crypto_mb-utils.cmake")
mbx_getlibversion("${CMAKE_CURRENT_SOURCE_DIR}/include/crypto_mb/version.h")

if ((NOT DEFINED MBX_VER_MAJOR) OR
    (NOT DEFINED MBX_VER_MINOR) OR
    (NOT DEFINED MBX_VER_REV)   OR 
    (NOT DEFINED MBX_INTERFACE_VERSION_MAJOR) OR
    (NOT DEFINED MBX_INTERFACE_VERSION_MINOR))
    message(WARNING "Cannot parse version from crypto_mb/version.h file. The project might be corrupted.")
endif()

set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)
if("${CMAKE_BUILD_TYPE}" STREQUAL "")
    message(STATUS "CMAKE_BUILD_TYPE is unset, defaulting to Release")
    set(CMAKE_BUILD_TYPE "Release")
endif()

if(NOT DEFINED MB_STANDALONE)
  set(MB_STANDALONE true)
endif()

set(MB_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/include")

set(MB_DYN_LIB_TARGET    "crypto_mb")   # Dynamic library
set(MB_STATIC_LIB_TARGET "crypto_mb_s") # Static library

if(MB_STANDALONE)
    foreach( OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES} )
        string( TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG )
        set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} "${CMAKE_BINARY_DIR}/bin")
        set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} "${CMAKE_BINARY_DIR}/bin")
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} "${CMAKE_BINARY_DIR}/bin")
    endforeach( OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES )
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
else()
    file(GLOB MB_PUBLIC_HEADERS "${MB_INCLUDE_DIRS}/crypto_mb/*.h*")

    # Make variables below accessible in parent CMakeLists.txt
    set(MBX_INTERFACE_VERSION ${MBX_INTERFACE_VERSION} PARENT_SCOPE)
    set(MB_PUBLIC_HEADERS ${MB_PUBLIC_HEADERS} PARENT_SCOPE)
    set(MB_DYN_LIB_TARGET    "${MB_DYN_LIB_TARGET}" PARENT_SCOPE)
    set(MB_STATIC_LIB_TARGET "${MB_STATIC_LIB_TARGET}" PARENT_SCOPE)
endif()

find_package(OpenSSL 1.1.0 REQUIRED) # set -DOPENSSL_INCLUDE_DIR= -DOPENSSL_LIBRARIES= -DOPENSSL_ROOT_DIR= to use patched
list(APPEND MB_INCLUDE_DIRS "${OPENSSL_INCLUDE_DIR}")

if(BN_OPENSSL_PATCH)
    # Link static OpenSSL only for patched version
    set(OPENSSL_USE_STATIC_LIBS TRUE)
endif()

include_directories(
     ${MB_INCLUDE_DIRS}
     $<$<C_COMPILER_ID:Intel>:$ENV{ROOT}/compiler/include $ENV{ROOT}/compiler/include/icc>
     $<$<NOT:$<C_COMPILER_ID:Intel>>:${CMAKE_SYSTEM_INCLUDE_PATH}>
     $<$<OR:$<C_COMPILER_ID:Intel>,$<BOOL:${MSVC_IDE}>>:$ENV{INCLUDE}>
)

add_subdirectory(src)
