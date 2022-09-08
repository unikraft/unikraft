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

TOP_DIR = ../..
include ../../buildenv.mk

CXXFLAGS += $(ENCLAVE_CXXFLAGS) -Werror -fno-exceptions -fno-rtti

#libsgx_pthread.a must be built with -fpic option, or pthread will crash in SIM mode
CXXFLAGS := $(subst -fpie,, $(CXXFLAGS))
CXXFLAGS += -fpic

CPPFLAGS := -I$(COMMON_DIR)/inc/internal \
            -I$(COMMON_DIR)/inc          \
            -I$(COMMON_DIR)/inc/tlibc    \
            -I$(COMMON_DIR)/../sdk/trts  \
            -I$(LINUX_PSW_DIR)

OBJ := pthread.o \
       pthread_once.o \
       pthread_mutex.o \
       pthread_cond.o \
       pthread_tls.o \
       pthread_rwlock.o

EDGER8R_DIR = $(LINUX_SDK_DIR)/edger8r/linux
EDGER8R = $(EDGER8R_DIR)/_build/Edger8r.native
LIBPTHREAD := libsgx_pthread.a

.PHONY:all
all: $(LIBPTHREAD) | $(BUILD_DIR)
	$(CP) $(LIBPTHREAD) $|

pthread.o: $(COMMON_DIR)/inc/sgx_pthread_t.h

$(COMMON_DIR)/inc/sgx_pthread_t.h: $(COMMON_DIR)/inc/sgx_pthread.edl $(EDGER8R)
	$(EDGER8R) --header-only --trusted --search-path $(TOP_DIR)/common/inc sgx_pthread.edl

$(EDGER8R):
	$(MAKE) -C $(EDGER8R_DIR)

$(LIBPTHREAD): $(OBJ)
	$(AR) rcs $@ $^

$(OBJ): %.o: %.cpp
	$(CXX) -c $(CXXFLAGS) $(CPPFLAGS) $< -o $@

$(BUILD_DIR):
	@$(MKDIR) $@

.PHONY: clean
clean:
	@$(RM) *.o *.a $(BUILD_DIR)/$(LIBPTHREAD)
	@$(RM) sgx_pthread_t.*
	$(MAKE) -C $(EDGER8R_DIR) clean

.PHONY: rebuild
rebuild: 
	$(MAKE) clean
	$(MAKE) all
