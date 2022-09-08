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

ENV := $(strip $(wildcard $(TOP_DIR)/buildenv.mk))

SGX_MODE ?= HW

ifeq ($(ENV),)
    $(error "Can't find $(TOP_DIR)/buildenv.mk")
endif

include $(TOP_DIR)/buildenv.mk

WORK_DIR := $(shell pwd)
AENAME   := $(notdir $(WORK_DIR))
SONAME  := $(AENAME).so
ifdef DEBUG
CONFIG   := config_debug.xml
else
CONFIG   := config.xml
endif
EDLFILE  := $(wildcard *.edl)

EPID_SDK_DIR := $(LINUX_EXTERNAL_DIR)/epid-sdk

ifneq ($(SGX_MODE), HW)
	URTSLIB := -lsgx_urts_sim
	TRTSLIB := -lsgx_trts_sim
else
	URTSLIB := -lsgx_urts
	TRTSLIB := -lsgx_trts
endif
EXTERNAL_LIB := -lsgx_tservice

EXTERNAL_LIB += -lsgx_tstdc -lsgx_tcrypto -lsgx_tcxx

INCLUDE := -I$(LINUX_PSW_DIR)/ae/inc                   \
           -I$(SGX_HEADER_DIR)                         \
           -I$(SGX_HEADER_DIR)/tlibc                   \
           -I$(COMMON_DIR)/inc                         \
           -I$(COMMON_DIR)/inc/internal

SGXSIGN   := $(SGX_BIN_DIR)/sgx_sign
EDGER8R   := $(SGX_BIN_DIR)/sgx_edger8r

CXXFLAGS  += $(ENCLAVE_CXXFLAGS)
CFLAGS    += $(ENCLAVE_CFLAGS)

LDTFLAGS  = -L$(SGX_LIB_DIR) -Wl,--whole-archive $(TRTSLIB) -Wl,--no-whole-archive \
            -Wl,--start-group $(EXTERNAL_LIB) -Wl,--end-group -Wl,--build-id       \
            -Wl,--version-script=$(ROOT_DIR)/build-scripts/enclave.lds $(ENCLAVE_LDFLAGS)

LDTFLAGS += -Wl,-Map=out.map -Wl,--undefined=version -Wl,--gc-sections

DEFINES := -D__linux__

vpath %.cpp $(COMMON_DIR)/src:$(LINUX_PSW_DIR)/ae/common

.PHONY : version

version.o: $(LINUX_PSW_DIR)/ae/common/version.cpp
	$(CXX) $(CXXFLAGS) -fno-exceptions -fno-rtti $(INCLUDE) $(DEFINES) -c $(LINUX_PSW_DIR)/ae/common/version.cpp -o $@
