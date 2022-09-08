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

WORKING_DIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
CPP_SRCS := $(wildcard src/*.cpp src/c_wrapper/*.cpp)
TARGET_LIB := libmpa_management
include ../buildenv.mk

MPA_UEFI_DIR := ../uefi
LIBS_DIR := ../build/lib64

INCLUDE += -I$(SGX_SDK)/include
LIB_SGX_CAPABLE := $(SGX_SDK)/lib64/libsgx_capable.a

$(TARGET_LIB).a : $(CPP_OBJS)
	mkdir -p $(LIBS_DIR)/.tmp
	rm -rf $(LIBS_DIR)/.tmp/*
	cd $(LIBS_DIR)/.tmp && \
	ar x $(LIB_SGX_CAPABLE)
	ar rsD $(TARGET_LIB).a $(CPP_OBJS) $(LIBS_DIR)/.tmp/*.o
	rm -rf $(LIBS_DIR)/.tmp	
	$(CP) $(TARGET_LIB).a ../build/lib64/	
