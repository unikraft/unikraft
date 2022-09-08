# Copyright (c) 2017-2018, Intel Corporation
#

# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# Redistribution and use in source and binary forms, with or without modification,
# are permitted provided that the following conditions are met:
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

set(SUBPROJECT_NAME ${PROJECT_NAME}_UT)

hunter_add_package(OpenSSL)
find_package(OpenSSL 1.1.1 EXACT REQUIRED)

hunter_add_package(GTest)
find_package(GTest CONFIG REQUIRED)

enable_testing()

set(QVL_SRC_DIR ${CMAKE_SOURCE_DIR}/AttestationLibrary/src)
set(QVL_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/AttestationLibrary/include)
set(QVL_COMMON_TEST_UTILS_DIR ${CMAKE_SOURCE_DIR}/AttestationLibrary/test/CommonTestUtils)
set(PARSERS_COMMON_TEST_UTILS_DIR ${CMAKE_SOURCE_DIR}/AttestationParsers/test/CommonTestUtils)

file(GLOB SOURCE_FILES *.cpp
    ${QVL_COMMON_TEST_UTILS_DIR}/*.cpp
    ${PARSERS_COMMON_TEST_UTILS_DIR}/*.cpp
)

add_executable(${SUBPROJECT_NAME} ${SOURCE_FILES})

include_directories(
    ${QVL_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${QVL_SRC_DIR}
    ${QVL_COMMON_TEST_UTILS_DIR}
    ${PARSERS_COMMON_TEST_UTILS_DIR}
)

target_link_libraries(${SUBPROJECT_NAME}
    AttestationLibraryStatic
    AttestationParsersStatic
    rapidjson
    OpenSSL::Crypto
    GTest::gtest_main
    GTest::gmock_main
)

install(TARGETS ${SUBPROJECT_NAME} DESTINATION bin)
add_test(NAME ${SUBPROJECT_NAME} COMMAND <TARGET_FILE:${SUBPROJECT_NAME}>)