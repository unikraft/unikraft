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

# -----------------------------------------------------------------------------
# Function : root-dir
# Arguments: 1: path
# Returns  : Parent dir or path of $1, with final separator removed.
# -----------------------------------------------------------------------------
root-dir = $(patsubst %/,%,$(dir $(1:%/=%)))

# -----------------------------------------------------------------------------
# Macro    : cur-dir
# Returns  : the directory of the current Makefile
# Usage    : $(cur-dir)
# -----------------------------------------------------------------------------
cur-dir = $(call root-dir,$(lastword $(MAKEFILE_LIST)))


CUR_DIR              := $(call cur-dir)


include $(CUR_DIR)/../QuoteGeneration/buildenv.mk

MODE					?= HW
DEBUG					?= 0
DCAP_QG_DIR				:= $(ROOT_DIR)
DCAP_QV_DIR				:= $(DCAP_QG_DIR)/../QuoteVerification
QVL_SRC_PATH 			?= $(DCAP_QV_DIR)/QVL/Src
SGXSSL_PACKAGE_PATH 	?= $(DCAP_QV_DIR)/sgxssl/Linux/package
PREBUILD_OPENSSL_PATH	?= $(DCAP_QV_DIR)/../prebuilt/openssl

SGX_COMMON_CFLAGS := $(COMMON_FLAGS) -m64 -Wjump-misses-init -Wstrict-prototypes -Wunsuffixed-float-constants
SGX_COMMON_CXXFLAGS := $(COMMON_FLAGS) -m64 -Wnon-virtual-dtor -std=c++14


QVL_LIB_PATH := $(QVL_SRC_PATH)/AttestationLibrary
QVL_PARSER_PATH := $(QVL_SRC_PATH)/AttestationParsers
QVL_COMMON_PATH := $(QVL_SRC_PATH)/AttestationCommons

COMMON_INCLUDE := -I$(SGX_SDK)/include -I$(SGX_SDK)/include/tlibc -I$(SGX_SDK)/include/libcxx -I$(SGXSSL_PACKAGE_PATH)/include

QVL_LIB_INC := -I$(QVL_COMMON_PATH)/include -I$(QVL_COMMON_PATH)/include/Utils -I$(QVL_LIB_PATH)/include -I$(QVL_LIB_PATH)/src -I$(QVL_PARSER_PATH)/include -I$(QVL_SRC_PATH)/ThirdParty/rapidjson/include

QVL_PARSER_INC := -I$(QVL_COMMON_PATH)/include -I$(QVL_COMMON_PATH)/include/Utils -I$(QVL_SRC_PATH) -I$(QVL_PARSER_PATH)/include -I$(QVL_PARSER_PATH)/src -I$(QVL_LIB_PATH)/include -I$(QVL_SRC_PATH)/ThirdParty/rapidjson/include

QVL_LIB_FILES := $(sort $(wildcard $(QVL_LIB_PATH)/src/*.cpp) $(wildcard $(QVL_LIB_PATH)/src/*/*.cpp) $(wildcard $(QVL_COMMON_PATH)/src/Utils/*.cpp))
QVL_PARSER_FILES := $(sort $(wildcard $(QVL_PARSER_PATH)/src/*.cpp) $(wildcard $(QVL_PARSER_PATH)/src/*/*.cpp))
