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

macro(ippcp_extend_variable target added_value)
  set("${target}" "${${target}} ${added_value}")
endmacro()

include_directories(
  ${IPP_CRYPTO_INCLUDE_DIR}/
  ${CMAKE_CURRENT_SOURCE_DIR}/utils/
  ${CMAKE_SYSTEM_INCLUDE_PATH}
  $<$<CXX_COMPILER_ID:Intel>:$ENV{ROOT}/compiler/include
  $ENV{ROOT}/compiler/include/icc>
  # $<$<BOOL:${MSVC_IDE}>:$ENV{INCLUDE}>
  )

# Windows
if(WIN32)
  set(LINK_LIB_STATIC_RELEASE libcmt.lib  libcpmt.lib)
  set(LINK_LIB_STATIC_DEBUG   libcmtd.lib libcpmtd.lib)
  # VS2015 or later (added Universal CRT)
  if (NOT (MSVC_VERSION LESS 1900))
    set(LINK_LIB_STATIC_RELEASE ${LINK_LIB_STATIC_RELEASE} libucrt.lib  libvcruntime.lib)
    set(LINK_LIB_STATIC_DEBUG   ${LINK_LIB_STATIC_DEBUG}   libucrtd.lib libvcruntimed.lib)
  endif()

  set(LINK_FLAG_S_ST_WINDOWS "/nologo /NODEFAULTLIB /VERBOSE:SAFESEH /INCREMENTAL:NO /NXCOMPAT /DYNAMICBASE /SUBSYSTEM:CONSOLE")

  ippcp_extend_variable(CMAKE_CXX_FLAGS "/TP /nologo /W3 /EHa /Zm512 /GS")
  # Intel compiler-specific option
  if(${CMAKE_CXX_COMPILER_ID} STREQUAL "Intel")
    ippcp_extend_variable(CMAKE_CXX_FLAGS "-nologo -Qfp-speculation:safe -Qfreestanding")
  endif()

  set(OPT_FLAG "/Od")
endif(WIN32)

if(UNIX)
  # Common for Linux and macOS
  set(OPT_FLAG "-O2")
  if ((${ARCH} MATCHES "ia32") OR (NOT NONPIC_LIB))
    ippcp_extend_variable(CMAKE_CXX_FLAGS "-fstack-protector")
  endif()

  # Linux
  if(NOT APPLE)
    set(LINK_FLAG_S_ST_LINUX "-Wl,-z,noexecstack -Wl,-z,relro -Wl,-z,now")
    if(NOT NONPIC_LIB)
      ippcp_extend_variable(LINK_FLAG_S_ST_LINUX "-fpie")
    endif()

    ippcp_extend_variable(CMAKE_CXX_FLAGS "-D_FORTIFY_SOURCE=2 -Wformat -Wformat-security -fpie -fPIE")

    if(${ARCH} MATCHES "ia32")
      ippcp_extend_variable(LINK_FLAG_S_ST_LINUX "-m32")
      ippcp_extend_variable(CMAKE_CXX_FLAGS "-m32")
    endif()
  else()
    # macOS
    set(LINK_FLAG_S_ST_MACOSX "-Wl,-macosx_version_min,10.12")

    ippcp_extend_variable(CMAKE_CXX_FLAGS "-fpic")
    if(${ARCH} MATCHES "ia32")
      ippcp_extend_variable(CMAKE_CXX_FLAGS "-arch i386")
    else()
      ippcp_extend_variable(CMAKE_CXX_FLAGS "-arch x86_64")
    endif()
  endif()
endif()

macro(ippcp_example_set_build_options target link_libraries)
  if("${CMAKE_C_COMPILER_ID}" STREQUAL "GNU")
    target_link_libraries(${target} -static-libgcc -static-libstdc++)
  endif()
  target_link_libraries(${target} ${link_libraries})
  set_target_properties(${target} PROPERTIES COMPILE_FLAGS ${OPT_FLAG})
  if(WIN32)
    foreach(link ${LINK_LIB_STATIC_DEBUG})
      target_link_libraries(${target} debug ${link})
    endforeach()
    foreach(link ${LINK_LIB_STATIC_RELEASE})
      target_link_libraries(${target} optimized ${link})
    endforeach()
    set_target_properties(${target} PROPERTIES LINK_FLAGS ${LINK_FLAG_S_ST_WINDOWS})
    target_compile_options(${target} PRIVATE $<$<CONFIG:Debug>:/MTd /Zi> $<$<CONFIG:Release>:/MT /Zl>)
  else()
    if(NOT APPLE)
      set_target_properties(${target} PROPERTIES LINK_FLAGS "${LINK_FLAG_S_ST_LINUX}")
      target_link_libraries(${target} pthread)
    else()
      set_target_properties(${target} PROPERTIES LINK_FLAGS ${LINK_FLAG_S_ST_MACOSX})
    endif()
    if(CODE_COVERAGE)
      target_link_libraries(${target} ipgo)
    endif()
  endif()
endmacro()