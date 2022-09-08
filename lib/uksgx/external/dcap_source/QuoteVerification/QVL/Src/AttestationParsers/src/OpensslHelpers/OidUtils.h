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

#ifndef SGX_DCAP_PARSERS_OID_UTILS_H
#define SGX_DCAP_PARSERS_OID_UTILS_H

#include "OpensslHelpers/OpensslTypes.h"

#include <openssl/asn1.h>

#include <string>
#include <vector>

namespace intel { namespace sgx { namespace dcap { namespace crypto {

void validateOid(const std::string& oidName, const ASN1_TYPE *oidValue, int expectedType);
void validateOid(const std::string& oidName, const ASN1_TYPE *oidValue, int expectedType, int expectedLength);
std::vector<uint8_t> oidToBytes(const ASN1_TYPE *oidValue);
uint8_t oidToByte(const ASN1_TYPE *oidValue);
uint32_t oidToUInt(const ASN1_TYPE *oidValue);
int oidToEnum(const ASN1_TYPE *oidValue);
STACK_OF_ASN1TYPE_uptr oidToStack(const ASN1_TYPE *oidValue);

}}}} // namespace intel { namespace sgx { namespace dcap { namespace crypto {

#endif // SGX_DCAP_PARSERS_OID_UTILS_H
