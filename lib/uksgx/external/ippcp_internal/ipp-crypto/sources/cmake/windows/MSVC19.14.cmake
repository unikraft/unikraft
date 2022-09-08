#===============================================================================
# Copyright 2018-2021 Intel Corporation
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

# linker
set(LINK_FLAG_STATIC_WINDOWS "/ignore:4221") # ignore warnings about empty obj files
# Suppresses the display of the copyright banner when the compiler starts up and display of informational messages during compiling.
set(LINK_FLAG_DYNAMIC_WINDOWS "/nologo")
# Displays information about modules that are incompatible with safe structured exception handling when /SAFESEH isn't specified.
set(LINK_FLAG_DYNAMIC_WINDOWS "${LINK_FLAG_DYNAMIC_WINDOWS} /VERBOSE:SAFESEH")
# Disable incremental linking
set(LINK_FLAG_DYNAMIC_WINDOWS "${LINK_FLAG_DYNAMIC_WINDOWS} /INCREMENTAL:NO")
# The /NODEFAULTLIB option tells the linker to remove one or more default libraries from the list of libraries it searches when resolving external references.
set(LINK_FLAG_DYNAMIC_WINDOWS "${LINK_FLAG_DYNAMIC_WINDOWS} /NODEFAULTLIB")
# Indicates that an executable was tested to be compatible with the Windows Data Execution Prevention feature.
set(LINK_FLAG_DYNAMIC_WINDOWS "${LINK_FLAG_DYNAMIC_WINDOWS} /NXCOMPAT")
# Specifies whether to generate an executable image that can be randomly rebased at load time.
set(LINK_FLAG_DYNAMIC_WINDOWS "${LINK_FLAG_DYNAMIC_WINDOWS} /DYNAMICBASE")
if(${ARCH} MATCHES "ia32")
  # When /SAFESEH is specified, the linker will only produce an image if it can also produce a table of the image's safe exception handlers.
  set(LINK_FLAG_DYNAMIC_WINDOWS "${LINK_FLAG_DYNAMIC_WINDOWS} /SAFESEH")
else()
  # The /LARGEADDRESSAWARE option tells the linker that the application can handle addresses larger than 2 gigabytes.
  set(LINK_FLAG_DYNAMIC_WINDOWS "${LINK_FLAG_DYNAMIC_WINDOWS} /LARGEADDRESSAWARE")
  # This option modifies the header of an executable image, a .dll file or .exe file, to indicate whether ASLR with 64-bit addresses is supported.
  set(LINK_FLAG_DYNAMIC_WINDOWS "${LINK_FLAG_DYNAMIC_WINDOWS} /HIGHENTROPYVA")
endif(${ARCH} MATCHES "ia32")

if (MSVC_VERSION LESS_EQUAL 1800) # VS2013
  # Link to C runtime, used in dlls
  set(LINK_LIB_STATIC_RELEASE libcmt)
  set(LINK_LIB_STATIC_DEBUG libcmtd)
else()
  # Link to universal C runtime and MSVC runtime. Used in dlls.
  set(LINK_LIB_STATIC_RELEASE libcmt libucrt libvcruntime)
  set(LINK_LIB_STATIC_DEBUG libcmtd libucrtd libvcruntime)
endif()

# compiler
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${LIBRARY_DEFINES}")

# Suppresses the display of the copyright banner when the compiler starts up and display of informational messages during compiling.
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /nologo")
# Prevents the compiler from searching for include files in directories specified in the PATH and INCLUDE environment variables.
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /X")
# Warning level = 4
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /W4")
# Detects some buffer overruns that overwrite a function's return address, exception handler address, or certain types of parameters. 
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /GS")
# Controls how the members of a structure are packed into memory and specifies the same packing for all structures in a module.
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /Zp16")
# Allows the compiler to package individual functions in the form of packaged functions. Smaller resulting size.
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /Gy")

# Causes the application to use the multithread, static version of the run-time library (debug version).
set(CMAKE_C_FLAGS_DEBUG "/MTd")
# The /Zi option produces a separate PDB file that contains all the symbolic debugging information for use with the debugger.
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /Zi")
# Turns off all optimizations in the program and speeds compilation.
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /Od")
# Debug macro
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /DDEBUG")

# Causes the application to use the multithread, static version of the run-time library.
set(CMAKE_C_FLAGS_RELEASE "/MT")
# Omits the default C runtime library name from the .obj file.
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /Zl")
# "Maximize Speed". Selects a predefined set of options that affect the size and speed of generated code.
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /O2") # /Ob2 is included in /O2
# No-debug macro
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /DNDEBUG")
# Warnings = errors
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /WX")

set(w7_opt "${w7_opt} /arch:SSE2")
set(s8_opt "${s8_opt} /arch:SSE2")
set(p8_opt "${p8_opt} /arch:SSE2")
set(g9_opt "${g9_opt} /arch:AVX")
set(h9_opt "${h9_opt} /arch:AVX2")

set(m7_opt "${m7_opt}")
set(n8_opt "${n8_opt}")
set(y8_opt "${y8_opt}")
set(e9_opt "${e9_opt} /arch:AVX")
set(l9_opt "${l9_opt} /arch:AVX2")
set(n0_opt "${n0_opt} /arch:AVX2")
set(k0_opt "${k0_opt} /arch:AVX2")
set(k1_opt "${k1_opt} /arch:AVX2")
