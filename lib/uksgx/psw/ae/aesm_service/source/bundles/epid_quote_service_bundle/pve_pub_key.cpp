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

#include "epid_pve_type.h"
#include "ssl_crypto.h"
#include "sgx_error.h"

static void get_provision_server_rsa_key_little_endian_order(const signed_pek_t& pek, signed_pek_t& little_endian_key)
{
    uint32_t i;
    for(i=0;i<sizeof(pek.n);i++){
        little_endian_key.n[i] = pek.n[sizeof(pek.n)-1-i];
    }
    for(i=0;i<sizeof(pek.e);i++){
        little_endian_key.e[i] = pek.e[sizeof(pek.e)-1-i];
    }
}

//Function to get the rsa public key of backend server
//The output rsa_pub_key should be released by function sgx_free_rsa_key
sgx_status_t get_provision_server_rsa_pub_key(const signed_pek_t& pek, void **rsa_pub_key)
{
    signed_pek_t little_endian_key;
    get_provision_server_rsa_key_little_endian_order(pek, little_endian_key);
    return sgx_create_rsa_pub1_key(RSA_3072_KEY_BYTES,
                       sizeof(little_endian_key.e),
                       reinterpret_cast<const unsigned char *>(little_endian_key.n),
                       reinterpret_cast<const unsigned char *>(little_endian_key.e),
                       rsa_pub_key);
}
