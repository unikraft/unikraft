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

macro(ippcp_getlibversion VERSION_FILE)
    unset(IPPCP_VERSION_MAJOR)
    unset(IPPCP_VERSION_MINOR)
    unset(IPPCP_VERSION_UPDATE)
    unset(IPPCP_VERSION)
    unset(IPPCP_INTERFACE_VERSION_MAJOR)
    unset(IPPCP_INTERFACE_VERSION_MINOR)
    file(STRINGS "${VERSION_FILE}" FILE_CONTENTS)
    foreach(LINE ${FILE_CONTENTS})
        if ("${LINE}" MATCHES "#define IPP_VERSION_MAJOR")
            string(REGEX REPLACE "^#define +IPP_VERSION_MAJOR +\([0-9]+\).*$" "\\1" IPPCP_VERSION_MAJOR ${LINE})
        endif()
        if ("${LINE}" MATCHES "#define IPP_VERSION_MINOR")
            string(REGEX REPLACE "^#define +IPP_VERSION_MINOR +\([0-9]+\).*$" "\\1" IPPCP_VERSION_MINOR ${LINE})
        endif()
        if ("${LINE}" MATCHES "#define IPP_VERSION_UPDATE")
            string(REGEX REPLACE "^#define +IPP_VERSION_UPDATE +\([0-9]+\).*$" "\\1" IPPCP_VERSION_UPDATE ${LINE})
        endif()
        if ("${LINE}" MATCHES "#define IPP_INTERFACE_VERSION_MAJOR")
            string(REGEX REPLACE "^#define +IPP_INTERFACE_VERSION_MAJOR +\([0-9]+\).*$" "\\1" IPPCP_INTERFACE_VERSION_MAJOR ${LINE})
        endif()
        if ("${LINE}" MATCHES "#define IPP_INTERFACE_VERSION_MINOR")
            string(REGEX REPLACE "^#define +IPP_INTERFACE_VERSION_MINOR +\([0-9]+\).*$" "\\1" IPPCP_INTERFACE_VERSION_MINOR ${LINE})
        endif()
    endforeach()
    set(IPPCP_VERSION "${IPPCP_VERSION_MAJOR}.${IPPCP_VERSION_MINOR}.${IPPCP_VERSION_UPDATE}")
    set(IPPCP_INTERFACE_VERSION "${IPPCP_INTERFACE_VERSION_MAJOR}.${IPPCP_INTERFACE_VERSION_MINOR}")
    unset(FILE_CONTENTS)
endmacro(ippcp_getlibversion)
