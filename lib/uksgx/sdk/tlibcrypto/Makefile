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

CPPFLAGS := -I$(COMMON_DIR)/inc/internal \
            -I$(COMMON_DIR)/inc          \
            -I$(COMMON_DIR)/inc/tlibc    \
	    -I$(SGX_IPP_INC)

CXXFLAGS += $(ENCLAVE_CXXFLAGS) -fno-exceptions -fno-rtti #-Werror

OBJ = init_tcrypto_lib.o sgx_aes_ctr.o sgx_rsa_encryption.o sgx_aes_gcm.o sgx_cmac128.o sgx_hmac.o sgx_ecc256.o sgx_ecc256_ecdsa.o sgx_sha384.o sgx_sha384_msg.o sgx_sha256.o sgx_sha1.o sgx_sha256_msg.o sgx_ecc256_internal.o sgx_rsa3072.o sgx_internal.o
SHARED_OBJ = tcrypto_version.o sgx_common_init_ipp.o

ifeq ($(USE_CRYPTO_LIB), 0)
# Build SGXSSL based sgx_tcrypto library
ifeq ($(ARCH), x86_64)
	OPENSSL_PACKAGE = $(LINUX_EXTERNAL_DIR)/sgxssl/Linux/package
else
	$(error SGXSSL doesn't support 32bit)
endif #($(ARCH), x86_64)

ifeq ($(MITIGATION-CVE-2020-0551), LOAD)
OPENSSL_LIBRARY_PATH := $(OPENSSL_PACKAGE)/lib64/cve_2020_0551_load
else ifeq ($(MITIGATION-CVE-2020-0551), CF)
OPENSSL_LIBRARY_PATH := $(OPENSSL_PACKAGE)/lib64/cve_2020_0551_cf
else
OPENSSL_LIBRARY_PATH := $(OPENSSL_PACKAGE)/lib64
endif

ifdef DEBUG
	OpenSSL_Crypto_Library_Name := sgx_tsgxssl_cryptod
	SGXSSL_Library_Name := sgx_tsgxssld
	SGX_COMMON_CFLAGS += -O0
else
	OpenSSL_Crypto_Library_Name := sgx_tsgxssl_crypto
	SGXSSL_Library_Name := sgx_tsgxssl
endif

PREPARE_SGXSSL := $(LINUX_EXTERNAL_DIR)/sgxssl/prepare_sgxssl.sh
PREPARE_SGX_SSL:
	chmod 755 $(PREPARE_SGXSSL)
	test -f $(OPENSSL_LIBRARY_PATH)/lib$(SGXSSL_Library_Name).a || MITIGATION=$(MITIGATION-CVE-2020-0551) $(PREPARE_SGXSSL)

CPPFLAGS += -I$(OPENSSL_PACKAGE)/include -fPIC
CXXFLAGS += -DUSE_SGXSSL
SRCDIR := sgxssl

LIB_NAME := libsgx_tcrypto_sgxssl.a

else
# Build IPP crypto based sgx_tcrypto library
CFLAGS += $(CPPFLAGS) $(ENCLAVE_CFLAGS) $(SGX_COMMON_CFLAGS) -fno-exceptions -fPIC #-Werror

OBJ += sgx_tcrypto_common.o sgx_rsa_internal.o
SRCDIR := ipp

LIB_NAME := libsgx_tcrypto_ipp.a

endif #($(USE_CRYPTO_LIB), 0)

C_Files := $(wildcard ipp/ipp_disp/*.c)
C_OBJS  := $(C_Files:.c=.o)
C_OBJS  := $(sort $(C_OBJS))

DISP_LIB_NAME := libsgx_disp.a

OBJ := $(addprefix $(SRCDIR)/, $(OBJ))
LIB_NAME := $(addprefix $(SRCDIR)/, $(LIB_NAME))
TARGET := libsgx_tcrypto.a

ifeq ($(USE_CRYPTO_LIB), 0)

$(TARGET): PREPARE_SGX_SSL
	$(MAKE) $(OBJ) 
	$(MAKE) $(SHARED_OBJ)
	$(MKDIR) $(BUILD_DIR)/.libs
	$(RM) $(BUILD_DIR)/.libs/*
	cd $(BUILD_DIR)/.libs && \
	$(AR) x $(OPENSSL_LIBRARY_PATH)/lib$(OpenSSL_Crypto_Library_Name).a && \
	$(AR) x $(OPENSSL_LIBRARY_PATH)/lib$(SGXSSL_Library_Name).a 
	$(AR) rsD $(LIB_NAME) $(OBJ) $(SHARED_OBJ) $(BUILD_DIR)/.libs/*.o
	$(CP) $(LIB_NAME) $@
	$(RM) -r $(BUILD_DIR)/.libs 
else

$(TARGET): $(OBJ) $(SHARED_OBJ) $(C_OBJS)
	$(CP) $(IPP_LIBS_DIR)/libippcp.a $(LIB_NAME)
	$(AR) rsD $(LIB_NAME) $(OBJ) $(SHARED_OBJ) $(C_OBJS)
	$(CP) $(LIB_NAME) $@

endif #($(USE_CRYPTO_LIB), 0)

$(DISP_LIB_NAME): $(C_OBJS)
	$(AR) rsD $(DISP_LIB_NAME) $(C_OBJS)

$(SRCDIR)/%.o: $(SRCDIR)/%.cpp
	$(CXX) -c $(CXXFLAGS) $(CPPFLAGS) $< -o $@

$(SHARED_OBJ): %.o: %.cpp
	$(CXX) -c $(CXXFLAGS) $(CPPFLAGS) $< -o $@

$(BUILD_DIR):
	@$(MKDIR) $@

.PHONY: all
all: $(TARGET) $(DISP_LIB_NAME) | $(BUILD_DIR)
	@$(CP) $^ $|

.PHONY: clean
clean:
	@$(RM) *.o ipp/*.o ipp/ipp_disp/*.o sgxssl/*.o $(TARGET) $(BUILD_DIR)/$(TARGET) $(LIB_NAME) $(DISP_LIB_NAME) $(BUILD_DIR)/$(DISP_LIB_NAME)

.PHONY: rebuild
rebuild:
	$(MAKE) clean
	$(MAKE) all