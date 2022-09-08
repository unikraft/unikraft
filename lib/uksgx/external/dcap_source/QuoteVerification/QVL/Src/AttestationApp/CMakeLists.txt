# Copyright (c) 2017, Intel Corporation
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

project(AttestationApp LANGUAGES CXX)

file(GLOB SOURCE_FILES src/AppCore/*.cpp)

add_library(AppCore ${SOURCE_FILES})

target_include_directories(AppCore PUBLIC ${ATTESTATION_LIBRARY_API_INCLUDE} src)

target_link_libraries(AppCore
        PUBLIC
        argtable3
        PRIVATE
        AttestationLibrary
        )

add_executable(${PROJECT_NAME} src/main.cpp)

target_link_libraries(${PROJECT_NAME} PRIVATE AppCore)

######### QVL Enclave related settings #################################################################################

if (BUILD_ENCLAVE)
    set_source_files_properties(src/Enclave/Enclave_u.c PROPERTIES GENERATED TRUE)

    set_property(DIRECTORY PROPERTY ADDITIONAL_MAKE_CLEAN_FILES
            "${CMAKE_CURRENT_SOURCE_DIR}/src/Enclave/Enclave_u.c"
            "${CMAKE_CURRENT_SOURCE_DIR}/src/Enclave/Enclave_u.h")

    add_library(AppCoreEnclave
            ${SOURCE_FILES}
            ${CMAKE_CURRENT_SOURCE_DIR}/src/Enclave/EnclaveAdapter.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/src/Enclave/Enclave_u.c
            )

    target_include_directories(AppCoreEnclave
            SYSTEM PUBLIC
            "${SGX_SDK}/include"
            "${SGX_OPENSSL}/include/openssl"
            )

    target_include_directories(AppCoreEnclave
            PUBLIC
            ${ATTESTATION_LIBRARY_API_INCLUDE}
            src
            src/Enclave
            )

    target_link_libraries(AppCoreEnclave
            PUBLIC
            argtable3
            PRIVATE
            AttestationLibraryEnclave
            "-Wl,--no-undefined -L${SGX_SDK}/lib64 -L${SGX_OPENSSL}/lib64"
            "-Wl,--whole-archive -l${URTS_LIBRARY_NAME} -Wl,--no-whole-archive"
            "-Wl,--start-group -l${USERVICE_LIBRARY_NAME} -lsgx_usgxssl -Wl,--end-group"
            "-Wl,-Bstatic -lsgx_capable -Wl,-Bdynamic"
            )

    add_dependencies(AppCoreEnclave SignEnclave)

    target_compile_definitions(AppCoreEnclave PUBLIC SGX_TRUSTED=true)

    add_executable(${PROJECT_NAME}Enclave src/main.cpp)

    target_link_libraries(${PROJECT_NAME}Enclave PRIVATE AppCoreEnclave)

    target_compile_definitions(${PROJECT_NAME}Enclave PUBLIC SGX_TRUSTED=true)

    install(TARGETS ${PROJECT_NAME}Enclave DESTINATION bin)

endif()

########################################################################################################################

install(TARGETS ${PROJECT_NAME} DESTINATION bin)
install(DIRECTORY sampleData DESTINATION bin)

if(BUILD_TESTS)
    add_subdirectory(test)
endif()

