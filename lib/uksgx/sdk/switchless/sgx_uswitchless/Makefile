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

include ../../../buildenv.mk




# SL = Switch-Less
SL_DIR := $(LINUX_SDK_DIR)/switchless
SL_DIR_SRCS := $(SL_DIR)/sgx_uswitchless


EDGER8R_DIR = $(LINUX_SDK_DIR)/edger8r/linux
EDGER8R = $(EDGER8R_DIR)/_build/Edger8r.native

CPP_SRCS := $(wildcard $(SL_DIR_SRCS)/*.cpp)

# Files that do NOT match *_untrusted.c or *_untrusted.S are trusted components
C_SRCS := $(wildcard $(SL_DIR_SRCS)/*.c)
C_OBJS := $(C_SRCS:.c=.o)
CPP_OBJS := $(CPP_SRCS:.cpp=.o)

ALL_OBJS := $(CPP_OBJS) $(C_OBJS) 

SL_CXXFLAGS := $(CXXFLAGS)
SL_CXXFLAGS += -fPIC $(CET_FLAGS)
SL_CXXFLAGS += -I$(COMMON_DIR)/inc          \
          -I$(COMMON_DIR)/inc/internal \
          -I$(SL_DIR)/inc


SL_CFLAGS := $(CFLAGS)
SL_CFLAGS += -Werror -D_GNU_SOURCE -fPIC $(CET_FLAGS)
SL_CFLAGS += $(ADDED_INC)
SL_CFLAGS += -I$(COMMON_DIR)/inc          \
          -I$(COMMON_DIR)/inc/internal \
          -I$(SL_DIR)/inc


ifdef DEBUG
	SL_CFLAGS += -DSL_DEBUG
	SL_CXXFLAGS += -DSL_DEBUG
endif

OUT_LIB := libsgx_uswitchless.a

GREEN :=\033[1;32m
NC :=\033[0m

.PHONY: all build_src clean
all: build_src

build_src: $(OUT_LIB)

$(OUT_LIB): $(ALL_OBJS)
	@$(AR) rcs $@ $^
	@$(CP) $(OUT_LIB) $(BUILD_DIR)/
	@echo "$(GREEN)LINK => $@$(NC)"

$(C_OBJS): %.o: %.c sgx_tswitchless_u.h
	@$(CC) $(SL_CFLAGS) -c $< -o $@
	@echo "$(CC) <= $@" 

$(CPP_OBJS): %.o: %.cpp 
	@$(CXX) $(SL_CXXFLAGS) -c $< -o $@
	@echo "$(CXX) <= $@" 


sgx_tswitchless_u.h: $(EDGER8R)
	$(EDGER8R) --header-only --untrusted $(COMMON_DIR)/inc/sgx_tswitchless.edl --search-path $(COMMON_DIR)/inc

$(EDGER8R):
	$(MAKE) -C $(EDGER8R_DIR)


clean:
	@$(RM) -f $(OUT_LIB) $(ALL_OBJS) $(CPP_OBJS) $(BUILD_DIR)/$(OUT_LIB) sgx_tswitchless_u.h

