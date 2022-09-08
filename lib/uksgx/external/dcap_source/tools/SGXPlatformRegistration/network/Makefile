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

CPP_SRCS := $(wildcard src/*.cpp src/c_wrapper/*.cpp src/3rdParty/base64.cpp)
TARGET_LIB := libmpa_network
include ../buildenv.mk
MPA_LINUX_DIR = ../linux
PREPARE_OPENSSL := $(MPA_LINUX_DIR)/prepare_openssl.sh
PREPARE_LIBCURL := $(MPA_LINUX_DIR)/prepare_libcurl.sh
OPENSSL_PATH := $(MPA_LINUX_DIR)/openssl_source/openssl-1.1.1b
LIBCURL_PATH := $(MPA_LINUX_DIR)/curl_source/curl-7.64.1/lib/.libs

#PREPRARE_CURL:
#	chmod 755 $(PREPARE_OPENSSL)
#	chmod 755 $(PREPARE_LIBCURL)
#	test -f $(OPENSSL_PATH)/libcrypto.a || $(PREPARE_OPENSSL)
#	test -f $(LIBCURL_PATH)/libcurl.a || $(PREPARE_LIBCURL)

#$(TARGET_LIB).a : $(CPP_OBJS) PREPRARE_CURL
#	$(MKDIR) .libs
#	$(RM) .libs/*
#	cd .libs && \
#	$(AR) x ../$(OPENSSL_PATH)/libcrypto.a && \
#	$(AR) x ../$(OPENSSL_PATH)/libssl.a && \
#	$(AR) x ../$(LIBCURL_PATH)/libcurl.a 
#	$(AR) rsD $(TARGET_LIB).a $(CPP_OBJS) .libs/*.o
#	$(RM) -r .libs 
