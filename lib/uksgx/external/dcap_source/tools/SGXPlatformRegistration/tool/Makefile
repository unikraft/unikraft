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

include ../../../QuoteGeneration/buildenv.mk


LIBS_DIR := ../build/lib64
BINS_DIR := ../build/bin
MPA_UEFI_DIR = ../uefi
MPA_MANAGEMENT_DIR = ../management
MPA_REGISTRATION_COMMON_DIR = ../common
MPA_REGISTRATION_CORE_DIR = ../agent

# turn on cet
CC_GREAT_EQUAL_8 := $(shell expr "`$(CC) -dumpversion`" \>= "8")
ifeq ($(CC_GREAT_EQUAL_8), 1)
    COMMON_FLAGS += -fcf-protection
endif


CXXFLAGS += $(COMMON_FLAGS)
CXXFLAGS += -fPIC 

MPA_MANAGEMENT_EXECUTABLE_OUTPUT_FILE := ../build/bin/mpa_manage

LIB_MPA_MANAGEMENT := $(LIBS_DIR)/libmpa_management.a
LIBS_MPA := $(LIB_MPA_MANAGEMENT) -L$(LIBS_DIR) -lmpa_uefi

INCLUDE_DIR := ../include
LOCAL_INCLUDE_DIR := ./inc

INCLUDE += -I$(MPA_UEFI_DIR)/inc
INCLUDE += -I$(MPA_REGISTRATION_COMMON_DIR)/inc
INCLUDE += -I$(MPA_REGISTRATION_CORE_DIR)/inc
INCLUDE += -I$(MPA_MANAGEMENT_DIR)/inc
INCLUDE += -I$(INCLUDE_DIR)
INCLUDE += -I$(LOCAL_INCLUDE_DIR)

CPP_SRCS := $(wildcard src/*.cpp)
CPP_SRCS += $(MPA_REGISTRATION_CORE_DIR)/src/AgentConfiguration.cpp $(MPA_REGISTRATION_CORE_DIR)/src/agent_logger.cpp $(MPA_REGISTRATION_COMMON_DIR)/src/common.cpp
CPP_OBJS := $(CPP_SRCS:.cpp=.o)
CPP_DEPS := $(CPP_OBJS:%.o=%.d)

LDFLAGS := $(COMMON_LDFLAGS)

.PHONY: clean all
all: $(LIBS_MPA) $(CPP_OBJS)
	echo $(CPP_SRCS)
	$(CXX) -g -I. $(INCLUDE) $(CPP_OBJS) $(LIBS_MPA) $(LDFLAGS) -o $(MPA_MANAGEMENT_EXECUTABLE_OUTPUT_FILE)

$(CPP_OBJS): %.o: %.cpp
	$(CXX) -c -I. $(CXXFLAGS) $(INCLUDE) $(LDFLAGS) -MMD $< -o $@

$(LIBS_MPA):
	@mkdir -p $(LIBS_DIR)
	@mkdir -p $(BINS_DIR)
	@make -C $(MPA_UEFI_DIR)
	@make -C $(MPA_MANAGEMENT_DIR) static

clean:
	@$(RM) $(CPP_OBJS) $(MPA_MANAGEMENT_EXECUTABLE_OUTPUT_FILE) $(CPP_DEPS)
	@make -C $(MPA_UEFI_DIR) clean
	@make -C $(MPA_MANAGEMENT_DIR) clean
	@rm -rf $(LIBS_DIR)
	@rm -rf $(BINS_DIR)



