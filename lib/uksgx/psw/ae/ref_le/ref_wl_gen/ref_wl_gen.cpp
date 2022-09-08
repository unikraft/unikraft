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

#include "ref_wl_gen.h"
#include "arch.h"
#include "parse_key_file.h"
#include "ref_le.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

#define USAGE \
    "ref_wl_gen <Command> <Options>\n" \
    "Command:\n"\
    "   gen-wl:    Generate a white-list file based on the information provided in the config file. \n" \
    "Options:\n" \
    "   -out <file-name>:  The output file name for the white-list. \n" \
    "   -cfg <file-name>:  A CSV configuration file with the list of hash values or keys to sign. \n" \
    "   -key <file-name>:  The private key to sign the white-list with. \n" \
    "   -ver <version>:    An integer value of the white-list version. \n" \
    "   -verbose:          Print extended report while generating the white-list. \n" \
    "CSV file columns: \n"  \
    "   allow provision key, mr_enclave valid, mr_signer hash, mr_signer file, mr_enclave hash, mr_enclave file, comments (ignored) \n" \
    "Notes:  \n" \
    "   * Column 1 and 2 should be true or false. \n" \
    "   * If mr_enclave valid is false the mr_enclave columns will be ignored. \n" \
    "   * If mr_signer/mr_enclave hash is not empty the mr_signer/mr_enclave file will be ignored. \n" \
    "   * mr_signer file should be key file (pem), mr_enclave file should be sigstruct (bin). \n" \
    "   * Key hash representation should be in little endian, i.e. LSB byte first. \n" \
    "Example:\n" \
    "   ref_wl_gen gen-wl -out wl.bin -cfg cfg.csv -key private.pem \n"

#define LINE_LEN 1024
#define MAX_NUM_OF_RECORDS 512
#define WL_FILE_VERSION 1

CRefWLGen::CRefWLGen() : m_cfgfile(NULL), m_outfile(NULL), m_keyfile(NULL), m_version(0), m_verbose(false)
{
}


CRefWLGen::~CRefWLGen()
{
}

void CRefWLGen::print_line(bool always_print, const char* format, ...)
{
    if (!always_print && !m_verbose)
    {
        return;
    }

    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}

void CRefWLGen::print_byte_array(bool always_print, const uint8_t *array, size_t size, const char *line_prefix, int line_len)
{
    if (!always_print && !m_verbose)
    {
        return;
    }

    for (size_t i = 0; i < size; ++i)
    {
        if (i % line_len == 0)
        {
            print_line(always_print, "\n%s", line_prefix);
        }
        print_line(always_print, "%02X ", array[i]);
    }
    print_line(always_print, "\n");
}

void CRefWLGen::reverse_byte_array(uint8_t *array, size_t size)
{
    for (size_t i = 0; i < size / 2; i++)
    {
        uint8_t temp = array[i];
        array[i] = array[size - i - 1];
        array[size - i - 1] = temp;
    }
}

gen_wl_cmd_t CRefWLGen::parse_cmd(int argc, char **argv)
{
    gen_wl_cmd_t cmd = UNKNOWN;
    argc--;
    argv++;
    if (argc <= 0 || argv == NULL)
    {
        return cmd;
    }

    if (strcmp(*argv, "gen-wl") == 0)
        cmd = GEN_WL;
    argc--;
    argv++;

    while (argc >= 1)
    {
        if (strcmp(*argv, "-out") == 0)
        {
            if (--argc < 1) { return UNKNOWN; }
            m_outfile = *(++argv);
        }
        else if (strcmp(*argv, "-cfg") == 0)
        {
            if (--argc < 1) { return UNKNOWN; }
            m_cfgfile = *(++argv);
        }
        else if (strcmp(*argv, "-key") == 0)
        {
            if (--argc < 1) { return UNKNOWN; }
            m_keyfile = *(++argv);
        }
        else if (strcmp(*argv, "-ver") == 0)
        {
            if (--argc < 1) { return UNKNOWN; }
            m_version = atoi(*(++argv));
        }
        else if (strcmp(*argv, "-verbose") == 0)
        {
            m_verbose = true;
        }
        else
        {
            return UNKNOWN;
        }
        argc--;
        argv++;
    }

    return cmd;
}

char* CRefWLGen::clean_start(char *str)
{
    if (str == NULL)
    {
        return NULL;
    }

    while (isspace(*str))
    {
        str++;
    }
    return str;
}

bool CRefWLGen::get_hash_from_pubkey_file(const char* pubkey_file, sgx_measurement_t* p_hash)
{
    if (pubkey_file == NULL || p_hash == NULL || *pubkey_file == 0)
    {
        return false;
    }

    rsa_params_t rsa_params;
    int key_type;

    if (!parse_key_file(pubkey_file, &rsa_params, &key_type))
    {
        return false;
    }

    if (sgx_sha256_msg((uint8_t*)&(rsa_params.n), sizeof(rsa_params.n), (sgx_sha256_hash_t*)p_hash) != SGX_SUCCESS)
    {
        return false;
    }

    return true;
}

bool CRefWLGen::get_hash_from_string(char* hash_str, sgx_measurement_t* p_hash)
{
    if (hash_str == NULL || p_hash == NULL)
    {
        return false;
    }

    char* ptr = clean_start(hash_str);
    for (size_t count = 0; count < sizeof(sgx_measurement_t); ++count)
    {
        ptr = clean_start(ptr);
        if (*ptr == 0)
        {
            return false;
        }

        p_hash->m[count] = (char)strtol(ptr, &ptr, 16);
    }

    return true;
}

bool  CRefWLGen::get_hash_from_sigstruct_file(const char* sig_file, sgx_measurement_t* p_hash)
{
    if (sig_file == NULL || p_hash == NULL || *sig_file == 0)
    {
        return false;
    }

    FILE *p_sig_file = fopen(sig_file, "rb");
    if (!p_sig_file)
    {
        print_line(true, "Failed to open the input file '%s'.\n", sig_file);
        return false;
    }

    fseek(p_sig_file, 0, SEEK_END);
    size_t length = ftell(p_sig_file);
    rewind(p_sig_file);

    if (length < sizeof(enclave_css_t))
    {
        fclose(p_sig_file);
        return false;
    }

    enclave_css_t sigstruct;
    length = fread((&sigstruct), 1, sizeof(enclave_css_t), p_sig_file);
    fclose(p_sig_file);

    if (length < sizeof(enclave_css_t))
    {
        return false;
    }

    memcpy((char*)p_hash, &(sigstruct.body.enclave_hash), sizeof(sgx_measurement_t));

    return true;
}

bool CRefWLGen::process_line(char *line, ref_le_white_list_entry_t *entry)
{
    char *item = strtok(line, ",");

    // parse item: allow provision key
    item = clean_start(item);
    if (strncmp(item, "true", 4) == 0)
    {
        entry->provision_key = 1;
    }
    else
    {
        entry->provision_key = 0;
    }
    print_line(false, "  provision_key: %d\n", entry->provision_key);

    // parse item: mr_enclave valid
    item = strtok(NULL, ",");
    item = clean_start(item);
    if (strncmp(item, "true", 4) == 0)
    {
        entry->match_mr_enclave = 1;
    }
    else
    {
        entry->match_mr_enclave = 0;
    }
    print_line(false, "  match_mr_enclave: %d\n", entry->match_mr_enclave);

    // parse item: mr_signer hash/file
    item = strtok(NULL, ",");
    item = clean_start(item);

    if (*item != 0)
    { // hash
        if (get_hash_from_string(item, &(entry->mr_signer)) == false)
        {
            return false;
        }

        item = strtok(NULL, ","); // skip the next item
    }
    else
    { // file
        item = strtok(NULL, ",");
        item = clean_start(item);

        if (get_hash_from_pubkey_file(item, &(entry->mr_signer)) == false)
        {
            return false;
        }
    }
    print_line(false, "  mr_signer: ");
    print_byte_array(false, (uint8_t*)&(entry->mr_signer), sizeof(sgx_measurement_t), "    ");
    reverse_byte_array((uint8_t*)&(entry->mr_signer), sizeof(entry->mr_signer));

    // parse item: mr_enclave hash/file
    if (entry->match_mr_enclave == 1)
    {
        item = strtok(NULL, ",");
        item = clean_start(item);
        if (*item != 0)
        { // hash
            if (get_hash_from_string(item, &(entry->mr_enclave)) == false)
            {
                return false;
            }

            item = strtok(NULL, ","); // skip the next item
        }
        else
        { // file
            item = strtok(NULL, ",");
            item = clean_start(item);

            if (get_hash_from_sigstruct_file(item, &(entry->mr_enclave)) == false)
            {
                return false;
            }
        }
        print_line(false, "  mr_enclave: ");
        print_byte_array(false, (uint8_t*)&(entry->mr_enclave), sizeof(sgx_measurement_t), "    ");
        reverse_byte_array((uint8_t*)&(entry->mr_enclave), sizeof(entry->mr_enclave));
    }

    print_line(false, "  Full entry (big endian): ");
    print_byte_array(false, (uint8_t*)entry, sizeof(ref_le_white_list_entry_t), "    ");

    return true;
}

bool CRefWLGen::set_key_and_sign(const char* prikey_file, ref_le_white_list_t *p_wl, uint16_t wl_count, sgx_rsa3072_signature_t* p_signature)
{
    rsa_params_t rsa_params;
    int key_type;

    if (!parse_key_file(prikey_file, &rsa_params, &key_type))
    {
        return false;
    }
    rsa_params.e[0] = 3;

    sgx_rsa3072_key_t rsa_key;
    memcpy(&(rsa_key.mod), &(rsa_params.n), sizeof(rsa_key.mod));
    memcpy(&(rsa_key.d), &(rsa_params.d), sizeof(rsa_key.d));
    memcpy(&(rsa_key.e), &(rsa_params.e), sizeof(rsa_key.e));
    
    memcpy(&(p_wl->signer_pubkey.mod), &(rsa_params.n), sizeof(p_wl->signer_pubkey.mod));
    memcpy(&(p_wl->signer_pubkey.exp), &(rsa_params.e), sizeof(p_wl->signer_pubkey.exp));
    print_line(false, "  Signer public key modolus: ");
    print_byte_array(false, (uint8_t*)&(p_wl->signer_pubkey.mod), sizeof(p_wl->signer_pubkey.mod), "    ");
    print_line(false, "  Signer public key exponent: %d\n", p_wl->signer_pubkey.exp);

    reverse_byte_array((uint8_t*)&(p_wl->signer_pubkey.mod), sizeof(p_wl->signer_pubkey.mod));
    reverse_byte_array((uint8_t*)&(p_wl->signer_pubkey.exp), sizeof(p_wl->signer_pubkey.exp));

    int len = REF_LE_WL_SIZE(wl_count);

    sgx_status_t res = sgx_rsa3072_sign((const uint8_t*) p_wl, len, &rsa_key, p_signature);

    if (res != SGX_SUCCESS)
    {
        return false;
    }

    return true;
}

bool CRefWLGen::generate_wl()
{
    print_line(true, "Building white list...\n");
    ref_le_white_list_t* wl = (ref_le_white_list_t*)malloc(REF_LE_WL_SIZE(MAX_NUM_OF_RECORDS));
    memset(wl, 0, REF_LE_WL_SIZE(MAX_NUM_OF_RECORDS));

    // Init values of white-list
    print_line(true, "While list format version: %d\n", WL_FILE_VERSION);
    wl->version = WL_FILE_VERSION;

    print_line(true, "While list instance version: %d\n", m_version);
    wl->wl_version = m_version;

    // Read configuration information

    print_line(true, "Reading configuration file: %s\n", m_cfgfile);

    FILE *cfgfile = fopen(m_cfgfile, "r");
    if (!cfgfile)
    {
        print_line(true, "Failed to open the configuration file '%s'.\n", m_cfgfile);
        free(wl);
        return false;
    }

    char line[LINE_LEN];
    // TODO: what if line len is not enough?
    while (fgets(line, LINE_LEN, cfgfile) != NULL)
    {
        char* start = clean_start(line);
        if (*start == 0)
        {
            continue;
        }

        print_line(false, "Entry #%d:\n", wl->entries_count);

        if (process_line(line, &(wl->wl_entries[wl->entries_count])) == false)
        {
            print_line(true, "Failed to process the configuration line.\n");
            fclose(cfgfile);
            free(wl);
            return false;
        }

        wl->entries_count++;
    }

    fclose(cfgfile);
    print_line(false, "Parsed entries count: %d\n", wl->entries_count);
    uint16_t wl_count = wl->entries_count;

    reverse_byte_array((uint8_t*)&(wl->version), sizeof(wl->version));
    reverse_byte_array((uint8_t*)&(wl->wl_version), sizeof(wl->wl_version));
    reverse_byte_array((uint8_t*)&(wl->entries_count), sizeof(wl->entries_count));

    print_line(true, "Signing using key file: %s\n", m_keyfile);
    sgx_rsa3072_signature_t sig;
    if (!set_key_and_sign(m_keyfile, wl, wl_count, &sig))
    {
        free(wl);
        return false;
    }

    reverse_byte_array((uint8_t*)&sig, sizeof(sgx_rsa3072_signature_t));

    print_line(false, "Complete white list (big endian): ");
    print_byte_array(false, (uint8_t*)wl, REF_LE_WL_SIZE(wl_count), "  ");
    print_line(false, "Signature (big endian): ");
    print_byte_array(false, (uint8_t*)&sig, sizeof(sgx_rsa3072_signature_t), "  ");

    print_line(true, "Writing output file: %s\n", m_outfile);

    FILE *pOut = fopen(m_outfile, "wb");
    size_t written = fwrite(wl, 1, REF_LE_WL_SIZE(wl_count), pOut);
    written += fwrite(&sig, 1, sizeof(sgx_rsa3072_signature_t), pOut);
    fclose(pOut);

    free(wl);

    if (written != REF_LE_WL_SIZE(wl_count) + sizeof(sgx_rsa3072_signature_t))
    {
        return false;
    }

    print_line(true, "White list generation completed successfully. \n");

    return true;
}


bool CRefWLGen::run(int argc, char **argv)
{
    gen_wl_cmd_t cmd = parse_cmd(argc, argv);
    switch (cmd)
    {
    case GEN_WL:
        if (m_cfgfile == NULL || m_outfile == NULL || m_keyfile == NULL)
        {
            print_line(true, "Mising parameters. \n%s", USAGE);
            return false;
        }
        if (generate_wl() == false)
        {
            print_line(true, "Failed to generate white-list.\n");
            return false;
        }
        break;
    default:
        print_line(true, "Command line is not correct. \n%s", USAGE);
        return false;
    }
    return true;
}
