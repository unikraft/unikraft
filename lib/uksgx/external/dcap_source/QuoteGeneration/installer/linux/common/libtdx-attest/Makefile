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
OPT_SAMPLE_PATH=/opt/intel/tdx-quote-generation-sample

USR_LIB_VER=1.0.0
SPLIT_VERSION=$(word $2,$(subst ., ,$1))

default:

install: $(PACKAGES)
	cd $(shell readlink -m $(DESTDIR)/$(TDX_ATTEST_PACKAGE_NAME)/$(USR_LIB_PATH)) && \
	mv libtdx_attest.so libtdx_attest.so.$(USR_LIB_VER) && \
	ln -fs libtdx_attest.so.$(USR_LIB_VER) libtdx_attest.so.$(call SPLIT_VERSION,$(USR_LIB_VER),1)

$(PACKAGES):
	install -d $(shell readlink -m $(DESTDIR)/$@)
	$(if $(wildcard $(PACKAGE_ROOT_FOLDER)/$@/$(LIB_DIR)/*.so), \
		install -d $(shell readlink -m $(DESTDIR)/$@/$(USR_LIB_PATH)) && \
		mv $(PACKAGE_ROOT_FOLDER)/$@/$(LIB_DIR)/*.so $(DESTDIR)/$@/$(USR_LIB_PATH))
	$(if $(wildcard $(PACKAGE_ROOT_FOLDER)/$@/$(INC_DIR)/.*), \
		install -d $(shell readlink -m $(DESTDIR)/$@/$(USR_INC_PATH)) && \
		install -d $(shell readlink -m $(DESTDIR)/$@/$(USR_LIB_PATH)) && \
		mv $(PACKAGE_ROOT_FOLDER)/$@/$(INC_DIR)/* $(DESTDIR)/$@/$(USR_INC_PATH))
	$(if $(wildcard $(PACKAGE_ROOT_FOLDER)/$@/$(SAMPLE_DIR)/.*), \
		install -d $(shell readlink -m $(DESTDIR)/$@/$(OPT_SAMPLE_PATH)) && \
		mv $(PACKAGE_ROOT_FOLDER)/$@/$(SAMPLE_DIR)/* $(DESTDIR)/$@/$(OPT_SAMPLE_PATH))
