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
# main make file for PCK Cert Selection sample project
#

ifndef $(VERBOSE)
	VERBOSE:=@
endif

######## Project Settings ########

# project root directory and output directory
# output dir must exist
ifndef PROJ_ROOT_DIR
	PROJ_ROOT_DIR		:= $(CURDIR)/..
endif
ifndef BIN_DIR
	BIN_DIR		:= $(PROJ_ROOT_DIR)/out
endif


######## App Settings ########

# source files 
APP_CPP_FILES		:= main.cpp 

# generate object files in local dir, also for parser files
APP_CPP_OBJECTS 	:= $(APP_CPP_FILES:.cpp=.o)

# include paths, local, parser and openssl
APP_INCLUDE_PATHS	:= -I. -I$(PROJ_ROOT_DIR)/include


# the application executable name
APP_NAME		:= PCKSelectionSample
LIB_NAME		:= PCKCertSelection


####### Build Flags ##############

# debug mode
DEBUG_FLAGS := -m64 -O0 -g

# release mode
RELEASE_FLAGS := -m64 -O2

# basic application c build flags
C_FLAGS	:= -DLINUX -fPIC -Werror $(APP_INCLUDE_PATHS) 

# link flags, link CPUSVNCompare library
LINK_FLAGS := -L$(BIN_DIR) -l$(LIB_NAME)

# debug/release switch
ifeq ($(DEBUG), 1)
        C_FLAGS += $(DEBUG_FLAGS)
        LINK_FLAGS += $(DEBUG_FLAGS)
else
        C_FLAGS += $(RELEASE_FLAGS)
        LINK_FLAGS += $(DEBUG_FLAGS)
endif

# c++ flags
CPP_FLAGS	:= $(C_FLAGS) -std=c++14


####### Build Targets ##############

.PHONY: all clean 

# default target
all: $(BIN_DIR) $(APP_NAME)

# local source files compiling
%.o: %.cpp 
	$(VERBOSE)echo "Compiling $<..."
	$(VERBOSE)$(CXX) $(CPP_FLAGS) -c $< -o $@
	$(VERBOSE)echo "\t -> $@ done"

# build application - link into output dir
$(APP_NAME): $(APP_CPP_OBJECTS)
	$(VERBOSE)echo "Building..."
	$(VERBOSE)$(CXX) $^ -o $(BIN_DIR)/$@ $(LINK_FLAGS)
	$(VERBOSE)echo "\t -> $@ done"

debug:
	$(VERBOSE)$(MAKE) DEBUG=1 all

release:
	$(VERBOSE)$(MAKE) all

clean:
	$(VERBOSE)echo -n "Clean $(APP_NAME)..."
	$(VERBOSE)rm -f $(BIN_DIR)/$(APP_NAME) $(APP_CPP_OBJECTS)
	$(VERBOSE)echo done

# make sure output dir exist
$(BIN_DIR):
	$(VERBOSE)mkdir -p $@

