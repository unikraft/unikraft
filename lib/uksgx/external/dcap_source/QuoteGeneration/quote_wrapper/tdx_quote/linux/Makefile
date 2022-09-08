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

TDQE_WRAPPER_VER:= $(shell awk '$$2 ~ /TDQE_WRAPPER_VERSION/ { print substr($$3, 2, length($$3) - 2); }' \
	$(COMMON_DIR)/inc/internal/se_version.h)

PCE_Library_Dir := $(TOP_DIR)/pce_wrapper/linux
PCE_Library := $(PCE_Library_Dir)/libsgx_pce_logic.so

TDQE_Dir := ../enclave/linux
TDQE_Signed_Name := $(TDQE_Dir)/libsgx_tdqe.signed.so

ID_ENCLAVE_Dir := ../../quote/id_enclave/linux
ID_ENCLAVE_Signed_Name := $(ID_ENCLAVE_Dir)/libsgx_id_enclave.signed.so

######## Quote Settings ########


vpath %.c $(COMMON_DIR)/src
Quote_C_Files := tdqe_u.c id_enclave_u.c se_thread.c se_trace.c
Quote_Cpp_Files := $(wildcard ../*.cpp)

Quote_Include_Paths := -I$(SGX_SDK)/include -I../../common/inc -I./ \
                       -I../enclave -I$(TOP_DIR)/pce_wrapper/inc \
					   -I$(COMMON_DIR)/inc/internal \
					   -I$(COMMON_DIR)/inc/internal/linux

Quote_C_Flags := $(CFLAGS) -g -MMD -fPIC -Wno-attributes $(Quote_Include_Paths)

Quote_Cpp_Flags := $(CXXFLAGS) -g -MMD -fPIC -Wno-attributes $(Quote_Include_Paths)
Quote_Link_Flags := $(COMMON_LDFLAGS) -g -L$(ROOT_DIR)/build/linux \
                    -L$(PCE_Library_Dir) -lsgx_pce_logic -L$(SGX_SDK)/lib64 \
					-lsgx_urts -lpthread -ldl

Quote_Link_Flags += -Wl,--version-script=td_ql_logic.lds -Wl,--gc-sections

Quote_C_Objects := $(Quote_C_Files:.c=.o)
Quote_C_Depends := $(Quote_C_Files:.c=.d)
Quote_Cpp_Objects := $(Quote_Cpp_Files:.cpp=.o)
Quote_Cpp_Depends := $(Quote_Cpp_Files:.cpp=.d)

Quote_Name := libsgx_tdx_logic.so
Quote_Soname := $(Quote_Name).$(call SPLIT_VERSION,$(TDQE_WRAPPER_VER),1)

-include $(Quote_C_Depends)
-include $(Quote_Cpp_Depends)

.PHONY: all
all:  install_lib

install_lib: $(Quote_Name) | $(BUILD_DIR)
	@$(CP) $(Quote_Name) $|
	@ln -fs $(Quote_Name) $(BUILD_DIR)/$(Quote_Soname)

######## Quote Objects ########

tdqe_u.c: $(SGX_EDGER8R) ../enclave/tdqe.edl
	$(SGX_EDGER8R) --untrusted ../enclave/tdqe.edl --search-path ../enclave
	@echo "GEN  =>  $@"

id_enclave_u.c: $(SGX_EDGER8R) ../../quote/id_enclave/id_enclave.edl
	$(SGX_EDGER8R) --untrusted ../../quote/id_enclave/id_enclave.edl
	@echo "GEN  =>  $@"

%.o: %.c
	@$(CC) $(Quote_C_Flags) -c $< -o $@
	@echo "CC   <=  $<"

$(Quote_Cpp_Objects): %.o: %.cpp
	$(CXX) $(Quote_Cpp_Flags) -c $< -o $@
	@echo "CXX  <=  $<"

$(Quote_Name): $(Quote_C_Objects) $(Quote_Cpp_Objects) $(PCE_Library)
	$(CXX) $(Quote_C_Objects) $(Quote_Cpp_Objects) -shared $(Quote_Link_Flags) \
	       -Wl,-soname=$(Quote_Soname) -o $@
	@echo "LINK =>  $@"

$(PCE_Library): force_look
	@make -C $(PCE_Library_Dir)

$(TDQE_Signed_Name): force_look
	@make -C $(TDQE_Dir)

$(ID_ENCLAVE_Signed_Name): force_look
	@make -C $(ID_ENCLAVE_Dir)

force_look:
	true

.PHONY: clean

clean:
	@rm -f .config_* $(Quote_Name)  $(Quote_C_Objects) $(Quote_Cpp_Objects) \
	    tdqe_u.* id_enclave_u.*
	@make -C $(PCE_Library_Dir) clean
	@make -C $(TDQE_Dir) clean
	@make -C $(ID_ENCLAVE_Dir) clean

