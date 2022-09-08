#
# Copyright (C) 2011-2019 Intel Corporation. All rights reserved.
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

C_ENCLAVE_FLAGS = $(ENCLAVE_CFLAGS)
#SGX dnnl will be built with -fpic
C_ENCLAVE_FLAGS := $(subst -fpie,, $(C_ENCLAVE_FLAGS))

CXX_ENCLAVE_FLAGS = $(C_ENCLAVE_FLAGS)
CXX_ENCLAVE_FLAGS := $(subst -nostdinc,, $(CXX_ENCLAVE_FLAGS))
CXX_ENCLAVE_FLAGS += -nostdinc++

DNNL_DIR = dnnl
LIBDNNL = $(DNNL_DIR)/build/src/libdnnl.a
SGX_DNNL_LOG = $(shell cd ./$(DNNL_DIR) && git log --oneline --grep='SGX mkl-dnn' | cut -d' ' -f 2)
SGX_DNNL_DIR = sgx_dnnl
SGX_DNNL_LIB = $(SGX_DNNL_DIR)/lib
SGX_DNNL_INCLUDE = $(SGX_DNNL_DIR)/include

DNNL_CONFIG = -DCMAKE_BUILD_TYPE=Release
ifdef DEBUG
	DNNL_CONFIG = -DCMAKE_BUILD_TYPE=Debug
endif

DNNL_CONFIG += -DSGX_PROGRAM_SEARCH_PATH="$(EXT_BINUTILS_DIR)"

CHECK_SOURCE :=
ifneq ($(SGX_DNNL_LOG), SGX)
CHECK_SOURCE := dnnl_src
endif

.PHONY: all
all: $(LIBDNNL) $(SGX_DNNL_LIB) $(SGX_DNNL_INCLUDE)
	@$(CP) $(LIBDNNL) $(SGX_DNNL_LIB)/libsgx_dnnl.a
	@$(CP) $(DNNL_DIR)/include/dnnl.h $(SGX_DNNL_INCLUDE)
	@$(CP) $(DNNL_DIR)/include/dnnl.hpp $(SGX_DNNL_INCLUDE)
	@$(CP) $(DNNL_DIR)/include/mkldnn.hpp $(SGX_DNNL_INCLUDE)
	@$(CP) $(DNNL_DIR)/include/mkldnn_dnnl_mangling.h $(SGX_DNNL_INCLUDE)
	@$(CP) $(DNNL_DIR)/include/dnnl_debug.h $(SGX_DNNL_INCLUDE)
	@$(CP) $(DNNL_DIR)/include/dnnl_types.h $(SGX_DNNL_INCLUDE)
	@$(CP) $(DNNL_DIR)/build/include/dnnl_config.h $(SGX_DNNL_INCLUDE)
	@$(CP) $(DNNL_DIR)/build/include/dnnl_version.h $(SGX_DNNL_INCLUDE)

$(SGX_DNNL_LIB):
	@$(MKDIR) $@

$(SGX_DNNL_INCLUDE):
	@$(MKDIR) $@

.PHONY: dnnl_src
dnnl_src:
ifeq ($(shell git rev-parse --is-inside-work-tree), true)
	@$(RM) -r $(DNNL_DIR)/*
	git submodule update -f --init dnnl
	cd $(DNNL_DIR) && git am ../sgx_dnnl.patch
else
	@$(RM) -r $(DNNL_DIR)
	git clone https://github.com/intel/mkl-dnn.git -b v1.1.1 --depth 1  $(DNNL_DIR)
	cd $(DNNL_DIR) && git am ../sgx_dnnl.patch
endif

$(LIBDNNL):$(CHECK_SOURCE)
	mkdir -p $(DNNL_DIR)/build && cd $(DNNL_DIR)/build && cmake -DCMAKE_CXX_ENCLAVE_FLAGS="$(CXX_ENCLAVE_FLAGS)" -DCMAKE_C_ENCLAVE_FLAGS="$(C_ENCLAVE_FLAGS)" $(DNNL_CONFIG) .. && $(MAKE)

.PHONY: clean
clean:
	rm -fr $(DNNL_DIR)/build/* $(SGX_DNNL_DIR)/

.PHONY: rebuild
rebuild:
	$(MAKE) clean
	$(MAKE) all

