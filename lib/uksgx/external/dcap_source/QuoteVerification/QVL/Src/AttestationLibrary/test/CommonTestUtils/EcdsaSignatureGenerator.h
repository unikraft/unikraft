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

#ifndef SGXECDSAATTESTATION_ECDSASIGNATUREGENERATOR_H
#define SGXECDSAATTESTATION_ECDSASIGNATUREGENERATOR_H

#include <OpensslHelpers/Bytes.h>
#include <OpensslHelpers/OpensslTypes.h>

using namespace intel::sgx::dcap;

class EcdsaSignatureGenerator
{
public:
    /**
     * Generate ECDSA signature of a message
     *
     * @param message - data for signature creation
     * @param prvKey - ecdsa private key
     * @return calculated signature will be returned
     */
    static Bytes signECDSA_SHA256(const Bytes& message, EVP_PKEY* prvKey);

    static std::string signatureToHexString(const Bytes& signature);

    static Bytes convertECDSASignatureToRaw(Bytes& ecdsaSig);

    static std::array<uint8_t, 64> convertECDSASignatureToRawArray(Bytes& ecdsaSig);

};


#endif //SGXECDSAATTESTATION_ECDSASIGNATUREGENERATOR_H
