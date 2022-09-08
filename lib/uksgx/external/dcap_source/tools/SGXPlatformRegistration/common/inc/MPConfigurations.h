/*
 * Copyright (C) 2011-2021 Intel Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of Intel Corporation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
/**
 * File: common.h
 *  
 * Description: Definitions of the shared configuration data.
 */
#ifndef __MP_CONFIGURATIONS_H
#define __MP_CONFIGURATIONS_H

#include "MultiPackageDefs.h"

#define MP_REG_CONFIG_FILE  "/etc/"
#define LINUX_INSTALL_PATH  "/opt/intel/sgx-ra-service/"
#define MAX_RESPONSE_SIZE   1024*30
#define MAX_REQUEST_SIZE    1024*56
#define MAX_DATA_SIZE       1024*30
#define SUBSCRIPTION_KEY_SIZE   32

typedef struct _MPConfigurations{
    ProxyConf proxy;
    LogLevel log_level;
    char uefi_path[MAX_PATH_SIZE];
    char server_add_package_subscription_key[SUBSCRIPTION_KEY_SIZE+1];
    char installation_path[MAX_PATH_SIZE];
} MPConfigurations;

#endif  // #ifndef __MP_CONFIGURATION_H
