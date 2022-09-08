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

export BUILD := LINUX
include ../../../../buildenv.mk

TOOL_NAME := ref_wl_gen

CPPFLAGS    += -I$(SGX_HEADER_DIR)                      \
            -I$(COMMON_DIR)/inc/internal                \
            -I$(LINUX_PSW_DIR)/ae/data/constants/linux  \
            -I$(LINUX_PSW_DIR)/ae/inc/internal          \
            -I$(LINUX_PSW_DIR)/ae/inc                   \
            -I$(LINUX_SDK_DIR)/sign_tool/SignTool/      \
            -I$(LINUX_EXTERNAL_DIR)/rdrand

CXXFLAGS += -Werror
CFLAGS += -Werror

RDRAND_LIBDIR := $(LINUX_EXTERNAL_DIR)/rdrand/src
RDRAND_MAKEFILE := $(RDRAND_LIBDIR)/Makefile

EXTERNAL_LIB += -L$(RDRAND_LIBDIR) -lrdrand -L$(SGX_LIB_DIR) -lsgx_tcrypto

LOCAL_OBJS     := ref_wl.o ref_wl_gen.o parse_key_file.o
COMMON_OBJS    := sgx_memset_s.o sgx_read_rand.o
C_OBJS         := se_trace.o

OBJS           := $(LOCAL_OBJS) $(COMMON_OBJS) $(C_OBJS)

vpath %.cpp ./ $(COMMON_DIR)/src/
vpath %.c  $(COMMON_DIR)/src/

.PHONY: all
all: $(OBJS) rdrand | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(OBJS) $(EXTERNAL_LIB) -o $(TOOL_NAME)
	@$(CP) $(TOOL_NAME) $|

$(BUILD_DIR):
	 @$(MKDIR) $@

.PHONY: rdrand
rdrand: $(RDRAND_MAKEFILE)
	$(MAKE) -C $(RDRAND_LIBDIR)

$(RDRAND_MAKEFILE):
ifeq ($(ARCH), x86)
	@cd $(RDRAND_LIBDIR);./configure
else
	@cd $(RDRAND_LIBDIR);./configure CFLAGS=-fPIC
endif


%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@
%.o: %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

.PHONY: clean
clean:
	@$(RM) *~ *.o $(TOOL_NAME) $(BUILD_DIR)/$(TOOL_NAME)
ifeq ($(RDRAND_MAKEFILE), $(wildcard $(RDRAND_MAKEFILE)))
	@$(MAKE) distclean -C $(RDRAND_LIBDIR)
endif

.PHONY: rebuild
rebuild:
	$(MAKE) clean
	$(MAKE) all
