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

######## SGX SDK Settings ########
TOP_DIR  = ../../..
include $(TOP_DIR)/buildenv.mk

Quote_Library_Dir := ../../quote/linux
Quote_Library := $(Quote_Library_Dir)/libsgx_qe3_logic.so

PCE_Library_Dir := ../../../pce_wrapper/linux/
PCE_Library := $(PCE_Library_Dir)/libsgx_pce_logic.so

######## SGX QL Library Settings ########
vpath %.c $(COMMON_DIR)/src

QL_Lib_Cpp_Files := $(wildcard ../*.cpp)
QL_Lib_C_Files := se_trace.c se_thread.c

QL_Lib_Include_Paths := -I../../common/inc -I./ -I$(SGX_SDK)/include -I../../../common/inc/internal 
QL_Lib_Include_Paths += -I../../quote/inc -I../../../pce_wrapper/inc -I../inc

QL_Lib_C_Flags := $(COMMON_FLAGS) -g -fPIC -Wno-attributes $(QL_Lib_Include_Paths)

LDUFLAGS:= -pthread $(COMMON_LDFLAGS)
LDUFLAGS += -Wl,--version-script=dcap_ql_wrapper.lds -Wl,--gc-sections

QL_Lib_Cpp_Flags := $(QL_Lib_C_Flags) -std=c++11
QL_Lib_Link_Flags := $(SGX_COMMON_FLAGS) -g -L$(Quote_Library_Dir) -lsgx_qe3_logic -L$(PCE_Library_Dir) -lsgx_pce_logic -L$(TOP_DIR)/build/linux -L$(SGX_SDK)/lib64 -lpthread -ldl

QL_Lib_Cpp_Flags += -DDISABLE_TRACE
QL_Lib_Link_Flags += -DDISABLE_TRACE

QL_Lib_Cpp_Objects := $(QL_Lib_Cpp_Files:.cpp=.o)
QL_Lib_C_Objects := $(QL_Lib_C_Files:.c=.o)

QL_Lib_Name_Dynamic  := libsgx_dcap_ql.so
QL_VERSION:= $(shell awk '$$2 ~ /QUOTE_LOADER_VERSION/ { print substr($$3, 2, length($$3) - 2); }' $(COMMON_DIR)/inc/internal/se_version.h)
QL_SO_VERSION:= $(shell echo $(QL_VERSION) |awk -F. '{print $$1}') 

.PHONY: all run

all: install_lib

install_lib: $(QL_Lib_Name_Dynamic) | $(BUILD_DIR)
	@$(CP) $(QL_Lib_Name_Dynamic) $|

run: all

######## QL Library Objects ########

$(QL_Lib_C_Objects): %.o: %.c
	@$(CC) $(QL_Lib_C_Flags) -c $< -o $@
	@echo "CC  <=  $<"

$(QL_Lib_Cpp_Objects): %.o: %.cpp
	@$(CXX) $(QL_Lib_Cpp_Flags) -c $< -o $@
	@echo "CXX  <=  $<"

$(QL_Lib_Name_Dynamic): $(QL_Lib_Cpp_Objects) $(QL_Lib_C_Objects) $(PCE_Library) $(Quote_Library)
	$(CXX) $(CXXFLAGS) $(QL_Lib_Cpp_Objects) $(QL_Lib_C_Objects) -shared -Wl,-soname=$@.$(QL_SO_VERSION) $(LDUFLAGS) $(QL_Lib_Link_Flags) -o $@

$(Quote_Library): force_look
	@make -C $(Quote_Library_Dir)

$(PCE_Library): force_look
	@make -C $(PCE_Library_Dir)

force_look:
	true

.PHONY: clean

clean:
	@rm -f .config_* $(QL_Lib_Name) $(QL_Lib_Cpp_Objects) $(QL_Lib_Cpp_Deps) $(QL_Lib_Name_Dynamic) 
	@make -C $(Quote_Library_Dir) clean


