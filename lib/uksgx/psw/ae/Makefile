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

CHECK_CMAKE := $(shell which cmake)

ifeq ($(CHECK_CMAKE), )
    CMAKE_RESULT := cmake_check_fail
else
    CMAKE_RESULT :=
endif

CHECK_PREBUILT :=
ifeq ("$(wildcard data/prebuilt/libsgx_le.signed.so)", "")
CHECK_PREBUILT := prebuilt_check_fail
endif

.PHONY: all
ifeq ($(BUILD_REF_LE), 1)
all: $(CMAKE_RESULT) AESM REF_LE COPY_AES| $(BUILD_DIR)
else
all: $(CMAKE_RESULT) AESM COPY_AES| $(BUILD_DIR)
endif #($(BUILD_REF_LE), 1)

# COPY_AES: currently copy le, qe, pve, pce, qe3
.PHONY: COPY_AES
COPY_AES: $(CHECK_PREBUILT) | $(BUILD_DIR)
	$(CP) data/prebuilt/* $(BUILD_DIR)

.PHONY: AESM
AESM: | $(BUILD_DIR)
	$(MAKE) -C aesm_service
	$(CP) aesm_service/config/aesmd_service/aesmd.conf $(BUILD_DIR)
	$(CP) aesm_service/config/aesmd_service/aesmd.service $(BUILD_DIR)

.PHONY: REF_LE
REF_LE:
	$(MAKE) -C ref_le

$(BUILD_DIR):
	@$(MKDIR) $@

.PHONY: clean
clean:
	$(MAKE) -C aesm_service/                   clean
	$(MAKE) -C ref_le                          clean

.PHONY: rebuild
rebuild:
	$(MAKE) clean
	$(MAKE) all


.PHONY: cmake_check_fail
cmake_check_fail:
	@echo ERROR: Please install cmake by command such as
	@echo apt-get install cmake
	@echo or
	@echo yum install cmake
	@echo with root account
	@exit 2

.PHONY: prebuilt_check_fail
prebuilt_check_fail:
	@echo "ERROR: Please run 'download_prebuilt.sh' to download the prebuilt AEs before compiling."
	@echo "Exiting......"
	@exit -2
