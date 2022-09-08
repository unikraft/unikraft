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

#include "SgxEcdsaAttestation/AttestationParsers.h"

#include "OpensslHelpers/OpensslTypes.h"
#include "ParserUtils.h"
#include "Utils/SafeMemcpy.h"

#include <cstring>

namespace intel { namespace sgx { namespace dcap { namespace parser { namespace x509 {

Signature::Signature(): _rawDer{},
                        _r{},
                        _s{}
{}

Signature::Signature(const std::vector<uint8_t>& rawDer,
                             const std::vector<uint8_t>& r,
                             const std::vector<uint8_t>& s):
                                _rawDer(rawDer),
                                _r(r),
                                _s(s)
{}

bool Signature::operator==(const Signature& other) const
{
    return _rawDer == other._rawDer &&
            _r == other._r &&
            _s == other._s;
}

const std::vector<uint8_t>& Signature::getRawDer() const
{
    return _rawDer;
}

const std::vector<uint8_t>& Signature::getR() const
{
    return _r;
}

const std::vector<uint8_t>& Signature::getS() const
{
    return _s;
}

// Private
Signature::Signature(const ASN1_BIT_STRING* pSig)
{
    _rawDer = std::vector<uint8_t>(static_cast<size_t>(pSig->length));
    safeMemcpy(_rawDer.data(), pSig->data, static_cast<size_t>(pSig->length));

    const uint8_t *derSeqIt = _rawDer.data();
    const auto sig = crypto::make_unique(d2i_ECDSA_SIG(nullptr, &derSeqIt , static_cast<long>(_rawDer.size())));

    if(!sig)
    {
        throw FormatException("Invalid certificate signature value: NULL");
    }

    // internal pointers
    const BIGNUM *r,*s;
    ECDSA_SIG_get0(sig.get(), &r, &s);

    _r = bn2Vec(r);
    _s = bn2Vec(s);
}

}}}}} // namespace intel { namespace sgx { namespace dcap { namespace parser { namespace x509 {
