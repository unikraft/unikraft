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


TOP_DIR := ../../../..
MITIGATION-CVE-2020-0551 := LOAD
include $(TOP_DIR)/ae/buildenv.mk

AENAME := qe3
SONAME := $(AENAME).so

QE3_VER:= $(shell awk '$$2 ~ /QE3_VERSION/ { print substr($$3, 2, length($$3) - 2); }' $(COMMON_DIR)/inc/internal/se_version.h)

TCRYPTO_LIB_NAME := sgx_tcrypto

INCLUDE := -I$(SGX_SDK)/include                        \
           -I$(SGX_SDK)/include/tlibc                  \
           -I$(SGX_SDK)/include/libcxx                 \
           -I$(COMMON_DIR)/inc/internal                \
           -I$(COMMON_DIR)/inc/internal/linux          \
           -I$(TOP_DIR)/ae/inc/internal                \
           -I../../inc                                 \
           -I../../../common/inc                       \
           -I../../../../pce_wrapper/inc               \
           -I..                                        \
           -I.

EXTERNAL_LIB = $(EXTERNAL_LIB_NO_CRYPTO) -lsgx_tcxx -l$(TCRYPTO_LIB_NAME)

vpath %.cpp .. .
SRC := quoting_enclave_3.cpp

OBJ := $(SRC:.cpp=.o)
OBJS := $(sort $(OBJ) version.o)

.PHONY: all
all: $(SONAME)

$(SONAME): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@  $(OBJS)  -nostdlib -nodefaultlibs -nostartfiles -Wl,-soname=libsgx_${AENAME}.signed.so.$(call SPLIT_VERSION,$(QE3_VER),1)  $(LDTFLAGS) -fno-exceptions -fno-rtti
	$(STRIP) --strip-unneeded --remove-section=.comment --remove-section=.note $@

quoting_enclave_3.o: $(AENAME)_t.c

%.o:%.cpp 
	$(CXX) $(CXXFLAGS) $(INCLUDE) $(DEFINES) -fno-exceptions -c $< -o $@

$(AENAME)_t.c: ../$(AENAME).edl
	@$(SGX_EDGER8R) --trusted $<

.PHONY: clean
clean:
	@$(RM) *.o
	@$(RM) *.so
	@$(RM) *.map
	@$(RM) *_t.* 

.PHONY: rebuild
rebuild:
	$(MAKE) clean
	$(MAKE) all
