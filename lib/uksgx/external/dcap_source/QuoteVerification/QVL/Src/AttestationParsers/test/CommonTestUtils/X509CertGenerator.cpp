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

#include "X509CertGenerator.h"

#include <openssl/asn1.h>
#include <openssl/bio.h>

#include <algorithm>

namespace intel { namespace sgx { namespace dcap { namespace parser { namespace test {

const std::string SGX_EXTENSIONS_OID_STR            = "1.2.840.113741.1.13.1";
const std::string SGX_PPID_OID_STR                  = "1.2.840.113741.1.13.1.1";
const std::string SGX_TCB_OID_STR                   = "1.2.840.113741.1.13.1.2";
const std::string SGX_PCESVN_OID_STR                = "1.2.840.113741.1.13.1.2.17";
const std::string SGX_CPUSVN_OID_STR                = "1.2.840.113741.1.13.1.2.18";
const std::string SGX_PCEID_OID_STR                 = "1.2.840.113741.1.13.1.3";
const std::string SGX_FMSPC_OID_STR                 = "1.2.840.113741.1.13.1.4";
const std::string SGX_SGXTYPE_OID_STR               = "1.2.840.113741.1.13.1.5";
const std::string SGX_PLATFORM_INSTANCE_ID_OID_STR  = "1.2.840.113741.1.13.1.6";
const std::string SGX_CONFIGURATION_OID_STR         = "1.2.840.113741.1.13.1.7";
const std::string SGX_DYNAMIC_PLATFORM_OID_STR      = "1.2.840.113741.1.13.1.7.1";
const std::string SGX_CACHED_KEYS_OID_STR           = "1.2.840.113741.1.13.1.7.2";
const std::string SGX_SMT_ENABLED_OID_STR           = "1.2.840.113741.1.13.1.7.3";
const std::string SGX_UNEXPECTED_EXTENSION_OID_STR  = "1.2.840.113741.1.13.1.20";

const int OID_FORMAT_NUMERIC = 1;

namespace {
    void setName(X509_NAME* name, const dcap::parser::x509::DistinguishedName& data)
    {
        X509_NAME_add_entry_by_txt(name, "C",  MBSTRING_ASC, (const unsigned char*)(data.getCountryName().c_str()), -1, -1, 0);
        X509_NAME_add_entry_by_txt(name, "O",  MBSTRING_ASC, (const unsigned char*)(data.getOrganizationName().c_str()), -1, -1, 0);
        X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC, (const unsigned char*)(data.getCommonName().c_str()), -1, -1, 0);
        X509_NAME_add_entry_by_txt(name, "L", MBSTRING_ASC,  (const unsigned char*)(data.getLocationName().c_str()), -1, -1, 0);
        X509_NAME_add_entry_by_txt(name, "ST", MBSTRING_ASC, (const unsigned char*)(data.getStateName().c_str()), -1, -1, 0);
    }

    void setSerialNumber(X509* cert, const Bytes& serialNumber)
    {
        crypto::BIGNUM_uptr serialNumberBN = crypto::make_unique<BIGNUM>(
            BN_bin2bn(serialNumber.data(), static_cast<int>(serialNumber.size()), nullptr));
        crypto::ASN1_INTEGER_uptr serialNumberAsn = crypto::make_unique<ASN1_INTEGER>(BN_to_ASN1_INTEGER(serialNumberBN.get(), nullptr));

        X509_set_serialNumber(cert, serialNumberAsn.get());
    }
}

crypto::X509_uptr X509CertGenerator::generateCaCert(int version, const Bytes &serialNumber, long notBeforeOffset,
                                                    long notAfterOffset,
                                                    const EVP_PKEY *pubKey, const EVP_PKEY *signingKey,
                                                    const dcap::parser::x509::DistinguishedName &subject,
                                                    const dcap::parser::x509::DistinguishedName &issuer) const
{
    auto cert = generateBaseCert(version, serialNumber, notBeforeOffset, notAfterOffset, pubKey, subject, issuer);
    addStandardCaExtensions(cert);
    signCert(cert, signingKey);
    return cert;
}

crypto::X509_uptr X509CertGenerator::generateTcbSigningCert(int version, const Bytes &serialNumber,
                                                            long notBeforeOffset, long notAfterOffset,
                                                            const EVP_PKEY *pubKey, const EVP_PKEY *signingKey,
                                                            const dcap::parser::x509::DistinguishedName &subject,
                                                            const dcap::parser::x509::DistinguishedName &issuer) const
{
    auto cert = generateBaseCert(version, serialNumber, notBeforeOffset, notAfterOffset, pubKey, subject, issuer);
    addStandardNonCaExtensions(cert);
    signCert(cert, signingKey);
    return cert;
}

crypto::X509_uptr X509CertGenerator::generatePCKCert(int version, const Bytes &serialNumber, long notBeforeOffset, long notAfterOffset,
                               const EVP_PKEY *pubKey, const EVP_PKEY *signingKey,
                               const dcap::parser::x509::DistinguishedName &subject,
                               const dcap::parser::x509::DistinguishedName &issuer,
                               const Bytes& ppid, const Bytes& cpusvn, const Bytes& pcesvn,
                               const Bytes& pceId, const Bytes &fmspc, const int sgxType,
                               const Bytes& platformInstanceId, const bool dynamicPlatform,
                               const bool cachedKeys, const bool smtEnabled,
                               const bool unexpectedExtension) const
{
    auto cert = generateBaseCert(version, serialNumber, notBeforeOffset, notAfterOffset, pubKey, subject, issuer);
    addStandardNonCaExtensions(cert);
    addSGXPckExtensions(cert, ppid, cpusvn, pcesvn, pceId, fmspc, sgxType, platformInstanceId,
            dynamicPlatform, cachedKeys, smtEnabled, unexpectedExtension);
    signCert(cert, signingKey);
    return cert;
}

crypto::X509_uptr X509CertGenerator::generateBaseCert(int version, const Bytes &serialNumber, long notBeforeOffset,
                                                  long notAfterOffset,
                                                  const EVP_PKEY *pubKey,
                                                  const dcap::parser::x509::DistinguishedName &subject,
                                                  const dcap::parser::x509::DistinguishedName &issuer) const
{
    auto* evpPubKey = const_cast<EVP_PKEY*>(pubKey);
    crypto::X509_uptr cert = crypto::make_unique(X509_new());
    auto* x509 = cert.get();

    X509_set_version(x509, version);

    setSerialNumber(x509, serialNumber);

    X509_gmtime_adj(X509_get_notBefore(x509), notBeforeOffset);
    X509_gmtime_adj(X509_get_notAfter(x509), notAfterOffset);

    X509_set_pubkey(x509, evpPubKey);

    auto subjectName = X509_get_subject_name(x509);
    setName(subjectName, subject);

    auto issuerName = X509_get_issuer_name(x509);
    setName(issuerName, issuer);

    return cert;
}

void X509CertGenerator::addStandardCaExtensions(const crypto::X509_uptr& cert) const {
    auto certPtr = cert.get();
    add_ext(certPtr, NID_basic_constraints, const_cast<char *>((const char *)"critical,CA:TRUE"));
    add_ext(certPtr, NID_key_usage, const_cast<char *>((const char *)"critical,keyCertSign,cRLSign"));
    add_ext(certPtr, NID_subject_key_identifier, const_cast<char *>((const char *)"hash"));
    add_ext(certPtr, NID_crl_distribution_points, const_cast<char *>((const char *)"URI:https://certificates.trustedservices.intel.com/IntelSGXRootCA.crl"));
    add_ext(certPtr, NID_authority_key_identifier, const_cast<char *>((const char *)"keyid:always"));
}

void X509CertGenerator::addStandardNonCaExtensions(const crypto::X509_uptr& cert) const {
    auto certPtr = cert.get();
    add_ext(certPtr, NID_basic_constraints, const_cast<char *>((const char *)"critical,CA:FALSE"));
    add_ext(certPtr, NID_key_usage, const_cast<char *>((const char *)"critical,digitalSignature, nonRepudiation"));
    add_ext(certPtr, NID_subject_key_identifier, const_cast<char *>((const char *)"hash"));
    add_ext(certPtr, NID_crl_distribution_points, const_cast<char *>((const char *)"URI:https://certificates.trustedservices.intel.com/IntelSGXRootCA.crl"));
    add_ext(certPtr, NID_authority_key_identifier, const_cast<char *>((const char *) "keyid:always"));
}

void X509CertGenerator::createTcbComp(const std::string &parentOid, int index, unsigned char value, crypto::SGX_INT* dest) const
{
    std::string oid = parentOid + "." + std::to_string(index);
    dest->oid = OBJ_txt2obj(oid.c_str(), OID_FORMAT_NUMERIC);
    ASN1_INTEGER_set(dest->value, value);
}

void X509CertGenerator::addSGXPckExtensions(const crypto::X509_uptr &cert, const Bytes &ppid, const Bytes &cpusvn,
                                            const Bytes &pcesvn, const Bytes &pceId, const Bytes &fmspc, const int sgxType,
                                            const Bytes &platformInstanceId, const bool dynamicPlatform,
                                            const bool cachedKeys, const bool smtEnabled, const bool unexpectedExtension) const
{
    auto mainSequenceOID         = crypto::make_unique(OBJ_txt2obj(SGX_EXTENSIONS_OID_STR.c_str(), OID_FORMAT_NUMERIC));
    auto ppidOID                 = crypto::make_unique(OBJ_txt2obj(SGX_PPID_OID_STR.c_str(), OID_FORMAT_NUMERIC));
    auto tcbSequenceOID          = crypto::make_unique(OBJ_txt2obj(SGX_TCB_OID_STR.c_str(), OID_FORMAT_NUMERIC));
    auto tcbPceSvnOID            = crypto::make_unique(OBJ_txt2obj(SGX_PCESVN_OID_STR.c_str(), OID_FORMAT_NUMERIC));
    auto tcbCpuSvnOID            = crypto::make_unique(OBJ_txt2obj(SGX_CPUSVN_OID_STR.c_str(), OID_FORMAT_NUMERIC));
    auto pceIdOID                = crypto::make_unique(OBJ_txt2obj(SGX_PCEID_OID_STR.c_str(), OID_FORMAT_NUMERIC));
    auto fmspcOID                = crypto::make_unique(OBJ_txt2obj(SGX_FMSPC_OID_STR.c_str(), OID_FORMAT_NUMERIC));
    auto sgxTypeOID              = crypto::make_unique(OBJ_txt2obj(SGX_SGXTYPE_OID_STR.c_str(), OID_FORMAT_NUMERIC));
    auto platformInstanceIDOID   = crypto::make_unique(OBJ_txt2obj(SGX_PLATFORM_INSTANCE_ID_OID_STR.c_str(), OID_FORMAT_NUMERIC));
    auto configurationOID        = crypto::make_unique(OBJ_txt2obj(SGX_CONFIGURATION_OID_STR.c_str(), OID_FORMAT_NUMERIC));
    auto dynamicPlatformOID      = crypto::make_unique(OBJ_txt2obj(SGX_DYNAMIC_PLATFORM_OID_STR.c_str(), OID_FORMAT_NUMERIC));
    auto cachedKeysOID           = crypto::make_unique(OBJ_txt2obj(SGX_CACHED_KEYS_OID_STR.c_str(), OID_FORMAT_NUMERIC));
    auto smtEnabledOID           = crypto::make_unique(OBJ_txt2obj(SGX_SMT_ENABLED_OID_STR.c_str(), OID_FORMAT_NUMERIC));
    auto unexpectedExtensionOID  = crypto::make_unique(OBJ_txt2obj(SGX_UNEXPECTED_EXTENSION_OID_STR.c_str(), OID_FORMAT_NUMERIC));

    auto sgxExtensions = crypto::make_unique(crypto::SGX_EXTENSIONS_new()); // everything in this struct should be freed by uptr!

    auto sgxPpid = sgxExtensions->ppid;
    sgxPpid->oid = ppidOID.release();
    ASN1_OCTET_STRING_set(sgxPpid->value, ppid.data(), static_cast<int>(ppid.size()));

    sgxExtensions->sgxType->oid = sgxTypeOID.release();
    ASN1_ENUMERATED_set(sgxExtensions->sgxType->value, sgxType);

    auto sgxPceId = sgxExtensions->pceid;
    sgxPceId->oid = pceIdOID.release();
    ASN1_OCTET_STRING_set(sgxPceId->value, pceId.data(), static_cast<int>(pceId.size()));

    auto sgxFmspc = sgxExtensions->fmspc;
    sgxFmspc->oid = fmspcOID.release();
    ASN1_OCTET_STRING_set(sgxFmspc->value, fmspc.data(), static_cast<int>(fmspc.size()));

    auto sgxTcbSeq = sgxExtensions->tcb;
    sgxTcbSeq->oid = tcbSequenceOID.release();
    
    auto sgxTcb = sgxTcbSeq->tcb;

    auto sgxPcesvn = sgxTcb->pcesvn;
    sgxPcesvn->oid = tcbPceSvnOID.release();
    ASN1_OCTET_STRING_set(sgxPcesvn->value, pcesvn.data(), static_cast<int>(pcesvn.size()));

    createTcbComp(SGX_TCB_OID_STR, 1, cpusvn[0], sgxTcb->comp01);
    createTcbComp(SGX_TCB_OID_STR, 2, cpusvn[1], sgxTcb->comp02);
    createTcbComp(SGX_TCB_OID_STR, 3, cpusvn[2], sgxTcb->comp03);
    createTcbComp(SGX_TCB_OID_STR, 4, cpusvn[3], sgxTcb->comp04);
    createTcbComp(SGX_TCB_OID_STR, 5, cpusvn[4], sgxTcb->comp05);
    createTcbComp(SGX_TCB_OID_STR, 6, cpusvn[5], sgxTcb->comp06);
    createTcbComp(SGX_TCB_OID_STR, 7, cpusvn[6], sgxTcb->comp07);
    createTcbComp(SGX_TCB_OID_STR, 8, cpusvn[7], sgxTcb->comp08);
    createTcbComp(SGX_TCB_OID_STR, 9, cpusvn[8], sgxTcb->comp09);
    createTcbComp(SGX_TCB_OID_STR, 10, cpusvn[9], sgxTcb->comp10);
    createTcbComp(SGX_TCB_OID_STR, 11, cpusvn[10], sgxTcb->comp11);
    createTcbComp(SGX_TCB_OID_STR, 12, cpusvn[11], sgxTcb->comp12);
    createTcbComp(SGX_TCB_OID_STR, 13, cpusvn[12], sgxTcb->comp13);
    createTcbComp(SGX_TCB_OID_STR, 14, cpusvn[13], sgxTcb->comp14);
    createTcbComp(SGX_TCB_OID_STR, 15, cpusvn[14], sgxTcb->comp15);
    createTcbComp(SGX_TCB_OID_STR, 16, cpusvn[15], sgxTcb->comp16);

    auto sgxCpusvn = sgxTcb->cpusvn;
    sgxCpusvn->oid = tcbCpuSvnOID.release();
    ASN1_OCTET_STRING_set(sgxCpusvn->value, cpusvn.data(), static_cast<int>(cpusvn.size()));

    if (!platformInstanceId.empty())
    {
        auto sgxPlatformInstanceId = crypto::make_unique(crypto::SGX_OCTET_STRING_new());
        sgxPlatformInstanceId->oid = platformInstanceIDOID.release();
        ASN1_OCTET_STRING_set(sgxPlatformInstanceId->value, platformInstanceId.data(),
                              static_cast<int>(platformInstanceId.size()));
        sgxExtensions->platformInstanceId = sgxPlatformInstanceId.release();
    }

    if (dynamicPlatform || cachedKeys || smtEnabled)
    {
        auto sgxConfigurationSeq = crypto::make_unique(crypto::SGX_CONFIGURATION_SEQ_new());
        sgxConfigurationSeq->oid = configurationOID.release();

        auto sgxConfiguration = sgxConfigurationSeq->configuration;

        auto sgxDynamicPlatform = crypto::make_unique(crypto::SGX_BOOL_new());
        sgxDynamicPlatform->oid = dynamicPlatformOID.release();
        sgxDynamicPlatform->value = reinterpret_cast<ASN1_BOOLEAN *>(dynamicPlatform);
        sgxConfiguration->dynamicPlatform = sgxDynamicPlatform.release();

        auto sgxCachedKeys = crypto::make_unique(crypto::SGX_BOOL_new());
        sgxCachedKeys->oid = cachedKeysOID.release();
        sgxCachedKeys->value = reinterpret_cast<ASN1_BOOLEAN *>(cachedKeys);
        sgxConfiguration->cachedKeys = sgxCachedKeys.release();

        auto sgxSmtEnabled = crypto::make_unique(crypto::SGX_BOOL_new());
        sgxSmtEnabled->oid = smtEnabledOID.release();
        sgxSmtEnabled->value = reinterpret_cast<ASN1_BOOLEAN *>(smtEnabled);
        sgxConfiguration->smtEnabled = sgxSmtEnabled.release();

        sgxExtensions->configuration = sgxConfigurationSeq.release();
    }

    if(unexpectedExtension)
    {
        auto sgxUnexpectedExtension = crypto::make_unique(crypto::SGX_BOOL_new());
        sgxUnexpectedExtension->oid = unexpectedExtensionOID.release();
        sgxUnexpectedExtension->value = reinterpret_cast<ASN1_BOOLEAN *>(sgxUnexpectedExtension ? &sgxUnexpectedExtension : nullptr);
        sgxExtensions->unexpectedExtension = sgxUnexpectedExtension.release();
    }

    auto sgxExtensionsOctet = crypto::make_unique(ASN1_OCTET_STRING_new());
    sgxExtensionsOctet->length =
        ASN1_item_i2d((ASN1_VALUE *)sgxExtensions.get(), &sgxExtensionsOctet->data, ASN1_ITEM_rptr(crypto::SGX_EXTENSIONS));

    auto sgxExtension = crypto::make_unique(X509_EXTENSION_create_by_OBJ(nullptr, mainSequenceOID.get(), 1, sgxExtensionsOctet.get()));
    const int atPosition = 1;
    X509_add_ext(cert.get(), sgxExtension.get(), atPosition);
}

void X509CertGenerator::signCert(const crypto::X509_uptr& cert, const EVP_PKEY *signingKey) const
{
    auto* evpSigningKey = const_cast<EVP_PKEY*>(signingKey);
    X509_sign(cert.get(), evpSigningKey, EVP_sha256());
}

crypto::EVP_PKEY_uptr X509CertGenerator::generateEcKeypair() const
{
    crypto::EVP_PKEY_uptr evpKey = crypto::make_unique<EVP_PKEY>(EVP_PKEY_new());

    int ecGroup = OBJ_txt2nid("prime256v1");
    crypto::EC_KEY_uptr ecKey = crypto::make_unique<EC_KEY>(EC_KEY_new_by_curve_name(ecGroup));
    EC_KEY_set_asn1_flag(ecKey.get(), OPENSSL_EC_NAMED_CURVE);
	if (1 != EC_KEY_generate_key(ecKey.get()))
	{
        throw std::runtime_error("Error generating the ECC key.");
    }
    if (1 != EVP_PKEY_set1_EC_KEY(evpKey.get(), ecKey.get()))
    {
        throw std::runtime_error("Error assigning ECC key to EVP_PKEY structure.");
    }
    return evpKey;
}

void X509CertGenerator::add_ext(X509 *cert, int nid, char *value) const
{
	X509V3_CTX ctx;
	X509V3_set_ctx_nodb(&ctx);
	X509V3_set_ctx(&ctx, cert, cert, nullptr, nullptr, 0);
    crypto::X509_EXTENSION_uptr ext = crypto::make_unique<X509_EXTENSION>(X509V3_EXT_conf_nid(nullptr, &ctx, nid, value));
    const int atPosition = -1;
    X509_add_ext(cert, ext.get(), atPosition);
}

std::string X509CertGenerator::x509ToString(const X509 *cert)
{
    if (nullptr == cert)
    {
        return "";
    }
    auto certVar = const_cast<X509*>(cert);

    auto bio = crypto::make_unique<BIO>(BIO_new(BIO_s_mem()));
    if (!bio)
    {
        return "";
    }

    if (0 == PEM_write_bio_X509(bio.get(), certVar))
    {
        return "";
    }

    char *dataStart;
    const long nameLength = BIO_get_mem_data(bio.get(), &dataStart);

    if(nameLength <= 0)
    {
        return "";
    }

    std::string ret;
    std::copy_n(dataStart, nameLength, std::back_inserter(ret));
    return ret;
}

}}}}}
