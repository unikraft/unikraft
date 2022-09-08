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

if(UNIX)
    if(APPLE)
        set(OS_STRING "macosx")
        set(OS_DEFAULT_COMPILER Intel)
    else()
        set(OS_STRING "linux")
        set(OS_DEFAULT_COMPILER GNU)
    endif()
else()
    set(OS_STRING "windows")
    set(OS_DEFAULT_COMPILER Intel)
endif()

# Find appropriate compiler options file
set(COMPILER_OPTIONS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/cmake/${OS_STRING}")
if(EXISTS "${COMPILER_OPTIONS_DIR}/${CMAKE_C_COMPILER_ID}.cmake")
    set(COMPILER_OPTIONS_FILE "${COMPILER_OPTIONS_DIR}/${CMAKE_C_COMPILER_ID}.cmake")
else()
    set(COMPILER_OPTIONS_FILE "${COMPILER_OPTIONS_DIR}/${OS_DEFAULT_COMPILER}.cmake")
    message(WARNING "Unknown compiler, using options from the OS default one: ${OS_DEFAULT_COMPILER}")
endif()