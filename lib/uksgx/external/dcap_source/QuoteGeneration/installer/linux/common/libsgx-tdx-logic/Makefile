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

include installConfig

PACKAGE_ROOT_FOLDER=pkgroot
PACKAGES=$(notdir $(wildcard $(PACKAGE_ROOT_FOLDER)/*))

USR_LIB_PATH=/usr/$(notdir $(shell gcc -print-multi-os-directory))/$(shell dpkg-architecture -qDEB_HOST_MULTIARCH 2> /dev/null)
USR_INC_PATH=/usr/$(INC_DIR)
USR_LIB_VER=1.0.0
SPLIT_VERSION=$(word $2,$(subst ., ,$1))

default:

install: $(PACKAGES)


$(PACKAGES):
	install -d $(shell readlink -m $(DESTDIR)/$@)
	$(if $(wildcard $(PACKAGE_ROOT_FOLDER)/$@/$(LIB_DIR)/*.so), \
		install -d $(shell readlink -m $(DESTDIR)/$@/$(USR_LIB_PATH)) && \
		mv $(PACKAGE_ROOT_FOLDER)/$@/$(LIB_DIR)/*.so $(DESTDIR)/$@/$(USR_LIB_PATH) && \
		cd $(DESTDIR)/$@/$(USR_LIB_PATH) && \
		mv libsgx_tdx_logic.so libsgx_tdx_logic.so.$(USR_LIB_VER) && \
		ln -fs libsgx_tdx_logic.so.$(USR_LIB_VER) libsgx_tdx_logic.so.$(call SPLIT_VERSION,$(USR_LIB_VER),1) && \
		ln -fs libsgx_tdx_logic.so.$(call SPLIT_VERSION,$(USR_LIB_VER),1) libsgx_tdx_logic.so)
	$(if $(wildcard $(PACKAGE_ROOT_FOLDER)/$@/$(INC_DIR)/.*), \
		install -d $(shell readlink -m $(DESTDIR)/$@/$(USR_INC_PATH)) && \
		install -d $(shell readlink -m $(DESTDIR)/$@/$(USR_LIB_PATH)) && \
		mv $(PACKAGE_ROOT_FOLDER)/$@/$(INC_DIR)/* $(DESTDIR)/$@/$(USR_INC_PATH))
