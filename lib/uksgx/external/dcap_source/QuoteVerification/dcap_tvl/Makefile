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

# part of SDK
# include buildenv in linux trunk
include ../../../../buildenv.mk

TVL_CPP_SRCS := $(wildcard *.cpp)
TVL_OBJS := $(TVL_CPP_SRCS:.cpp=.o)

COMMON_INCLUDE	:= -I$(COMMON_DIR)/inc -I$(COMMON_DIR)/inc/tlibc -I$(LINUX_SDK_DIR)/tlibcxx/include
TVL_INC  += $(COMMON_INCLUDE) -I../QvE/Include -I$(DCAP_DIR)/QuoteGeneration/quote_wrapper/common/inc -I$(DCAP_DIR)/QuoteGeneration/pce_wrapper/inc


TVL_LIB  := sgx_dcap_tvl
TVL_NAME := lib$(TVL_LIB).a


ifeq ($(MITIGATION-CVE-2020-0551), LOAD)
	BUILD_DIR := $(DCAP_DIR)/QuoteGeneration/build/linuxLOAD
else ifeq ($(MITIGATION-CVE-2020-0551), CF)
	BUILD_DIR := $(DCAP_DIR)/QuoteGeneration/build/linuxCF
else
	BUILD_DIR := $(DCAP_DIR)/QuoteGeneration/build/linux
endif


ifneq ($(DEBUG), 1)
    COMMON_FLAGS += -ffunction-sections -fdata-sections
endif
CC_BELOW_4_9 := $(shell expr "`$(CC) -dumpversion`" \< "4.9")
ifeq ($(CC_BELOW_4_9), 1)
    COMMON_FLAGS += -fstack-protector
else
    COMMON_FLAGS += -fstack-protector-strong
endif

ENCLAVE_CXXFLAGS += $(SGX_COMMON_CXXFLAGS) $(COMMON_FLAGS) -fPIC -std=c++11


.PHONY: all

all: install_lib $(TVL_NAME)

$(BUILD_DIR):
	@$(MKDIR) $@

install_lib: $(TVL_NAME) | $(BUILD_DIR)
	@$(CP) $(TVL_NAME) $|

$(TVL_OBJS): %.o: %.cpp
	$(CXX) $(ENCLAVE_CXXFLAGS) $(TVL_INC) -c $< -o $@
	echo "CXX  <=  $<"

$(TVL_NAME): $(TVL_OBJS)
	$(AR) rsD $(TVL_NAME) $(TVL_OBJS)


.PHONY: clean

clean:
	@echo "Cleaning objects"
	@rm -f $(TVL_OBJS) $(TVL_NAME)
	@rm -f $(BUILD_DIR)/$(TVL_NAME)
