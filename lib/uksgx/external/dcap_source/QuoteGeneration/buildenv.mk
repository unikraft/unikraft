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
my-dir = $(call parent-dir,$(lastword $(MAKEFILE_LIST)))


ROOT_DIR              := $(call my-dir)
COMMON_DIR            := $(ROOT_DIR)/common

SGX_VER:= $(shell awk '$$2 ~ /STRFILEVER/ { print substr($$3, 2, length($$3) - 2); }' $(COMMON_DIR)/inc/internal/se_version.h)
SGX_MAJOR_VER:= $(shell echo $(SGX_VER) |awk -F. '{print $$1}')
SPLIT_VERSION=$(word $2,$(subst ., ,$1))

CP    := cp -f
MKDIR := mkdir -p
STRIP := strip
OBJCOPY := objcopy
NIPX := .nipx
NIPD := .nipd
NIPRODT := .niprod

######## SGX SDK Settings ########
SGX_SDK ?= /opt/intel/sgxsdk
SGX_MODE ?= HW
SGX_ARCH ?= x64
SGX_DEBUG ?= 0

ifneq ($(MAKECMDGOALS),clean)
include $(SGX_SDK)/buildenv.mk
endif

ifeq ($(shell getconf LONG_BIT), 32)
	SGX_ARCH := x86
else ifeq ($(findstring -m32, $(CXXFLAGS)), -m32)
	SGX_ARCH := x86
endif

ifeq ($(SGX_ARCH), x86)
	SGX_COMMON_CFLAGS := -m32
	SGX_LIBRARY_PATH := $(SGX_SDK)/lib
	SGX_ENCLAVE_SIGNER := $(SGX_SDK)/bin/x86/sgx_sign
	SGX_EDGER8R := $(SGX_SDK)/bin/x86/sgx_edger8r
else
	SGX_COMMON_CFLAGS := -m64
	SGX_LIBRARY_PATH := $(SGX_TRUSTED_LIBRARY_PATH)
	SGX_ENCLAVE_SIGNER := $(SGX_SDK)/bin/x64/sgx_sign
	SGX_EDGER8R := $(SGX_SDK)/bin/x64/sgx_edger8r
endif

# clean the content of 'INCLUDE' - this variable will be set by vcvars32.bat
# thus it will cause build error when this variable is used by our Makefile,
# when compiling the code under Cygwin tainted by MSVC environment settings.
INCLUDE :=

# this will return the path to the file that included the buildenv.mk file
CUR_DIR := $(realpath $(call parent-dir,$(lastword $(wordlist 2,$(words $(MAKEFILE_LIST)),x $(MAKEFILE_LIST)))))

CET_FLAGS :=
CC_VERSION := $(shell $(CC) -dumpversion)
CC_NO_LESS_THAN_8 := $(shell expr $(CC_VERSION) \>\= "8")
ifeq ($(CC_NO_LESS_THAN_8), 1)
    CET_FLAGS += -fcf-protection
endif

# turn on stack protector for SDK
CC_BELOW_4_9 := $(shell expr "`$(CC) -dumpversion`" \< "4.9")
ifeq ($(CC_BELOW_4_9), 1)
    COMMON_FLAGS += -fstack-protector
else
    COMMON_FLAGS += -fstack-protector-strong
endif

ifdef DEBUG
    COMMON_FLAGS += -O0 -ggdb -DDEBUG -UNDEBUG
    COMMON_FLAGS += -DSE_DEBUG_LEVEL=SE_TRACE_DEBUG -DDEBUG_MODE=1
else
    COMMON_FLAGS += -O2 -D_FORTIFY_SOURCE=2 -UDEBUG -DNDEBUG
endif

ifdef SE_SIM
    COMMON_FLAGS += -DSE_SIM
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

ifneq ($(MITIGATION-CVE-2020-0551), LOAD)
    ifneq ($(MITIGATION-CVE-2020-0551), CF)
        COMMON_FLAGS += $(CET_FLAGS)
    endif
endif

CFLAGS   += $(COMMON_FLAGS)
CXXFLAGS += $(COMMON_FLAGS)

# Enable the security flags
COMMON_LDFLAGS := -Wl,-z,relro,-z,now,-z,noexecstack

# Compiler and linker options for an Enclave
#
# We are using '--export-dynamic' so that `g_global_data_sim' etc.
# will be exported to dynamic symbol table.
#
# When `pie' is enabled, the linker (both BFD and Gold) under Ubuntu 14.04
# will hide all symbols from dynamic symbol table even if they are marked
# as `global' in the LD version script.
ENCLAVE_CFLAGS   = -ffreestanding -nostdinc -fvisibility=hidden -fpie -fno-strict-overflow -fno-delete-null-pointer-checks $(MITIGATION_CFLAGS)
ENCLAVE_CXXFLAGS = $(ENCLAVE_CFLAGS) -nostdinc++
ENCLAVE_LDFLAGS  = $(COMMON_LDFLAGS) -Wl,-Bstatic -Wl,-Bsymbolic -Wl,--no-undefined \
                   -Wl,-pie,-eenclave_entry -Wl,--export-dynamic  \
                   -Wl,--defsym,__ImageBase=0
