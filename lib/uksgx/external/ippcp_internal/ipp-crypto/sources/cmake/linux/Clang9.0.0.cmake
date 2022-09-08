#===============================================================================
# Copyright 2020-2021 Intel Corporation
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

# Security Linker flags
set(LINK_FLAG_SECURITY "")
# Disallows undefined symbols in object files. Undefined symbols in shared libraries are still allowed
set(LINK_FLAG_SECURITY "${LINK_FLAG_SECURITY} -Wl,-z,defs")
# Stack execution protection
set(LINK_FLAG_SECURITY "${LINK_FLAG_SECURITY} -Wl,-z,noexecstack")
# Data relocation and protection (RELRO)
set(LINK_FLAG_SECURITY "${LINK_FLAG_SECURITY} -Wl,-z,relro -Wl,-z,now")

# Prevents the compiler from using standard libraries and startup files when linking.
set(LINK_FLAG_DYNAMIC_MACOSX "${LINK_FLAG_SECURITY} -nostdlib")
# Dynamically link lib c (libdl is for old apps)
set(LINK_FLAG_DYNAMIC_MACOSX "${LINK_FLAG_DYNAMIC_MACOSX} -Wl,-call_shared,-lc")
# Create a shared library
set(LINK_FLAG_DYNAMIC_MACOSX "-Wl,-shared")
if(${ARCH} MATCHES "ia32")
  # Tells the compiler to generate code for a specific architecture (32)
  set(LINK_FLAG_DYNAMIC_MACOSX "${LINK_FLAG_DYNAMIC_MACOSX} -m32")
endif(${ARCH} MATCHES "ia32")

set(LINK_FLAG_PCS_MACOSX "-nostdlib -Wl,-lc")
if(${ARCH} MATCHES "ia32")
  set(LINK_FLAG_PCS_MACOSX "${LINK_FLAG_PCS_MACOSX} -m32")
endif(${ARCH} MATCHES "ia32")

# compiler
# Enables the use of blocks and entire functions of assembly code within a C or C++ file
set(CC_FLAGS_INLINE_ASM_UNIX "-fasm-blocks")

# Disables all warning messages
set(CC_FLAGS_INLINE_ASM_UNIX_IA32 "${CC_FLAGS_INLINE_ASM_UNIX} -w")
# Tells the compiler to generate code for a specific architecture (32)
set(CC_FLAGS_INLINE_ASM_UNIX_IA32 "${CC_FLAGS_INLINE_ASM_UNIX_IA32} -m32")
# EBP is used as a general-purpose register in optimizations
set(CC_FLAGS_INLINE_ASM_UNIX_IA32 "${CC_FLAGS_INLINE_ASM_UNIX_IA32} -fomit-frame-pointer")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${LIBRARY_DEFINES}")

# Ensures that compilation takes place in a freestanding environment
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ffreestanding")
# Tells the compiler to align functions
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -falign-functions=32")
# Format string vulnerabilities
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wformat -Wformat-security")
#Diagnostic flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wdouble-promotion -Wshadow -Wunreachable-code -Wpointer-arith -Wuninitialized -Wconversion")
# Enable __declspec attribute
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fdeclspec")
# Variables explicitly initialized with zeros are placed in the BSS section. This can save space in the resulting code
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-zero-initialized-in-bss")
# C std
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99")
if ((${ARCH} MATCHES "ia32") OR (NOT NONPIC_LIB))
  # Stack-based Buffer Overrun Detection (only when not nonpic intel64)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fstack-protector")
endif()

# Security flag that adds compile-time and run-time checks
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_FORTIFY_SOURCE=2")

if(NOT NONPIC_LIB)
  # Position Independent Execution (PIE)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fpic -fPIC")
elseif(NOT ${ARCH} MATCHES "ia32")
  # NONPIC intel64: specify kernel code model
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mno-red-zone -mcmodel=kernel")
endif()

# When a value is specified (which must be a small power of two), pack structure members according to this value, representing the maximum alignment
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fpack-struct=16")
if(${ARCH} MATCHES "ia32")
  # Stack alignment = 16 bytes
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mstack-alignment=4")
  # 32bit linker command
  #set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Xassembler --32")
  # Tells the compiler to generate code for a specific architecture (32)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32")
endif(${ARCH} MATCHES "ia32")

# Optimization level = 3, no-debug definition (turns off asserts), warnings=errors
set (CMAKE_C_FLAGS_RELEASE " -O3 -DNDEBUG -Werror")

set(w7_opt "${w7_opt} -march=pentium4 -msse2")
set(s8_opt "${s8_opt} -march=core2 -mssse3")
set(p8_opt "${p8_opt} -march=nehalem -msse4.2 -maes -mpclmul -msha")
set(g9_opt "${g9_opt} -march=sandybridge -mavx -maes -mpclmul -msha -mrdrnd -mrdseed")
set(h9_opt "${h9_opt} -march=haswell -mavx2 -maes -mpclmul -msha -mrdrnd -mrdseed")

set(m7_opt "${m7_opt} -march=nocona -msse3")
set(n8_opt "${n8_opt} -march=core2 -mssse3")
set(y8_opt "${y8_opt} -march=nehalem -msse4.2 -maes -mpclmul -msha")
set(e9_opt "${e9_opt} -march=sandybridge -mavx -maes -mpclmul -msha -mrdrnd -mrdseed")
set(l9_opt "${l9_opt} -march=haswell -mavx2 -maes -mpclmul -msha -mrdrnd -mrdseed")
set(n0_opt "${n0_opt} -march=knl -mavx2 -maes -mavx512f -mavx512cd -mavx512pf -mavx512er -mpclmul -msha -mrdrnd -mrdseed")
#set(k0_opt "${k0_opt} -march=skylake-avx512")
#set(k0_opt "${k0_opt} -maes -mavx512f -mavx512cd -mavx512vl -mavx512bw -mavx512dq -mavx512ifma -mpclmul -msha -mrdrnd -mrdseed -madx -mgfni -mvaes -mvpclmulqdq -mavx512vbmi -mavx512vbmi2")
set(k0_opt "${k0_opt} -march=skylake-avx512")
set(k0_opt "${k0_opt} -maes -mavx512f -mavx512cd -mavx512vl -mavx512bw -mavx512dq -mpclmul -mrdrnd -mrdseed -madx")
set(k1_opt "${k1_opt} -march=icelake-server")
set(k1_opt "${k1_opt} -maes -mavx512f -mavx512cd -mavx512vl -mavx512bw -mavx512dq -mavx512ifma -mpclmul -msha -mrdrnd -mrdseed -madx -mgfni -mvaes -mvpclmulqdq -mavx512vbmi -mavx512vbmi2")
