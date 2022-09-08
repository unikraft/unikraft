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

#ifndef SGX_ECDSA_CERTIFICATECHAIN_H_
#define SGX_ECDSA_CERTIFICATECHAIN_H_

#include <string>
#include <vector>
#include <memory>
#include <PckParser/PckParser.h>
#include <Verifiers/BaseVerifier.h>
#include <SgxEcdsaAttestation/AttestationParsers.h>
#include <SgxEcdsaAttestation/QuoteVerification.h>

namespace intel { namespace sgx { namespace dcap {

class CertificateChain
{
public:

    virtual ~CertificateChain() = default;
    CertificateChain() = default;
    CertificateChain(const CertificateChain&) = delete;
    CertificateChain(CertificateChain&&) = default;
    

    CertificateChain& operator=(const CertificateChain&) = delete;
    CertificateChain& operator=(CertificateChain&&) = default;

    /**
    * Parse certificate chain.
    * Check if there is at least one valid x.509 certificate in the chain.
    *
    * @param pemCertChain - string of concatenated PEM certificates
    * @return true if chain has been successfully parsed
    */
    virtual Status parse(const std::string& pemCertChain);

    /**
    * Get length of the parsed chain
    * @return chain length.
    */
    virtual size_t length() const;

    /**
    * Get certificate from chain by subject
    *
    * @param subject - certificate subject to get
    * @return shared pointer to certificate
    */
    virtual std::shared_ptr<const dcap::parser::x509::Certificate> get(const dcap::parser::x509::DistinguishedName &subject) const;

    /**
    * Get certificate of intermediate CA from chain (middle one)
    *
    * @return shared pointer to certificate
    */
    virtual std::shared_ptr<const dcap::parser::x509::Certificate> getIntermediateCert() const;

    /**
    * Get root certificate from chain (position 0)
    *
    * @return shared pointer to certificate
    */
    virtual std::shared_ptr<const dcap::parser::x509::Certificate> getRootCert() const;

    /**
    * Get topmost certificate from chain (last position)
    *
    * @return shared pointer to certificate
    */
    virtual std::shared_ptr<const dcap::parser::x509::Certificate> getTopmostCert() const;

    /**
    * Get PCK certificate from chain (recognized by common name)
    *
    * @return shared pointer to certificate
    */
    virtual std::shared_ptr<const dcap::parser::x509::PckCertificate> getPckCert() const;

    /**
    * Get all certificates in chain
    *
    * @return vector of shared pointers with certificates
    */
    virtual std::vector<std::shared_ptr<const dcap::parser::x509::Certificate>> getCerts() const;

private:
    BaseVerifier _baseVerifier{};

    std::vector<std::string> splitChain(const std::string &pemChain) const;
    std::vector<std::shared_ptr<const dcap::parser::x509::Certificate>> certs{};
    std::shared_ptr<const dcap::parser::x509::Certificate> rootCert{};
    std::shared_ptr<const dcap::parser::x509::Certificate> topmostCert{};
    std::shared_ptr<const dcap::parser::x509::PckCertificate> pckCert{};
};

}}} // namespace intel { namespace sgx { namespace dcap {

#endif //SGX_ECDSA_CERTIFICATECHAIN_H_
