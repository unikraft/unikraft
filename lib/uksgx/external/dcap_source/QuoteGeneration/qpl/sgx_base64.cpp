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
#include <iostream>
#include <stdlib.h>
#ifndef _MSC_VER
#include <openssl/evp.h>
#else
#include <Windows.h>
#include <wincrypt.h>
#endif
#include "sgx_base64.h"

#ifndef _MSC_VER
char *base64_encode(const char *input, int length) {
  const auto encoded_len = 4*((length+2)/3);
  auto output = reinterpret_cast<char *>(calloc(encoded_len+1, 1)); //+1 for the terminating null that EVP_EncodeBlock adds on
  const auto output_len = EVP_EncodeBlock(reinterpret_cast<unsigned char *>(output), reinterpret_cast<const unsigned char *>(input), length);
  if (encoded_len != output_len)
    return NULL;
  return output;
}

#else
char* base64_encode(const char* input, int length) {
    DWORD output_len = 0;
    if (CryptBinaryToStringA(reinterpret_cast<const BYTE*>(input), length, CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, nullptr, &output_len))
    {
        char* output = static_cast<char*>(calloc(output_len, 1));
        if (output)
        {
            if (CryptBinaryToStringA(reinterpret_cast<const BYTE*>(input), length, CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, output, &output_len))
            {
                return output;
            }
        }
    }
    return NULL;
}

#endif