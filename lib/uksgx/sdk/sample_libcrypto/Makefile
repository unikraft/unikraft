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


include ../../buildenv.mk


CXXFLAGS += -fno-rtti -fno-exceptions -fPIC #-Werror

CPPFLAGS := -isystem $(SGX_IPP_INC) \
            -I$(COMMON_DIR)/inc/internal/           

CFLAGS +=  -fPIC -fno-exceptions -I$(SGX_IPP_INC) #-Werror

IPPLIB_DIR = $(SGX_IPP_DIR)/lib/linux/intel64/no_mitigation

LDFLAGS := -Bdynamic -L$(IPPLIB_DIR) $(LD_IPP) 
LDFLAGS += $(COMMON_LDFLAGS) -Wl,--version-script,sample_libcrypto.lds -Wl,--gc-sections

OBJS := sample_libcrypto.o

IPPDISP_DIR := $(LINUX_SDK_DIR)/tlibcrypto/ipp/ipp_disp
C_SRC := $(wildcard $(IPPDISP_DIR)/*.c)
C_OBJS  := $(patsubst $(IPPDISP_DIR)/%.c, %.o, $(C_SRC))
C_OBJS  := $(sort $(C_OBJS))

LIBSAMPLECRYPTO := libsample_libcrypto.so

$(BUILD_DIR):
	@$(MKDIR) $@

.PHONY: all
all: $(LIBSAMPLECRYPTO) | $(BUILD_DIR)
	@$(CP) $< $|

$(LIBSAMPLECRYPTO): $(OBJS) $(C_OBJS)
	$(CXX) $(CXXFLAGS) -shared -Wl,-soname,$(LIBSAMPLECRYPTO) $^ -o $@ $(LDFLAGS)	

$(C_OBJS): %.o: $(IPPDISP_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJS): %.o: %.cpp
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@

.PHONY: clean
clean:
	@$(RM) $(OBJS) $(C_OBJS) $(LIBSAMPLECRYPTO) $(BUILD_DIR)/$(LIBSAMPLECRYPTO)

.PHONY: rebuild
rebuild:
	$(MAKE) clean
	$(MAKE) all
