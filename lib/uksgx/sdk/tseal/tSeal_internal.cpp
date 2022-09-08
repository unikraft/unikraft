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



#include <sgx_secure_align.h>
#include <sgx_random_buffers.h>
#include "sgx_tseal.h"
#include "sgx_lfence.h"
#include "tSeal_internal.h"
#include "sgx_utils.h"
#include <string.h>


// sgx_seal_data_iv
//
// Parameters:
//      additional_MACtext_length - [IN] length of the plaintext data stream in bytes
//      p_additional_MACtext - [IN] pointer to the plaintext data stream to be GCM protected
//      text2encrypt_length - [IN] length of the data stream to encrypt in bytes
//      p_text2encrypt - [IN] pointer to data stream to encrypt
//      p_payload_iv - [IN] Pointer to Initialization Vector to be used during AES GCM encryption
//      p_key_request - [IN] Pointer to the key request structure to be utilized to obtain the SEAL key
//      p_sealed_data - [OUT] pointer to the sealed data structure containing protected data
//
// Return Value:
//      sgx_status_t - SGX Error code
sgx_status_t sgx_seal_data_iv(const uint32_t additional_MACtext_length,
    const uint8_t *p_additional_MACtext, const uint32_t text2encrypt_length,
    const uint8_t *p_text2encrypt, const uint8_t *p_payload_iv,
    const sgx_key_request_t* p_key_request, sgx_sealed_data_t *p_sealed_data)
{
    sgx_status_t err = SGX_ERROR_UNEXPECTED;

    // Parameter checking performed in sgx_seal_data

    // Generate the seal key
    //randomly_placed_buffer<sgx_key_128bit_t, sizeof(sgx_key_128bit_t), 0x200> seal_key_buffer{};
    //auto *seal_key = seal_key_buffer.randomize_object();
    using cseal_key200 = randomly_placed_object<sgx::custom_alignment_aligned<sgx_key_128bit_t, sizeof(sgx_key_128bit_t), 0, sizeof(sgx_key_128bit_t)>, 0x200>;
    cseal_key200 oseal_key_buf;
    auto* oseal_key = oseal_key_buf.instantiate_object();
    auto* seal_key = &oseal_key->v;
    err = sgx_get_key(p_key_request, seal_key);
    if (err != SGX_SUCCESS)
    {
        // Clear temp state

        if (err != SGX_ERROR_OUT_OF_MEMORY)
            err = SGX_ERROR_UNEXPECTED;
        return err;
    }

    // Encrypt the content with the random seal key and the static payload_iv
    err = random_stack_advance<0x200>(sgx_rijndael128GCM_encrypt, seal_key, p_text2encrypt, text2encrypt_length,
        reinterpret_cast<uint8_t *>(&(p_sealed_data->aes_data.payload)), p_payload_iv,
        SGX_SEAL_IV_SIZE, p_additional_MACtext, additional_MACtext_length,
        &(p_sealed_data->aes_data.payload_tag));

    if (err == SGX_SUCCESS)
    {
        // Copy additional MAC text
        uint8_t* p_aad = NULL;
        if (additional_MACtext_length > 0)
        {
            p_aad = &(p_sealed_data->aes_data.payload[text2encrypt_length]);
            memcpy(p_aad, p_additional_MACtext, additional_MACtext_length);
        }

        // populate the plain_text_offset, payload_size in the data_blob
        p_sealed_data->plain_text_offset = text2encrypt_length;
        p_sealed_data->aes_data.payload_size = additional_MACtext_length + text2encrypt_length;
    }
    // Clear temp state
    return err;
}

// sgx_unseal_data_helper
//
// Parameters:
//      p_sealed_data - [IN] pointer to the sealed data structure containing protected data
//      p_additional_MACtext - [OUT] pointer to the plaintext data stream which was GCM protected
//      additional_MACtext_length - [IN] length of the plaintext data stream in bytes
//      p_decrypted_text - [OUT] pointer to decrypted data stream
//      decrypted_text_length - [IN] length of the decrypted data stream to encrypt in bytes
//
// Return Value:
//      sgx_status_t - SGX Error code
sgx_status_t sgx_unseal_data_helper(const sgx_sealed_data_t *p_sealed_data, uint8_t *p_additional_MACtext,
    uint32_t additional_MACtext_length, uint8_t *p_decrypted_text, uint32_t decrypted_text_length)
{
    sgx_status_t err = SGX_ERROR_UNEXPECTED;
    randomly_placed_buffer<sgx_key_128bit_t, sizeof(sgx_key_128bit_t), 0x200> seal_key_buf{};

    uint8_t payload_iv[SGX_SEAL_IV_SIZE];
    memset(&payload_iv, 0, SGX_SEAL_IV_SIZE);

    if (decrypted_text_length > 0)
        memset(p_decrypted_text, 0, decrypted_text_length);

    if (additional_MACtext_length > 0)
        memset(p_additional_MACtext, 0, additional_MACtext_length);

    // Get the seal key
    //auto *seal_key = seal_key_buf.randomize_object();
    using cseal_key200 = randomly_placed_object<sgx::custom_alignment_aligned<sgx_key_128bit_t, sizeof(sgx_key_128bit_t), 0, sizeof(sgx_key_128bit_t)>, 0x200>;
    cseal_key200 oseal_key_buf;
    auto* oseal_key = oseal_key_buf.instantiate_object();
    auto* seal_key = &oseal_key->v;
    err = sgx_get_key(&p_sealed_data->key_request, seal_key);
    if (err != SGX_SUCCESS)
    {
        // Clear temp state
        // Provide only error codes that the calling code could act on
        if ((err == SGX_ERROR_INVALID_CPUSVN) || (err == SGX_ERROR_INVALID_ISVSVN) || (err == SGX_ERROR_OUT_OF_MEMORY))
            return err;
        // Return error indicating the blob is corrupted
        return SGX_ERROR_MAC_MISMATCH;
    }

    //
    // code that calls sgx_unseal_data commonly does some sanity checks
    // related to plain_text_offset.  We add fence here since we don't 
    // know what crypto code does and if plain_text_offset-related 
    // checks mispredict the crypto code could operate on unintended data
    //
    sgx_lfence();

    err = random_stack_advance<0x200>(sgx_rijndael128GCM_decrypt, seal_key, const_cast<uint8_t *>(p_sealed_data->aes_data.payload),
        decrypted_text_length, p_decrypted_text, &payload_iv[0], SGX_SEAL_IV_SIZE,
        const_cast<uint8_t *>(&(p_sealed_data->aes_data.payload[decrypted_text_length])), additional_MACtext_length,
        const_cast<sgx_aes_gcm_128bit_tag_t *>(&p_sealed_data->aes_data.payload_tag));

    if (err != SGX_SUCCESS)
    {
        // Clear temp state
        return err;
    }

    if (additional_MACtext_length > 0)
    {
        memcpy(p_additional_MACtext, &(p_sealed_data->aes_data.payload[decrypted_text_length]), additional_MACtext_length);
    }
    // Clear temp state
    return SGX_SUCCESS;
}
