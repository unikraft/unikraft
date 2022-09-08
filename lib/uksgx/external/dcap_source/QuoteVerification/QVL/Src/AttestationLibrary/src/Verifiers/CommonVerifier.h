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

#ifndef INTEL_SGX_QVL_VERIFIERS_INTERFACE_H_
#define INTEL_SGX_QVL_VERIFIERS_INTERFACE_H_

#include <SgxEcdsaAttestation/QuoteVerification.h>
#include <SgxEcdsaAttestation/AttestationParsers.h>

#include <CertVerification/CertificateChain.h>

#include "PckParser/CrlStore.h"

namespace intel { namespace sgx { namespace dcap {

class CommonVerifier
{
public: 
    CommonVerifier() = default;
    CommonVerifier(const CommonVerifier&) = default;
    CommonVerifier(CommonVerifier&&) = default;
    virtual ~CommonVerifier() = default;

    CommonVerifier& operator=(const CommonVerifier&) = default;
    CommonVerifier& operator=(CommonVerifier&&) = default;

    /**
    * Verify correctness of root certificate
    * Checks subject, issuer, validity period, extensions and signature.
    *
    * @param rootCa - root certificate to verify
    * @return Status code of the operation
    */
    virtual Status verifyRootCACert(const dcap::parser::x509::Certificate &rootCa) const;
    
    /**
    * Verify correctness of intermediate PCK certificate
    * Checks subject, issuer, validity period, extensions and signature.
    *
    * @param intermediate - certificate to verify
    * @param root - root certificate
    * @return Status code of the operation
    */ 
    virtual Status verifyIntermediate(const dcap::parser::x509::Certificate &intermediate,
                                      const dcap::parser::x509::Certificate &root) const;

    virtual bool checkStandardExtensions(const std::vector<pckparser::Extension> &extensions, const std::vector<int> &opensslExtensionNids) const;

    virtual bool checkSignature(const dcap::parser::x509::Certificate &certificate, const dcap::parser::x509::Certificate &issuer) const;
    virtual bool checkSignature(const pckparser::CrlStore &crl, const dcap::parser::x509::Certificate &crlIssuer) const;
    virtual bool checkSha256EcdsaSignature(const Bytes &signature, const std::vector<uint8_t> &message, const std::vector<uint8_t> &publicKey) const;
};

}}} // namespace intel { namespace sgx { namespace dcap {

#endif
