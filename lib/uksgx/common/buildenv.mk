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

# Mitigation options

SGX_TRUSTED_LIBRARY_PATH ?= $(SGX_SDK)/lib64

CC ?= gcc
CC_VERSION := $(shell $(CC) -dumpversion)
CC_NO_LESS_THAN_8 := $(shell expr $(CC_VERSION) \>\= "8")
BINUTILS_DIR ?= /usr/local/bin

MITIGATION_CFLAGS += -B$(BINUTILS_DIR)
MITIGATION_LDFLAGS += -B$(BINUTILS_DIR)

ifeq ($(MITIGATION-CVE-2020-0551), LOAD)
ifeq ($(CC_NO_LESS_THAN_8), 1)
    MITIGATION_CFLAGS += -fcf-protection=none
endif
    MITIGATION_CFLAGS += -mindirect-branch-register -mfunction-return=thunk-extern
    MITIGATION_ASFLAGS := -Wa,-mlfence-after-load=yes -Wa,-mlfence-before-indirect-branch=memory -Wa,-mlfence-before-ret=shl
    MITIGATION_ASFLAGS += -fno-plt
    SGX_TRUSTED_LIBRARY_PATH := $(SGX_TRUSTED_LIBRARY_PATH)/cve_2020_0551_load
else ifeq ($(MITIGATION-CVE-2020-0551), CF)
ifeq ($(CC_NO_LESS_THAN_8), 1)
    MITIGATION_CFLAGS += -fcf-protection=none
endif
    MITIGATION_CFLAGS += -mindirect-branch-register -mfunction-return=thunk-extern
    MITIGATION_ASFLAGS := -Wa,-mlfence-before-indirect-branch=all -Wa,-mlfence-before-ret=shl
    MITIGATION_ASFLAGS += -fno-plt
    SGX_TRUSTED_LIBRARY_PATH := $(SGX_TRUSTED_LIBRARY_PATH)/cve_2020_0551_cf
endif


MITIGATION_CFLAGS += $(MITIGATION_ASFLAGS)
