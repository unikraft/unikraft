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

VAR_OPT_PATH=/var/opt/sgxra
USR_LIB_PATH=/usr/$(notdir $(shell gcc -print-multi-os-directory))/$(shell dpkg-architecture -qDEB_HOST_MULTIARCH 2> /dev/null)

RAD_CONF_NAME=$(if $(wildcard /run/systemd/system/.),mpa_registration_tool.service,$(if $(wildcard /etc/init/.),mpa_registration_tool.conf,))
RAD_CONF_DEL=$(if $(wildcard /run/systemd/system/.),mpa_registration_tool.conf,$(if $(wildcard /etc/init/.),mpa_registration_tool.service,))
RAD_CONF_PATH=$(if $(wildcard /run/systemd/system/.),$(if $(wildcard /lib/systemd/system/.),/lib/systemd/system,/usr/lib/systemd/system),$(if $(wildcard /etc/init/.),/etc/init/))

ifeq ($(RAD_CONF_NAME),)
ifneq ($(shell awk -F/ '$$2 == "docker"' /proc/self/cgroup),)
RAD_CONF_NAME=mpa_registration_tool.service
RAD_CONF_DEL=mpa_registration.conf
RAD_CONF_PATH=/lib/systemd/system
else
$(error "Unsupported platform - neither systemctl nor initctl is found!")
endif
endif

	#install -d $(shell readlink -m $(DESTDIR)/$(SGX_RA_SERVICE_PACKAGE_NAME)/$(RAD_CONF_PATH))
default:

install: $(PACKAGES)
	install -d $(shell readlink -m $(DESTDIR)/$(RAD_CONF_PATH))
	sed -e "s:@mpa_folder@:$(SGX_RA_SERVICE_PACKAGE_PATH)/$(SGX_RA_SERVICE_PACKAGE_NAME)/:" \
		 $(DESTDIR)/$(SGX_RA_SERVICE_PACKAGE_PATH)/$(SGX_RA_SERVICE_PACKAGE_NAME)/$(RAD_CONF_NAME) \
		 > $(DESTDIR)/$(RAD_CONF_PATH)/$(RAD_CONF_NAME)
	rm -f $(DESTDIR)/$(SGX_RA_SERVICE_PACKAGE_PATH)/$(SGX_RA_SERVICE_PACKAGE_NAME)/$(RAD_CONF_NAME)
	rm -f $(DESTDIR)/$(SGX_RA_SERVICE_PACKAGE_PATH)/$(SGX_RA_SERVICE_PACKAGE_NAME)/$(RAD_CONF_DEL)
	$(if $(wildcard $(DESTDIR)/$(SGX_RA_SERVICE_PACKAGE_PATH)/$(SGX_RA_SERVICE_PACKAGE_NAME)/conf/*), \
		install -d $(shell readlink -m $(DESTDIR)/$(ETC_DIR)) && \
		cp -fr $(DESTDIR)/$(SGX_RA_SERVICE_PACKAGE_PATH)/$(SGX_RA_SERVICE_PACKAGE_NAME)/conf/* \
                       $(DESTDIR)/$(ETC_DIR) && \
		rm -fr $(DESTDIR)/$(SGX_RA_SERVICE_PACKAGE_PATH)/$(SGX_RA_SERVICE_PACKAGE_NAME)/conf)

$(PACKAGES):
#	install -d $(shell readlink -m $(DESTDIR)/$@)
	$(if $(wildcard $(PACKAGE_ROOT_FOLDER)/$@/$(LIB_DIR)/*), \
                install -d $(shell readlink -m $(DESTDIR)/$@/$(USR_LIB_PATH)) && \
                install $(PACKAGE_ROOT_FOLDER)/$@/$(LIB_DIR)/* $(DESTDIR)/$@/$(USR_LIB_PATH), \
                install -d $(shell readlink -m $(DESTDIR)/$(SGX_RA_SERVICE_PACKAGE_PATH)/$(SGX_RA_SERVICE_PACKAGE_NAME)) && \
                cp -r $(PACKAGE_ROOT_FOLDER)/$@/* $(DESTDIR)/$(SGX_RA_SERVICE_PACKAGE_PATH)/$(SGX_RA_SERVICE_PACKAGE_NAME))
                                                                      
