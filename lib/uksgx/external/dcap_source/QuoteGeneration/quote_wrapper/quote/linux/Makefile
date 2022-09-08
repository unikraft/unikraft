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

QE3_Dir := ../enclave/linux
QE3_Signed_Name := $(QE3_Dir)/libsgx_qe3.signed.so

ID_ENCLAVE_Dir := ../id_enclave/linux
ID_ENCLAVE_Signed_Name := $(ID_ENCLAVE_Dir)/libsgx_id_enclave.signed.so

######## Quote Settings ########


vpath %.c $(COMMON_DIR)/src
vpath %.cpp .. .
Quote_C_Files := qe3_u.c id_enclave_u.c se_thread.c se_trace.c
Quote_Cpp_Files := qe_logic.cpp sgx_ql_core_wrapper.cpp

Quote_Include_Paths := -I$(SGX_SDK)/include -I../inc -I../../common/inc -I./ -I../enclave -I$(TOP_DIR)/pce_wrapper/inc -I$(COMMON_DIR)/inc/internal -I$(COMMON_DIR)/inc/internal/linux

Quote_C_Flags := $(COMMON_FLAGS) -g -fPIC -Wno-attributes $(Quote_Include_Paths)

Quote_Cpp_Flags := $(Quote_C_Flags) -std=c++11
Quote_Link_Flags := $(COMMON_FLAGS) -g -L$(ROOT_DIR)/build/linux -L$(SGX_SDK)/lib64 -lsgx_urts -lpthread -ldl

ifndef DEBUG
Quote_Cpp_Flags += -DDISABLE_TRACE
Quote_Link_Flags += -DDISABLE_TRACE
endif

Quote_Link_Flags += -Wl,--version-script=qe3_logic.lds -Wl,--gc-sections

Quote_C_Objects := $(Quote_C_Files:.c=.o)
Quote_Cpp_Objects := $(Quote_Cpp_Files:.cpp=.o)

Quote_Name := libsgx_qe3_logic.so

.PHONY: all
all:  install_lib

install_lib: $(Quote_Name) | $(BUILD_DIR)
	@$(CP) $(Quote_Name) $|


######## Quote Objects ########

qe3_u.c: $(SGX_EDGER8R) ../enclave/qe3.edl
	$(SGX_EDGER8R) --untrusted ../enclave/qe3.edl --search-path ../enclave
	@echo "GEN  =>  $@"

id_enclave_u.c: $(SGX_EDGER8R) ../id_enclave/id_enclave.edl
	$(SGX_EDGER8R) --untrusted ../id_enclave/id_enclave.edl
	@echo "GEN  =>  $@"

qe_logic.o : qe3_u.c id_enclave_u.c
 
%.o: %.c
	@$(CC) $(Quote_C_Flags) -c $< -o $@
	@echo "CC   <=  $<"

$(Quote_Cpp_Objects): %.o: %.cpp
	@$(CXX) $(Quote_Cpp_Flags) -c $< -o $@
	@echo "CXX  <=  $<"

$(Quote_Name): $(Quote_C_Objects) $(Quote_Cpp_Objects)
	$(CXX) $(Quote_C_Objects) $(Quote_Cpp_Objects) -shared -Wl,-soname=$@ $(Quote_Link_Flags) -o $@
	@echo "LINK =>  $@"

$(BUILD_DIR):
	@$(MKDIR) $@

$(QE3_Signed_Name): force_look
	@make -C $(QE3_Dir)

$(ID_ENCLAVE_Signed_Name): force_look
	@make -C $(ID_ENCLAVE_Dir)

force_look:
	true

.PHONY: clean

clean:
	@rm -f .config_* $(Quote_Name)  $(Quote_C_Objects) $(Quote_Cpp_Objects) qe3_u.* id_enclave_u.*
	@make -C $(QE3_Dir) clean
	@make -C $(ID_ENCLAVE_Dir) clean


