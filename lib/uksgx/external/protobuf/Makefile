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

PB_DIR = protobuf_code
LIBPB = $(PB_DIR)/final_tlibPB/build/libprotobuf.a
PROTOCPB = $(PB_DIR)/final_protocPB/build/protoc
INCLUDEPB = $(PB_DIR)/src/google/protobuf

PB_CONFIG = -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER_WORKS=TRUE -DCMAKE_CXX_COMPILER_WORKS=TRUE
ifdef DEBUG
	PB_CONFIG = -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_COMPILER_WORKS=TRUE -DCMAKE_CXX_COMPILER_WORKS=TRUE
	LIBPB = $(PB_DIR)/final_tlibPB/build/libprotobufd.a
endif
ENC_CXXFLAGS = $(patsubst -fpie, -fpic, $(ENCLAVE_CXXFLAGS))
PB_CONFIG += -DCMAKE_CXX_FLAGS="$(ENC_CXXFLAGS)"
PB_CONFIG += -DSGX_PROGRAM_SEARCH_PATH="$(EXT_BINUTILS_DIR)"

CHECK_SOURCE :=
ifeq ("$(wildcard $(PB_DIR)/src)", "")
CHECK_SOURCE := protobuf_code
endif

.PHONY: all
ifeq ($(BUILD_PROTOC), 1)
all: $(LIBPB) $(PROTOCPB) $(BUILD_DIR)
	$(CP) $(LIBPB) $(BUILD_DIR)/libsgx_protobuf.a
	$(CP) $(PROTOCPB) $(BUILD_DIR)/sgx_protoc
else
all: $(LIBPB) $(BUILD_DIR)
	$(CP) $(LIBPB) $(BUILD_DIR)/libsgx_protobuf.a
endif

$(BUILD_DIR):
	@$(MKDIR) $@

.PHONY: protobuf_code
protobuf_code:
ifeq ($(shell git rev-parse --is-inside-work-tree), true)
	$(RM) -r $(PB_DIR)/
	git submodule update -f --init $(PB_DIR)
else
	$(RM) -r $(PB_DIR)/
	git clone https://github.com/protocolbuffers/protobuf.git -b 3.20.x --depth 1 $(PB_DIR)
endif


$(LIBPB): $(CHECK_SOURCE)
ifeq ("$(wildcard $(PB_DIR)/cmake/libsgx_protobuf.cmake)", "")
	cd $(PB_DIR) && git apply ../sgx_protobuf.patch&& cd ..
endif
	$(MKDIR) protobuf_code/final_tlibPB/build && cd protobuf_code/final_tlibPB/build && cmake $(PB_CONFIG) -Dprotobuf_BUILD_TESTS=FALSE -DSGX=TRUE ../../cmake && make libprotobuf

ifeq ($(BUILD_PROTOC), 1)
$(PROTOCPB): $(CHECK_SOURCE)
	$(MKDIR) protobuf_code/final_protocPB/build && cd protobuf_code/final_protocPB/build && cmake -Dprotobuf_BUILD_TESTS=FALSE -DSGX=FALSE -DCMAKE_SKIP_RPATH=TRUE ../../cmake && make protoc
endif

.PHONY: clean
clean:
	$(RM) -rf protobuf_code/final_tlibPB/build  protobuf_code/final_protocPB/build  $(BUILD_DIR)/libsgx_protobuf.a $(BUILD_DIR)/sgx_protoc
	

.PHONY: rebuild
rebuild:
	 $(MAKE) clean
	 $(MAKE) all


