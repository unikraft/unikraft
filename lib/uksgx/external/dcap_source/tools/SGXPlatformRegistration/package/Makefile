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
MPA_REGISTRATION_CORE_DIR = ../agent
MPA_NETWORK_DIR = ../network
MPA_UEFI_DIR = ../uefi
MPA_MANAGEMENT_LIB_DIR = ../management
MPA_MANAGEMENT_TOOL_DIR = ../tool
MPA_REGISTRATION_COMMON_DIR = ../common

INCLUDE_DIR := ../include

# turn on cet
CC_GREAT_EQUAL_8 := $(shell expr "`$(CC) -dumpversion`" \>= "8")
ifeq ($(CC_GREAT_EQUAL_8), 1)
    COMMON_FLAGS += -fcf-protection
endif


.PHONY: all 

LIB_MPA_REGISTRATION := $(LIBS_DIR)/libmpa_registration.a
LIBS_MPA := $(LIB_MPA_REGISTRATION) -L$(LIBS_DIR) -lmpa_network -lmpa_uefi

MPA_REGISTRATION_EXEC := mpa_registration

INCLUDE += -I$(MPA_REGISTRATION_COMMON_DIR)/inc
INCLUDE += -I$(MPA_REGISTRATION_CORE_DIR)/inc
INCLUDE += -I$(MPA_NETWORK_DIR)/inc
INCLUDE += -I$(MPA_UEFI_DIR)/inc
INCLUDE += -I$(INCLUDE_DIR)

CPP_SRCS := src/mpa_registration_service.cpp $(MPA_REGISTRATION_COMMON_DIR)/src/logger.cpp $(MPA_REGISTRATION_COMMON_DIR)/src/common.cpp
CPP_OBJS := $(CPP_SRCS:%.cpp=%.o)
CPP_DEPS := $(CPP_OBJS:%.o=%.d)

ifeq ($(MP_VERIFY_DATA_STRUCTS), 1)
	CXXFLAGS += -DMP_VERIFY_INTERNAL_DATA_STRUCT_WRITE=1 -DMP_VERIFY_INTERNAL_DATA_STRUCT_READ=1 $(COMMON_FLAGS)
else
	CXXFLAGS += -DMP_VERIFY_INTERNAL_DATA_STRUCT_WRITE=0 -DMP_VERIFY_INTERNAL_DATA_STRUCT_READ=0 $(COMMON_FLAGS)
endif

LDFLAGS := $(COMMON_LDFLAGS)

all: $(MPA_REGISTRATION_EXEC) 

$(MPA_REGISTRATION_EXEC): $(LIBS_MPA) $(CPP_OBJS)
	$(CXX) -I. $(INCLUDE) $(CXXFLAGS) $(CPP_OBJS) $(LIBS_MPA) $(LDFLAGS) -lcurl -o $@
	@cp -f $@ $(BINS_DIR)
-include $(CPP_DEPS)

$(CPP_OBJS): %.o: %.cpp
	$(CXX) -c $(CXXFLAGS) $(INCLUDE)  -MMD $< -o $@ 


$(LIBS_MPA):
	@mkdir -p $(LIBS_DIR)
	@mkdir -p $(BINS_DIR)
	@make -C $(MPA_REGISTRATION_CORE_DIR) static
	@make -C $(MPA_NETWORK_DIR)
	@make -C $(MPA_UEFI_DIR) MP_VERIFY_DATA_STRUCTS=$(MP_VERIFY_DATA_STRUCTS)
	@make -C $(MPA_MANAGEMENT_LIB_DIR) static
	@make -C $(MPA_MANAGEMENT_TOOL_DIR)

.PHONY: clean

clean:
	@rm -rf $(CPP_OBJS) $(MPA_REGISTRATION_EXEC) $(CPP_DEPS)
	@make -C $(MPA_REGISTRATION_CORE_DIR) clean
	@make -C $(MPA_NETWORK_DIR) clean
	@make -C $(MPA_UEFI_DIR) clean
	@make -C $(MPA_MANAGEMENT_LIB_DIR) clean
	@make -C $(MPA_MANAGEMENT_TOOL_DIR) clean
	@rm -rf $(LIBS_DIR)
	@rm -rf $(BINS_DIR)

build: $(MPA_REGISTRATION_EXEC)

rebuild: clean all
