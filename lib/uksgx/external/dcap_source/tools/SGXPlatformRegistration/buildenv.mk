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

ROOT_DIR := ./..

LOCAL_COMMON_DIR  := $(ROOT_DIR)/common
INCLUDE_DIR := $(ROOT_DIR)/include
LIBS_DIR := $(ROOT_DIR)/build/lib64
BINS_DIR := $(ROOT_DIR)/build/bin
CP := cp -f


RA_VERSION= $(shell awk '$$2 ~ /STRFILEVER/ { print substr($$3, 2, length($$3) - 2); }' $(ROOT_DIR)/../../QuoteGeneration/common/inc/internal/se_version.h)
SPLIT_VERSION=$(word $2,$(subst ., ,$1))

# turn on cet
CC_GREAT_EQUAL_8 := $(shell expr "`$(CC) -dumpversion`" \>= "8")
ifeq ($(CC_GREAT_EQUAL_8), 1)
    COMMON_FLAGS += -fcf-protection
endif

CXXFLAGS += -fPIC 

LDFLAGS += -shared -Wl,-z,relro,-z,now,-z,noexecstack

INCLUDE += -I$(INCLUDE_DIR)
INCLUDE += -I$(INCLUDE_DIR)/c_wrapper
INCLUDE += -I$(LOCAL_COMMON_DIR)/inc
INCLUDE += -I$(ROOT_DIR)/../../QuoteGeneration/common/inc/internal
INCLUDE += -Iinc

CPP_OBJS := $(CPP_SRCS:%.cpp=%.o)
CPP_DEPS := $(CPP_OBJS:%.o=%.d)

all: $(TARGET_LIB).so $(CPP_OBJS)
static: $(TARGET_LIB).a

.PHONY: clean all

$(TARGET_LIB).so : $(CPP_OBJS)
	$(CC) $(CCFLAGS) $(CFLAGS) $(CPP_OBJS) -Wl,-soname=$@.$(call SPLIT_VERSION,$(RA_VERSION),1)  $(LDFLAGS) -o $@
	$(CP) $@ $(LIBS_DIR)

$(CPP_OBJS): %.o: %.cpp
	$(CXX) -c $(CXXFLAGS) $(INCLUDE) -MMD $< -o $@

clean:
	@$(RM) $(CPP_OBJS) $(TARGET_LIB).a $(TARGET_LIB).so $(CPP_DEPS)
