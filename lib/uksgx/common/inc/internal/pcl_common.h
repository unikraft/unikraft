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

#ifndef PCL_COMMON_H
#define PCL_COMMON_H

/*
 * This file includes definition used by PCL library and encryption tool
 */

#define IN
#define OUT
#define INOUT

// Define both ASSERT_CONCAT and ASSERT_CONCAT_ so that __COUNTER__ receives a value
#define ASSERT_CONCAT_(a, b) a##b
#ifndef ASSERT_CONCAT
#define ASSERT_CONCAT(a, b) ASSERT_CONCAT_(a, b)
#endif // #ifndef ASSERT_CONCAT
#define PCL_COMPILE_TIME_ASSERT(exp) \
enum { ASSERT_CONCAT(static_assert_, __COUNTER__) = 1/(!!(exp)) }
    
// PCL uses AES with 16 Bytes block size
#define PCL_AES_BLOCK_LEN (16)
#define PCL_COUNTER_SIZE  (16)
#define PCL_AES_BLOCK_LEN_BITS (128)

#define PCLTBL_SECTION_NAME                ".pcltbl"

typedef struct iv_t_
{
    uint8_t val[SGX_AESGCM_IV_SIZE];
    uint8_t reserved[4];
}iv_t;

typedef struct rva_size_tag_iv_t_
{
    size_t rva;
    size_t size;
    sgx_cmac_128bit_tag_t tag;
    iv_t     iv;
}rva_size_tag_iv_t;

// Hardcoded maximal size of sealed bolb. ISV can modify if requried
#define PCL_SEALED_BLOB_SIZE       (0x250)
#define SGX_PCL_GUID_SIZE              (16)
// Hardcoded maximal number of encrypted sections. ISV can modify if requried
#define PCL_MAX_NUM_ENCRYPTED_SECTIONS (0x80)

typedef enum pcl_status_e_
{
    PCL_PLAIN   = 0xABABABAB,
    PCL_CIPHER  = 0xBCBCBCBC,
    PCL_RUNNING = 0xDEDEDEDE,
    PCL_DONE    = 0xFAFAFAFA,        
}pcl_status_e;

typedef struct pcl_table_t_
{
    pcl_status_e pcl_state;                   // Current state of PCL
    uint32_t     reserved1[3];                // Must be 0
    uint8_t      pcl_guid[SGX_PCL_GUID_SIZE]; // GUID must match GUID in Sealed blob
    size_t       sealed_blob_size;            // Size of selaed blob
    uint32_t     reserved2[2];                // Must be 0
    uint8_t      sealed_blob[PCL_SEALED_BLOB_SIZE]; // For security, sealed blob is copied into enclave
    uint8_t      decryption_key_hash[SGX_SHA256_HASH_SIZE]; // SHA256 digest of decryption key
    uint32_t     num_rvas;                    // Number of RVAs
    uint32_t     reserved3[3];                // Must be 0
    rva_size_tag_iv_t rvas_sizes_tags_ivs[PCL_MAX_NUM_ENCRYPTED_SECTIONS]; // Array of rva_size_tag_iv_t
}pcl_table_t;

#endif // #ifndef PCL_COMMON_H

