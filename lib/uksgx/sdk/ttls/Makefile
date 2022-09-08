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

TOP_DIR  = ../..
include $(TOP_DIR)/buildenv.mk

TLSNAME = sgx_ttls
EDLFILE = $(TOP_DIR)/common/inc/$(TLSNAME).edl

ifeq ($(DEBUG), 1)
        SGXSSL_TLIB = sgx_tsgxssld
        SGXSSL_TCRYPTO = sgx_tsgxssl_cryptod
else
        SGXSSL_TLIB = sgx_tsgxssl
        SGXSSL_TCRYPTO = sgx_tsgxssl_crypto
endif

SGX_QV_PATH ?= ../../external/dcap_source/QuoteVerification
PREBUILT_OPENSSL_PATH ?= $(SGX_QV_PATH)/../prebuilt/openssl

INCLUDE += -I$(COMMON_DIR)/inc/tlibc    \
           -I$(COMMON_DIR)/inc/internal \
           -I$(COMMON_DIR)/inc          \
           -I$(LINUX_SDK_DIR)/tlibcxx/include \
           -I$(SGX_QV_PATH)/QvE/Include \
           -I$(SGX_QV_PATH)/../QuoteGeneration/quote_wrapper/common/inc \
           -I$(SGX_QV_PATH)/../QuoteGeneration/pce_wrapper/inc \
           -I$(SGX_QV_PATH)/dcap_tvl \
           -include./tsgxsslio.h \
           -I$(PREBUILT_OPENSSL_PATH)/inc

CXXFLAGS += $(COMMON_FLAGS) $(ENCLAVE_CXXFLAGS)

EDGER8R_DIR    := $(LINUX_SDK_DIR)/edger8r/linux
EDGER8R        := $(EDGER8R_DIR)/_build/Edger8r.native

SRC := $(wildcard *.cpp)

OBJ := $(sort $(SRC:.cpp=.o))

LIBNAME := libsgx_ttls.a

.PHONY: all PREPARE_SGX_SSL
all: $(LIBNAME) | $(BUILD_DIR)
	$(CP) $(LIBNAME) $|

$(TLSNAME)_t.h: $(EDLFILE) $(EDGER8R)
	$(EDGER8R) --trusted --header-only --search-path $(TOP_DIR)/common/inc $<

$(EDGER8R):
	$(MAKE) -C $(EDGER8R_DIR)

$(LIBNAME): $(OBJ)
	$(AR) rs $@ $(OBJ)

$(OBJ): $(TLSNAME)_t.h
$(OBJ): %.o :%.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDE)  -c $< -o $@

.PHONY: clean
clean:
	@$(RM) *.o
	@$(RM) $(COMMON_DIR)/src/*.o
	@$(RM) *_t.* *_u.*
	@$(RM) $(LIBNAME) $(BUILD_DIR)/$(LIBNAME)
	$(MAKE) -C $(EDGER8R_DIR) clean

.PHONY: rebuild
rebuild:
	$(MAKE) clean
	$(MAKE) all
