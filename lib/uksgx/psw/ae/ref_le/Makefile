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
include ../buildenv.mk

SIGNED_NAME      = libsgx_$(AENAME).signed.so
KEYS_DIR         = ./ref_keys
LE_PRIVATE_KEY   = le_private_test_key.pem
WL_CFG_FILE      = wl_cfg.csv
WHITE_LIST_NAME  = ref_white_list.bin
REF_WL_GEN_NAME  = ref_wl_gen
REF_WL_GEN_DIR   = ./ref_wl_gen

WL_VERSION       = 1

CFLAGS   += -Werror
CXXFLAGS += -Werror

INCLUDE += -I$(LINUX_PSW_DIR)/ae/common
INCLUDE += -I$(LINUX_PSW_DIR)/ae/data/constants/linux
INCLUDE += -I$(LINUX_PSW_DIR)/ae/inc/internal

SRC := $(wildcard *.cpp)
OBJ := $(SRC:.cpp=.o)
OBJS = $(sort $(OBJ) version.o)

CONFIG := config_linux.xml

.PHONY: all
all: $(SONAME) REF_WL_GEN | $(BUILD_DIR)
	@$(SGXSIGN) sign -key $(KEYS_DIR)/$(LE_PRIVATE_KEY) -enclave $(SONAME) -out $(BUILD_DIR)/$(SIGNED_NAME) -config $(CONFIG)
	@$(REF_WL_GEN_DIR)/$(REF_WL_GEN_NAME) gen-wl -out $(BUILD_DIR)/$(WHITE_LIST_NAME) -cfg $(KEYS_DIR)/$(WL_CFG_FILE) -key $(KEYS_DIR)/$(LE_PRIVATE_KEY) -ver $(WL_VERSION)

.PHONY: REF_WL_GEN
REF_WL_GEN:
	$(MAKE) -C $(REF_WL_GEN_DIR)

$(SONAME): $(AENAME)_t.o $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@  $(OBJ) $(AENAME)_t.o -nostdlib -nodefaultlibs -nostartfiles -Wl,-soname=${SIGNED_NAME}.$(call SPLIT_VERSION,$(LE_VER),1) $(LDTFLAGS)

$(OBJ): %.o:%.cpp
	$(CXX) $(CXXFLAGS) -fno-exceptions -fno-rtti $(INCLUDE) $(DEFINES) -c $< -o $@

$(AENAME)_t.c: $(EDLFILE)
	@$(EDGER8R) --trusted $<

$(AENAME)_t.o: $(AENAME)_t.c
	$(CC) $(filter-out -O2,$(CFLAGS)) -fPIC -O1 $(INCLUDE) -c $< -o $@

$(BUILD_DIR):
	@$(MKDIR) $@

.PHONY: clean
clean:
	@$(RM) *.o
	@$(RM) *.so
	@$(RM) *.map
	@$(RM) *_t.*
	@$(MAKE) clean -C $(REF_WL_GEN_DIR)
	@$(RM) $(BUILD_DIR)/$(SIGNED_NAME)
	@$(RM) $(BUILD_DIR)/$(WHITE_LIST_NAME)

.PHONY: rebuild
rebuild:
	$(MAKE)	clean
	$(MAKE) all
