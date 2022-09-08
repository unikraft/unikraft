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

CFLAGS   += $(ENCLAVE_CFLAGS) -D_TLIBC_GNU_
ASFLAGS  := $(CFLAGS)
CFLAGS   += -std=c99
CXXFLAGS += $(ENCLAVE_CXXFLAGS) -fno-exceptions -fno-rtti

CPPFLAGS += -I.                          \
            -I$(COMMON_DIR)/inc          \
            -I$(COMMON_DIR)/inc/tlibc    \
            -I$(COMMON_DIR)/inc/internal \
            -I$(LINUX_SDK_DIR)/trts      \
            -I./string/asm               \
            -I./gdtoa

LIBM_PATH   := $(LINUX_EXTERNAL_DIR)/libm/Libm_ct_Linux
LIBIRC_PATH := $(LINUX_EXTERNAL_DIR)/libirc/efi2
SVML_PATH   := $(LINUX_EXTERNAL_DIR)/libm/SVML_Intel64_Linux

ifeq ($(USE_STRING_LIB), 0)
CFLAGS += -D_TLIBC_USE_INTEL_FAST_STRING_
CXXFLAGS += -D_TLIBC_USE_INTEL_FAST_STRING_
endif

ifeq ($(USE_MATH_LIB), 0)
C_SRCS := $(wildcard gen/*.c gdtoa/*.c locale/*.c math/opt/*.c stdlib/*.c string/*.c stdio/*.c time/*.c)
LIBC_C_SRCS := $(filter-out $(wildcard gen/fpclassify*.c gen/isfinite*.c gen/isinf*.c gen/isnan*.c gen/isnormal*.c gen/signbit*.c), $(C_SRCS))
LIBC_CPP_SRCS := $(wildcard gen/*.cpp) tstdc_version.cpp
LIBC_ASM_SRCS := $(wildcard gen/*.S string/asm/*.S)
else
LIBC_C_SRCS := $(wildcard gen/*.c gdtoa/*.c locale/*.c math/*.c stdlib/*.c string/*.c stdio/*.c time/*.c) 
LIBC_CPP_SRCS := $(wildcard gen/*.cpp) tstdc_version.cpp
LIBC_ASM_SRCS := $(wildcard gen/*.S math/*.S string/asm/*.S)
endif

LIBC_OBJS := $(LIBC_C_SRCS:.c=.o)
LIBC_OBJS += $(LIBC_CPP_SRCS:.cpp=.o)
LIBC_OBJS += $(LIBC_ASM_SRCS:.S=.o)
LIBC_OBJS := $(sort $(LIBC_OBJS))

LIBC_NAME    := libtlibc.a

.PHONY: all
all: $(LIBC_NAME) | $(BUILD_DIR) 

$(LIBC_NAME): $(LIBC_OBJS)
	$(AR) rcs $@ $^
ifeq ($(USE_STRING_LIB), 0)
	$(MKDIR) $(BUILD_DIR)/.libirc
	$(RM) -f $(BUILD_DIR)/.libirc/* && cd $(BUILD_DIR)/.libirc && $(AR) x $(LIBIRC_PATH)/libirc.a
	$(AR) qD $@ $(BUILD_DIR)/.libirc/*.o
	@$(RM) -rf $(BUILD_DIR)/.libirc
endif
ifeq ($(USE_MATH_LIB), 0)
	$(MKDIR) $(BUILD_DIR)/.libimf  $(BUILD_DIR)/.libsvml
	$(RM) -f $(BUILD_DIR)/.libimf/* && cd $(BUILD_DIR)/.libimf && $(AR) x $(LIBM_PATH)/libimf.a
	$(RM) -f $(BUILD_DIR)/.libsvml/* && cd $(BUILD_DIR)/.libsvml && $(AR) x $(SVML_PATH)/libsvml.a
	$(AR) qD $@ $(BUILD_DIR)/.libimf/*.o
	$(AR) qD $@ $(BUILD_DIR)/.libsvml/*.o
	@$(RM) -rf $(BUILD_DIR)/.libimf $(BUILD_DIR)/.libsvml
endif

string/consttime_memequal.o: string/consttime_memequal.c
	$(CC)  $(filter-out -O2,$(CFLAGS)) -O1 $(CPPFLAGS) -c $< -o $@
	
%.o: %.c
	$(CC)  $(CFLAGS)   $(CPPFLAGS) -c $< -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@

%.o: %.S
	$(CC)  $(ASFLAGS)  $(CPPFLAGS) -c $< -o $@

$(BUILD_DIR):
	$(MKDIR) $@

.PHONY: clean
clean:
	@$(RM) $(LIBC_NAME) $(LIBC_OBJS) math/opt/*.o 
