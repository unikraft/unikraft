#
# Copyright (C) 2011-2021 Intel Corporation. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
#   * Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
#   * Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in
#     the documentation and/or other materials provided with the
#     distribution.
#   * Neither the name of Intel Corporation nor the names of its
#     contributors may be used to endorse or promote products derived
#     from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
#


# -----------------------------------------------------------------------------
# Function : parent-dir
# Arguments: 1: path
# Returns  : Parent dir or path of $1, with final separator removed.
# -----------------------------------------------------------------------------
parent-dir = $(patsubst %/,%,$(dir $(1:%/=%)))

# -----------------------------------------------------------------------------
# Macro    : my-dir
# Returns  : the directory of the current Makefile
# Usage    : $(my-dir)
# -----------------------------------------------------------------------------
my-dir = $(realpath $(call parent-dir,$(lastword $(MAKEFILE_LIST))))


ROOT_DIR              := $(call my-dir)
ifneq ($(words $(subst :, ,$(ROOT_DIR))), 1)
  $(error main directory cannot contain spaces nor colons)
endif


#--------------------------------------------------------------------------------------
# Function: get_distr_info
# Arguments: 1: the grep keyword to be searched from /etc/os-release
# Returns: Return the value for the Linux distribution info corresponding to the keyword
#---------------------------------------------------------------------------------------
get_distr_info = $(patsubst "%",%,$(shell grep $(1) /etc/os-release 2> /dev/null | awk -F'=' '{print $$2}'))

DISTR_ID := $(call get_distr_info, '^ID=')
DISTR_VER := $(call get_distr_info, '^VERSION_ID=')


COMMON_DIR            := $(ROOT_DIR)/common
LINUX_EXTERNAL_DIR    := $(ROOT_DIR)/external
LINUX_PSW_DIR         := $(ROOT_DIR)/psw
LINUX_SDK_DIR         := $(ROOT_DIR)/sdk
LINUX_UNITTESTS       := $(ROOT_DIR)/unittests
DCAP_DIR              := $(LINUX_EXTERNAL_DIR)/dcap_source
LIBUNWIND_DIR         := $(ROOT_DIR)/sdk/cpprt/linux/libunwind

CP    := cp -f
MKDIR := mkdir -p
STRIP := strip
OBJCOPY := objcopy
NIPX := .nipx
NIPD := .nipd
NIPRODT := .niprod
CC ?= gcc

# clean the content of 'INCLUDE' - this variable will be set by vcvars32.bat
# thus it will cause build error when this variable is used by our Makefile,
# when compiling the code under Cygwin tainted by MSVC environment settings.
INCLUDE :=

# this will return the path to the file that included the buildenv.mk file
CUR_DIR := $(realpath $(call parent-dir,$(lastword $(wordlist 2,$(words $(MAKEFILE_LIST)),x $(MAKEFILE_LIST)))))

CC_VERSION := $(shell $(CC) -dumpversion)
CC_VERSION_MAJOR := $(shell echo $(CC_VERSION) | cut -f1 -d.)
CC_VERSION_MINOR := $(shell echo $(CC_VERSION) | cut -f2 -d.)
CC_BELOW_4_9 := $(shell [ $(CC_VERSION_MAJOR) -lt 4 -o \( $(CC_VERSION_MAJOR) -eq 4 -a $(CC_VERSION_MINOR) -le 9 \) ] && echo 1)
CC_BELOW_5_2 := $(shell [ $(CC_VERSION_MAJOR) -lt 5 -o \( $(CC_VERSION_MAJOR) -eq 5 -a $(CC_VERSION_MINOR) -le 2 \) ] && echo 1)
CC_NO_LESS_THAN_8 := $(shell expr $(CC_VERSION) \>\= "8")

# turn on stack protector for SDK
ifeq ($(CC_BELOW_4_9), 1)
    COMMON_FLAGS += -fstack-protector
else
    COMMON_FLAGS += -fstack-protector-strong
endif

ifdef DEBUG
    COMMON_FLAGS += -O0 -ggdb -DDEBUG -UNDEBUG
    COMMON_FLAGS += -DSE_DEBUG_LEVEL=SE_TRACE_DEBUG
else
    COMMON_FLAGS += -O2 -D_FORTIFY_SOURCE=2 -UDEBUG -DNDEBUG
endif

ifdef SE_SIM
    COMMON_FLAGS += -DSE_SIM
endif

# Disable ref-LE build by default.
# Users could enable the ref-LE build 
# by explicitly specifying 'BUILD_REF_LE=1'
BUILD_REF_LE ?= 0
ifeq ($(BUILD_REF_LE), 1)
    COMMON_FLAGS += -DREF_LE
endif

COMMON_FLAGS += -ffunction-sections -fdata-sections

# turn on compiler warnings as much as possible
COMMON_FLAGS += -Wall -Wextra -Winit-self -Wpointer-arith -Wreturn-type \
		-Waddress -Wsequence-point -Wformat-security \
		-Wmissing-include-dirs -Wfloat-equal -Wundef -Wshadow \
		-Wcast-align -Wconversion -Wredundant-decls

# additional warnings flags for C
CFLAGS += -Wjump-misses-init -Wstrict-prototypes -Wunsuffixed-float-constants

# additional warnings flags for C++
CXXFLAGS += -Wnon-virtual-dtor

CXXFLAGS += -std=c++14

.DEFAULT_GOAL := all
# this turns off the RCS / SCCS implicit rules of GNU Make
% : RCS/%,v
% : RCS/%
% : %,v
% : s.%
% : SCCS/s.%

# If a rule fails, delete $@.
.DELETE_ON_ERROR:

HOST_FILE_PROGRAM := file

UNAME := $(shell uname -m)
ifneq (,$(findstring 86,$(UNAME)))
    HOST_ARCH := x86
    ifneq (,$(shell $(HOST_FILE_PROGRAM) -L $(SHELL) | grep 'x86[_-]64'))
        HOST_ARCH := x86_64
    endif
else
    $(info Unknown host CPU arhitecture $(UNAME))
    $(error Aborting)
endif

BUILD_DIR := $(ROOT_DIR)/build/linux

ifeq "$(findstring __INTEL_COMPILER, $(shell $(CC) -E -dM -xc /dev/null))" "__INTEL_COMPILER"
  ifeq ($(shell test -f /usr/bin/dpkg; echo $$?), 0)
    ADDED_INC := -I /usr/include/$(shell dpkg-architecture -qDEB_BUILD_MULTIARCH)
  endif
endif

ARCH := $(HOST_ARCH)
ifeq "$(findstring -m32, $(CXXFLAGS))" "-m32"
  ARCH := x86
endif

ifeq ($(ARCH), x86)
COMMON_FLAGS += -DITT_ARCH_IA32
else
COMMON_FLAGS += -DITT_ARCH_IA64
endif


CET_FLAGS := 
ifeq ($(CC_NO_LESS_THAN_8), 1)
    CET_FLAGS += -fcf-protection
endif

CFLAGS   += $(COMMON_FLAGS)
CXXFLAGS += $(COMMON_FLAGS)

# Enable the security flags
COMMON_LDFLAGS := -Wl,-z,relro,-z,now,-z,noexecstack

# mitigation options
MITIGATION_INDIRECT ?= 0
MITIGATION_RET ?= 0
MITIGATION_C ?= 0
MITIGATION_ASM ?= 0
MITIGATION_AFTERLOAD ?= 0
MITIGATION_LIB_PATH :=

ifeq ($(MITIGATION-CVE-2020-0551), LOAD)
    MITIGATION_C := 1
    MITIGATION_ASM := 1
    MITIGATION_INDIRECT := 1
    MITIGATION_RET := 1
    MITIGATION_AFTERLOAD := 1
    MITIGATION_LIB_PATH := cve_2020_0551_load
else ifeq ($(MITIGATION-CVE-2020-0551), CF)
    MITIGATION_C := 1
    MITIGATION_ASM := 1
    MITIGATION_INDIRECT := 1
    MITIGATION_RET := 1
    MITIGATION_AFTERLOAD := 0
    MITIGATION_LIB_PATH := cve_2020_0551_cf
endif

ifneq ($(origin NIX_STORE), environment)
BINUTILS_DIR ?= /usr/local/bin
EXT_BINUTILS_DIR = $(ROOT_DIR)/external/toolset/$(DISTR_ID)$(DISTR_VER)
else
BINUTILS_DIR ?= $(ROOT_DIR)/external/toolset/nix/
EXT_BINUTILS_DIR = $(ROOT_DIR)/external/toolset/nix/
endif

# enable -B option for all the build
MITIGATION_CFLAGS += -B$(BINUTILS_DIR)

ifeq ($(MITIGATION_C), 1)
ifeq ($(MITIGATION_INDIRECT), 1)
    MITIGATION_CFLAGS += -mindirect-branch-register
endif
ifeq ($(MITIGATION_RET), 1)
ifeq ($(CC_NO_LESS_THAN_8), 1)
    MITIGATION_CFLAGS += -fcf-protection=none
endif
    MITIGATION_CFLAGS += -mfunction-return=thunk-extern
endif
endif

ifeq ($(MITIGATION_ASM), 1)
    MITIGATION_ASFLAGS += -fno-plt
ifeq ($(MITIGATION_AFTERLOAD), 1)
    MITIGATION_ASFLAGS += -Wa,-mlfence-after-load=yes -Wa,-mlfence-before-indirect-branch=memory
else
    MITIGATION_ASFLAGS += -Wa,-mlfence-before-indirect-branch=all
endif
ifeq ($(MITIGATION_RET), 1)
    MITIGATION_ASFLAGS += -Wa,-mlfence-before-ret=shl
endif
endif

MITIGATION_CFLAGS += $(MITIGATION_ASFLAGS)

#fcf-protection is not compatible with MITIGATION
ifneq ($(MITIGATION_RET), 1)
    CFLAGS   += $(CET_FLAGS)
    CXXFLAGS += $(CET_FLAGS)
endif

# Compiler and linker options for an Enclave
#
# We are using '--export-dynamic' so that `g_global_data_sim' etc.
# will be exported to dynamic symbol table.
#
# When `pie' is enabled, the linker (both BFD and Gold) under Ubuntu 14.04
# will hide all symbols from dynamic symbol table even if they are marked
# as `global' in the LD version script.
ENCLAVE_CFLAGS   = -ffreestanding -nostdinc -fvisibility=hidden -fpie -fno-strict-overflow -fno-delete-null-pointer-checks
ENCLAVE_CXXFLAGS = $(ENCLAVE_CFLAGS) -nostdinc++
ENCLAVE_LDFLAGS  = -B$(BINUTILS_DIR) $(COMMON_LDFLAGS) -Wl,-Bstatic -Wl,-Bsymbolic -Wl,--no-undefined \
                   -Wl,-pie,-eenclave_entry -Wl,--export-dynamic  \
                   -Wl,--defsym,__ImageBase=0

ENCLAVE_CFLAGS += $(MITIGATION_CFLAGS)
ENCLAVE_ASFLAGS = $(MITIGATION_ASFLAGS)

# We have below choices as to crypto, math and string libs:
# 1. crypto - SGXSSL (0), IPP crypto (1)
# 2. math   - optimized (0), open sourced (1)
# 3. string - optimized (0), open sourced (1)
#
# A macro 'USE_OPT_LIBS' is provided to allow users to build 
# SGX SDK with different library combination by setting different 
# value to 'USE_OPT_LIBS'.
# By default, choose to build SDK using optimized IPP crypto +
# open sourced string + open sourced math.
#
# IPP + open sourced string + open sourced math
USE_OPT_LIBS ?= 1
USE_CRYPTO_LIB ?= 1
USE_STRING_LIB ?= 1
USE_MATH_LIB ?= 1

ifeq ($(USE_OPT_LIBS), 0)
# SGXSSL + open sourced string + open sourced math
    USE_CRYPTO_LIB := 0
    USE_MATH_LIB := 1
    USE_STRING_LIB := 1
else ifeq ($(USE_OPT_LIBS), 2)
# SGXSSL + optimized string + optimized math
    USE_CRYPTO_LIB := 0
    USE_MATH_LIB := 0
    USE_STRING_LIB := 0
else ifeq ($(USE_OPT_LIBS), 3)
# IPP + optimized string + optimized math
    USE_CRYPTO_LIB := 1
    USE_MATH_LIB := 0
    USE_STRING_LIB := 0
endif

# macro check
ifeq ($(USE_MATH_LIB), 0)
ifneq ($(USE_STRING_LIB), 0)
$(error ERROR: Optimized math library depends on Optimized string library)
endif
endif

ifneq ($(MITIGATION-CVE-2020-0551),)
ifeq ($(USE_STRING_LIB), 0)
$(error ERROR: Cannot build a mitigation SDK with Optimized string/math)
endif
ifeq ($(USE_MATH_LIB), 0)
$(error ERROR: Cannot build a mitigation SDK with Optimized string/math)
endif
endif


IPP_SUBDIR = no_mitigation
ifeq ($(MITIGATION-CVE-2020-0551), LOAD)
    IPP_SUBDIR = cve_2020_0551_load
else ifeq ($(MITIGATION-CVE-2020-0551), CF)
    IPP_SUBDIR = cve_2020_0551_cf
endif


SGX_IPP_DIR     := $(ROOT_DIR)/external/ippcp_internal
SGX_IPP_INC     := $(SGX_IPP_DIR)/inc
IPP_LIBS_DIR    := $(SGX_IPP_DIR)/lib/linux/intel64/$(IPP_SUBDIR)
LD_IPP          := -lippcp

######## SGX SDK Settings ########
SGX_SDK ?= /opt/intel/sgxsdk
SGX_HEADER_DIR ?= $(SGX_SDK)/include

ifeq ($(ARCH), x86)
	SGX_COMMON_CFLAGS := -m32
	SGX_LIB_DIR := $(SGX_SDK)/lib
	SGX_BIN_DIR := $(SGX_SDK)/bin/x86
else
	SGX_COMMON_CFLAGS := -m64
	SGX_LIB_DIR := $(SGX_SDK)/lib64/$(MITIGATION_LIB_PATH)
	SGX_BIN_DIR := $(SGX_SDK)/bin/x64
endif

SPLIT_VERSION=$(word $2,$(subst ., ,$1))
