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

#
# Intel® Integrated Performance Primitives Cryptography (Intel® IPP Cryptography) examples
#

# List of examples for targets generation
set(IPPCP_EXAMPLES
  # AES examples
  aes/aes-256-ctr-encryption.cpp
  aes/aes-256-ctr-decryption.cpp
  # RSA SSA-PSS examples
  rsa/rsa-3k-pss-sha384-type1-signature.cpp
  rsa/rsa-1k-pss-sha1-verification.cpp
  # RSA OAEP examples
  rsa/rsa-1k-oaep-sha1-encryption.cpp
  rsa/rsa-1k-oaep-sha1-type2-decryption.cpp
  # SMS4 examples
  sms4/sms4-128-cbc-encryption.cpp
  sms4/sms4-128-cbc-decryption.cpp
  )

cmake_policy(SET CMP0003 NEW)

# Custom target to build ALL examples at once
add_custom_target(ippcp_examples_all)
set_target_properties(ippcp_examples_all PROPERTIES FOLDER "examples")

function(ippcp_define_example out_target source_file category)
  # Extract file name without directory or longest extension
  get_filename_component(name "${source_file}" NAME_WE)
  # Add suffix for nonpic build
  if (NONPIC_LIB)
    set(suffix "-nonpic")
  endif()
  set(local_target "example_${name}${suffix}")
  # link additional sources if defined in categoryOptions.cmake
  set(additional_sources "${category}_CATEGORY_COMMON_SOURCES")
  add_executable(${local_target} "${source_file}"
                                 $<$<BOOL:${${additional_sources}}>:${${additional_sources}}>)
  # Static linking with merged lib is only supported
  set(LIBRARY_NAMES_LIST ${IPPCP_LIB_MERGED})
  ippcp_example_set_build_options(${local_target} "${LIBRARY_NAMES_LIST}")
  set_target_properties(${local_target} PROPERTIES
                        PROJECT_LABEL "(example) ${name}"    # Set name of the target in IDE
                        FOLDER "examples/${category}"        # Group projects in solution folder
                        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_OUTPUT_DIR}/$<UPPER_CASE:$<CONFIG>>/examples"  # Set output directory for examples in the build folder
                        RUNTIME_OUTPUT_DIRECTORY_DEBUG   "${CMAKE_OUTPUT_DIR}/$<UPPER_CASE:$<CONFIG>>/examples")

  # Add a single target to build all examples of the same category (e.g. 'make ippcp_examples_aes')
  set(parent_target ippcp_examples_${category})
  if(NOT TARGET ${parent_target})
    add_custom_target(${parent_target})
    set_target_properties(${parent_target} PROPERTIES FOLDER "examples")
    if(TARGET ippcp_examples_all)
      add_dependencies(ippcp_examples_all ${parent_target})
    endif()
  endif()
  add_dependencies(${parent_target} ${local_target})
  set(${out_target} ${local_target} PARENT_SCOPE)
endfunction()

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_LIST_DIR)
# Build with standalone library
  cmake_minimum_required(VERSION 3.1)

  project("Intel IPP Cryptography Examples" CXX)
  set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)

  option(BUILD_EXAMPLES "Build examples" ON)

  list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR})
  find_package(IPPCrypto REQUIRED MODULE)

  if (NOT IPPCRYPTO_FOUND)
    message(FATAL_ERROR "No Intel IPP Cryptography library found on the system. To build examples with pre-built library, please specify -DIPPCRYPTO_ROOT_DIR=<path> option, where <path> is the path to directory that contains include/ and lib/ folders of Intel IPP Cryptography product.")
  endif()

  # Define library to link
  list(GET IPPCRYPTO_LIBRARIES 0 IPPCP_LIB_MERGED)
  # Define include folder
  set(IPP_CRYPTO_INCLUDE_DIR ${IPPCRYPTO_INCLUDE_DIRS})
  # Define output directory
  if(NOT CMAKE_OUTPUT_DIR)
    set(CMAKE_OUTPUT_DIR "${CMAKE_BINARY_DIR}/.build")
  endif()
else()
# Build with library sources
  if(NOT BUILD_EXAMPLES OR NOT MERGED_BLD)
    message(FATAL_ERROR "Only merged library build is currently supported for Intel IPP Cryptography examples. Use -DMERGED_BLD:BOOL=on options.")
  endif()
endif()

include(examplesBuildOptions.cmake)

foreach(example_filename ${IPPCP_EXAMPLES})
  # Extract example category from its subdirectory
  get_filename_component(category "${example_filename}" DIRECTORY)

  # Source additional options that may exist for category
  include(${category}/categoryOptions.cmake OPTIONAL)

  ippcp_define_example(example ${example_filename} ${category})
endforeach()
