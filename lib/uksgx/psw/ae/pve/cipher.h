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
 * File: cipher.h
 * Description: Header file to wrap cipher related function from IPP for Provision Enclave such as ecdsa signature verification, aes-gcm, EPID private key generation
 */

#ifndef _CIPHER_H
#define _CIPHER_H

#if defined(PROVISIONSERVERSIM_EXPORTS) || defined(PvEUT) 
#define OUTSIDE_ENCLAVE
#endif

#ifdef OUTSIDE_ENCLAVE  
#include "provision_sim.h"
#endif

#include "se_cdefs.h"
#include "sgx_tseal.h"
#include "epid/common/types.h"
#include "se_types.h"
#include "sgx_key.h"
#include "provision_msg.h"
#include "se_sig_rl.h"

/*function to generate random number of num_bits
     return PVEC_SUCCESS on success*/
pve_status_t pve_rng_generate(
    int num_bits,                /*bits of random info to be generated*/
    unsigned char* p_rand_data); /*buffer to hold output, the length of it should be at least (num_bits+7)/8*/

/*function defined inside pve_verify_signature.cpp to Verify ECDSA signature by EPID signing key
  return PVEC_SUCCESS if the signature verification passed
  return PVEC_MSG_ERROR if signature not matched
  return other error code for other kinds of error*/
pve_status_t verify_epid_ecdsa_signature(
    const uint8_t *p_sig_rl_sign,             /*The ecdsa signature of message to be verify, the size of it should be 2*ECDSA_SIGN_SIZE which contains two big integers in big endian*/
    const extended_epid_group_blob_t& xegb,
    const se_ae_ecdsa_hash_t *p_sig_rl_hash); /*The sha256 hash value of message to be verify*/

/*Function to verify the ECDSA signature of Binary EPID Group Public Cert*/
pve_status_t check_signature_of_group_pub_cert(const signed_epid_group_cert_t *group_cert, const uint8_t* epid_sk);

/** Generate epid private key.
*
* Parameters:
*   Return: sgx_status_t - SGX_SUCCESS or failure as defined in sgx_error.h
*   Output: f - Pointer to key.
*
*/
extern "C" sgx_status_t sgx_gen_epid_priv_f(void* f);

#endif
