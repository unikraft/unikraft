#===============================================================================
# Copyright 2017-2021 Intel Corporation
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
#

include (GenerateExportHeader)

set(TARGET_NAME ${LIB_NAME})

#Set platform list
if (NOT MERGED_BLD)
  if(${ARCH} MATCHES "ia32")
    set(BASE_PLATFORM_LIST px)
  else()
    set(BASE_PLATFORM_LIST mx)
  endif()
endif()
if(WIN32)
  if(${ARCH} MATCHES "ia32")
    set(BASE_PLATFORM_LIST ${BASE_PLATFORM_LIST} w7 s8 p8 g9 h9)
  else()
    set(BASE_PLATFORM_LIST ${BASE_PLATFORM_LIST} m7 n8 y8 e9 l9 k0 k1)
  endif()
endif(WIN32)
if(UNIX)
  if(APPLE)
    set(BASE_PLATFORM_LIST ${BASE_PLATFORM_LIST} y8 e9 l9 k0)
  else()
    if (${ARCH} MATCHES "ia32")
      set(BASE_PLATFORM_LIST ${BASE_PLATFORM_LIST} w7 s8 p8 g9 h9)
    else()
      set(BASE_PLATFORM_LIST ${BASE_PLATFORM_LIST} m7 n8 y8 e9 l9 n0 k0 k1)
    endif(${ARCH} MATCHES "ia32")
  endif(APPLE)
endif(UNIX)

if(PLATFORM_LIST)
  if (NOT MERGED_BLD)
    foreach(opt ${PLATFORM_LIST})
      set(FOUND_PLATFORM false)
      foreach(base_opt ${BASE_PLATFORM_LIST})
        string(STRIP "${opt}" opt_strip)
        if(opt_strip STREQUAL base_opt)
          set(FOUND_PLATFORM true)
        endif()
      endforeach(base_opt ${BASE_PLATFORM_LIST})
      if(NOT FOUND_PLATFORM)
        message (FATAL_ERROR "Incorrect platform: " ${opt})
      endif()
    endforeach(opt ${PLATFORM_LIST})
    if (PLATFORM_LIST STREQUAL "")
      message (FATAL_ERROR "PLATFORM_LIST cannot be empty")
    endif(PLATFORM_LIST STREQUAL "")
    message (STATUS "Platform list ......................... " ${PLATFORM_LIST})
  else()
    message (FATAL_ERROR "PLATFORM_LIST cannot be set, when MERGED_BLD is on")
  endif(NOT MERGED_BLD)
endif()

# Platform-specific definitions
set(px_def "_PX")
set(w7_def "_W7")
set(s8_def "_S8")
set(p8_def "_P8")
set(g9_def "_G9")
set(h9_def "_H9")
set(mx_def "_PX")
set(m7_def "_M7")
set(n8_def "_N8")
set(y8_def "_Y8")
set(e9_def "_E9")
set(l9_def "_L9")
set(n0_def "_N0")
set(k0_def "_K0")
set(k1_def "_K1")

if(NOT PLATFORM_LIST)
  set(PLATFORM_LIST ${BASE_PLATFORM_LIST})
endif()

if (NOT MERGED_BLD)
  set(PLATFORM_LIST ${PLATFORM_LIST} PARENT_SCOPE)
endif(NOT MERGED_BLD)

# define defaults for every supported compiler
set(DEFAULT_Intel_COMPILER_VER 19.0.0)
set(DEFAULT_MSVC_COMPILER_VER 19.14)
set(DEFAULT_GNU_COMPILER_VER 8.2.0)
set(DEFAULT_APPLE_CLANG_COMPILER_VER 11.0.0)
set(DEFAULT_Clang_COMPILER_VER 9.0.0)

set(DEFAULT_Intel18_COMPILER_VER 18.0.0)
set(DEFAULT_Intel19_COMPILER_VER 19.0.0)
set(DEFAULT_MSVC19_COMPILER_VER 19.14)

string(REGEX REPLACE "^([0-9]+)\\.([0-9]+)\\.([0-9]+).*$" "\\1.\\2.\\3" CMAKE_C_COMPILER_VERSION_SHORT ${CMAKE_C_COMPILER_VERSION})
string(REGEX REPLACE "^([0-9]+)\\..*$" "\\1" CMAKE_C_COMPILER_VERSION_MAJOR ${CMAKE_C_COMPILER_VERSION})

if(UNIX)
  if(APPLE)
    set(OS_STRING "macosx")
  else()
    set(OS_STRING "linux")
  endif()
else()
  set(OS_STRING   "windows")
endif()

# common build options and ${OS_DEFAULT_COMPILER}
include("${IPP_CRYPTO_DIR}/sources/cmake/${OS_STRING}/common.cmake")

if(("${CMAKE_C_COMPILER_ID}" STREQUAL "MSVC") AND (CMAKE_C_COMPILER_VERSION VERSION_LESS DEFAULT_MSVC_COMPILER_VER))
  message(FATAL_ERROR "Microsoft Visual C++ Compiler version must be 19.14 or higher (MSVC 19.14 or higher)")
endif()

if(("${CMAKE_C_COMPILER_ID}" STREQUAL "GNU") AND (CMAKE_C_COMPILER_VERSION VERSION_LESS DEFAULT_GNU_COMPILER_VER))
  message(FATAL_ERROR "GNU C Compiler version must be 8.2 or higher")
endif()

if(("${CMAKE_C_COMPILER_ID}" STREQUAL "AppleClang") AND (CMAKE_C_COMPILER_VERSION VERSION_LESS DEFAULT_APPLE_CLANG_COMPILER_VER))
  message(FATAL_ERROR "Apple Clang C Compiler version must be 11.0 or higher")
endif()

if(("${CMAKE_C_COMPILER_ID}" STREQUAL "Clang") AND (CMAKE_C_COMPILER_VERSION VERSION_LESS DEFAULT_CLANG_COMPILER_VER))
  message(FATAL_ERROR "Clang C Compiler version must be 9.0 or higher")
endif()

if(("${CMAKE_C_COMPILER_ID}" STREQUAL "Clang") OR ("${CMAKE_C_COMPILER_ID}" STREQUAL "AppleClang"))
  set(CLANG_COMPILER ON)
  set(CLANG_COMPILER ${CLANG_COMPILER} PARENT_SCOPE)
endif()

# Compiler options file
set(COMPILER_OPTIONS_FILE "${IPP_CRYPTO_DIR}/sources/cmake/${OS_STRING}")
if (EXISTS "${COMPILER_OPTIONS_FILE}/${CMAKE_C_COMPILER_ID}${CMAKE_C_COMPILER_VERSION_SHORT}.cmake")
  set(COMPILER_OPTIONS_FILE "${COMPILER_OPTIONS_FILE}/${CMAKE_C_COMPILER_ID}${CMAKE_C_COMPILER_VERSION_SHORT}.cmake")
else()
  if ("${DEFAULT_${CMAKE_C_COMPILER_ID}${CMAKE_C_COMPILER_VERSION_MAJOR}_COMPILER_VER}" STREQUAL "")
    if ("${DEFAULT_${CMAKE_C_COMPILER_ID}_COMPILER_VER}" STREQUAL "")
      set(COMPILER_OPTIONS_FILE "${COMPILER_OPTIONS_FILE}/${OS_DEFAULT_COMPILER}.cmake")
      message(WARNING "Unknown compiler, using options from the OS default one: ${OS_DEFAULT_COMPILER}")
    else()
      set(COMPILER_OPTIONS_FILE "${COMPILER_OPTIONS_FILE}/${CMAKE_C_COMPILER_ID}${DEFAULT_${CMAKE_C_COMPILER_ID}_COMPILER_VER}.cmake")
      if(CMAKE_C_COMPILER_VERSION VERSION_LESS ${DEFAULT_${CMAKE_C_COMPILER_ID}_COMPILER_VER})
        message(WARNING "Version of the compiler is lower than default, using options from: ${DEFAULT_${CMAKE_C_COMPILER_ID}_COMPILER_VER}")
      endif()
    endif()
  else()
    set(COMPILER_OPTIONS_FILE "${COMPILER_OPTIONS_FILE}/${CMAKE_C_COMPILER_ID}${DEFAULT_${CMAKE_C_COMPILER_ID}${CMAKE_C_COMPILER_VERSION_MAJOR}_COMPILER_VER}.cmake")
  endif()
endif()
message (STATUS "Using compiler options from ........... " ${COMPILER_OPTIONS_FILE})

# Assembler options
# Note: do not move this initialization after LIBRARY_DEFINES extension
set(CMAKE_USER_MAKE_RULES_OVERRIDE_ASM "${IPP_CRYPTO_DIR}/sources/cmake/CMakeASM_NASMOptions.txt")
enable_language(ASM_NASM)
if(NOT CMAKE_ASM_NASM_COMPILER_LOADED)
  message(FATAL_ERROR "Can't find assembler")
endif()
message(STATUS "ASM compiler version .................. " ${CMAKE_ASM_NASM_COMPILER})
message(STATUS "ASM object format ..................... " ${CMAKE_ASM_NASM_OBJECT_FORMAT})

# set Intel IPP Cryptography revision, if required
if(IPP_REVISION)
  # IPP_REVISION - release revision id, added into LibraryVersion
  set(LIBRARY_DEFINES "${LIBRARY_DEFINES} -DIPP_REVISION=${IPP_REVISION}")
endif()

set(LIBRARY_DEFINES "${LIBRARY_DEFINES} -D_NO_IPP_DEPRECATED") # do not warn about ippcp deprecated functions
# set BN_OPENSSL_DISABLE for Intel IPP Cryptography
# set(LIBRARY_DEFINES "${LIBRARY_DEFINES} -DBN_OPENSSL_DISABLE")

include(${COMPILER_OPTIONS_FILE}) # uses ${CMAKE_C_FLAGS} ${LIBRARY_DEFINES} ${<letter>opt} from the scope

if(UNIX)
  set(CMAKE_C_CREATE_SHARED_LIBRARY "<CMAKE_C_COMPILER> <CMAKE_SHARED_LIBRARY_C_FLAGS> <LANGUAGE_COMPILE_FLAGS> <LINK_FLAGS> ${LIB_EXPORT_FILE} <CMAKE_SHARED_LIBRARY_CREATE_C_FLAGS> <SONAME_FLAG><TARGET_SONAME> -o <TARGET> <OBJECTS> <LINK_LIBRARIES>")
  set(CMAKE_CXX_CREATE_SHARED_LIBRARY "<CMAKE_CXX_COMPILER> <CMAKE_SHARED_LIBRARY_CXX_FLAGS> <LANGUAGE_COMPILE_FLAGS> <LINK_FLAGS> ${LIB_EXPORT_FILE} <CMAKE_SHARED_LIBRARY_CREATE_CXX_FLAGS> <SONAME_FLAG><TARGET_SONAME> -o <TARGET> <OBJECTS> <LINK_LIBRARIES>")
  if("${CMAKE_C_COMPILER_ID}" STREQUAL "AppleClang")
    string(REPLACE "<SONAME_FLAG><TARGET_SONAME>" "<SONAME_FLAG> <TARGET_SONAME>" CMAKE_C_CREATE_SHARED_LIBRARY ${CMAKE_C_CREATE_SHARED_LIBRARY})
    string(REPLACE "<SONAME_FLAG><TARGET_SONAME>" "<SONAME_FLAG> <TARGET_SONAME>" CMAKE_CXX_CREATE_SHARED_LIBRARY ${CMAKE_CXX_CREATE_SHARED_LIBRARY})
  endif()
endif(UNIX)

if(NOT MERGED_BLD)
  set(IPPCP_LIB_STATIC "")
  set(IPPCP_LIB_DYNAMIC "")
endif(NOT MERGED_BLD)

file(GLOB IPPCP_PUBLIC_HEADERS
    ${IPP_CRYPTO_INCLUDE_DIR}/*.h)

file(GLOB LIBRARY_HEADERS
    ${IPP_CRYPTO_SOURCES_DIR}/*.h
    ${IPP_CRYPTO_SOURCES_INCLUDE_DIR}/*.h
    ${IPP_CRYPTO_INCLUDE_DIR}/ippcp*.h
    # RSA_SB (ifma) uses crypto_mb headers
    ${IPP_CRYPTO_SOURCES_DIR}/crypto_mb/include/crypto_mb/*.h
    ${IPP_CRYPTO_SOURCES_DIR}/crypto_mb/include/internal/*.h
   )

file(GLOB LIBRARY_C_SOURCES_ORIGINAL
    ${IPP_CRYPTO_SOURCES_DIR}/*.c
   )

file(GLOB LIBRARY_ASM_SOURCES_ORIGINAL
        ${IPP_CRYPTO_SOURCES_DIR}/asm_${ARCH}/*.asm
    )

set(INTERNAL_INCLUDE_DIR ${CMAKE_BINARY_DIR}/include/autogen)

set (C_INCLUDE_DIRECTORIES
    ${IPP_CRYPTO_SOURCES_DIR}
    ${IPP_CRYPTO_SOURCES_INCLUDE_DIR}
    ${IPP_CRYPTO_INCLUDE_DIR}
    ${INTERNAL_INCLUDE_DIR}
    # RSA_SB (ifma) uses crypto_mb headers
    ${IPP_CRYPTO_SOURCES_DIR}/crypto_mb/include
    $<$<C_COMPILER_ID:Intel>:$ENV{ROOT}/compiler/include $ENV{ROOT}/compiler/include/icc>
    $<$<NOT:$<C_COMPILER_ID:Intel>>:${CMAKE_SYSTEM_INCLUDE_PATH}>
    $<$<OR:$<C_COMPILER_ID:Intel>,$<BOOL:${MSVC_IDE}>>:$ENV{INCLUDE}>
    )

set (ASM_INCLUDE_DIRECTORIES
    ${IPP_CRYPTO_SOURCES_INCLUDE_DIR}
    ${IPP_CRYPTO_SOURCES_DIR}/asm_${ARCH}
    )

# Because of CMake bug (https://gitlab.kitware.com/cmake/cmake/issues/19178) it is impossible to add
# target specific NASM compiler options/definitions when using VS generator, but it is possible to specify
# them based on source files. So, here is the workaround - we spawn same assembler sources across different
# platform-specific directories and assign different definitions for each directory using set_source_files_properties().
#
# The same trick is used for C sources, but it aims a different purpose: to rename object files in the library archive,
# so that they do not intersect in a 'merged' library.
foreach(opt ${PLATFORM_LIST})
    # Popullate ASM source files in to corresponding folders per library 'letter'
    set(asm_cache_dir "${CMAKE_BINARY_DIR}/asm_sources/${opt}/asm_${ARCH}")
    file(MAKE_DIRECTORY ${asm_cache_dir})
    # Prefer configure_file() over simple file(COPY) as it creates dependencies, so that cmake project
    # will be regenerated each time when any original file is changed. This is needed to keep all copied files up-to-date.
    foreach (file ${LIBRARY_ASM_SOURCES_ORIGINAL})
        get_filename_component(basename ${file} NAME)
        configure_file(${file} ${asm_cache_dir}/${opt}_${basename} COPYONLY)
    endforeach()
    file (GLOB LIBRARY_ASM_SOURCES_${opt}
        ${asm_cache_dir}/*.asm
    )
    set_source_files_properties(${LIBRARY_ASM_SOURCES_${opt}} PROPERTIES COMPILE_DEFINITIONS "$<$<BOOL:${MERGED_BLD}>:_MERGED_BLD>;${${opt}_def}"
                                                                         INCLUDE_DIRECTORIES "${ASM_INCLUDE_DIRECTORIES}")

    #########################################################################################
    # Popullate C source files in to corresponding folders per library 'letter'
    if (NOT CODE_COVERAGE)
        set(c_cache_dir "${CMAKE_BINARY_DIR}/c_sources/${opt}/c_${ARCH}")
        file(MAKE_DIRECTORY ${c_cache_dir})
        # Add a prefix to the source files, so that corresponding object files in the merged library are unique named
        foreach (file ${LIBRARY_C_SOURCES_ORIGINAL})
          get_filename_component(basename ${file} NAME)
          configure_file(${file} ${c_cache_dir}/${opt}_${basename} COPYONLY)
        endforeach()
        file (GLOB LIBRARY_C_SOURCES_${opt}
            ${c_cache_dir}/*.c
        )
    else()
      # Code coverage data gathering is a bit easier, when sources for ISA-specific builds point to the same location on a filesystem
      set(LIBRARY_C_SOURCES_${opt} "${LIBRARY_C_SOURCES_ORIGINAL}")
    endif()
    set_source_files_properties(${LIBRARY_C_SOURCES_${opt}} pcpver.rc PROPERTIES INCLUDE_DIRECTORIES "${C_INCLUDE_DIRECTORIES}")
    # Set optimization flag for cpinit.c to -O1
    # Fix for problem related with MSVC compiler -O2 optimization
    if((${ARCH} STREQUAL "intel64") AND ("${CMAKE_C_COMPILER_ID}" STREQUAL "MSVC"))
        set_source_files_properties("${c_cache_dir}/${opt}_cpinit.c" PROPERTIES COMPILE_FLAGS " /O1")
    endif()
endforeach()

# Generate single-CPU headers
if(MERGED_BLD)
  set(ONE_CPU_FOLDER ${INTERNAL_INCLUDE_DIR}/single_cpu)
  set(ONE_CPU_GENERATOR   ${IPP_CRYPTO_DIR}/sources/gen_cpu_spc_header/gen_cpu_spc_1cpu_header.py)
  execute_process(COMMAND ${Python_EXECUTABLE} ${ONE_CPU_GENERATOR} ${IPP_CRYPTO_INCLUDE_DIR}/ippcp.h ${ONE_CPU_FOLDER})
  file(GLOB ONE_CPU_HEADERS "${ONE_CPU_FOLDER}/*.h")

  set(INTERNAL_GENERATOR  ${IPP_CRYPTO_DIR}/sources/gen_cpu_spc_header/gen_cpu_spc_header.py)
  execute_process(COMMAND ${Python_EXECUTABLE} ${INTERNAL_GENERATOR} ${IPP_CRYPTO_INCLUDE_DIR}/ippcp.h ${INTERNAL_INCLUDE_DIR})
  set(HEADERS ${LIBRARY_HEADERS} ${INTERNAL_INCLUDE_DIR}/ippcp_cpuspc.h)
endif(MERGED_BLD)

# Copy public headers to the output directory
foreach( OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
   string( TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG )
   file(COPY ${IPP_CRYPTO_INCLUDE_DIR}/ippcp.h DESTINATION "${CMAKE_OUTPUT_DIR}/${OUTPUTCONFIG}/include")
   file(COPY ${IPP_CRYPTO_INCLUDE_DIR}/ippcpdefs.h DESTINATION "${CMAKE_OUTPUT_DIR}/${OUTPUTCONFIG}/include")
   file(COPY ${IPP_CRYPTO_INCLUDE_DIR}/ippversion.h DESTINATION "${CMAKE_OUTPUT_DIR}/${OUTPUTCONFIG}/include")
endforeach( OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES )

foreach(opt ${PLATFORM_LIST})
  set(OPT_FLAGS_${opt} ${${opt}_opt})

  if(DYNAMIC_LIB AND NOT MERGED_BLD)
    set(IPPCP_DYN_ITER ${TARGET_NAME}_dyn_${opt})
    set(IPPCP_DYN_ITER_ASMOBJS ${TARGET_NAME}_dyn_${opt}-asmobjs)
    if(WIN32)
      add_library(${IPPCP_DYN_ITER_ASMOBJS} OBJECT ${LIBRARY_ASM_SOURCES_${opt}})
      add_library(${IPPCP_DYN_ITER} SHARED ippcp.def
                                           pcpver.rc
                                           ${LIBRARY_HEADERS}
                                           ${LIBRARY_C_SOURCES_${opt}}
                                           $<TARGET_OBJECTS:${IPPCP_DYN_ITER_ASMOBJS}>)

      set_target_properties(${IPPCP_DYN_ITER} PROPERTIES LINK_FLAGS ${LINK_FLAG_DYNAMIC_WINDOWS})
      foreach(link ${LINK_LIB_STATIC_DEBUG})
        target_link_libraries(${IPPCP_DYN_ITER} debug ${link})
      endforeach()
      foreach(link ${LINK_LIB_STATIC_RELEASE})
        target_link_libraries(${IPPCP_DYN_ITER} optimized ${link})
      endforeach()
    else()
      add_library(${IPPCP_DYN_ITER_ASMOBJS} OBJECT ${LIBRARY_ASM_SOURCES_${opt}})
      add_library(${IPPCP_DYN_ITER} SHARED ${LIBRARY_HEADERS}
                                           ${LIBRARY_C_SOURCES_${opt}}
                                           $<TARGET_OBJECTS:${IPPCP_DYN_ITER_ASMOBJS}>)
      if(APPLE)
        ## gres: copy LINK_FLAG_DYNAMIC_LINUX
        set(LINK_FLAGS ${LINK_FLAG_DYNAMIC_MACOSX})
        ## add export file
        set(LINK_FLAGS "${LINK_FLAGS} -exported_symbols_list ${IPP_CRYPTO_SOURCES_DIR}/exports.macosx.lib-export")
        set_target_properties(${IPPCP_DYN_ITER} PROPERTIES LINK_FLAGS "${LINK_FLAGS}")
        add_custom_command(TARGET ${IPPCP_DYN_ITER} POST_BUILD COMMAND
                           ${CMAKE_INSTALL_NAME_TOOL} -id @rpath/lib${TARGET_NAME}${opt}.dylib $<TARGET_FILE:${IPPCP_DYN_ITER}>)
      else()
        ## gres: copy LINK_FLAG_DYNAMIC_LINUX
        set(LINK_FLAGS ${LINK_FLAG_DYNAMIC_LINUX})
        ## add export file
        set(LINK_FLAGS "${LINK_FLAGS} ${IPP_CRYPTO_SOURCES_DIR}/exports.linux.lib-export")
        set_target_properties(${IPPCP_DYN_ITER} PROPERTIES LINK_FLAGS ${LINK_FLAGS})
      endif()
      if(NOT APPLE)
        if("${CMAKE_C_COMPILER_ID}" STREQUAL "GNU")
            target_link_libraries(${IPPCP_DYN_ITER} gcc) # gcc is because of -nostdlib
            target_link_libraries(${IPPCP_DYN_ITER} c) # for printf - debug purpose only, not used
        endif()
      endif()
    endif()
    set_target_properties(${IPPCP_DYN_ITER} PROPERTIES OUTPUT_NAME "${TARGET_NAME}${opt}"
                                                       COMPILE_FLAGS "${OPT_FLAGS_${opt}}"
                                                       COMPILE_DEFINITIONS "_IPP_DYNAMIC;${${opt}_def}"
                                                       PUBLIC_HEADER "${IPPCP_PUBLIC_HEADERS}"
                                                       )
    if(UNIX)
      set_target_properties(${IPPCP_DYN_ITER} PROPERTIES  VERSION   ${IPPCP_INTERFACE_VERSION}
                                                          SOVERSION ${IPPCP_INTERFACE_VERSION_MAJOR})
    endif()

    install(TARGETS ${IPPCP_DYN_ITER}
            LIBRARY DESTINATION "lib/${ARCH}/$<$<BOOL:${NONPIC_LIB}>:nonpic>"
            RUNTIME DESTINATION "lib/${ARCH}/$<$<BOOL:${NONPIC_LIB}>:nonpic>"
            PUBLIC_HEADER DESTINATION "include")
    list(APPEND IPPCP_LIB_DYNAMIC ${IPPCP_DYN_ITER})
  endif(DYNAMIC_LIB AND NOT MERGED_BLD)

  set(IPPCP_ST_ITER ${TARGET_NAME}_s_${opt})
  set(IPPCP_ST_ITER_ASMOBJS ${TARGET_NAME}_s_${opt}-asmobjs)
  if(MERGED_BLD)
    add_library(${IPPCP_ST_ITER} OBJECT ${LIBRARY_HEADERS} ${LIBRARY_C_SOURCES_${opt}})
    add_library(${IPPCP_ST_ITER_ASMOBJS} OBJECT ${LIBRARY_ASM_SOURCES_${opt}})
    set(merged_dependency ${merged_dependency} $<TARGET_OBJECTS:${IPPCP_ST_ITER}>
                                               $<TARGET_OBJECTS:${IPPCP_ST_ITER_ASMOBJS}>)
  else()
    add_library(${IPPCP_ST_ITER_ASMOBJS} OBJECT ${LIBRARY_ASM_SOURCES_${opt}})
    add_library(${IPPCP_ST_ITER} STATIC ${LIBRARY_HEADERS}
                                        ${LIBRARY_C_SOURCES_${opt}}
                                        $<TARGET_OBJECTS:${IPPCP_ST_ITER_ASMOBJS}>)
  endif()

  if(WIN32)
    set_target_properties(${IPPCP_ST_ITER} ${IPPCP_ST_ITER_ASMOBJS} PROPERTIES STATIC_LIBRARY_FLAGS ${LINK_FLAG_STATIC_WINDOWS})
  endif()

  set_target_properties(${IPPCP_ST_ITER} PROPERTIES COMPILE_FLAGS "${OPT_FLAGS_${opt}}"
                                                    COMPILE_DEFINITIONS "$<$<BOOL:${MERGED_BLD}>:_MERGED_BLD>;${${opt}_def}")
  # Merged build install is handled in another target
  if (NOT MERGED_BLD)
    set_target_properties(${IPPCP_ST_ITER} PROPERTIES PUBLIC_HEADER "${IPPCP_PUBLIC_HEADERS}")
    install(TARGETS ${IPPCP_ST_ITER}
            ARCHIVE DESTINATION "lib/${ARCH}/$<$<BOOL:${NONPIC_LIB}>:nonpic>"
            PUBLIC_HEADER DESTINATION "include")
  endif()

  list(APPEND IPPCP_LIB_STATIC ${IPPCP_ST_ITER})
endforeach()

if(NOT MERGED_BLD)
  set(IPPCP_LIB_STATIC  ${IPPCP_LIB_STATIC}  PARENT_SCOPE)
  set(IPPCP_LIB_DYNAMIC ${IPPCP_LIB_DYNAMIC} PARENT_SCOPE)
endif()

if(MERGED_BLD)
  set(DISPATCHER ${CMAKE_BINARY_DIR}/dispatcher)
  set(IPPCP_API ${IPP_CRYPTO_INCLUDE_DIR}/ippcp.h)
  file(MAKE_DIRECTORY ${DISPATCHER})

  if(WIN32)
    if(${ARCH} MATCHES "ia32")
      set(DISPATCHER_GENERATOR ${IPP_CRYPTO_DISPATCHER_DIR}/gen_disp_win32.py)
    else()
      set(DISPATCHER_GENERATOR ${IPP_CRYPTO_DISPATCHER_DIR}/gen_disp_win64.py)
    endif()
  elseif(UNIX)
    if(APPLE)
      set(DISPATCHER_GENERATOR ${IPP_CRYPTO_DISPATCHER_DIR}/gen_disp_mac64.py)
    else()
      if(NOT NONPIC_LIB)
        if(${ARCH} MATCHES "ia32")
          set(DISPATCHER_GENERATOR ${IPP_CRYPTO_DISPATCHER_DIR}/gen_disp_lin32.py)
        else()
          set(DISPATCHER_GENERATOR ${IPP_CRYPTO_DISPATCHER_DIR}/gen_disp_lin64.py)
        endif()
      else()
        if(${ARCH} MATCHES "ia32")
          set(DISPATCHER_GENERATOR ${IPP_CRYPTO_DISPATCHER_DIR}/gen_disp_lin32.nonpic.py)
        else()
          set(DISPATCHER_GENERATOR ${IPP_CRYPTO_DISPATCHER_DIR}/gen_disp_lin64.nonpic.py)
        endif()
      endif()
    endif()
  endif(WIN32)

  execute_process(COMMAND ${Python_EXECUTABLE} ${DISPATCHER_GENERATOR} -i ${IPPCP_API} -o ${DISPATCHER} -l "${PLATFORM_LIST}" -c ${CMAKE_C_COMPILER_ID}
    RESULT_VARIABLE result
    )

  file(GLOB DISPATCHER_HEADERS
    ${CMAKE_BINARY_DIR}/dispatcher/*.h
    )

  file(GLOB DISPATCHER_C_SOURCES
    ${CMAKE_BINARY_DIR}/dispatcher/*.c
    )

  file(GLOB DISPATCHER_ASM_SOURCES
    ${CMAKE_BINARY_DIR}/dispatcher/*.asm
    )

  set(IPPCP_LIB_MERGED ${TARGET_NAME}_s)
  add_library(${IPPCP_LIB_MERGED} STATIC ${DISPATCHER_HEADERS} ${DISPATCHER_C_SOURCES} ${DISPATCHER_ASM_SOURCES} ${merged_dependency})

  set(IPPCP_LIB_MERGED ${IPPCP_LIB_MERGED} PARENT_SCOPE)

  if(WIN32)
    set_target_properties(${IPPCP_LIB_MERGED} PROPERTIES OUTPUT_NAME "${TARGET_NAME}mt")
    set_target_properties(${IPPCP_LIB_MERGED} PROPERTIES STATIC_LIBRARY_FLAGS ${LINK_FLAG_STATIC_WINDOWS})
  else()
    set_target_properties(${IPPCP_LIB_MERGED} PROPERTIES OUTPUT_NAME "${TARGET_NAME}")
  endif(WIN32)

  set_target_properties(${IPPCP_LIB_MERGED} PROPERTIES PUBLIC_HEADER "${IPPCP_PUBLIC_HEADERS}"
                                                       PRIVATE_HEADER "${ONE_CPU_HEADERS}")

  install(TARGETS ${IPPCP_LIB_MERGED}
          ARCHIVE DESTINATION "lib/${ARCH}/$<$<BOOL:${NONPIC_LIB}>:nonpic>"
          PUBLIC_HEADER DESTINATION "include"
          PRIVATE_HEADER DESTINATION "tools/${ARCH}/staticlib")

  set_source_files_properties(${DISPATCHER_C_SOURCES} pcpver.rc PROPERTIES INCLUDE_DIRECTORIES "${C_INCLUDE_DIRECTORIES}")
  # protection (_FORTIFY_SOURCE) and optimization flags for dispatcher
  if(UNIX)
    if(${ARCH} MATCHES "ia32")
      set_source_files_properties(${DISPATCHER_C_SOURCES} PROPERTIES COMPILE_FLAGS "${CC_FLAGS_INLINE_ASM_UNIX_IA32} -D_GNU_SOURCE -D_FORTIFY_SOURCE=2 -O2")
    else()
      set_source_files_properties(${DISPATCHER_C_SOURCES} PROPERTIES COMPILE_FLAGS "${CC_FLAGS_INLINE_ASM_UNIX_INTEL64} -D_GNU_SOURCE -D_FORTIFY_SOURCE=2 -O2")
    endif()
  endif()

  # Single merged dynamic lib
  if(DYNAMIC_LIB)
    set(IPPCP_LIB_PCS ${TARGET_NAME}_dyn)
    set(IPPCP_LIB_PCS ${IPPCP_LIB_PCS} PARENT_SCOPE)

    if(WIN32)
      add_library(${IPPCP_LIB_PCS} SHARED ippcp.def emptyfile.c pcpver.rc) # emptyfile.c - Visual Studio does not produce a .dll without source files
    else()
      add_library(${IPPCP_LIB_PCS} SHARED emptyfile.c) # emptyfile.c to suppress the cmake warning
    endif()

    set_target_properties(${IPPCP_LIB_PCS} PROPERTIES LINKER_LANGUAGE C
                                                      COMPILE_DEFINITIONS  "_MERGED_BLD"
                                                      PUBLIC_HEADER "${IPPCP_PUBLIC_HEADERS}"
                                                      PRIVATE_HEADER "${ONE_CPU_HEADERS}")
    if(UNIX)
      set_target_properties(${IPPCP_LIB_PCS} PROPERTIES  VERSION   ${IPPCP_INTERFACE_VERSION}
                                                         SOVERSION ${IPPCP_INTERFACE_VERSION_MAJOR})
    endif()

    target_link_libraries(${IPPCP_LIB_PCS} ${IPPCP_LIB_MERGED}) # link to the static merged
    set_target_properties(${IPPCP_LIB_PCS} PROPERTIES OUTPUT_NAME "${TARGET_NAME}")

    install(TARGETS ${IPPCP_LIB_PCS}
            LIBRARY DESTINATION "lib/${ARCH}/$<$<BOOL:${NONPIC_LIB}>:nonpic>"
            RUNTIME DESTINATION "lib/${ARCH}/$<$<BOOL:${NONPIC_LIB}>:nonpic>"
            PUBLIC_HEADER DESTINATION "include"
            PRIVATE_HEADER DESTINATION "tools/${ARCH}/staticlib")

    if(WIN32)
      set_target_properties(${IPPCP_LIB_PCS} PROPERTIES LINK_FLAGS ${LINK_FLAG_DYNAMIC_WINDOWS})
      foreach(link ${LINK_LIB_STATIC_DEBUG})
        target_link_libraries(${IPPCP_LIB_PCS} debug ${link})
      endforeach()
      foreach(link ${LINK_LIB_STATIC_RELEASE})
        target_link_libraries(${IPPCP_LIB_PCS} optimized ${link})
      endforeach()
    else()
      if(APPLE)
        set(LINK_FLAGS ${LINK_FLAG_PCS_MACOSX})
        ## add export file
        set(LINK_FLAGS "${LINK_FLAGS} -exported_symbols_list ${IPP_CRYPTO_SOURCES_DIR}/exports.macosx.lib-export")
        set_target_properties(${IPPCP_LIB_PCS} PROPERTIES LINK_FLAGS "${LINK_FLAGS}")
        add_custom_command(TARGET ${IPPCP_LIB_PCS} POST_BUILD COMMAND
                           ${CMAKE_INSTALL_NAME_TOOL} -id @rpath/lib${TARGET_NAME}.dylib $<TARGET_FILE:${IPPCP_LIB_PCS}>)
      else()
        set(LINK_FLAGS ${LINK_FLAG_PCS_LINUX})
        ## add export file
        set(LINK_FLAGS "${LINK_FLAGS} ${IPP_CRYPTO_SOURCES_DIR}/exports.linux.lib-export")
        set_target_properties(${IPPCP_LIB_PCS} PROPERTIES LINK_FLAGS ${LINK_FLAGS})
        if("${CMAKE_C_COMPILER_ID}" STREQUAL "GNU")
            target_link_libraries(${IPPCP_LIB_PCS} gcc) # gcc is because of -nostdlib
            target_link_libraries(${IPPCP_LIB_PCS} c) # for stack check on gcc
        endif()
      endif()
    endif(WIN32)

  endif(DYNAMIC_LIB)
endif()

# Generate CMake configs to let external projects find ippcp and crypto_mb libraries easily
include("${IPP_CRYPTO_DIR}/sources/cmake/ippcp-gen-config.cmake")

# Install Custom Library tool
install(DIRECTORY "${TOOLS_DIR}/ipp_custom_library_tool_python/"
        DESTINATION "tools/custom_library_tool_python")

# Crypto multi-buffer library
if ("${ARCH}" STREQUAL "intel64")
    # MB_STANDALONE adjusts some build settings in crypto_mb
    # (e.g. output directories, make some crypto_mb build variables visible in parent scope, etc)
    set(MB_STANDALONE false)
    add_subdirectory(crypto_mb)

    # Throw these variables further up, so that tests can grab them and link to crypto_mb libraries
    set(MB_DYN_LIB_TARGET    "${MB_DYN_LIB_TARGET}" PARENT_SCOPE)
    set(MB_STATIC_LIB_TARGET "${MB_STATIC_LIB_TARGET}" PARENT_SCOPE)

    # Copy crypto_mb public headers to the output directory
    foreach( OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
        string( TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG )
        foreach(HEADER ${MB_PUBLIC_HEADERS})
            file(COPY "${HEADER}" DESTINATION "${CMAKE_OUTPUT_DIR}/${OUTPUTCONFIG}/include/crypto_mb")
        endforeach(HEADER ${MB_PUBLIC_HEADERS})
    endforeach( OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES )

    # Interface versions (a.k.a. shared library versions) of ippcp and crypto_mb shall be equal,
    # as they share single cmake-config routine with same "package name" IPPCP (in terms of find_package()).
    # The intention behind this is ippcp and crypto_mb are part of the same product, and should
    # share single find_package() call that can accept just one version common for all package components.
    if(NOT "${IPPCP_INTERFACE_VERSION}" MATCHES "${MBX_INTERFACE_VERSION}")
      message(SEND_ERROR  "IPPCP_INTERFACE_VERSION and MBX_INTERFACE_VERSION don't match")
    endif()
endif()
