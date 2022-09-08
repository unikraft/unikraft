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

TOP_DIR  = ../../..
include $(TOP_DIR)/buildenv.mk

LAUNCH_VERSION:= $(shell awk '$$2 ~ /LAUNCH_VERSION/ { print substr($$3, 2, length($$3) - 2); }' $(COMMON_DIR)/inc/internal/se_version.h)
EPID_VERSION:= $(shell awk '$$2 ~ /EPID_VERSION/ { print substr($$3, 2, length($$3) - 2); }' $(COMMON_DIR)/inc/internal/se_version.h)
QUOTE_EX_VERSION:= $(shell awk '$$2 ~ /QUOTE_EX_VERSION/ { print substr($$3, 2, length($$3) - 2); }' $(COMMON_DIR)/inc/internal/se_version.h)

IPC_COMMON_DIR        := $(TOP_DIR)/psw/ae/aesm_service/source/core/ipc
IPC_COMMON_SRC_DIR    := $(IPC_COMMON_DIR)
IPC_COMMON_INC_DIR    := $(IPC_COMMON_DIR)
IPC_COMMON_PROTO_DIR  := $(IPC_COMMON_DIR)
UAE_WRAPPER_DIR        := ../uae_wrapper
UAE_SRC_DIR    := $(UAE_WRAPPER_DIR)/src
UAE_INC_DIR    := $(UAE_WRAPPER_DIR)/inc
AE_COMMON_DIR    := $(LINUX_PSW_DIR)/ae/common 

INCLUDE += -I.
INCLUDE += -I$(COMMON_DIR)              \
           -I$(COMMON_DIR)/inc          \
           -I$(COMMON_DIR)/inc/internal \

INCLUDE += -I$(LINUX_PSW_DIR)/ae/common         \
           -I$(LINUX_PSW_DIR)/ae/inc            \
           -I$(LINUX_PSW_DIR)/ae/inc/internal   \
           -I$(SGX_HEADER_DIR)

INCLUDE += -I$(LINUX_EXTERNAL_DIR)/epid-sdk \
           -I$(IPC_COMMON_INC_DIR) \
           -I$(UAE_INC_DIR) \
           -I$(IPC_COMMON_PROTO_DIR) \
           -I$(LINUX_PSW_DIR)/ae/aesm_service/source \
           -I$(LINUX_PSW_DIR)/ae/aesm_service/source/common

CXXFLAGS += -fPIC -Werror -Wno-unused-parameter -g -DPROTOBUF_INLINE_NOT_IN_HEADERS=0

EXTERNAL_LIB += -lprotobuf 

vpath %.cpp .. $(COMMON_DIR)/src  $(IPC_COMMON_SRC_DIR) $(IPC_COMMON_PROTO_DIR) $(UAE_SRC_DIR) $(AE_COMMON_DIR)
vpath %.c   $(COMMON_DIR)/src

C_SRC   := se_trace.c

IPC_SRC  :=  AEGetQuoteResponse.cpp \
             AEInitQuoteRequest.cpp \
             AEInitQuoteResponse.cpp \
             AEReportAttestationRequest.cpp \
             AEReportAttestationResponse.cpp \
             AECheckUpdateStatusRequest.cpp \
             AECheckUpdateStatusResponse.cpp \
             ProtobufSerializer.cpp \
             AEGetLaunchTokenRequest.cpp  \
             AEGetWhiteListSizeRequest.cpp \
             AEGetWhiteListSizeResponse.cpp \
             AEGetWhiteListRequest.cpp \
             AEGetWhiteListResponse.cpp \
             AESGXGetExtendedEpidGroupIdRequest.cpp \
             AESGXGetExtendedEpidGroupIdResponse.cpp \
             AESGXSwitchExtendedEpidGroupRequest.cpp \
             AESGXSwitchExtendedEpidGroupResponse.cpp \
             AESGXRegisterRequest.cpp \
             AESGXRegisterResponse.cpp \
             SocketTransporter.cpp \
             AEGetLaunchTokenResponse.cpp  \
             UnixCommunicationSocket.cpp \
             AEGetQuoteRequest.cpp \
             UnixSocketFactory.cpp \
             NonBlockingUnixCommunicationSocket.cpp \
             NonBlockingUnixSocketFactory.cpp \
             AESelectAttKeyIDRequest.cpp \
             AESelectAttKeyIDResponse.cpp \
             AEInitQuoteExRequest.cpp \
             AEInitQuoteExResponse.cpp \
             AEGetQuoteExRequest.cpp \
             AEGetQuoteExResponse.cpp \
             AEGetQuoteSizeExRequest.cpp \
             AEGetQuoteSizeExResponse.cpp \
             AEGetSupportedAttKeyIDNumRequest.cpp \
             AEGetSupportedAttKeyIDNumResponse.cpp \
             AEGetSupportedAttKeyIDsRequest.cpp \
             AEGetSupportedAttKeyIDsResponse.cpp

PROTOBUF_SRC  :=  messages.pb.cc

SRC :=       AEServicesImpl.cpp \
             AEServicesProvider.cpp \
             uae_api.cpp \
             se_sig_rl.cpp \
             sgx_uae_service.cpp \
             uae_service_assert.cpp 

LEGACY_SRC := legacy_uae_service.cpp \
              uae_service_version.cpp

OBJ := $(C_SRC:.c=.o) $(SRC:.cpp=.o) $(IPC_SRC:.cpp=.o)  $(PROTOBUF_SRC:.cc=.o) 

LEGACY_OBJ := $(LEGACY_SRC:.cpp=.o)

LDUFLAGS:= -pthread $(COMMON_LDFLAGS)

LIBNAME  := libsgx_epid.so libsgx_launch.so libsgx_quote_ex.so
ifndef DEBUG
LIBNAME_DEBUG  =  $(LIBNAME:.so=.so.debug)
endif

LEGACY_LIBNAME  = libsgx_uae_service.so
LEGACY_LIBNAME_DEBUG  = libsgx_uae_service.so.debug

.PHONY: all
all:  install_lib

.PHONY: install_lib
install_lib: $(LIBNAME) $(LIBNAME_DEBUG) $(LEGACY_LIBNAME) $(LEGACY_LIBNAME_DEBUG) | $(BUILD_DIR)
	@$(foreach lib,$(LIBNAME),$(CP) $(lib) $|;)
	@$(CP) $(LEGACY_LIBNAME) $|
ifndef DEBUG
	@$(foreach lib,$(LIBNAME_DEBUG),$(CP) $(lib) $|;)
	@$(CP) $(LEGACY_LIBNAME_DEBUG) $|
endif

libsgx_%.so: $(OBJ) %_version.o
	$(CXX) $(CXXFLAGS) $^ -shared $(LDUFLAGS) -Wl,--version-script=$(@:.so=.lds) -Wl,--gc-sections $(EXTERNAL_LIB) -Wl,-soname=$@.$(call SPLIT_VERSION,$($(shell echo $(@:libsgx_%.so=%_version)|tr a-z A-Z)),1) -o $@
%.so.debug: %.so
ifndef DEBUG
	((test -f $@) ||( \
	$(CP) $< $<.orig &&\
	$(OBJCOPY) --only-keep-debug $< $@ &&\
	$(STRIP) -g $< &&\
	$(OBJCOPY) --add-gnu-debuglink=$@ $< ))
endif

$(LEGACY_LIBNAME): $(LEGACY_OBJ)
	$(CXX) $(CXXFLAGS) $^ -shared $(LDUFLAGS) -ldl -Wl,--version-script=uae_service.lds -Wl,--gc-sections -Wl,-soname=$@ -o $@
$(LEGACY_LIBNAME_DEBUG): $(LEGACY_LIBNAME)
	((test -f $(LEGACY_LIBNAME_DEBUG)) || $(MAKE) separate_debug_info)

.PHONY: separate_debug_info
separate_debug_info:
ifndef DEBUG
	$(CP) $(LEGACY_LIBNAME) $(LEGACY_LIBNAME).orig
	$(OBJCOPY) --only-keep-debug $(LEGACY_LIBNAME) $(LEGACY_LIBNAME_DEBUG)
	$(STRIP) -g $(LEGACY_LIBNAME)
	$(OBJCOPY) --add-gnu-debuglink=$(LEGACY_LIBNAME_DEBUG) $(LEGACY_LIBNAME)
endif

$(IPC_SRC:.cpp=.o) : $(IPC_COMMON_PROTO_DIR)/messages.pb.cc
AEServicesImpl.o : $(IPC_COMMON_PROTO_DIR)/messages.pb.cc

messages.pb.o : $(IPC_COMMON_PROTO_DIR)/messages.pb.cc
	$(CXX) $(filter-out -Wshadow -Wredundant-decls, $(CXXFLAGS)) -Wno-array-bounds -Wno-conversion -c $< -o $@
%.o :%.cpp
	$(CXX) $(CXXFLAGS) -Wno-deprecated-declarations $(INCLUDE) -c $< -o $@
%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDE) -Werror -fPIC -c $< -o $@

$(BUILD_DIR):
	@$(MKDIR) $@

$(IPC_COMMON_PROTO_DIR)/messages.pb.cc: $(IPC_COMMON_PROTO_DIR)/messages.proto
	$(MAKE) -C $(IPC_COMMON_PROTO_DIR)

.PHONY: clean
clean:
	$(MAKE) -C $(IPC_COMMON_PROTO_DIR) clean
	@$(RM) $(OBJ) $(LEGACY_OBJ) *.orig
	@$(RM) $(LIBNAME) $(addprefix $(BUILD_DIR)/,$(LIBNAME))
	@$(RM) $(LIBNAME_DEBUG) $(addprefix $(BUILD_DIR)/,$(LIBNAME_DEBUG))
	@$(RM) $(LEGACY_LIBNAME) $(BUILD_DIR)/$(LEGACY_LIBNAME)
	@$(RM) $(LEGACY_LIBNAME_DEBUG) $(BUILD_DIR)/$(LEGACY_LIBNAME_DEBUG)

.PHONY: rebuild
rebuild:
	$(MAKE) clean
	$(MAKE) all
