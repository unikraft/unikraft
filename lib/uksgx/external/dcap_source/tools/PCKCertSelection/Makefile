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

#
# main make file for PCK Cert Selection project
#

ifndef $(VERBOSE)
	VERBOSE:=@
endif

# this is project root directory 
ifndef PROJ_ROOT_DIR
	PROJ_ROOT_DIR		:= $(CURDIR)
endif
export PROJ_ROOT_DIR

# output binaries dir 
BIN_DIR		:= $(PROJ_ROOT_DIR)/out
export BIN_DIR

# PCK Cert Selection projects list
PROJECTS := 		PCKCertSelectionLib \
			PCKSelectionSample

ZIPFILE := PCKCertSelectionLinux.zip

# targets

.PHONY: all clean $(PROJECTS)

.NOTPARALLEL: $(PROJECTS) $(ZIPFILE)

all: $(PROJECTS) $(ZIPFILE)
	$(VERBOSE)echo projects all: done

debug:
	$(VERBOSE)$(MAKE) DEBUG=1 all

release:
	$(VERBOSE)$(MAKE) all

clean: $(PROJECTS)
	$(VERBOSE)rm -rf out/*.zip
	$(VERBOSE)echo projects clean: done

PCKSelectionSample : PCKCertSelectionLib

$(PROJECTS): $(BIN_DIR)
	$(VERBOSE)$(MAKE) -C $@ $(MAKECMDGOALS)
	$(VERBOSE)echo "$@ : done"
	$(VERBOSE)echo

$(BIN_DIR):
	$(VERBOSE)mkdir -p $@

$(ZIPFILE):
	bash pack.sh
	$(VERBOSE)echo "$@ : done"
	$(VERBOSE)echo

