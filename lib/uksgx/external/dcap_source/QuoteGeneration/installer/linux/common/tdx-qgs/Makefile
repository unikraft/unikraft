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

QGSD_CONF_NAME=$(if $(wildcard /run/systemd/system/.*),qgsd.service,$(if $(wildcard /etc/init/.*),qgsd.conf,))
QGSD_CONF_DEL=$(if $(wildcard /run/systemd/system/.*),qgsd.conf,$(if $(wildcard /etc/init/.*),qgsd.service,))
QGSD_CONF_PATH=$(if $(wildcard /run/systemd/system/.*),$(if $(wildcard /lib/systemd/system/.*),/lib/systemd/system,/usr/lib/systemd/system),$(if $(wildcard /etc/init/.*),/etc/init/))

ifeq ($(QGSD_CONF_NAME),)
ifneq ($(shell awk -F/ '$$2 == "docker"' /proc/self/cgroup),)
QGSD_CONF_NAME=qgsd.service
QGSD_CONF_DEL=qgsd.conf
QGSD_CONF_PATH=/lib/systemd/system
$(warning "You may need to start qgsd manually after it's installed!")
else
$(error "Unsupported platform - neither systemctl nor initctl is found!")
endif
endif

default:

install: $(PACKAGES)
	install -d $(shell readlink -m $(DESTDIR)/$(QGSD_CONF_PATH))
	sed -e "s:@qgs_folder@:$(TDX_QGS_PACKAGE_PATH)/$(TDX_QGS_PACKAGE_NAME):" \
		$(DESTDIR)/$(TDX_QGS_PACKAGE_PATH)/$(TDX_QGS_PACKAGE_NAME)/$(QGSD_CONF_NAME) \
		> $(DESTDIR)/$(QGSD_CONF_PATH)/$(QGSD_CONF_NAME)
	rm -f $(DESTDIR)/$(TDX_QGS_PACKAGE_PATH)/$(TDX_QGS_PACKAGE_NAME)/$(QGSD_CONF_NAME)
	rm -f $(DESTDIR)/$(TDX_QGS_PACKAGE_PATH)/$(TDX_QGS_PACKAGE_NAME)/$(QGSD_CONF_DEL)
	$(if $(shell grep '^ID=clear-linux-os' /usr/lib/os-release 2> /dev/null), \
		$(if $(wildcard $(DESTDIR)/$(TDX_QGS_PACKAGE_PATH)/$(TDX_QGS_PACKAGE_NAME)/conf/qgsd.conf), \
			mv $(DESTDIR)/$(TDX_QGS_PACKAGE_PATH)/$(TDX_QGS_PACKAGE_NAME)/conf/qgsd.conf \
				$(DESTDIR)/$(TDX_QGS_PACKAGE_PATH)/$(TDX_QGS_PACKAGE_NAME)/conf/qgsd.conf.sample), \
		$(if $(wildcard $(DESTDIR)/$(TDX_QGS_PACKAGE_PATH)/$(TDX_QGS_PACKAGE_NAME)/conf/.*), \
			install -d $(shell readlink -m $(DESTDIR)/$(ETC_DIR)) && \
			cp -fr $(DESTDIR)/$(TDX_QGS_PACKAGE_PATH)/$(TDX_QGS_PACKAGE_NAME)/conf/* \
				$(DESTDIR)/$(ETC_DIR) && \
			rm -fr $(DESTDIR)/$(TDX_QGS_PACKAGE_PATH)/$(TDX_QGS_PACKAGE_NAME)/conf))
	
$(PACKAGES):
	install -d $(shell readlink -m $(DESTDIR)/$(TDX_QGS_PACKAGE_PATH)/$(TDX_QGS_PACKAGE_NAME))
	cp -r $(PACKAGE_ROOT_FOLDER)/$@/* $(DESTDIR)/$(TDX_QGS_PACKAGE_PATH)/$(TDX_QGS_PACKAGE_NAME)
