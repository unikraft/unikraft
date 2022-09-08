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

#ifndef ENCRYPTIP_H
#define ENCRYPTIP_H

// PCL table must be alligned to PCL_TABLE_ALLIGNMENT bytes
#define PCL_TABLE_ALLIGNMENT (16)

#define PCL_TEXT_SECTION_NAME              ".nipx"
#define PCL_DATA_SECTION_NAME              ".nipd"
#define PCL_RODATA_SECTION_NAME            ".niprod"

#define NUM_NON_IP_OR_DEBUG_SEC_NAMES (24)
#define NUM_NON_IP_SEC_NAMES (13)

/*
 * When running CPUID with leaf 0, bit 30 of the output ECX 
 * indicates if platform supports RDRAND
 */
#define SUPPORT_RDRAND (1<<30)

// GCM can encrypt up to (2^32 - 2) blocks per single IV
#define PCL_GCM_MAX_NUM_BLOCKS   \
    ((size_t)((uint32_t)(-2)))

#define PCL_GCM_NUM_BLOCKS(size) \
    ((size + PCL_AES_BLOCK_LEN - 1)/ PCL_AES_BLOCK_LEN)

#define EVP_SUCCESS (1)
#define RDRAND_SUCCESS (1)
#define ENCIP_ERROR(r) (ENCIP_SUCCESS != (r))

typedef enum encip_ret_e_
{
    ENCIP_SUCCESS                         = 0x0,
    ENCIP_ERROR_FAIL                      = 0x80,
    ENCIP_ERROR_INVALID_PARAM             = 0x82,
    ENCIP_ERROR_KEY_FILE_SIZE             = 0x84,
    ENCIP_ERROR_ENCLAVE_SIZE              = 0x86,
    ENCIP_ERROR_MEM_ALLOC                 = 0x88,
    ENCIP_ERROR_PARSE_ARGS                = 0x8a,
    ENCIP_ERROR_PARSE_INVALID_PARAM       = 0x8c,
    ENCIP_ERROR_READF_INVALID_PARAM       = 0x8e,
    ENCIP_ERROR_READF_OPEN                = 0x90,
    ENCIP_ERROR_READF_ALLOC               = 0x92,
    ENCIP_ERROR_READF_READ                = 0x94,
    ENCIP_ERROR_WRITEF_INVALID_PARAM      = 0x96,
    ENCIP_ERROR_WRITEF_OPEN               = 0x98,
    ENCIP_ERROR_WRITEF_WRITE              = 0x9a,
    ENCIP_ERROR_SEALED_BUF_SIZE           = 0x9c,
    ENCIP_ERROR_GETTBL_INVALID_PARAM      = 0x9e,
    ENCIP_ERROR_TBL_NOT_FOUND             = 0xa0,
    ENCIP_ERROR_TBL_NOT_ALIGNED           = 0xa2,
    ENCIP_ERROR_ALREADY_ENCRYPTED         = 0xa4,
    ENCIP_ERROR_IMPROPER_STATE            = 0xa6,
    ENCIP_ERROR_RANDIV_INVALID_PARAM      = 0xa8,
    ENCIP_ERROR_RDRAND_NOT_SUPPORTED      = 0xa9,
    ENCIP_ERROR_RDRAND_FAILED             = 0xaa,
    ENCIP_ERROR_GCM_ENCRYPT_INVALID_PARAM = 0xac, 
    ENCIP_ERROR_ENCRYPT_ALLOC             = 0xae,
    ENCIP_ERROR_ENCRYPT_INIT_EX           = 0xb0,
    ENCIP_ERROR_ENCRYPT_IV_LEN            = 0xb2,
    ENCIP_ERROR_ENCRYPT_INIT_KEY          = 0xb4,
    ENCIP_ERROR_ENCRYPT_AAD               = 0xb6,
    ENCIP_ERROR_ENCRYPT_UPDATE            = 0xb8,
    ENCIP_ERROR_ENCRYPT_FINAL             = 0xba,
    ENCIP_ERROR_ENCRYPT_TAG               = 0xbc,
    ENCIP_ERROR_INCIV_INVALID_PARAM       = 0xbe,
    ENCIP_ERROR_IV_OVERFLOW               = 0xc0,
    ENCIP_ERROR_SHA_INVALID_PARAM         = 0xc2,
    ENCIP_ERROR_SHA_ALLOC                 = 0xc4,
    ENCIP_ERROR_SHA_INIT                  = 0xc6,
    ENCIP_ERROR_SHA_UPDATE                = 0xc8,
    ENCIP_ERROR_SHA_FINAL                 = 0xca,
    ENCIP_ERROR_ENCSECS_INVALID_PARAM     = 0xcc,
    ENCIP_ERROR_ENCSECS_COUNTER_OVERFLOW  = 0xce,
    ENCIP_ERROR_ENCSECS_RVAS_OVERFLOW     = 0xd0,
    ENCIP_ERROR_UPDATEF_INVALID_PAR       = 0xd2,
    ENCIP_ERROR_ENCRYPTIP_INVALID_PARAM   = 0xd4,
    ENCIP_ERROR_PARSE_ELF_INVALID_PARAM   = 0xd6,
    ENCIP_ERROR_PARSE_ELF_INVALID_IMAGE   = 0xd8,
    ENCIP_ERROR_SEGMENT_NOT_READABLE      = 0xda,
}encip_ret_e;

typedef struct pcl_data_t_
{

    Elf64_Shdr* elf_sec;
    Elf64_Half  shstrndx;
    char*       sections_names;
    Elf64_Phdr* phdr;
    uint16_t    nsections;
    uint16_t    nsegments;
}pcl_data_t;

encip_ret_e parse_args(
                int argc, 
                IN char* argv[], 
                OUT char** ifname, 
                OUT char** ofname, 
                OUT char** kfname, 
                OUT bool* debug);
static encip_ret_e read_file(IN const char* const name,OUT uint8_t** data,OUT size_t* size_out); 
static encip_ret_e write_file(const char* const name, uint8_t* data, size_t size); 
encip_ret_e gcm_encrypt(
    unsigned char *plaintext, 
    size_t plaintext_len, 
    unsigned char *aad,
    size_t aad_len, 
    unsigned char *key, 
    unsigned char *iv,
    unsigned char *ciphertext, 
    unsigned char *tag);
encip_ret_e sha256(const void* const buf, size_t buflen, uint8_t* hash);
encip_ret_e encrypt_ip(uint8_t* elf_buf, size_t elf_size, uint8_t* key, bool debug);
encip_ret_e write_tbl_strs(pcl_table_t* tbl);

#endif // #ifndef ENCRYPTIP_H

