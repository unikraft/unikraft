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

project(AttestationLibrary CXX)

set(VERSION "1.0.0")
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/templates/Version.h.in" "${CMAKE_CURRENT_SOURCE_DIR}/src/Version/Version.h" @ONLY)

if(BUILD_DOCS)
    find_package(Doxygen REQUIRED)
    set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/templates/Doxyfile.in)
    set(DOXYGEN_OUT ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile)
    set(DOXYGEN_BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR}/doc)
    set(DOXYGEN_INSTALL_DIR ${QVL_DIST_DIR}/doc)

    configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)

    add_custom_target( doc_doxygen ALL
            COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            COMMENT "Generating API documentation with Doxygen"
            VERBATIM )

    install(DIRECTORY ${DOXYGEN_BUILD_DIR}/ DESTINATION doc)
endif()

hunter_add_package(OpenSSL)
find_package(OpenSSL 1.1.1 EXACT REQUIRED)

file(GLOB SOURCE_FILES src/*.cpp
        src/OpensslHelpers/*.cpp
        src/PckParser/*.cpp
        src/CertVerification/*.cpp
        src/QuoteVerification/*.cpp
        src/Verifiers/*.cpp
        src/Utils/*.cpp
        )

add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES})
add_library(${PROJECT_NAME}Static STATIC ${SOURCE_FILES})

set(ATTESTATION_LIBRARY_API_INCLUDE ${PROJECT_SOURCE_DIR}/include CACHE INTERNAL "${PROJECT_NAME} API")

include_directories(
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
        ${ATTESTATION_COMMONS_API_INCLUDE}
        ${ATTESTATION_PARSERS_API_INCLUDE}
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(${PROJECT_NAME} PRIVATE
        AttestationCommonsStatic
        AttestationParsersStatic
        rapidjson
        OpenSSL::SSL
        OpenSSL::Crypto
        )

target_link_libraries(${PROJECT_NAME}Static PRIVATE
        AttestationCommonsStatic
        AttestationParsersStatic
        rapidjson
        OpenSSL::SSL
        OpenSSL::Crypto
        )

if(MSVC)
    target_compile_definitions(${PROJECT_NAME}Static PUBLIC ATTESTATIONLIBRARY_STATIC)
    target_compile_definitions(${PROJECT_NAME} PUBLIC _ATTESTATIONLIBRARY_EXPORTS)
else()
    set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "-Wl,--exclude-libs,ALL") # hide external libs symbols in shared object, do not add it for Enclave as we use LDS file here
endif()

install(TARGETS ${PROJECT_NAME} DESTINATION lib)
install(TARGETS ${PROJECT_NAME}Static DESTINATION lib)
install(DIRECTORY include/ DESTINATION include)

set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "QuoteVerification")
set_target_properties(AttestationLibraryStatic PROPERTIES OUTPUT_NAME "QuoteVerificationStatic")

######### QVL Enclave related settings #################################################################################

if (BUILD_ENCLAVE)
    set_source_files_properties(src/Enclave/Enclave_t.c PROPERTIES GENERATED TRUE)

    set(SGX_EDGER8R_GENERATOR_COMMAND ${SGX_SDK}/bin/x64/sgx_edger8r
            --untrusted ${CMAKE_CURRENT_SOURCE_DIR}/src/Enclave/Enclave.edl
            --trusted   ${CMAKE_CURRENT_SOURCE_DIR}/src/Enclave/Enclave.edl
            --search-path ${CMAKE_CURRENT_SOURCE_DIR}/src/Enclave
            --search-path ${SGX_SDK}/include
            --search-path ${SGX_OPENSSL}/include
            --untrusted-dir ${CMAKE_CURRENT_SOURCE_DIR}/../AttestationApp/src/Enclave/
            --trusted-dir   ${CMAKE_CURRENT_SOURCE_DIR}/src/Enclave
            )

    add_custom_target(GenerateEnclaveFiles
            COMMAND ${SGX_EDGER8R_GENERATOR_COMMAND}
            )

    set_property(DIRECTORY PROPERTY ADDITIONAL_MAKE_CLEAN_FILES
            "${CMAKE_CURRENT_SOURCE_DIR}/src/Enclave/Enclave_t.c"
            "${CMAKE_CURRENT_SOURCE_DIR}/src/Enclave/Enclave_t.h"
            "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libQuoteVerificationEnclave.signed.so"
            )

    add_library(${PROJECT_NAME}Enclave SHARED ${SOURCE_FILES}
            ${CMAKE_CURRENT_SOURCE_DIR}/src/Enclave/Enclave_t.c
            )

    set_target_properties(${PROJECT_NAME}Enclave PROPERTIES DEBUG_POSTFIX "")
    set_target_properties(${PROJECT_NAME}Enclave PROPERTIES OUTPUT_NAME "QuoteVerificationEnclave")

    add_dependencies(${PROJECT_NAME}Enclave GenerateEnclaveFiles)

    set(SGX_EDGER8R_SIGNER_COMMAND ${SGX_SDK}/bin/x64/sgx_sign sign
            -key ${CMAKE_CURRENT_SOURCE_DIR}/src/Enclave/Enclave_private.pem
            -enclave ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libQuoteVerificationEnclave.so
            -out ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libQuoteVerificationEnclave.signed.so
            -config ${CMAKE_CURRENT_SOURCE_DIR}/src/Enclave/Enclave.config.xml
            )

    add_custom_target(SignEnclave
            COMMAND ${SGX_EDGER8R_SIGNER_COMMAND}
            DEPENDS ${PROJECT_NAME}Enclave
            )

    target_include_directories(${PROJECT_NAME}Enclave SYSTEM PRIVATE
            "${SGX_SDK}/include"
            "${SGX_SDK}/include/tlibc"
            "${SGX_SDK}/include/libcxx"
            "${SGX_SDK}/include/stdc++"
            "${SGX_OPENSSL}/include"
            )

    set(LDSCRIPT_FLAG "-Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/src/Enclave/Enclave.lds")

    target_link_libraries(${PROJECT_NAME}Enclave PRIVATE
            rapidjson
            AttestationCommonsStaticEnclave
            AttestationParsersStaticEnclave
            "-Wl,--no-undefined -nostdlib -nodefaultlibs -nostartfiles -L${SGX_OPENSSL}/lib64 -L${SGX_SDK}/lib64"
            "-Wl,--whole-archive -l${TRTS_LIBRARY_NAME} -lsgx_tsgxssl -Wl,--no-whole-archive"
            "-Wl,--start-group -lsgx_tstdc -lsgx_tcxx -lsgx_tsgxssl_crypto -l${TSERVICE_LIBRARY_NAME} -Wl,--end-group"
            "-Wl,-Bstatic -Wl,-Bsymbolic"
            "-Wl,-pie,-eenclave_entry -Wl,--export-dynamic"
            "-Wl,--defsym,__ImageBase=0"
            "${LDSCRIPT_FLAG}"
            )

    target_compile_definitions(${PROJECT_NAME}Enclave PUBLIC SGX_TRUSTED=true)

    target_compile_options(${PROJECT_NAME}Enclave PRIVATE
            $<$<COMPILE_LANGUAGE:CXX>:-nostdinc++>
            $<$<COMPILE_LANGUAGE:C>:-nostdinc>
            )

    install(TARGETS ${PROJECT_NAME}Enclave DESTINATION lib)
    install(FILES ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libQuoteVerificationEnclave.signed.so DESTINATION lib)

endif()

########################################################################################################################

if(BUILD_TESTS)
    add_subdirectory(test)
endif()