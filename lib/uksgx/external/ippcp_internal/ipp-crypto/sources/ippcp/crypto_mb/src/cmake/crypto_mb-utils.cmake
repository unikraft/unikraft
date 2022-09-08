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

macro(mbx_getlibversion VERSION_FILE)
    unset(MBX_VER_MAJOR)
    unset(MBX_VER_MINOR)
    unset(MBX_VER_REV)
    unset(MBX_VER_FULL)
    file(STRINGS "${VERSION_FILE}" FILE_CONTENTS)
    foreach(LINE ${FILE_CONTENTS})
        if ("${LINE}" MATCHES "#define +MBX_VER_MAJOR")
            string(REGEX REPLACE "^#define +MBX_VER_MAJOR +\([0-9]+\).*$" "\\1" MBX_VER_MAJOR ${LINE})
        endif()
        if ("${LINE}" MATCHES "#define +MBX_VER_MINOR")
            string(REGEX REPLACE "^#define +MBX_VER_MINOR +\([0-9]+\).*$" "\\1" MBX_VER_MINOR ${LINE})
        endif()
        if ("${LINE}" MATCHES "#define +MBX_VER_REV")
            string(REGEX REPLACE "^#define +MBX_VER_REV +\([0-9]+\).*$" "\\1" MBX_VER_REV ${LINE})
        endif()
        if ("${LINE}" MATCHES "#define MBX_INTERFACE_VERSION_MAJOR")
            string(REGEX REPLACE "^#define +MBX_INTERFACE_VERSION_MAJOR +\([0-9]+\).*$" "\\1" MBX_INTERFACE_VERSION_MAJOR ${LINE})
        endif()
        if ("${LINE}" MATCHES "#define MBX_INTERFACE_VERSION_MINOR")
            string(REGEX REPLACE "^#define +MBX_INTERFACE_VERSION_MINOR +\([0-9]+\).*$" "\\1" MBX_INTERFACE_VERSION_MINOR ${LINE})
        endif()
    endforeach()
    set(MBX_VER_FULL "${MBX_VER_MAJOR}.${MBX_VER_MINOR}.${MBX_VER_REV}")
    set(MBX_INTERFACE_VERSION "${MBX_INTERFACE_VERSION_MAJOR}.${MBX_INTERFACE_VERSION_MINOR}")
    unset(FILE_CONTENTS)
endmacro(mbx_getlibversion)
