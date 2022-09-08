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

#ifndef _ES_INFO_H_
#define _ES_INFO_H_

#define AESM_DATA_SERVER_URL_INFOS          'A'
#define AESM_DATA_ENDPOINT_SELECTION_INFOS  'B'
#define AESM_DATA_SERVER_URL_VERSION_1       1
#define AESM_DATA_SERVER_URL_VERSION         2
#define AESM_DATA_ENDPOINT_SELECTION_VERSION 1

#include "se_stdio.h"
#include "epid_pve_type.h"
#pragma pack(1)

/*Struct for PCE based url information which will be installed by PSW Installer*/
typedef struct _aesm_server_url_infos_t{
    uint8_t aesm_data_type;
    uint8_t aesm_data_version;
    char endpoint_url[MAX_PATH]; /*URL for endpoint selection protocol server*/
    char pse_rl_url[MAX_PATH];   /*URL to retrieve PSE rovocation List*/
    char pse_ocsp_url[MAX_PATH]; 
}aesm_server_url_infos_t;

/*Struct for data to save endpoint selection protocol result into persistent data storage*/
typedef struct _endpoint_selection_infos_t{
    uint8_t      aesm_data_type;
    uint8_t      aesm_data_version;
    signed_pek_t pek;
    char         provision_url[MAX_PATH];
}endpoint_selection_infos_t;
#pragma pack()

#endif /*_ES_INFO_H_*/
