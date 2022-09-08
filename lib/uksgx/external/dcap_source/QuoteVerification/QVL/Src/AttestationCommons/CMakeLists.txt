# Copyright (c) 2020, Intel Corporation
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
# 3. Neither the name of the copyright holder nor the names of its contributors
#    may be used to endorse or promote products derived from this software
#    without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
# THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
# BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
# OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
# OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
# OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
# OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
# EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

cmake_minimum_required(VERSION 3.12)

project(AttestationCommons CXX)

hunter_add_package(OpenSSL)
find_package(OpenSSL 1.1.1 EXACT REQUIRED)

file(GLOB SOURCE_FILES src/*.cpp
        src/Utils/*.cpp
)

add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES})
add_library(${PROJECT_NAME}Static STATIC ${SOURCE_FILES})

set(ATTESTATION_COMMONS_API_INCLUDE ${PROJECT_SOURCE_DIR}/include CACHE INTERNAL "${PROJECT_NAME} API")

include_directories(
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
        include/OpensslHelpers
        include/Utils
)

target_link_libraries(${PROJECT_NAME} PUBLIC
        OpenSSL::SSL
        OpenSSL::Crypto
)

target_link_libraries(${PROJECT_NAME}Static PUBLIC
        OpenSSL::SSL
        OpenSSL::Crypto
)

######### QVL Enclave related settings #################################################################################

if (BUILD_ENCLAVE)

    add_library(${PROJECT_NAME}StaticEnclave STATIC ${SOURCE_FILES})

    target_link_libraries(${PROJECT_NAME}StaticEnclave PRIVATE
            "-Wl,--no-undefined -nostdlib -nodefaultlibs -nostartfiles -L${SGX_OPENSSL}/lib64 -L${SGX_SDK}/lib64"
            "-Wl,--start-group -lsgx_tstdc -lsgx_tcxx -lsgx_tcrypto -lsgx_tsgxssl_crypto -Wl,--end-group"
            "-Wl,-Bstatic -Wl,-Bsymbolic"
            "-Wl,-pie,-eenclave_entry -Wl,--export-dynamic"
            "-Wl,--defsym,__ImageBase=0"
            )

    target_include_directories(${PROJECT_NAME}StaticEnclave SYSTEM PRIVATE
            "${SGX_SDK}/include"
            "${SGX_SDK}/include/tlibc"
            "${SGX_SDK}/include/libcxx"
            "${SGX_SDK}/include/stdc++"
            "${SGX_OPENSSL}/include"
            )

    target_compile_definitions(${PROJECT_NAME}StaticEnclave PUBLIC SGX_TRUSTED=true)

    target_compile_options(${PROJECT_NAME}StaticEnclave PRIVATE
            $<$<COMPILE_LANGUAGE:CXX>:-nostdinc++>
            $<$<COMPILE_LANGUAGE:C>:-nostdinc>
            )

    install(TARGETS ${PROJECT_NAME}StaticEnclave DESTINATION lib)

endif()

########################################################################################################################

if(MSVC)
    target_link_libraries(${PROJECT_NAME} PUBLIC wsock32 ws2_32 crypt32)
    target_link_libraries(${PROJECT_NAME}Static PUBLIC wsock32 ws2_32 crypt32)

    target_compile_definitions(${PROJECT_NAME} PUBLIC _ATTESTATIONCOMMONS_EXPORTS)
    target_compile_definitions(${PROJECT_NAME}Static PUBLIC ATTESTATIONCOMMONS_STATIC)
else()
    set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "-Wl,--exclude-libs,ALL") # hide external libs symbols in shared object, do not add it for Enclave as we use LDS file here
endif()

install(TARGETS ${PROJECT_NAME} DESTINATION lib)
install(TARGETS ${PROJECT_NAME}Static DESTINATION lib)

if(BUILD_TESTS)
    add_subdirectory(test)
endif()
