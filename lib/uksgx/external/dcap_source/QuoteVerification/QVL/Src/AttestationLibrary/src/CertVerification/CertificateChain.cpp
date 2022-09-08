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

#include "CertificateChain.h"

#include "X509Constants.h"

#include <algorithm>

namespace intel { namespace sgx { namespace dcap {


Status CertificateChain::parse(const std::string& pemCertChain)
{
    const auto certStrs = splitChain(pemCertChain);

    certs.reserve(certStrs.size());
    for(const auto& certPem : certStrs)
    {
        try {
            auto cert = std::make_shared<dcap::parser::x509::Certificate>(dcap::parser::x509::Certificate::parse(certPem));

            if (cert->getSubject() == cert->getIssuer())
            {
                rootCert = cert;
            }

            certs.emplace_back(cert);
        }
        // any cert in chain has wrong format
        // then whole chain should be considered invalid
        catch (const dcap::parser::FormatException&)
        {
            return STATUS_UNSUPPORTED_CERT_FORMAT;
        }
        catch (const dcap::parser::InvalidExtensionException&)
        {
            // Since cert order in the chain is not deterministic
            // we do not know which cert we failed to parse (subject is also an extension)
            // Is that RootCA? IntermediateCA? PCKCert? TCBSigningCert?
            // We will do some guessing... I mean some heuristics
            if (certs.empty()) // we failed parsing first cert, in most cases it will be a root CA
            {
                return STATUS_SGX_ROOT_CA_INVALID_EXTENSIONS;
            }
            if (certs.size() == 1) // second cert wll be probably an intermediate CA
            {
                if (certStrs.size() == 2)
                {
                    return STATUS_SGX_TCB_SIGNING_CERT_INVALID_EXTENSIONS;
                }
                return STATUS_SGX_INTERMEDIATE_CA_INVALID_EXTENSIONS;
            }
            else // third cert wll be probably PCK CA
            {
                return STATUS_SGX_PCK_INVALID_EXTENSIONS;
            }
        }
    }

    for(auto const &cert: certs)
    {
        auto signedCertIter = std::find_if(certs.cbegin(), certs.cend(), [cert](const std::shared_ptr<const dcap::parser::x509::Certificate> &found)
        {
            return found->getSubject() != cert->getSubject()
                   && found->getIssuer() == cert->getSubject();
        });
        if(signedCertIter == certs.cend())
        {
            topmostCert = cert;
        }
        if (cert->getSubject().getCommonName().find(constants::SGX_PCK_CN_PHRASE) != std::string::npos)
        {
            try
            {
                pckCert = std::make_shared<dcap::parser::x509::PckCertificate>(dcap::parser::x509::PckCertificate(*cert));
            }
            catch (const dcap::parser::FormatException&)
            {
                return STATUS_UNSUPPORTED_CERT_FORMAT;
            }
            catch (const dcap::parser::InvalidExtensionException&)
            {
                return STATUS_SGX_PCK_INVALID_EXTENSIONS;
            }
        }
    }

    if (length() == 0)
    {
        return STATUS_UNSUPPORTED_CERT_FORMAT;
    }

    return STATUS_OK;
}

size_t CertificateChain::length() const
{
    return certs.size();
}

std::shared_ptr<const dcap::parser::x509::Certificate> CertificateChain::get(const dcap::parser::x509::DistinguishedName &subject) const
{
    auto it = std::find_if(certs.cbegin(), certs.cend(), [&subject](const std::shared_ptr<const dcap::parser::x509::Certificate> &cert)
    {
        return cert->getSubject() == subject;
    });
    if(it == certs.end())
    {
        return nullptr;
    }
    return *it;
}

std::shared_ptr<const dcap::parser::x509::Certificate> CertificateChain::getIntermediateCert() const
{
    auto verifier = _baseVerifier;
    auto it = std::find_if(certs.cbegin(), certs.cend(), [&verifier](const std::shared_ptr<const dcap::parser::x509::Certificate> &cert)
    {
        return cert->getSubject() != cert->getIssuer() && verifier.commonNameContains(cert->getSubject(), constants::SGX_INTERMEDIATE_CN_PHRASE);
    });
    if(it == certs.end())
    {
        return nullptr;
    }
    return *it;
}

std::shared_ptr<const dcap::parser::x509::Certificate> CertificateChain::getRootCert() const
{
    return rootCert;
}

std::shared_ptr<const dcap::parser::x509::Certificate> CertificateChain::getTopmostCert() const
{
    return topmostCert;
}

std::shared_ptr<const dcap::parser::x509::PckCertificate> CertificateChain::getPckCert() const
{
    return pckCert;
}

std::vector<std::shared_ptr<const dcap::parser::x509::Certificate>> CertificateChain::getCerts() const
{
    return certs;
}

std::vector<std::string> CertificateChain::splitChain(const std::string &pemChain) const
{
    if(pemChain.empty())
    {
        return {};
    }

    const std::string begCert = "-----BEGIN CERTIFICATE-----";
    const std::string endCert = "-----END CERTIFICATE-----";

    const size_t begPos = pemChain.find(begCert);
    const size_t endPos = pemChain.find(endCert);

    if(begPos == std::string::npos || endPos == std::string::npos || begPos >= endPos)
    {
        return {};
    }

    std::vector<std::string> ret;
    size_t newStartPos = begPos;
    size_t foundEndPos = endPos;
    while(foundEndPos != std::string::npos)
    {
        // second loop to be evenatually run in second and
        // further iteration
        // we could ommit this loop by simply newStartPos = newEndPos + 1;
        // at the end of main loop if we were sure new line would be simple '\n'
        // since it's more portable to assume it could be \r\n as well then here it is
        while(pemChain.at(newStartPos) != '-') ++newStartPos;

        const size_t newEndPos = foundEndPos + endCert.size();
        const std::string cert = pemChain.substr(newStartPos, newEndPos - newStartPos);

        // we do not check for this in second and further iteration
        // and it's cheaper to check on shorter string
        if(cert.find(begCert) != std::string::npos)
        {
            ret.push_back(cert);
        }

        newStartPos = newEndPos;
        foundEndPos = pemChain.find(endCert, newStartPos);

        if (foundEndPos <= newStartPos)
        {
            return {};
        }
    }

    return ret;
}

}}}
