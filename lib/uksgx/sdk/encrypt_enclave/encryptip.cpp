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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <elf.h>
#include <immintrin.h>
#include <cpuid.h>
#include <stdbool.h>
#include "openssl/evp.h"
#include "openssl/ossl_typ.h"
#include "openssl/sha.h"
#include <sgx_tseal.h>
#include <sgx_tcrypto.h>
#include "pcl_common.h"
#include "encryptip.h"
#include "sgx_pcl_guid.h"

/*
 * @func main - tool entry point
 * 1. Parses arguments
 * 2. Read binary key file 
 * 3. Read enclave so file
 * 4. Modify enclave binary
 * 5. Save enclave to file
 * @param int argc, number of command line parameters
 * @param IN char *argv[], array of command line parameters' strings
 * @return (int)(encip_ret_e)
 * Respective error values if either parse_args, read_file, encrypt_ip or write_file fail
 * ENCIP_ERROR_KEY_FILE_SIZE if size of binary key file is not 16 bytes
 * ENCIP_ERROR_ENCLAVE_SIZE if enclave size is 0
 * ENCIP_SUCCESS if success
 */
int main(int argc, IN char *argv[])
{
    char* keyfile_name = NULL;
    char* enclave_in_name = NULL;
    char* enclave_out_name = NULL;
    bool debug = false;
    uint8_t* enclave_buf = NULL;
    uint8_t* key_buf = NULL;
    size_t key_size = 0;
    size_t enclave_size = 0;

    // Parse the arguments: 
    encip_ret_e ret = parse_args(argc, argv, &enclave_in_name, &enclave_out_name, &keyfile_name, &debug);
    if(ENCIP_ERROR(ret))
        return (int)ret;

    // Read enclave file into buffer:
    ret = read_file(enclave_in_name, &enclave_buf, &enclave_size);
    if(ENCIP_ERROR(ret))
        return (int)ret;
    if(0 == enclave_size)
    {
        ret = ENCIP_ERROR_ENCLAVE_SIZE;
        goto Label_free_enclave_buffer;
    }

    // Read key file into buffer:
    ret = read_file(keyfile_name, &key_buf, &key_size);
    if(ENCIP_ERROR(ret))
        goto Label_free_enclave_buffer;
    if(SGX_AESGCM_KEY_SIZE != key_size)
    {
        ret = ENCIP_ERROR_KEY_FILE_SIZE;
        goto Label_free_key_and_enclave_buffers;
    }
    
    // Modify enclave for PCL:
    ret = encrypt_ip(enclave_buf, enclave_size, key_buf, debug);
    if(ENCIP_ERROR(ret)) 
        goto Label_free_key_and_enclave_buffers;

    // Write the buffer into enclave file:
    ret = write_file(enclave_out_name, enclave_buf, enclave_size);
    if(ENCIP_ERROR(ret))
        goto Label_free_key_and_enclave_buffers;

    // Set success:
    ret = ENCIP_SUCCESS;

Label_free_key_and_enclave_buffers:
    free(key_buf);

Label_free_enclave_buffer:
    free(enclave_buf);

    return (int)ret;
}

/*
 * @func can_modify checks if section content can be modified without disrupting 
 * enclave signing or loading flows 
 * @param IN const char* const sec_name is a pointer to the section's name
 * @return true iff section content can be modified without disrupting 
 * enclave signing or loading flows
 */
static inline bool can_modify(IN const char* const sec_name, bool debug)
{

    /*
     * non_ip_sec_names is an array of names of sections that must remain plain text
     * to enable enclave signing and loading flows
     */
    static char const* const non_ip_sec_names[NUM_NON_IP_OR_DEBUG_SEC_NAMES] = 
    {
        ".shstrtab",             // Sections' names string table. Pointed by e_shstrndx
        ".note.sgxmeta",         // SGX enclave metadata    
        ".bss",                  // Inited with zero - no IP. Section may overlap other sections
        ".tbss",                 // Inited with zero - no IP. Section may overlap other sections
        ".dynamic",              // Required to construct dyn_info by function parse_dyn at elfparser.cpp
        ".dynsym",               // Holds content pointed by entreis with index DT_SYMTAB in dyn_info 
        ".dynstr",               // Holds content pointed by entreis with index DT_STRTAB in dyn_info
        ".rela.dyn",             // Holds content pointed by entreis with index DT_REL in dyn_info
        ".rela.plt",             // Holds content pointed by entreis with index DT_JMPREL in dyn_info
        PCLTBL_SECTION_NAME,     // PCL table 
        PCL_TEXT_SECTION_NAME,   // code use by PCL flow
        PCL_DATA_SECTION_NAME,   // Data used by PCL flow
        PCL_RODATA_SECTION_NAME, // Read only data used by PCL flow 

        ".comment",              // Required for debugging
        ".debug_aranges",        // Required for debugging
        ".debug_info",           // Required for debugging
        ".debug_abbrev",         // Required for debugging
        ".debug_line",           // Required for debugging
        ".debug_str",            // Required for debugging
        ".debug_loc",            // Required for debugging
        ".debug_ranges",         // Required for debugging
        ".gnu.version_d",        // Required for debugging, allocables
        ".symtab",               // Required for comfortable debugging
        ".strtab",               // Required for comfortable debugging
    };

    if(NULL == sec_name)
        return false;
    uint32_t num_sec_names = debug ? NUM_NON_IP_OR_DEBUG_SEC_NAMES : NUM_NON_IP_SEC_NAMES;
    for(uint32_t secidx = 0; secidx < num_sec_names; secidx++)
    {
        if(!strcmp(non_ip_sec_names[secidx],sec_name))
            return false;
    }
    return true;
}

/*
 * @func parse_elf prases the ELF buffer and assigns dat astruct with:
 * 1. Pointer to elf sections array
 * 2. Index of sections names strings section
 * 3. Number of sections
 * 4. Pointer to sections names structure
 * 5. Pointer to elf segments array 
 * 6. Number of segments
 * @param IN const void* const elf_buf_in is a pointer to the ELF binary in memory
 * @param size_t elf_size, ELF binary size in bytes
 * @param OUT pcl_data_t* dat is a pointer to the struct holding output
 * @return:
 * ENCIP_ERROR_PARSE_ELF_INVALID_PARAM if input parameters are NULL
 * ENCIP_ERROR_PARSE_ELF_INVALID_IMAGE if ELF image is not valid
 * ENCIP_SUCCESS if success
 */
static encip_ret_e parse_elf(const void* const elf_buf_in, size_t elf_size, pcl_data_t* dat)
{
    if(NULL == elf_buf_in || NULL == dat)
        return ENCIP_ERROR_PARSE_ELF_INVALID_PARAM;
    uint8_t* elf_buf = (uint8_t*)elf_buf_in;
    // Elf header is at the encalve input file start address:
    if(sizeof(Elf64_Ehdr) > elf_size)
        return ENCIP_ERROR_PARSE_ELF_INVALID_IMAGE;

    Elf64_Ehdr* elf_hdr = (Elf64_Ehdr*)(elf_buf);
    // Verify magic: 
    if(ELFMAG0 != elf_hdr->e_ident[EI_MAG0] ||
       ELFMAG1 != elf_hdr->e_ident[EI_MAG1] ||
       ELFMAG2 != elf_hdr->e_ident[EI_MAG2] ||
       ELFMAG3 != elf_hdr->e_ident[EI_MAG3])
        return ENCIP_ERROR_PARSE_ELF_INVALID_IMAGE;

    // Find the number of sections:
    dat->nsections = elf_hdr->e_shnum;

    // Find the index of the section which contains the sections names strings:
    uint16_t shstrndx = elf_hdr->e_shstrndx;
    dat->shstrndx = shstrndx;
    if(dat->nsections <= shstrndx)
        return ENCIP_ERROR_PARSE_ELF_INVALID_IMAGE;

    // Find the array of the sections headers:
    if((elf_hdr->e_shoff >= elf_size) ||
       (elf_hdr->e_shoff + dat->nsections * sizeof(Elf64_Shdr) < elf_hdr->e_shoff) ||
       (elf_hdr->e_shoff + dat->nsections * sizeof(Elf64_Shdr) > elf_size))
        return ENCIP_ERROR_PARSE_ELF_INVALID_IMAGE;

    dat->elf_sec = (Elf64_Shdr*)(elf_buf + elf_hdr->e_shoff);

    // Find the begining of the section which contains the sections names strings
    if((dat->elf_sec[shstrndx].sh_offset >= elf_size) ||  
       (dat->elf_sec[shstrndx].sh_offset + dat->elf_sec[shstrndx].sh_size < dat->elf_sec[shstrndx].sh_offset) ||
       (dat->elf_sec[shstrndx].sh_offset + dat->elf_sec[shstrndx].sh_size > elf_size))
        return ENCIP_ERROR_PARSE_ELF_INVALID_IMAGE;

    dat->sections_names = (char*)(elf_buf + dat->elf_sec[shstrndx].sh_offset);

    // Find number of segments: 
    dat->nsegments = elf_hdr->e_phnum;

    if((elf_hdr->e_phoff >= elf_size) ||
       (elf_hdr->e_phoff + dat->nsegments * sizeof(Elf64_Phdr) < elf_hdr->e_phoff) ||
       (elf_hdr->e_phoff + dat->nsegments * sizeof(Elf64_Phdr) > elf_size))
        return ENCIP_ERROR_PARSE_ELF_INVALID_IMAGE;

    dat->phdr = (Elf64_Phdr*)(elf_buf + elf_hdr->e_phoff);
    
    return ENCIP_SUCCESS;
}

/*
 * @func get_pcl_tbl iterates over sections to find the PCL table
 * @param IN const void* const elf_buf_in is a pointer to the ELF binary in memory
 * @param size_t elf_size, ELF buffer size in bytes
 * @param IN const pcl_data_t* const dat is a pointer to the struct holding parsed elf content
 * @param OUT pcl_table_t** tbl_pp, address of pointer to table
 * @return encip_ret_e:
 * ENCIP_ERROR_GETTBL_INVALID_PARAM if input parameters are NULL
 * ENCIP_ERROR_TBL_NOT_ALIGNED if table not aligned to PCL_TABLE_ALLIGNMENT
 * ENCIP_ERROR_TBL_NOT_FOUND if table is not found in binary
 * ENCIP_ERROR_ALREADY_ENCRYPTED if pcl_state in PCL table equals PCL_CIPHER 
 * ENCIP_ERROR_IMPROPER_STATE if pcl_state in PCL table does not equal PCL_PLAIN
 * ENCIP_ERROR_PARSE_ELF_INVALID_IMAGE if ELF image is invalid
 * ENCIP_SUCCESS if success
 */
static encip_ret_e get_pcl_tbl(
                    IN const void* const elf_buf_in, 
                    size_t elf_size, 
                    IN const pcl_data_t* const dat,
                    OUT pcl_table_t** tbl_pp)
{
    if(NULL == elf_buf_in || NULL == dat || NULL == tbl_pp)
        return ENCIP_ERROR_GETTBL_INVALID_PARAM;

    bool tbl_found = false;

    uint8_t* elf_buf = (uint8_t*)elf_buf_in;

    // Go over sections headers to find the table (skip first section): 
    for(uint16_t secidx = 1; secidx < dat->nsections && !tbl_found; secidx++)
    {
        if(dat->elf_sec[secidx].sh_name >= dat->elf_sec[dat->shstrndx].sh_size)
            return ENCIP_ERROR_PARSE_ELF_INVALID_IMAGE;
        char* sec_name = dat->sections_names + dat->elf_sec[secidx].sh_name;
        /*
         * Verifying string starts before end of section. Assuming (but not checking) 
         * that string ends before end of section. Additional check will complicate code.
         * Assuming the platform this application is running on is not compromized. 
         */
        if((uint8_t*)sec_name >= elf_buf + elf_size)
            return ENCIP_ERROR_PARSE_ELF_INVALID_IMAGE;
        if(0 == strcmp(sec_name, PCLTBL_SECTION_NAME))
        {
            *tbl_pp = (pcl_table_t *)(elf_buf + dat->elf_sec[secidx].sh_offset); 
            if((uint8_t*)(*tbl_pp) + sizeof(pcl_table_t) >= elf_buf + elf_size)
                return ENCIP_ERROR_PARSE_ELF_INVALID_IMAGE;
            // Verify table is aligned: 
            if(0 != dat->elf_sec[secidx].sh_offset % PCL_TABLE_ALLIGNMENT)
                return ENCIP_ERROR_TBL_NOT_ALIGNED;
            tbl_found = true;
        }
    }

    if(!tbl_found)
        return ENCIP_ERROR_TBL_NOT_FOUND;
    return ENCIP_SUCCESS;
}

/*
 * @func rdrand_supported uses CPUID to check if platform supports RDRAND
 * @return true iff platform supports RDRAND
 */
bool rdrand_supported()
{
    int cpu_info[4] = {0, 0, 0, 0};
    __cpuid(cpu_info, 1);
    return (!!(cpu_info[2] & SUPPORT_RDRAND));
}

/*
 * @func init_random_iv initiates iv with a random number
 * @param OUT uint8_t* iv, pointer to output random IV
 * @return encip_ret_e: 
 * ENCIP_ERROR_RANDIV_INVALID_PARAM if iv is NULL
 * ENCIP_ERROR_RDRAND_NOT_SUPPORTED if platform does not support RDRAND
 * ENCIP_ERROR_RDRAND_FAILED if random number gneration fails
    * ENCIP_SUCCESS if successfull
 */
static encip_ret_e init_random_iv(OUT uint8_t* iv)
{
    if(NULL == iv)
        return ENCIP_ERROR_RANDIV_INVALID_PARAM;
    if(rdrand_supported())
    {
        uint32_t* ivp = (uint32_t*)iv;
        // Get a random IV for encryption: 
        for(uint32_t i=0;i < SGX_AESGCM_IV_SIZE / sizeof(uint32_t);i++)
        {
            uint32_t randval = 0;
            int rdrand32ret = _rdrand32_step(&randval);
            if(RDRAND_SUCCESS != rdrand32ret)
                return ENCIP_ERROR_RDRAND_FAILED;
            *ivp = randval;
            ivp++;
        }
    }
    else
    {
        return ENCIP_ERROR_RDRAND_NOT_SUPPORTED;
    }
    return ENCIP_SUCCESS;
}

/*
 * @func update_flags updates the flags of sections or segments that must become writable
 * @param uint16_t secidx, index of section for which the flags are currently updated
 * @param INOUT pcl_data_t* dat, ELF data
 * @return encip_ret_e, ENCIP_ERROR_UPDATEF_INVALID_PAR if dat is NULL, else ENCIP_SUCCESS
 */
static inline encip_ret_e update_flags(uint16_t secidx, INOUT pcl_data_t* dat)
{
    if(NULL == dat)
        return ENCIP_ERROR_UPDATEF_INVALID_PAR;

    // Mark section as writable:
    dat->elf_sec[secidx].sh_flags |= SHF_WRITE;
    Elf64_Addr secstart = dat->elf_sec[secidx].sh_addr;
    size_t     secsize  = dat->elf_sec[secidx].sh_size;
    /*
     * If section overlaps segment:
     * 1. Verify segment is readable
     * 2. Mark segment as writable
     */
    for(uint16_t segidx=0;segidx<dat->nsegments;segidx++)
    {
        Elf64_Addr segstart = dat->phdr[segidx].p_vaddr;
        size_t     segsize  = dat->phdr[segidx].p_memsz;
        if(((secstart           <  segstart + segsize) && (secstart           >= segstart          )) ||
           ((secstart + secsize >  segstart          ) && (secstart + secsize <= segstart + segsize)))
        {
            // Segment must be readible: 
            if(!(dat->phdr[segidx].p_flags & (Elf64_Word)PF_R))
            {
                printf("\n\nError: segment %d ", segidx);

                if(dat->elf_sec[secidx].sh_name < dat->elf_sec[dat->shstrndx].sh_size)
                {
                    char* sec_name = dat->sections_names + dat->elf_sec[secidx].sh_name;
                    /*
                     * Verifying string starts before end of section. Assuming (but not checking) 
                     * that string ends before end of section. Additional check will complicate code.
                     * Assuming the platform this application is running on is not compromized. 
                     */
                    printf("overlaps encrypted section \"%s\" and ", sec_name);
                }    
                printf("is not readable. Exiting!!!\n\n\n");   
                return ENCIP_ERROR_SEGMENT_NOT_READABLE;
            }
            // Mark segment as wirtable:
            dat->phdr[segidx].p_flags |= (Elf64_Word)PF_W;
        }
    }
    return ENCIP_SUCCESS;
}

/*
 * @func encrypt_or_clear_ip_sections modifies the content of some sections. 
 * 1. If section content cannot be modified without disrupting enclave signing or loading flows 
 *    then section content is not modified
 * 2. Allocable sections (copied to application address space at shared object's load time)
 *    are encrypted.
 * 3. The content of sections that are not allocable is zeroed
 * @param IN pcl_data_t* dat, ELF data
 * @param IN uint8_t* key, the AES key for GCM encrypt
 * @param INOUT uint8_t* elf_buf, base address of ELF binary buffer 
 * @param OUT pcl_table_t* tbl, pointer to PCL table 
 * @param OUT uint32_t* num_rvas_out, total number of sections that are encrypted
 * @param bool debug, true iff enclave is requried to support debug
 * @return encip_ret_e:
 * ENCIP_ERROR_ENCSECS_INVALID_PARAM any input parameter is NULL
 * PCL_MAX_NUM_ENCRYPTED_SECTIONS if out of entires in PCL table
 * Respective error results in case any of the functions encrypt or update_flags fail. 
 * ENCIP_SUCCESS if success
 */
static encip_ret_e encrypt_or_clear_ip_sections(
                IN pcl_data_t* dat, 
                IN uint8_t* key,
                INOUT uint8_t* elf_buf, 
                size_t elf_size,
                OUT pcl_table_t* tbl, 
                OUT uint32_t* num_rvas_out, 
                bool debug)
{
    if(
       NULL == dat     ||
       NULL == key     ||
       NULL == elf_buf || 
       NULL == tbl     || 
       NULL == num_rvas_out)
        return ENCIP_ERROR_ENCSECS_INVALID_PARAM;
    uint32_t num_rvas = 0;
    // Go over sections headers to find sections to encrypt or clear: 
    char* sec_name = NULL;
    for(uint16_t secidx = 1; secidx < dat->nsections; secidx++)
    {
        if(dat->elf_sec[secidx].sh_name >= dat->elf_sec[dat->shstrndx].sh_size)
            return ENCIP_ERROR_PARSE_ELF_INVALID_IMAGE;
        sec_name = dat->sections_names + dat->elf_sec[secidx].sh_name;    
        /*
         * Verifying string starts before end of section. Assuming (but not checking) 
         * that string ends before end of section. Additional check will complicate code.
         * Assuming the platform this application is running on is not compromized. 
         */
        if((uint8_t*)sec_name > elf_buf + elf_size) 
            return ENCIP_ERROR_PARSE_ELF_INVALID_IMAGE;
        if(can_modify(sec_name, debug))
        {
            uint8_t* va = (uint8_t *)(elf_buf + dat->elf_sec[secidx].sh_offset);
            size_t size = dat->elf_sec[secidx].sh_size;
            if((va >= elf_buf + elf_size) ||
               (va + size < va)           ||
               (va + size > elf_buf + elf_size))
                return ENCIP_ERROR_PARSE_ELF_INVALID_IMAGE;
            // If section is allocable (mapped into process's virtual memory), decrypt it: 
            if(SHF_ALLOC & dat->elf_sec[secidx].sh_flags)
            {
                
                if(PCL_MAX_NUM_ENCRYPTED_SECTIONS <= num_rvas)
                {
                    /* 
                     * No more empty entries in PCL table. 
                     * To fix - redefine PCL_MAX_NUM_ENCRYPTED_SECTIONS in pcl_common.h
                     */
                    printf("Error: No more empty entries in Intel(R) SGX PCL table\n");
                    printf("To fix - redefine PCL_MAX_NUM_ENCRYPTED_SECTIONS in pcl_common.h\n");
                    return ENCIP_ERROR_ENCSECS_RVAS_OVERFLOW;
                }
                            
                if(PCL_GCM_NUM_BLOCKS(size) > PCL_GCM_MAX_NUM_BLOCKS)
                {
                    /*
                     * Size in 16-bytes-blocks exceeds (2^32 - 2).
                     * Only happen if cipher-text size is ~64GB.
                     */
                    return ENCIP_ERROR_ENCSECS_COUNTER_OVERFLOW;
                }

                uint8_t* iv = (uint8_t*)&(tbl->rvas_sizes_tags_ivs[num_rvas].iv.val);
                encip_ret_e ret = init_random_iv(iv);
                if(ENCIP_ERROR(ret))
                    return ret;

                uint8_t* tag = (uint8_t*)&(tbl->rvas_sizes_tags_ivs[num_rvas].tag);
                ret = gcm_encrypt(va, size, NULL, 0, (uint8_t *)key, iv, va, tag);
                if(ENCIP_ERROR(ret))
                {
                    printf("Failed to gcm-encrypt section %s\n", sec_name);
                    return ret;
                }

                // Insert entry to table: 
                tbl->rvas_sizes_tags_ivs[num_rvas].rva  = dat->elf_sec[secidx].sh_addr;
                tbl->rvas_sizes_tags_ivs[num_rvas].size = size;

                // Update flags to writable: 
                ret = update_flags(secidx, dat);
                if(ENCIP_ERROR(ret))
                    return ret;

                // Increment num_rvas:
                num_rvas++;
            }
            // Else (section is not allocable), zero it:
            else
            {
                memset(va, 0, size);
            }
        }
    }
    *num_rvas_out = num_rvas;
    return ENCIP_SUCCESS;
}

/*
 * @func encrypt_ip modifies the content of some sections. 
 * @param INOUT uint8_t* elf_buf, buffer of ELF binary
 * @param size_t elf_size, size of ELF binary in bytes
 * @param IN uint8_t* key, AES-GCM-128 key
 * @param bool debug, true iff enclave is requried to support debug
 * @return encip_ret_e:
 * ENCIP_ERROR_ENCRYPTIP_INVALID_PARAM if any input parameter is NULL
 * Respective error results in case any of the following functions fail: 
 * parse_elf, get_pcl_tbl init_random_iv, encrypt_or_clear_ip_sections or sha256
 * ENCIP_ERROR_MEM_ALLOC if memory allocation fails
 * ENCIP_ERROR_SEALED_BUF_SIZE if sealed buf size exceeds the size allocated for it in PCL table
 * ENCIP_SUCCESS if success
 */
encip_ret_e encrypt_ip(INOUT uint8_t* elf_buf, size_t elf_size, IN uint8_t* key, bool debug)
{
    if(NULL == elf_buf || NULL == key)
        return ENCIP_ERROR_ENCRYPTIP_INVALID_PARAM;
    encip_ret_e ret = ENCIP_ERROR_FAIL;
    pcl_data_t dat = {
        .elf_sec = 0,
        .shstrndx = 0,
        .sections_names = NULL,
        .phdr = NULL,
        .nsections = 0, 
        .nsegments = 0,
    };
    pcl_table_t* tbl = NULL;

    ret = parse_elf(elf_buf, elf_size, &dat);
    if(ENCIP_ERROR(ret))
        return ret;

    ret = get_pcl_tbl(elf_buf, elf_size, &dat, &tbl);
    if(ENCIP_ERROR(ret))
        return ret;

    // Verify state of binary:
    if(PCL_CIPHER == tbl->pcl_state) 
        return ENCIP_ERROR_ALREADY_ENCRYPTED;
    if(PCL_PLAIN  != tbl->pcl_state)
        return ENCIP_ERROR_IMPROPER_STATE;

    // Encrypt or clear IP sections: 
    uint32_t num_rvas = 0;
    ret = encrypt_or_clear_ip_sections(&dat, key, elf_buf, elf_size, tbl, &num_rvas, debug);
    if(ENCIP_ERROR(ret))
        return ret;

    // Set GUID:
    memcpy(tbl->pcl_guid, g_pcl_guid, sizeof(tbl->pcl_guid));

    // Set sealed blob size:
    tbl->sealed_blob_size = (size_t)sgx_calc_sealed_data_size(SGX_PCL_GUID_SIZE, SGX_AESGCM_KEY_SIZE);
    // Verify calculated size equals hard coded size of buffer in PCL table:
    if(PCL_SEALED_BLOB_SIZE != tbl->sealed_blob_size)
        return ENCIP_ERROR_SEALED_BUF_SIZE;

    // Set num RVAs:
    tbl->num_rvas = num_rvas;

    // Set decryption key sha256 hash result:
    ret = sha256(key, SGX_AESGCM_KEY_SIZE, tbl->decryption_key_hash);
    if(ENCIP_ERROR(ret))
        return ret;

    // Set PCL state
    tbl->pcl_state = PCL_CIPHER;

    return ENCIP_SUCCESS;
}

/*
 * @func print_usage prints sgx_encrypt usage instructions 
 * @param IN char* encip_name is the name of the application
 */
void print_usage(IN char* encip_name)
{
    printf("\n");
    printf("\tUsage: \n");
    printf("\t  %s -i <input enclave so file name> -o <output enclave so file name> -k <key file name> [-d]\n",
            encip_name);
    printf("\t  -d (optional) prevents the tool from disabling the debug capabilities\n");
    printf("\n");
}

/*
 * @func parse_args parses the application's input argument. 
 * @param int argc is the number of arguments
 * @param IN char* argv[] is the array of input arguments
 * @param OUT char** ifname points to the name of the original input enclave binary file
 * @param OUT char** ofname points to the name of the modified output enclave binary file
 * @param OUT char** kfname points to the name of the input key file
 * @param OUT bool* debug, true if enclave needs to support debug
 * the encrypted enclave binary. 
 * @return encip_ret_e:
 * ENCIP_ERROR_PARSE_INVALID_PARAM if any of the input parameters is NULL
 * ENCIP_ERROR_PARSE_ARGS if input arguments are not supported
 * ENCIP_SUCCESS if success
 */
encip_ret_e parse_args(
                int argc, 
                IN char* argv[], 
                OUT char** ifname, 
                OUT char** ofname, 
                OUT char** kfname, 
                OUT bool* debug)
{
    if(NULL == argv)
        return ENCIP_ERROR_PARSE_INVALID_PARAM;

    char* encip_name = argv[0];

    if((argc != 7 && argc != 8) || 
        NULL == ifname || 
        NULL == ofname || 
        NULL == kfname)
    {
        print_usage(encip_name);
        return ENCIP_ERROR_PARSE_INVALID_PARAM;
    }
    encip_ret_e ret = ENCIP_SUCCESS;
	
    for(int argidx = 1; argidx < argc; argidx++)
    {
        if(!strcmp(argv[argidx],"-d"))
        {
            *debug = true;
        }
        else if(!strcmp(argv[argidx],"-i") && argidx + 1 < argc)
        {
            argidx++;
            *ifname = argv[argidx];
        }
        else if(!strcmp(argv[argidx],"-o") && argidx + 1 < argc)
        {
            argidx++;
            *ofname = argv[argidx];        }
        else if(!strcmp(argv[argidx],"-k") && argidx + 1 < argc)
        {
            argidx++;
            *kfname = argv[argidx];
        }
        else
        {
            ret = ENCIP_ERROR_PARSE_ARGS;
        }
    }
	
    if((ENCIP_SUCCESS != ret) || 
       (NULL == *ifname)      ||
       (NULL == *ofname)      || 
       (NULL == *kfname))
    {
        print_usage(encip_name);
        ret = ENCIP_ERROR_PARSE_ARGS;
    }
    return ret;
}

/*
 * @func read_file reads file into buffer. 
 * @param IN const char* const ifname is the input file name
 * @param OUT uint8_t** buf_pp points to the output buffer
 * @param OUT size_t* size_out points to the output data size
 * @return encip_ret_e:
 * ENCIP_ERROR_READF_INVALID_PARAM if any of the input parameters is NULL
 * ENCIP_ERROR_READF_OPEN if unable to open input file
 * ENCIP_ERROR_READF_ALLOC if unable to allocate output buffer
 * ENCIP_ERROR_READF_READ if unable to read file to buffer
 * ENCIP_SUCCESS if success
 */
static encip_ret_e read_file(IN const char* const ifname, OUT uint8_t** buf_pp, OUT size_t* size_out)
{
    if(NULL == ifname || NULL == buf_pp || NULL == size_out)
         return ENCIP_ERROR_READF_INVALID_PARAM;

    FILE* fin = fopen(ifname, "rb");
    if(NULL == fin)
        return ENCIP_ERROR_READF_OPEN;

    fseek(fin,0,SEEK_END);
    size_t const size = ftell(fin);
    fseek(fin, 0, SEEK_SET);
    
    *buf_pp = (uint8_t*)malloc(size);
    if(NULL == *buf_pp)
    {
        fclose(fin);
        return ENCIP_ERROR_MEM_ALLOC;
    }

    size_t const num_bytes = fread(*buf_pp, 1, size, fin);
    if(num_bytes != size)
    {
        fclose(fin);
        free (*buf_pp);
        return ENCIP_ERROR_READF_READ;
    }
    fclose(fin); 
    *size_out = size;   
    return ENCIP_SUCCESS;
}

/*
 * @func write_file writes buffer into file 
 * @param IN char* ofname is the output file name
 * @param IN uint8_t* buf is the input buffer
 * @param size_t size is the size of the buffer
 * @return encip_ret_e:
 * ENCIP_ERROR_WRITEF_INVALID_PARAM if any of the input parameters is NULL
 * ENCIP_ERROR_WRITEF_OPEN if unable to open output file
 * ENCIP_ERROR_WRITEF_WRITE if unable to write buf to file
 * ENCIP_SUCCESS if success
 */
static encip_ret_e write_file(IN const char* const ofname, IN uint8_t* buf, size_t size)
{
    if(NULL == ofname || NULL == buf)
         return ENCIP_ERROR_WRITEF_INVALID_PARAM;
    FILE* fout = fopen(ofname, "wb");
    if(NULL == fout)
        return ENCIP_ERROR_WRITEF_OPEN;

    size_t num_bytes = fwrite(buf, 1, size, fout);
    if(num_bytes != size)
    {
        fclose(fout);
        return ENCIP_ERROR_WRITEF_WRITE;
    }

    fclose(fout);
    return ENCIP_SUCCESS;
}

/*
 * @func sha256 calculates SHA256 
 * @param IN const void* const buf is the input payload
 * @param size_t buflen is the payload length in bytes
 * @param OUT uint8_t* hash is the resulting output hash
 * @return encip_ret_e:
 * ENCIP_ERROR_SHA_INVALID_PARAM if any of the input parameters is NULL
 * ENCIP_ERROR_SHA_ALLOC if EVP_MD_CTX_create is unable to allocate buffer
 * ENCIP_ERROR_SHA_INIT is EVP_DigestInit_ex fails
 * ENCIP_ERROR_SHA_UPDATE if EVP_DigestUpdate fails
 * ENCIP_ERROR_SHA_FINAL if EVP_DigestFinal_ex fails
 * ENCIP_SUCCESS if success
 */
encip_ret_e sha256(IN const void* const buf, size_t buflen, OUT uint8_t* hash)
{
    encip_ret_e ret = ENCIP_ERROR_FAIL; 
    unsigned int digest_len = 0;
    
    if(NULL== buf || NULL== hash)
        return ENCIP_ERROR_SHA_INVALID_PARAM;

    EVP_MD_CTX *mdctx = EVP_MD_CTX_create();
    if(NULL == mdctx)
        return ENCIP_ERROR_SHA_ALLOC;

    if(EVP_SUCCESS != EVP_DigestInit_ex(mdctx, EVP_sha256(), NULL)){
        ret = ENCIP_ERROR_SHA_INIT;
        goto Label_free_context;
    }

    if(EVP_SUCCESS != EVP_DigestUpdate(mdctx, buf, buflen)){
        ret = ENCIP_ERROR_SHA_UPDATE;
        goto Label_free_context;
    }

    if((EVP_SUCCESS != EVP_DigestFinal_ex(mdctx, hash, &digest_len)) ||
       (SGX_SHA256_HASH_SIZE != digest_len)){
        ret = ENCIP_ERROR_SHA_FINAL;
        goto Label_free_context;
    }

    ret = ENCIP_SUCCESS;

Label_free_context: 
    EVP_MD_CTX_destroy(mdctx);
    
    return ret;
}

/*
 * @func gcm_encrypt calculates AES-GCM-128
 * @param IN unsigned char *plaintext, input plain text
 * @param int plaintext_len, size of plain text in bytes
 * @param IN unsigned char *aad, AAD
 * @param int aad_len, size of AAD in bytes
 * @param IN unsigned char *key, key
 * @param IN unsigned char *iv, iv
 * @param OUT unsigned char *ciphertext, output cipher text
 * @param OUT unsigned char *tag, GCM TAG result
 * @return encip_ret_e:
 * ENCIP_ERROR_GCM_ENCRYPT_INVALID_PARAM if any of the input parameters is NULL
 * ENCIP_ERROR_ENCRYPT_ALLOC if EVP_CIPHER_CTX_new is unable to allocate the requried buffer
 * ENCIP_ERROR_ENCRYPT_INIT_EX if initializing encryption function with EVP_EncryptInit_ex fails
 * ENCIP_ERROR_ENCRYPT_IV_LEN if setting IV length with EVP_CIPHER_CTX_ctrl fails
 * ENCIP_ERROR_ENCRYPT_INIT_KEY if setting key with EVP_EncryptInit_ex fails
 * ENCIP_ERROR_ENCRYPT_AAD if initializing AAD using EVP_EncryptUpdate fails
 * ENCIP_ERROR_ENCRYPT_UPDATE if encryption using EVP_EncryptUpdate fails
 * ENCIP_ERROR_ENCRYPT_FINAL if call to EVP_EncryptFinal_ex fails
 * ENCIP_ERROR_ENCRYPT_TAG if calculating TAG result using EVP_CIPHER_CTX_ctrl fails
 * ENCIP_SUCCESS if success
 */
encip_ret_e gcm_encrypt(
    IN unsigned char *plaintext, 
    size_t plaintext_len, 
    IN unsigned char *aad,
    size_t aad_len, 
    IN unsigned char *key, 
    IN unsigned char *iv,
    OUT unsigned char *ciphertext, 
    OUT unsigned char *tag)
{
    EVP_CIPHER_CTX *ctx;
    int len;
    encip_ret_e ret = ENCIP_ERROR_GCM_ENCRYPT_INVALID_PARAM;
    if( NULL == plaintext  ||
        NULL == key        ||
        NULL == iv         ||
        NULL == ciphertext ||
        NULL == tag)
        return ENCIP_ERROR_GCM_ENCRYPT_INVALID_PARAM;
    // Create and init context
    if(NULL == (ctx = EVP_CIPHER_CTX_new())) 
        return ENCIP_ERROR_ENCRYPT_ALLOC;
    // Init the encryption function
    if(EVP_SUCCESS != EVP_EncryptInit_ex(ctx, EVP_aes_128_gcm(), NULL, NULL, NULL)) 
    {
        ret = ENCIP_ERROR_ENCRYPT_INIT_EX;
        goto Label_gcm_cleanup;
    }
    // Set IV length to SGX_AESGCM_IV_SIZE
    if(EVP_SUCCESS != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, SGX_AESGCM_IV_SIZE, NULL)) 
    {
        ret = ENCIP_ERROR_ENCRYPT_IV_LEN;
        goto Label_gcm_cleanup;
    }
    // Init key and IV:
    if(EVP_SUCCESS != EVP_EncryptInit_ex(ctx, NULL, NULL, key, iv)) 
    {
        ret = ENCIP_ERROR_ENCRYPT_INIT_KEY;
        goto Label_gcm_cleanup;
    }
    // Init AAD:
    if(NULL != aad)
    {
        if(EVP_SUCCESS != EVP_EncryptUpdate(ctx, NULL, &len, aad, (int)aad_len)) 
        {
            ret = ENCIP_ERROR_ENCRYPT_AAD;
            goto Label_gcm_cleanup;
        }
    }
    // Encrypt:
    if(EVP_SUCCESS != EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, (int)plaintext_len)) 
    {
        ret = ENCIP_ERROR_ENCRYPT_UPDATE;
        goto Label_gcm_cleanup;
    }
    // Final:
    if(EVP_SUCCESS != EVP_EncryptFinal_ex(ctx, ciphertext + len, &len)) 
    {
        ret = ENCIP_ERROR_ENCRYPT_FINAL;
        goto Label_gcm_cleanup;
    }
    // Get Tag:
    if(EVP_SUCCESS != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, SGX_CMAC_MAC_SIZE, tag)) 
    {
        ret = ENCIP_ERROR_ENCRYPT_TAG;
        goto Label_gcm_cleanup;
    }
    
    ret = ENCIP_SUCCESS;
    
    // Cleanup:
Label_gcm_cleanup:
    EVP_CIPHER_CTX_free(ctx);
    return ret;
}

extern "C" 
{
#pragma GCC push_options
#pragma GCC optimize ("-fomit-frame-pointer")

void __x86_return_thunk()
{
    __asm__("ret\n\t");
}

void __x86_indirect_thunk_rax()
{
    __asm__("jmp *%rax\n\t");
}
#pragma GCC pop_options
}
