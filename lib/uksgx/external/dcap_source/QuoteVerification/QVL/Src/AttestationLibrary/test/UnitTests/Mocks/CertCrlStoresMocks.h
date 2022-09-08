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


#ifndef INTEL_SGX_QVL_TEST_CERTCRLMOCKS_H_
#define INTEL_SGX_QVL_TEST_CERTCRLMOCKS_H_

#include <gmock/gmock.h>

#include <SgxEcdsaAttestation/AttestationParsers.h>
#include <PckParser/CrlStore.h>
#include <PckParser/PckParser.h>
#include <OpensslHelpers/OpensslTypes.h>
#include <CertVerification/CertificateChain.h>

// Gmock will try to print these out and fail, as the implementation is within OpenSSL internal headers.
static void PrintTo(const X509_crl_st &x, ::std::ostream *os)  { *os << "X509_crl_st at [" << std::hex << &x << std::dec << "]"; }

namespace intel { namespace sgx { namespace dcap { namespace test {

class PckCertificateMock: public virtual dcap::parser::x509::PckCertificate
{
public:
    MOCK_METHOD2(parse, PckCertificate(const std::string&, const time_t&));

    MOCK_CONST_METHOD0(getVersion, uint32_t());
    MOCK_CONST_METHOD0(getSubject, const dcap::parser::x509::DistinguishedName&());
    MOCK_CONST_METHOD0(getIssuer, const dcap::parser::x509::DistinguishedName&());
    MOCK_CONST_METHOD0(getValidity, const dcap::parser::x509::Validity&());
    MOCK_CONST_METHOD0(getSerialNumber, const std::vector<uint8_t>&());
    MOCK_CONST_METHOD0(getSignature, const dcap::parser::x509::Signature&());
    MOCK_CONST_METHOD0(getExtensions, const std::vector<dcap::parser::x509::Extension>&());
    MOCK_CONST_METHOD0(getPpid, const std::vector<uint8_t>&());
    MOCK_CONST_METHOD0(getTcb, const dcap::parser::x509::Tcb&());
    MOCK_CONST_METHOD0(getFmspc, const std::vector<uint8_t>&());
    MOCK_CONST_METHOD0(getPceId, const std::vector<uint8_t>&());

    MOCK_CONST_METHOD0(getPubKey, const std::vector<uint8_t>&());
    MOCK_CONST_METHOD0(getInfo, const std::vector<uint8_t>&());
};

class CrlStoreMock: public dcap::pckparser::CrlStore
{
public:
    MOCK_METHOD1(parse, bool(const std::string&));

    MOCK_CONST_METHOD1(expired, bool(const time_t&));
    MOCK_CONST_METHOD0(getIssuer, const dcap::pckparser::Issuer&());
    MOCK_CONST_METHOD0(getSignature, const dcap::pckparser::Signature&());
    MOCK_CONST_METHOD0(getExtensions, const std::vector<dcap::pckparser::Extension>&());
    MOCK_CONST_METHOD0(getRevoked, const std::vector<dcap::pckparser::Revoked>&());
    MOCK_CONST_METHOD1(isRevoked, bool(const dcap::parser::x509::Certificate&));
    MOCK_CONST_METHOD0(getCrl, X509_CRL&());
};

class CertificateChainMock: public dcap::CertificateChain
{
public:
    MOCK_METHOD1(parse, Status(const std::string&));

    MOCK_CONST_METHOD0(length, size_t());
    MOCK_CONST_METHOD1(get, std::shared_ptr<const dcap::parser::x509::Certificate>(const dcap::parser::x509::DistinguishedName &));
    MOCK_CONST_METHOD0(getRootCert, std::shared_ptr<const dcap::parser::x509::Certificate>());
    MOCK_CONST_METHOD0(getIntermediateCert, std::shared_ptr<const dcap::parser::x509::Certificate>());
    MOCK_CONST_METHOD0(getTopmostCert, std::shared_ptr<const dcap::parser::x509::Certificate>());
    MOCK_CONST_METHOD0(getPckCert, std::shared_ptr<const dcap::parser::x509::PckCertificate>());
};

class ValidityMock: public dcap::parser::x509::Validity
{
public:
    MOCK_CONST_METHOD0(getNotAfterTime, std::time_t());
};

class SignatureMock: public dcap::parser::x509::Signature
{
public:
    MOCK_CONST_METHOD0(getRawDer, const std::vector<uint8_t>&());
};

class TcbMock: public dcap::parser::x509::Tcb
{
public:
    MOCK_CONST_METHOD0(getPceSvn, uint32_t());
    MOCK_CONST_METHOD0(getCpuSvn, const std::vector<uint8_t>&());
    MOCK_CONST_METHOD1(getSgxTcbComponentSvn, uint32_t(uint32_t));
    MOCK_CONST_METHOD1(getTdxTcbComponentSvn, uint32_t(uint32_t));
};

}}}} // namespace intel { namespace sgx { namespace dcap { namespace test {

#endif //INTEL_SGX_QVL_TEST_CERTCRLMOCKS_H_
