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

class Constants {}

function define(name, value) {
  Object.defineProperty(Constants, name, {
    value: value,
    enumerable: true,
  });
}

define('DB_VERSION', 2);

define('PLATF_REG_NEW', 0);
define('PLATF_REG_NOT_AVAILABLE', 1);
define('PLATF_REG_DELETED', 9);

define('HTTP_SUCCESS', 200);

//
define('SGX_TCBM', 'SGX-TCBm');
define('SGX_FMSPC', 'SGX-FMSPC');
define('SGX_PCK_CERTIFICATE_CA_TYPE', 'SGX-PCK-Certificate-CA-Type');

// Certificate IDs
define('PROCESSOR_ROOT_CERT_ID', 1);
define('PROCESSOR_INTERMEDIATE_CERT_ID', 2);
define('PROCESSOR_SIGNING_CERT_ID', 3);
define('PLATFORM_INTERMEDIATE_CERT_ID', 4);

// Product Type : SGX or TDX
define('PROD_TYPE_SGX', 0);
define('PROD_TYPE_TDX', 1);

// Enclave Identity IDs
define('QE_IDENTITY_ID', 1);
define('QVE_IDENTITY_ID', 2);
define('TDQE_IDENTITY_ID', 3);

//CAs
define('CA_PROCESSOR', 'PROCESSOR');
define('CA_PLATFORM', 'PLATFORM');

//Certchain names
define('SGX_PCK_CERTIFICATE_ISSUER_CHAIN', 'SGX-PCK-Certificate-Issuer-Chain');
define('TCB_INFO_ISSUER_CHAIN', 'TCB-Info-Issuer-Chain');
define('SGX_TCB_INFO_ISSUER_CHAIN', 'SGX-TCB-Info-Issuer-Chain');
define('SGX_ENCLAVE_IDENTITY_ISSUER_CHAIN', 'SGX-Enclave-Identity-Issuer-Chain');
define('SGX_PCK_CRL_ISSUER_CHAIN', 'SGX-PCK-CRL-Issuer-Chain');

export default Constants;
