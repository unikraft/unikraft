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

#ifndef _REF_WL_GEN_H_
#define _REF_WL_GEN_H_

#include "ref_le.h"
#include "se_types.h"

typedef enum
{
    UNKNOWN = 0,
    GEN_WL
} gen_wl_cmd_t;

class CRefWLGen
{
public:
    CRefWLGen();
    ~CRefWLGen();

    bool run(int argc, char* argv[]);

private:
    gen_wl_cmd_t parse_cmd(int argc, char **argv);
    bool generate_wl();
    bool process_line(char *line, ref_le_white_list_entry_t *entry);
    bool get_hash_from_pubkey_file(const char* pubkey_file, sgx_measurement_t* p_hash);
    bool get_hash_from_sigstruct_file(const char* sig_file, sgx_measurement_t* p_hash);
    bool get_hash_from_string(char* hash_str, sgx_measurement_t* p_hash);

    bool set_key_and_sign(const char* prikey_file, ref_le_white_list_t *p_wl, uint16_t wl_count, sgx_rsa3072_signature_t* p_signature);

    void print_byte_array(bool always_print, const uint8_t *array, size_t size, const char *line_prefix, int line_len = 32);
    void print_line(bool always_print, const char* format, ...);
    void reverse_byte_array(uint8_t *array, size_t size);
    char* clean_start(char *str);

    char *m_cfgfile;
    char *m_outfile;
    char *m_keyfile;
    uint32_t m_version;
    bool m_verbose;
};

#endif // _REF_WL_GEN_H_
