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

# linker
# Create a dynamic lib
set(LINK_FLAG_DYNAMIC_MACOSX "-Wl,-dynamic")
# Alters how symbols are resolved at build time and runtime.
# The linker does not record which dylib an external symbol came from, so at runtime dyld again searches all images and uses the first definition it finds.
set(LINK_FLAG_DYNAMIC_MACOSX "${LINK_FLAG_DYNAMIC_MACOSX} -Wl,-flat_namespace")
# Automatically adds space for future expansion of load commands such that all paths could expand to MAXPATHLEN.
set(LINK_FLAG_DYNAMIC_MACOSX "${LINK_FLAG_DYNAMIC_MACOSX} -Wl,-headerpad_max_install_names")
# This is set to indicate the oldest Mac OS X version that that the output is to be used on.
set(LINK_FLAG_DYNAMIC_MACOSX "${LINK_FLAG_DYNAMIC_MACOSX} -Wl,-macosx_version_min,10.8")
# Prevents the compiler from using standard libraries and startup files when linking.
set(LINK_FLAG_DYNAMIC_MACOSX "${LINK_FLAG_DYNAMIC_MACOSX} -nostdlib")
# Dynamically link to lib c
set(LINK_FLAG_DYNAMIC_MACOSX "${LINK_FLAG_DYNAMIC_MACOSX} -Wl,-lc")
# The architecture for the output file
set(LINK_FLAG_DYNAMIC_MACOSX "${LINK_FLAG_DYNAMIC_MACOSX} -Wl,-arch,x86_64")
set(LINK_FLAG_PCS_MACOSX "${LINK_FLAG_DYNAMIC_MACOSX}")

# compiler
# Enables the use of blocks and entire functions of assembly code within a C or C++ file
set(CC_FLAGS_INLINE_ASM_UNIX "-fasm-blocks")

# Disables all warning messages
set(CC_FLAGS_INLINE_ASM_UNIX_IA32 "${CC_FLAGS_INLINE_ASM_UNIX} -w")
# Tells the compiler to generate code for a specific architecture (32)
set(CC_FLAGS_INLINE_ASM_UNIX_IA32 "${CC_FLAGS_INLINE_ASM_UNIX_IA32} -m32")
# EBP is used as a general-purpose register in optimizations
set(CC_FLAGS_INLINE_ASM_UNIX_IA32 "${CC_FLAGS_INLINE_ASM_UNIX_IA32} -fomit-frame-pointer")

# Do not use the specified registres in dispatcher compilation
set(CC_FLAGS_INLINE_ASM_UNIX_INTEL64 "${CC_FLAGS_INLINE_ASM_UNIX} -ffixed-rdi -ffixed-rsi -ffixed-rbx -ffixed-rcx -ffixed-rdx -ffixed-rbp -ffixed-r8 -ffixed-r9 -ffixed-r12 -ffixed-r13 -ffixed-r14 -ffixed-r15")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${LIBRARY_DEFINES}")

# Ensures that compilation takes place in a freestanding environment
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ffreestanding")
# Determines whether pointer disambiguation is enabled with the restrict qualifier
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -restrict")
# Tells the compiler to generate an optimization report
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -qopt-report2 -qopt-report-phase:vec")
# Tells the compiler to align functions and loops
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -falign-functions=32 -falign-loops=32")
# Other flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -diag-error 266 -diag-disable 13366")

# Security Compiler flags

# Stack-based Buffer Overrun Detection
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fstack-protector")

# Security flag that adds compile-time and run-time checks
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_FORTIFY_SOURCE=2")

# Format string vulnerabilities
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wformat -Wformat-security")

# Enable Intel® Control-Flow Enforcement Technology (Intel® CET) protection
# NOTE: Intel Compiler does not support CET code generation on macOS
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fcf-protection=full")

# Position Independent Execution (PIE)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fpic -fPIC")

if(CODE_COVERAGE)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -prof-gen:srcpos -prof-dir ${PROF_DATA_DIR}")
endif()

# Optimization level = 3, no-debug definition (turns off asserts), warning level = 3, treat warnings as errors
set (CMAKE_C_FLAGS_RELEASE " -O3 -DNDEBUG -w3 -Werror")

# Compile for x64
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -arch_only x86_64")

set(w7_opt "${w7_opt} -xSSE2")
set(s8_opt "${s8_opt} -xSSE3")
set(p8_opt "${p8_opt} -xSSE4.2")
set(g9_opt "${g9_opt} -xAVX")
set(h9_opt "${h9_opt} -xCORE-AVX2")
set(m7_opt "${m7_opt} -xSSE3")
set(n8_opt "${n8_opt} -xSSSE3")
set(y8_opt "${y8_opt} -xSSE4.2")
set(e9_opt "${e9_opt} -xAVX")
set(l9_opt "${l9_opt} -xCORE-AVX2")
set(n0_opt "${n0_opt} -xMIC-AVX512")
set(k0_opt "${k0_opt} -xCORE-AVX512")
set(k0_opt "${k0_opt} -qopt-zmm-usage:high")
