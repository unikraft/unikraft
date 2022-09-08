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

TOP_DIR := ../../..
MITIGATION-CVE-2020-0551=LOAD
include ../buildenv.mk

CXXFLAGS  += -Werror
CFLAGS	+= -Werror

QE_VER:= $(shell awk '$$2 ~ /QE_VERSION/ { print substr($$3, 2, length($$3) - 2); }' $(COMMON_DIR)/inc/internal/se_version.h)

EXTERNAL_LIB += -L$(EPID_SDK_DIR)/epid/member -lmember \
				-L$(EPID_SDK_DIR)/epid/common -lcommon

INCLUDE += -I$(LINUX_PSW_DIR)/ae/data/constants/linux \
           -I$(SGX_HEADER_DIR)/libcxx                 \
           -I$(LINUX_PSW_DIR)/ae/common	              \
           -I$(LINUX_PSW_DIR)/ae/inc/internal         \
           -I$(EPID_SDK_DIR) 


SRC := $(wildcard *.cpp)
SRC += se_sig_rl.cpp                  \
	   pve_qe_common.cpp 

OBJ := $(SRC:.cpp=.o)
OBJS := $(sort $(OBJ) version.o)

EPID_FILTER = -Werror -nostdinc -nostdinc++
EPID_CFLAGS = $(filter-out $(EPID_FILTER),$(ENCLAVE_CFLAGS))
EPID_CXXFLAGS = $(filter-out $(EPID_FILTER),$(ENCLAVE_CXXFLAGS))

.PHONY: all
all: $(SONAME) 

.PHONY: EPID
EPID:
	cd $(EPID_SDK_DIR) && CFLAGS="$(EPID_CFLAGS)" CXXFLAGS="$(EPID_CXXFLAGS)" ./configure prefix=/usr/local
	$(MAKE) -C $(EPID_SDK_DIR) 2> /dev/null

$(SONAME): $(OBJS) EPID
	$(CC) $(CFLAGS) -o $@ $(OBJS) -nostdlib -nodefaultlibs -nostartfiles -Wl,-soname=libsgx_$(AENAME).signed.so.$(call SPLIT_VERSION,$(QE_VER),1) $(LDTFLAGS)
	$(STRIP) --strip-unneeded --remove-section=.comment --remove-section=.note $(SONAME)

$(OBJ): $(AENAME)_t.c
$(OBJ): %.o: %.cpp
	$(CXX) $(CXXFLAGS) -fno-exceptions -fno-rtti $(INCLUDE) $(DEFINES) -c $< -o $@

$(AENAME)_t.c: $(EDLFILE)
	@$(EDGER8R) --trusted $< --search-path $(SGX_HEADER_DIR)

.PHONY: clean
clean:
	@$(RM) *.o
	@$(RM) *.so
	@$(RM) *.map
	@$(RM) *_t.*
ifeq ("$(shell test -f $(EPID_SDK_DIR)/Makefile && echo Makefile exists)", "Makefile exists")
	-$(MAKE) -C $(EPID_SDK_DIR) clean
endif
	@$(RM) -r $(EPID_SDK_DIR)/Makefile $(EPID_SDK_DIR)/config.log $(EPID_SDK_DIR)/config.status

.PHONY: rebuild
rebuild:
	$(MAKE) clean
	$(MAKE) all
