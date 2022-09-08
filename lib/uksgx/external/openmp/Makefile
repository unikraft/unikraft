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

OMP_DIR = openmp_code
LIBOMP = $(OMP_DIR)/final/build/runtime/src/libomp.a

OMP_CONFIG = -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER_WORKS=TRUE -DCMAKE_CXX_COMPILER_WORKS=TRUE
ifdef DEBUG
	OMP_CONFIG = -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_COMPILER_WORKS=TRUE -DCMAKE_CXX_COMPILER_WORKS=TRUE
endif
ENC_CXXFLAGS = $(patsubst -fpie, -fpic, $(ENCLAVE_CXXFLAGS))
OMP_CONFIG += -DCMAKE_CXX_FLAGS="$(ENC_CXXFLAGS)"
OMP_CONFIG += -DSGX_PROGRAM_SEARCH_PATH="$(EXT_BINUTILS_DIR)"

CHECK_SOURCE :=
ifeq ("$(wildcard $(OMP_DIR)/final)", "")
CHECK_SOURCE := omp_code
endif

.PHONY: all
all: $(LIBOMP) | $(BUILD_DIR)
	@$(CP) $(LIBOMP) $(BUILD_DIR)/libsgx_omp.a


$(BUILD_DIR):
	@$(MKDIR) $@

.PHONY: omp_code
omp_code:
ifeq ($(shell git rev-parse --is-inside-work-tree 2> /dev/null), true)
	git submodule update -f --init --recursive --remote -- $(OMP_DIR)
else
	$(RM) -rf openmp_code
	git clone -b svn-tags/RELEASE_801  https://github.com/llvm-mirror/openmp.git --depth 1 $(OMP_DIR)
endif


$(LIBOMP): $(CHECK_SOURCE)
ifeq ("$(wildcard $(OMP_DIR)/final/runtime/src/sgx_stub.h)", "")
	cd openmp_code && git apply ../0001-Enable-OpenMP-in-SGX.patch && cd ..
endif
	$(MKDIR) openmp_code/final/build && cd openmp_code/final/build && cmake $(OMP_CONFIG) .. && make 


.PHONY: clean
clean:
	$(RM) -rf openmp_code/final/build $(BUILD_DIR)/libsgx_omp.a

.PHONY: rebuild
rebuild:
	 $(MAKE) clean
	 $(MAKE) all


