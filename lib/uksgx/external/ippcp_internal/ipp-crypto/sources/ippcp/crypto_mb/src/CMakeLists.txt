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

# Define defaults for every supported compiler  
set(DEFAULT_GNU_COMPILER_VER 8.2.0)
set(DEFAULT_CLANG_COMPILER_VER 9.0.0)
set(DEFAULT_Intel_COMPILER_VER 19.0.0)

# Check compiler version
if(("${CMAKE_C_COMPILER_ID}" STREQUAL "GNU") AND (CMAKE_C_COMPILER_VERSION VERSION_LESS DEFAULT_GNU_COMPILER_VER))
    message(FATAL_ERROR "GNU C Compiler version must be 8.2 or higher")
endif()
if(("${CMAKE_C_COMPILER_ID}" STREQUAL "Clang") AND (CMAKE_C_COMPILER_VERSION VERSION_LESS DEFAULT_CLANG_COMPILER_VER))
  message(FATAL_ERROR "Clang C Compiler version must be 9.0 or higher")
endif()
if(("${CMAKE_C_COMPILER_ID}" STREQUAL "Intel") AND (CMAKE_C_COMPILER_VERSION VERSION_LESS DEFAULT_Intel_COMPILER_VER))
    message(FATAL_ERROR "Compiler version must be 19.0 or higher")
endif()

set(CRYPTO_MB_SOURCES_DIR ${CMAKE_CURRENT_SOURCE_DIR})

include("${CRYPTO_MB_SOURCES_DIR}/cmake/common.cmake")
include(${COMPILER_OPTIONS_FILE}) # Get ${CMAKE_C_FLAGS}, ${CMAKE_CXX_FLAGS} and ${AVX512_CFLAGS}

set(AVX512_LIBRARY_DEFINES "${AVX512_LIBRARY_DEFINES}" "${MB_LIBRARIES_DEFINES}" "USE_AMS_5x" "SIMD_LEN=512")

# Sources
file(GLOB RSA_AVX512_SOURCES    "${CRYPTO_MB_SOURCES_DIR}/rsa/*.c"
                                "${CRYPTO_MB_SOURCES_DIR}/rsa/avx512_primitives/*.c")
file(GLOB COMMON_SOURCES        "${CRYPTO_MB_SOURCES_DIR}/common/*.c")
file(GLOB X25519_SOURCES        "${CRYPTO_MB_SOURCES_DIR}/x25519/*.c")
file(GLOB ECNIST_SOURCES        "${CRYPTO_MB_SOURCES_DIR}/ecnist/*.c")
file(GLOB SM3_SOURCES           "${CRYPTO_MB_SOURCES_DIR}/sm3/*.c")
file(GLOB ED25519_SOURCES       "${CRYPTO_MB_SOURCES_DIR}/ed25519/*.c")

# Headers
file(GLOB PUB_HEADERS       "${CMAKE_CURRENT_SOURCE_DIR}/include/crypto_mb/*.h")
file(GLOB COMMON_HEADERS    "${CMAKE_CURRENT_SOURCE_DIR}/include/internal/common/*.h")
file(GLOB ECNIST_HEADERS    "${CMAKE_CURRENT_SOURCE_DIR}/include/internal/ecnist/*.h")
file(GLOB RSA_HEADERS       "${CMAKE_CURRENT_SOURCE_DIR}/include/internal/rsa/*.h")
file(GLOB SM3_HEADERS       "${CMAKE_CURRENT_SOURCE_DIR}/include/internal/sm3/*.h")
file(GLOB ED255193_HEADERS  "${CMAKE_CURRENT_SOURCE_DIR}/include/internal/ed25519/*.h")
file(GLOB OPENSSL_HEADERS   "${OPENSSL_INCLUDE_DIR}/openssl/*.h")

set(CRYPTO_MB_SOURCES ${RSA_AVX512_SOURCES} ${COMMON_SOURCES} ${X25519_SOURCES} ${ECNIST_SOURCES} ${SM3_SOURCES} ${ED25519_SOURCES})
set(CRYPTO_MB_HEADERS ${PUB_HEADERS} ${COMMON_HEADERS} ${ECNIST_HEADERS} ${RSA_HEADERS} ${SM3_HEADERS} ${ED255193_HEADERS} ${OPENSSL_HEADERS})

set(WIN_RESOURCE_FILE ${CMAKE_CURRENT_SOURCE_DIR}/common/crypto_mb_ver.rc)

list(REMOVE_ITEM CRYPTO_MB_SOURCES ${CRYPTO_MB_SOURCES_DIR}/common/cpu_features.c)

if("${OS_STRING}" STREQUAL "windows")
    set_source_files_properties(${CRYPTO_MB_SOURCES_DIR}/common/cpu_features.c PROPERTIES  COMPILE_FLAGS  "${CMAKE_C_FLAGS_SECURITY} /Od")
else()
    set_source_files_properties(${CRYPTO_MB_SOURCES_DIR}/common/cpu_features.c PROPERTIES  COMPILE_FLAGS  "${CMAKE_C_FLAGS_SECURITY} -O0")
endif()

if(BN_OPENSSL_PATCH) # Off by default
    list(APPEND AVX512_LIBRARY_DEFINES "BN_OPENSSL_PATCH")
endif()
                                      
set(MB_LIB_TARGET ${MB_DYN_LIB_TARGET})

set_source_files_properties(${CRYPTO_MB_SOURCES} PROPERTIES COMPILE_DEFINITIONS  "${AVX512_LIBRARY_DEFINES}"
                                                            COMPILE_FLAGS        "${AVX512_CFLAGS} ${CMAKE_C_FLAGS_SECURITY}")

# Create shared library
if(DYNAMIC_LIB OR MB_STANDALONE)
    if(WIN32)
        add_library(${MB_DYN_LIB_TARGET} SHARED ${CRYPTO_MB_HEADERS} ${CRYPTO_MB_SOURCES} ${CRYPTO_MB_SOURCES_DIR}/common/cpu_features.c ${WIN_RESOURCE_FILE})
    else()
        add_library(${MB_DYN_LIB_TARGET} SHARED ${CRYPTO_MB_HEADERS} ${CRYPTO_MB_SOURCES} ${CRYPTO_MB_SOURCES_DIR}/common/cpu_features.c)
    endif()

    set_target_properties(${MB_DYN_LIB_TARGET} PROPERTIES C_VISIBILITY_PRESET hidden)
    set_target_properties(${MB_DYN_LIB_TARGET} PROPERTIES VISIBILITY_INLINES_HIDDEN ON)
    set_target_properties(${MB_DYN_LIB_TARGET} PROPERTIES LINK_FLAGS "${LINK_FLAGS_DYNAMIC} ${LINK_FLAG_SECURITY}")

    if(UNIX)
        set_target_properties(${MB_DYN_LIB_TARGET} PROPERTIES  VERSION   ${MBX_INTERFACE_VERSION}
                                                               SOVERSION ${MBX_INTERFACE_VERSION_MAJOR})
    endif()

    target_link_libraries(${MB_DYN_LIB_TARGET} OpenSSL::Crypto)

    # Installation of the shared library by default 
    install(TARGETS ${MB_DYN_LIB_TARGET} DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
endif(DYNAMIC_LIB OR MB_STANDALONE)

if(WIN32)
    add_library(${MB_STATIC_LIB_TARGET} STATIC ${CRYPTO_MB_HEADERS} ${CRYPTO_MB_SOURCES} ${CRYPTO_MB_SOURCES_DIR}/common/cpu_features.c ${WIN_RESOURCE_FILE}) 
else()
    add_library(${MB_STATIC_LIB_TARGET} STATIC ${CRYPTO_MB_HEADERS} ${CRYPTO_MB_SOURCES} ${CRYPTO_MB_SOURCES_DIR}/common/cpu_features.c) 
endif()

set_target_properties(${MB_STATIC_LIB_TARGET} PROPERTIES C_VISIBILITY_PRESET hidden)
set_target_properties(${MB_STATIC_LIB_TARGET} PROPERTIES VISIBILITY_INLINES_HIDDEN ON)

target_link_libraries(${MB_STATIC_LIB_TARGET} OpenSSL::Crypto)
if(WIN32)
    set_target_properties(${MB_STATIC_LIB_TARGET} PROPERTIES OUTPUT_NAME "${MB_LIB_TARGET}mt")
else()
    set_target_properties(${MB_STATIC_LIB_TARGET} PROPERTIES OUTPUT_NAME "${MB_LIB_TARGET}")
endif()

# Static lib installation
if(MB_STANDALONE)
    set(PUBLIC_HEADERS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../include/crypto_mb/)
    install(TARGETS ${MB_STATIC_LIB_TARGET} DESTINATION ${CMAKE_INSTALL_PREFIX}/lib PUBLIC_HEADER DESTINATION include)
endif()

# Public headers installation
install(DIRECTORY ${PUBLIC_HEADERS_DIR}
        DESTINATION ${CMAKE_INSTALL_PREFIX}/include/crypto_mb
        FILES_MATCHING PATTERN "*.h*")

# add_subdirectory(apps)
