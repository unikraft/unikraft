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
# Intel® Integrated Performance Primitives Cryptography (Intel® IPP Cryptography)
# library detection routine.
#
# To use it, add the lines below to your CMakeLists.txt:
#    find_package(IPPCP REQUIRED)
#    target_link_libraries(mytarget ${IPPCP_LIBRARIES})
#
# List of the variables defined in this file:
#       IPPCP_FOUND
#       IPPCP_LIBRARIES  - list of all imported targets
#
# Configuration variables available:
#       IPPCP_SHARED     - set this to True before find_package() to search for shared library.
#       IPPCP_ARCH       - set this to 'ia32' or 'intel64' before find_package() to use library of particular arch
#                              (this variable can be auto-defined)
#

# Initialize to default values
if (NOT IPPCP_LIBRARIES)
    set(IPPCP_LIBRARIES "")
endif()

# Determine ARCH if not defined outside
if (NOT DEFINED IPPCP_ARCH)
    set(IPPCP_ARCH "ia32")
    if(CMAKE_CXX_SIZEOF_DATA_PTR EQUAL 8)
        set(IPPCP_ARCH "intel64")
    endif()
    if(CMAKE_SIZEOF_VOID_P)
        set(IPPCP_ARCH "intel64")
    endif()
endif()

if (NOT IPPCP_FIND_COMPONENTS)
    set(IPPCP_FIND_COMPONENTS "ippcp")
    
    # crypto_mb library is only for intel64
    if(${IPPCP_ARCH} MATCHES "intel64")
        set(IPPCP_FIND_COMPONENTS "${IPPCP_FIND_COMPONENTS}" "crypto_mb")
    endif()

    foreach (_component ${IPPCP_FIND_COMPONENTS})
        set(IPPCP_FIND_REQUIRED_${_component} 1)
    endforeach()
endif()

if (WIN32)
    set(_ippcp_library_prefix "")
    set(_ippcp_static_library_suffix "mt.lib")
    set(_ippcp_shared_library_suffix ".dll")
    set(_ippcp_import_library_suffix ".lib")
else()
    set(_ippcp_library_prefix "lib")
    set(_ippcp_static_library_suffix ".a")
    if (APPLE)
        set(_ippcp_shared_library_suffix ".dylib")
    else()
        set(_ippcp_shared_library_suffix ".so")
    endif()
    set(_ippcp_import_library_suffix "")
endif()

get_filename_component(_ippcrypto_root "${CMAKE_CURRENT_LIST_DIR}" REALPATH)
set(_ippcrypto_root "${_ippcrypto_root}/../../..")

macro(add_imported_library_target PATH_TO_LIBRARY PATH_TO_IMPORT_LIB LINKAGE_TYPE)
    if (EXISTS "${PATH_TO_LIBRARY}")
        if (NOT TARGET IPPCP::${_component})
            add_library(IPPCP::${_component} ${LINKAGE_TYPE} IMPORTED)
            get_filename_component(_include_dir "${_ippcrypto_root}/include" REALPATH)
            if (EXISTS "${_include_dir}")
                set_target_properties(IPPCP::${_component} PROPERTIES
                                      INTERFACE_INCLUDE_DIRECTORIES "${_include_dir}"
                                      IMPORTED_LOCATION "${PATH_TO_LIBRARY}")
                if (WIN32)
                    set_target_properties(IPPCP::${_component} PROPERTIES IMPORTED_IMPLIB "${PATH_TO_IMPORT_LIB}")
                endif()
            else()
                message(WARNING "IPPCP: Include directory does not exist: '${_include_dir}'. Intel IPP Cryptography installation might be broken.")
            endif()
            unset(_include_dir)
        endif()
        list(APPEND IPPCP_LIBRARIES IPPCP::${_component})
        set(IPPCP_${_component}_FOUND 1)
    elseif (IPPCP_FIND_REQUIRED AND IPPCP_FIND_REQUIRED_${_component})
        message(STATUS "Missed required Intel IPP Cryptography component: ${_component}")
        message(STATUS "  library not found:\n   ${PATH_TO_LIBRARY}")
        if (${LINKAGE_TYPE} MATCHES "SHARED")
            message(STATUS "You may try to search for static library by unsetting IPPCP_SHARED variable.")
        endif()
        set(IPPCP_FOUND FALSE)
    endif()
endmacro(add_imported_library_target)

foreach (_component ${IPPCP_FIND_COMPONENTS})
    set(IPPCP_${_component}_FOUND 0)

    if (IPPCP_SHARED)
        set(_ippcp_library_suffix "${_ippcp_shared_library_suffix}")
        set(_linkage_type "SHARED")
    else()
        set(_ippcp_library_suffix "${_ippcp_static_library_suffix}")
        set(_linkage_type "STATIC")
    endif()

    if (WIN32 AND ${_linkage_type} MATCHES "SHARED")
        get_filename_component(_lib     "${_ippcrypto_root}/redist/${IPPCP_ARCH}/${_ippcp_library_prefix}${_component}${_ippcp_library_suffix}" REALPATH)
        get_filename_component(_imp_lib "${_ippcrypto_root}/lib/${IPPCP_ARCH}/${_ippcp_library_prefix}${_component}${_ippcp_import_library_suffix}" REALPATH)
    else()
        get_filename_component(_lib     "${_ippcrypto_root}/lib/${IPPCP_ARCH}/${_ippcp_library_prefix}${_component}${_ippcp_library_suffix}" REALPATH)
        set(_imp_lib "")
    endif()

    add_imported_library_target("${_lib}" "${_imp_lib}" "${_linkage_type}")
endforeach()

list(REMOVE_DUPLICATES IPPCP_LIBRARIES)
unset(_ippcp_library_prefix)
unset(_ippcp_static_library_suffix)
unset(_ippcp_shared_library_suffix)
unset(_ippcp_import_library_suffix)
unset(_ippcp_library_suffix)
unset(_linkage_type)
unset(_lib)
unset(_imp_lib)
