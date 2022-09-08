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

#include <SgxEcdsaAttestation/AttestationParsers.h>

#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>

using namespace intel::sgx::dcap::parser;
using namespace ::testing;


struct ParseX509CertificateIT: public testing::Test {};

// Parsing certificate issued by Processor CA (SgxType::Standard)

std::string ProcessorPEM = "-----BEGIN CERTIFICATE-----\n"
                "MIIEjDCCBDKgAwIBAgIVALonBDd14S/1zfdU+ZtfOsI+ngLVMAoGCCqGSM49BAMC\n"
                "MHExIzAhBgNVBAMMGkludGVsIFNHWCBQQ0sgUHJvY2Vzc29yIENBMRowGAYDVQQK\n"
                "DBFJbnRlbCBDb3Jwb3JhdGlvbjEUMBIGA1UEBwwLU2FudGEgQ2xhcmExCzAJBgNV\n"
                "BAgMAkNBMQswCQYDVQQGEwJVUzAeFw0xOTA5MDUwNzQ3MDZaFw0yNjA5MDUwNzQ3\n"
                "MDZaMHAxIjAgBgNVBAMMGUludGVsIFNHWCBQQ0sgQ2VydGlmaWNhdGUxGjAYBgNV\n"
                "BAoMEUludGVsIENvcnBvcmF0aW9uMRQwEgYDVQQHDAtTYW50YSBDbGFyYTELMAkG\n"
                "A1UECAwCQ0ExCzAJBgNVBAYTAlVTMFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAE\n"
                "ZbnHpHbJ8kqQgySEX0M+qzcWpLAj6RcLi7vYKWqtityqaCWciAXHxlJvCJ1Kr35Y\n"
                "mWlpekwiEjo+XlEmg+NQVKOCAqYwggKiMB8GA1UdIwQYMBaAFJ8Gl+9TIUTU+kx+\n"
                "6LqNs9Ml5JKQMGsGA1UdHwRkMGIwYKBeoFyGWmh0dHBzOi8vZmFrZS1jcmwtZGlz\n"
                "dHJpYnV0aW9uLXBvaW50LXVybC5pbnRlbC5jb20vc2d4L2NlcnRpZmljYXRpb24v\n"
                "djIvcGNrY3JsP2NhPXByb2Nlc3NvcjAdBgNVHQ4EFgQUEULjJXxk96LC2FJd13qm\n"
                "5pKzckEwDgYDVR0PAQH/BAQDAgbAMAwGA1UdEwEB/wQCMAAwggHTBgkqhkiG+E0B\n"
                "DQEEggHEMIIBwDAeBgoqhkiG+E0BDQEBBBCEYYY7t+zjH2TI6bd/0xytMIIBYwYK\n"
                "KoZIhvhNAQ0BAjCCAVMwEAYLKoZIhvhNAQ0BAgECAQAwEAYLKoZIhvhNAQ0BAgIC\n"
                "AQMwEAYLKoZIhvhNAQ0BAgMCAQAwEAYLKoZIhvhNAQ0BAgQCAQAwEAYLKoZIhvhN\n"
                "AQ0BAgUCAQAwEAYLKoZIhvhNAQ0BAgYCAQEwEAYLKoZIhvhNAQ0BAgcCAQEwEAYL\n"
                "KoZIhvhNAQ0BAggCAQAwEAYLKoZIhvhNAQ0BAgkCAQAwEAYLKoZIhvhNAQ0BAgoC\n"
                "AQAwEAYLKoZIhvhNAQ0BAgsCAQAwEAYLKoZIhvhNAQ0BAgwCAQAwEAYLKoZIhvhN\n"
                "AQ0BAg0CAQAwEAYLKoZIhvhNAQ0BAg4CAQAwEAYLKoZIhvhNAQ0BAg8CAQAwEAYL\n"
                "KoZIhvhNAQ0BAhACAQAwEAYLKoZIhvhNAQ0BAhECAQMwHwYLKoZIhvhNAQ0BAhIE\n"
                "EAADAAAAAQEAAAAAAAAAAAAwEAYKKoZIhvhNAQ0BAwQCAAAwFAYKKoZIhvhNAQ0B\n"
                "BAQGAHB/AAAAMA8GCiqGSIb4TQENAQUKAQAwCgYIKoZIzj0EAwIDSAAwRQIhANmr\n"
                "mwJgah3SFMDCv7/JvCW8GsB0fIuhbHQtXRO0KN0WAiAsAY5USoy5uk0B7/sVEvng\n"
                "ILOJfSqEZlN7hTCJpjcEgw==\n"
                "-----END CERTIFICATE-----";

struct ExpectedProcessorPckCertData
{
    /// Example values for subject
    const std::string RAW_SUBJECT = "C=US,ST=CA,L=Santa Clara,O=Intel Corporation,CN=Intel SGX PCK Certificate";
    const std::string COMMON_NAME_ISSUER ="Intel SGX PCK Certificate";
    const std::string COUNTRY_NAME_ISSUER = "US";
    const std::string ORGANIZATION_NAME_ISSUER = "Intel Corporation";
    const std::string LOCATION_NAME_ISSUER = "Santa Clara";
    const std::string STATE_NAME_ISSUER = "CA";

    /// Example values for issuer
    const std::string RAW_ISSUER = "C=US,ST=CA,L=Santa Clara,O=Intel Corporation,CN=Intel SGX PCK Processor CA";
    const std::string COMMON_NAME_SUBJECT = "Intel SGX PCK Processor CA";
    const std::string COUNTRY_NAME_SUBJECT = "US";
    const std::string ORGANIZATION_NAME_SUBJECT = "Intel Corporation";
    const std::string LOCATION_NAME_SUBJECT = "Santa Clara";
    const std::string STATE_NAME_SUBJECT = "CA";

    const std::vector<uint8_t> RAW_DER = { 0x30, 0x45, 0x02, 0x21, 0x00, // header
                                           0xD9, 0xAB, 0x9B, 0x02, 0x60, 0x6A, 0x1D, 0xD2, // R
                                           0x14, 0xC0, 0xC2, 0xBF, 0xBF, 0xC9, 0xBC, 0x25,
                                           0xBC, 0x1A, 0xC0, 0x74, 0x7C, 0x8B, 0xA1, 0x6C,
                                           0x74, 0x2D, 0x5D, 0x13, 0xB4, 0x28, 0xDD, 0x16,
                                           0x02, 0x20, // marker
                                           0x2C, 0x01, 0x8E, 0x54, 0x4A, 0x8C, 0xB9, 0xBA, // S
                                           0x4D, 0x01, 0xEF, 0xFB, 0x15, 0x12, 0xF9, 0xE0,
                                           0x20, 0xB3, 0x89, 0x7D, 0x2A, 0x84, 0x66, 0x53,
                                           0x7B, 0x85, 0x30, 0x89, 0xA6, 0x37, 0x04, 0x83 };
    const std::vector<uint8_t> R = { 0xD9, 0xAB, 0x9B, 0x02, 0x60, 0x6A, 0x1D, 0xD2,
                                     0x14, 0xC0, 0xC2, 0xBF, 0xBF, 0xC9, 0xBC, 0x25,
                                     0xBC, 0x1A, 0xC0, 0x74, 0x7C, 0x8B, 0xA1, 0x6C,
                                     0x74, 0x2D, 0x5D, 0x13, 0xB4, 0x28, 0xDD, 0x16 };
    const std::vector<uint8_t> S = { 0x2C, 0x01, 0x8E, 0x54, 0x4A, 0x8C, 0xB9, 0xBA,
                                     0x4D, 0x01, 0xEF, 0xFB, 0x15, 0x12, 0xF9, 0xE0,
                                     0x20, 0xB3, 0x89, 0x7D, 0x2A, 0x84, 0x66, 0x53,
                                     0x7B, 0x85, 0x30, 0x89, 0xA6, 0x37, 0x04, 0x83 };

    const std::time_t NOT_BEFORE_TIME = 1567669626;
    const std::time_t NOT_AFTER_TIME = 1788594426;

    const std::vector<uint8_t> SERIAL_NUMBER { 0xBA, 0x27, 0x04, 0x37, 0x75, 0xE1, 0x2F, 0xF5, 0xCD, 0xF7,
                                               0x54, 0xF9, 0x9B, 0x5F, 0x3A, 0xC2, 0x3E, 0x9E, 0x02, 0xD5 };

    const std::vector<uint8_t> PUBLIC_KEY { 04, // header
                                            0x65, 0xB9, 0xC7, 0xA4, 0x76, 0xC9, 0xF2, 0x4A, // X
                                            0x90, 0x83, 0x24, 0x84, 0x5F, 0x43, 0x3E, 0xAB,
                                            0x37, 0x16, 0xA4, 0xB0, 0x23, 0xE9, 0x17, 0x0B,
                                            0x8B, 0xBB, 0xD8, 0x29, 0x6A, 0xAD, 0x8A, 0xDC,
                                            0xAA, 0x68, 0x25, 0x9C, 0x88, 0x05, 0xC7, 0xC6, // Y
                                            0x52, 0x6F, 0x08, 0x9D, 0x4A, 0xAF, 0x7E, 0x58,
                                            0x99, 0x69, 0x69, 0x7A, 0x4C, 0x22, 0x12, 0x3A,
                                            0x3E, 0x5E, 0x51, 0x26, 0x83, 0xE3, 0x50, 0x54 };

    const std::vector<uint8_t> CPUSVN { 0x00, 0x03, 0x00, 0x00, 0x00, 0x01, 0x01, 0x00,
                                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

    const uint32_t PCESVN = 3;

    const std::vector<uint8_t> PPID { 0x84, 0x61, 0x86, 0x3B, 0xB7, 0xEC, 0xE3, 0x1F,
                                      0x64, 0xC8, 0xE9, 0xB7, 0x7F, 0xD3, 0x1C, 0xAD };

    const std::vector<uint8_t> PCE_ID { 0x00, 0x00 };

    const std::vector<uint8_t> FMSPC { 0x00, 0x70, 0x7F, 0x00, 0x00, 0x00 };

    const x509::SgxType SGX_TYPE = x509::SgxType::Standard;
};

TEST_F(ParseX509CertificateIT, parsePemEncodedProcessorCAPckCert)
{
    // GIVEN
    const auto expected = ExpectedProcessorPckCertData();

    // WHEN
    const auto& pckCertificate = x509::PckCertificate::parse(ProcessorPEM);

    // THEN
    ASSERT_EQ(pckCertificate.getVersion(), 3);
    ASSERT_THAT(pckCertificate.getSerialNumber(), ElementsAreArray(expected.SERIAL_NUMBER));
    ASSERT_THAT(pckCertificate.getPubKey(), ElementsAreArray(expected.PUBLIC_KEY));

    const auto& subject = pckCertificate.getSubject();
    ASSERT_EQ(subject.getRaw(), expected.RAW_SUBJECT);
    ASSERT_EQ(subject.getCommonName(), expected.COMMON_NAME_ISSUER);
    ASSERT_EQ(subject.getCountryName(), expected.COUNTRY_NAME_ISSUER);
    ASSERT_EQ(subject.getOrganizationName(), expected.ORGANIZATION_NAME_ISSUER);
    ASSERT_EQ(subject.getLocationName(), expected.LOCATION_NAME_ISSUER);
    ASSERT_EQ(subject.getStateName(), expected.STATE_NAME_ISSUER);

    const auto& issuer = pckCertificate.getIssuer();
    ASSERT_EQ(issuer.getRaw(), expected.RAW_ISSUER);
    ASSERT_EQ(issuer.getCommonName(), expected.COMMON_NAME_SUBJECT);
    ASSERT_EQ(issuer.getCountryName(), expected.COUNTRY_NAME_SUBJECT);
    ASSERT_EQ(issuer.getOrganizationName(), expected.ORGANIZATION_NAME_SUBJECT);
    ASSERT_EQ(issuer.getLocationName(), expected.LOCATION_NAME_SUBJECT);
    ASSERT_EQ(issuer.getStateName(), expected.STATE_NAME_SUBJECT);

    ASSERT_NE(issuer, subject); // they may be equal if certificate is self-signed

    const auto& validity = pckCertificate.getValidity();
    ASSERT_EQ(validity.getNotBeforeTime(), expected.NOT_BEFORE_TIME);
    ASSERT_EQ(validity.getNotAfterTime(), expected.NOT_AFTER_TIME);

    const auto& signature = pckCertificate.getSignature();
    ASSERT_THAT(signature.getRawDer(), ElementsAreArray(expected.RAW_DER));
    ASSERT_THAT(signature.getR(), ElementsAreArray(expected.R));
    ASSERT_THAT(signature.getS(), ElementsAreArray(expected.S));

    const auto& ppid = pckCertificate.getPpid();
    ASSERT_THAT(ppid, ElementsAreArray(expected.PPID));

    const auto& tcb = pckCertificate.getTcb();
    ASSERT_THAT(tcb.getCpuSvn(), ElementsAreArray(expected.CPUSVN));
    ASSERT_THAT(tcb.getSgxTcbComponents(), ElementsAreArray(expected.CPUSVN));
    ASSERT_EQ(tcb.getPceSvn(), expected.PCESVN);
    ASSERT_EQ(tcb, x509::Tcb(expected.CPUSVN, expected.CPUSVN, expected.PCESVN));

    const auto& pceId = pckCertificate.getPceId();
    ASSERT_THAT(pceId, ElementsAreArray(expected.PCE_ID));

    const auto& fmspc = pckCertificate.getFmspc();
    ASSERT_THAT(fmspc, ElementsAreArray(expected.FMSPC));

    const auto& sgxType = pckCertificate.getSgxType();
    ASSERT_EQ(sgxType, expected.SGX_TYPE);
}

TEST_F(ParseX509CertificateIT, parsePemEncodedProcessorCAPckCertAsProcessorPckCertificateClass)
{
    // GIVEN
    const auto expected = ExpectedProcessorPckCertData();

    // WHEN
    const auto& pckCertificate = x509::ProcessorPckCertificate::parse(ProcessorPEM);

    // THEN
    ASSERT_EQ(pckCertificate.getVersion(), 3);
    ASSERT_THAT(pckCertificate.getSerialNumber(), ElementsAreArray(expected.SERIAL_NUMBER));
    ASSERT_THAT(pckCertificate.getPubKey(), ElementsAreArray(expected.PUBLIC_KEY));

    const auto& subject = pckCertificate.getSubject();
    ASSERT_EQ(subject.getRaw(), expected.RAW_SUBJECT);
    ASSERT_EQ(subject.getCommonName(), expected.COMMON_NAME_ISSUER);
    ASSERT_EQ(subject.getCountryName(), expected.COUNTRY_NAME_ISSUER);
    ASSERT_EQ(subject.getOrganizationName(), expected.ORGANIZATION_NAME_ISSUER);
    ASSERT_EQ(subject.getLocationName(), expected.LOCATION_NAME_ISSUER);
    ASSERT_EQ(subject.getStateName(), expected.STATE_NAME_ISSUER);

    const auto& issuer = pckCertificate.getIssuer();
    ASSERT_EQ(issuer.getRaw(), expected.RAW_ISSUER);
    ASSERT_EQ(issuer.getCommonName(), expected.COMMON_NAME_SUBJECT);
    ASSERT_EQ(issuer.getCountryName(), expected.COUNTRY_NAME_SUBJECT);
    ASSERT_EQ(issuer.getOrganizationName(), expected.ORGANIZATION_NAME_SUBJECT);
    ASSERT_EQ(issuer.getLocationName(), expected.LOCATION_NAME_SUBJECT);
    ASSERT_EQ(issuer.getStateName(), expected.STATE_NAME_SUBJECT);

    ASSERT_NE(issuer, subject); // they may be equal if certificate is self-signed

    const auto& validity = pckCertificate.getValidity();
    ASSERT_EQ(validity.getNotBeforeTime(), expected.NOT_BEFORE_TIME);
    ASSERT_EQ(validity.getNotAfterTime(), expected.NOT_AFTER_TIME);

    const auto& signature = pckCertificate.getSignature();
    ASSERT_THAT(signature.getRawDer(), ElementsAreArray(expected.RAW_DER));
    ASSERT_THAT(signature.getR(), ElementsAreArray(expected.R));
    ASSERT_THAT(signature.getS(), ElementsAreArray(expected.S));

    const auto& ppid = pckCertificate.getPpid();
    ASSERT_THAT(ppid, ElementsAreArray(expected.PPID));

    const auto& tcb = pckCertificate.getTcb();
    ASSERT_THAT(tcb.getCpuSvn(), ElementsAreArray(expected.CPUSVN));
    ASSERT_THAT(tcb.getSgxTcbComponents(), ElementsAreArray(expected.CPUSVN));
    ASSERT_EQ(tcb.getPceSvn(), expected.PCESVN);
    ASSERT_EQ(tcb, x509::Tcb(expected.CPUSVN, expected.CPUSVN, expected.PCESVN));

    const auto& pceId = pckCertificate.getPceId();
    ASSERT_THAT(pceId, ElementsAreArray(expected.PCE_ID));

    const auto& fmspc = pckCertificate.getFmspc();
    ASSERT_THAT(fmspc, ElementsAreArray(expected.FMSPC));

    const auto& sgxType = pckCertificate.getSgxType();
    ASSERT_EQ(sgxType, expected.SGX_TYPE);
}

// Parsing certificate issued by Platform CA (SgxType::Scalable)

std::string PlatformPEM =
        "-----BEGIN CERTIFICATE-----\n"
        "MIIE8jCCBJigAwIBAgIVAL2ntN6oDXniW91PanIORAaYoA12MAoGCCqGSM49BAMCMHAxIjAgBgNV\n"
        "BAMMGUludGVsIFNHWCBQQ0sgUGxhdGZvcm0gQ0ExGjAYBgNVBAoMEUludGVsIENvcnBvcmF0aW9u\n"
        "MRQwEgYDVQQHDAtTYW50YSBDbGFyYTELMAkGA1UECAwCQ0ExCzAJBgNVBAYTAlVTMB4XDTIwMDMx\n"
        "ODA5MzA0OVoXDTI3MDMxODA5MzA0OVowcDEiMCAGA1UEAwwZSW50ZWwgU0dYIFBDSyBDZXJ0aWZp\n"
        "Y2F0ZTEaMBgGA1UECgwRSW50ZWwgQ29ycG9yYXRpb24xFDASBgNVBAcMC1NhbnRhIENsYXJhMQsw\n"
        "CQYDVQQIDAJDQTELMAkGA1UEBhMCVVMwWTATBgcqhkjOPQIBBggqhkjOPQMBBwNCAASF0WfF3Bu6\n"
        "Ds3AAqfzKp4te+4FWvddvTVr5uHdszbvloIOTOgq6HIob0D/QZtyeeUBOKOM1Dq3mi1DWgAbVdkL\n"
        "o4IDDTCCAwkwHwYDVR0jBBgwFoAU7bmCA3TzblbsRZSTub7BGnDEPbQwYgYDVR0fBFswWTBXoFWg\n"
        "U4ZRaHR0cHM6Ly9wcmUxMy1ncmVlbi1wY3Muc2d4bnAuYWRzZGNzcC5jb20vc2d4L2NlcnRpZmlj\n"
        "YXRpb24vdjEvcGNrY3JsP2NhPXBsYXRmb3JtMB0GA1UdDgQWBBR3DHSU0wR85z1ekrDGwC8ckFB3\n"
        "WjAOBgNVHQ8BAf8EBAMCBsAwDAYDVR0TAQH/BAIwADCCAkMGCSqGSIb4TQENAQSCAjQwggIwMB4G\n"
        "CiqGSIb4TQENAQEEEDcDl5X+CwarSOOPiYq5LO8wggFtBgoqhkiG+E0BDQECMIIBXTAQBgsqhkiG\n"
        "+E0BDQECAQIBADAQBgsqhkiG+E0BDQECAgIBQzARBgsqhkiG+E0BDQECAwICAMcwEAYLKoZIhvhN\n"
        "AQ0BAgQCAVwwEQYLKoZIhvhNAQ0BAgUCAgDXMBEGCyqGSIb4TQENAQIGAgIAtDAQBgsqhkiG+E0B\n"
        "DQECBwIBOzARBgsqhkiG+E0BDQECCAICAK0wEAYLKoZIhvhNAQ0BAgkCAU4wEQYLKoZIhvhNAQ0B\n"
        "AgoCAgC/MBEGCyqGSIb4TQENAQILAgIAvjAQBgsqhkiG+E0BDQECDAIBQzARBgsqhkiG+E0BDQEC\n"
        "DQICANUwEAYLKoZIhvhNAQ0BAg4CAV8wEAYLKoZIhvhNAQ0BAg8CATQwEQYLKoZIhvhNAQ0BAhAC\n"
        "AgClMBIGCyqGSIb4TQENAQIRAgMA2GQwHwYLKoZIhvhNAQ0BAhIEEABDx1zXtDutTr++Q9VfNKUw\n"
        "EAYKKoZIhvhNAQ0BAwQCAAAwFAYKKoZIhvhNAQ0BBAQGEEdcDQAAMA8GCiqGSIb4TQENAQUKAQEw\n"
        "HgYKKoZIhvhNAQ0BBgQQ80TO9xJaqXgYpFUT9hf90jBEBgoqhkiG+E0BDQEHMDYwEAYLKoZIhvhN\n"
        "AQ0BBwEBAf8wEAYLKoZIhvhNAQ0BBwIBAf8wEAYLKoZIhvhNAQ0BBwMBAQAwCgYIKoZIzj0EAwID\n"
        "SAAwRQIgIwVrUZy1o//8kKk3NWmz5VhH9ppS+tzIbRdckz8psoMCIQD+pl4qgmoUsoz83e8HfRSR\n"
        "t1Uz20ThyeYg0+3EEXn5OA==\n"
        "-----END CERTIFICATE-----";

struct ExpectedPlatformPckCertData
{
    /// Example values for subject
    const std::string RAW_SUBJECT = "C=US,ST=CA,L=Santa Clara,O=Intel Corporation,CN=Intel SGX PCK Certificate";
    const std::string COMMON_NAME_ISSUER = "Intel SGX PCK Certificate";
    const std::string COUNTRY_NAME_ISSUER = "US";
    const std::string ORGANIZATION_NAME_ISSUER = "Intel Corporation";
    const std::string LOCATION_NAME_ISSUER = "Santa Clara";
    const std::string STATE_NAME_ISSUER = "CA";

    /// Example values for issuer
    const std::string RAW_ISSUER = "C=US,ST=CA,L=Santa Clara,O=Intel Corporation,CN=Intel SGX PCK Platform CA";
    const std::string COMMON_NAME_SUBJECT = "Intel SGX PCK Platform CA";
    const std::string COUNTRY_NAME_SUBJECT = "US";
    const std::string ORGANIZATION_NAME_SUBJECT = "Intel Corporation";
    const std::string LOCATION_NAME_SUBJECT = "Santa Clara";
    const std::string STATE_NAME_SUBJECT = "CA";

    const std::vector<uint8_t> RAW_DER = { 0x30, 0x45, 0x02, 0x20, // header
                                           0x23, 0x05, 0x6B, 0x51, 0x9C, 0xB5, 0xA3, 0xFF, // R
                                           0xFC, 0x90, 0xA9, 0x37, 0x35, 0x69, 0xB3, 0xE5,
                                           0x58, 0x47, 0xF6, 0x9A, 0x52, 0xFA, 0xDC, 0xC8,
                                           0x6D, 0x17, 0x5C, 0x93, 0x3F, 0x29, 0xB2, 0x83,
                                           0x02, 0x21, 0x00, // marker
                                           0xFE, 0xA6, 0x5E, 0x2A, 0x82, 0x6A, 0x14, 0xB2, // S
                                           0x8C, 0xFC, 0xDD, 0xEF, 0x07, 0x7D, 0x14, 0x91,
                                           0xB7, 0x55, 0x33, 0xDB, 0x44, 0xE1, 0xC9, 0xE6,
                                           0x20, 0xD3, 0xED, 0xC4, 0x11, 0x79, 0xF9, 0x38 };
    const std::vector<uint8_t> R = { 0x23, 0x05, 0x6B, 0x51, 0x9C, 0xB5, 0xA3, 0xFF,
                                     0xFC, 0x90, 0xA9, 0x37, 0x35, 0x69, 0xB3, 0xE5,
                                     0x58, 0x47, 0xF6, 0x9A, 0x52, 0xFA, 0xDC, 0xC8,
                                     0x6D, 0x17, 0x5C, 0x93, 0x3F, 0x29, 0xB2, 0x83 };
    const std::vector<uint8_t> S = { 0xFE, 0xA6, 0x5E, 0x2A, 0x82, 0x6A, 0x14, 0xB2,
                                     0x8C, 0xFC, 0xDD, 0xEF, 0x07, 0x7D, 0x14, 0x91,
                                     0xB7, 0x55, 0x33, 0xDB, 0x44, 0xE1, 0xC9, 0xE6,
                                     0x20, 0xD3, 0xED, 0xC4, 0x11, 0x79, 0xF9, 0x38 };

    const std::time_t NOT_BEFORE_TIME = 1584523849;
    const std::time_t NOT_AFTER_TIME = 1805362249;

    const std::vector<uint8_t> SERIAL_NUMBER { 0xBD, 0xA7, 0xB4, 0xDE, 0xA8, 0x0D, 0x79, 0xE2, 0x5B, 0xDD,
                                               0x4F, 0x6A, 0x72, 0x0E, 0x44, 0x06, 0x98, 0xA0, 0x0D, 0x76 };

    const std::vector<uint8_t> PUBLIC_KEY { 0x04, // header
                                            0x85, 0xD1, 0x67, 0xC5, 0xDC, 0x1B, 0xBA, 0x0E, // X
                                            0xCD, 0xC0, 0x02, 0xA7, 0xF3, 0x2A, 0x9E, 0x2D,
                                            0x7B, 0xEE, 0x05, 0x5A, 0xF7, 0x5D, 0xBD, 0x35,
                                            0x6B, 0xE6, 0xE1, 0xDD, 0xB3, 0x36, 0xEF, 0x96,
                                            0x82, 0x0E, 0x4C, 0xE8, 0x2A, 0xE8, 0x72, 0x28, // Y
                                            0x6F, 0x40, 0xFF, 0x41, 0x9B, 0x72, 0x79, 0xE5,
                                            0x01, 0x38, 0xA3, 0x8C, 0xD4, 0x3A, 0xB7, 0x9A,
                                            0x2D, 0x43, 0x5A, 0x00, 0x1B, 0x55, 0xD9, 0x0B };

    const std::vector<uint8_t> PPID { 0x37, 0x03, 0x97, 0x95, 0xFE, 0x0B, 0x06, 0xAB,
                                      0x48, 0xE3, 0x8F, 0x89, 0x8A, 0xB9, 0x2C, 0xEF };

    const std::vector<uint8_t> CPUSVN { 0x00, 0x43, 0xC7, 0x5C, 0xD7, 0xB4, 0x3B, 0xAD,
                                        0x4E, 0xBF, 0xBE, 0x43, 0xD5, 0x5F, 0x34, 0xA5 };

    const uint32_t PCESVN = 55396;

    const std::vector<uint8_t> PCE_ID { 0x00, 0x00 };

    const std::vector<uint8_t> FMSPC { 0x10, 0x47, 0x5C, 0x0D, 0x00, 0x00 };

    const x509::SgxType SGX_TYPE = x509::SgxType::Scalable;

    const std::vector<uint8_t> PLATFORM_INSTANCE_ID { 0xF3, 0x44, 0xCE, 0xF7, 0x12, 0x5A, 0xA9, 0x78,
                                                      0x18, 0xA4, 0x55, 0x13, 0xF6, 0x17, 0xFD, 0xD2 };
};

TEST_F(ParseX509CertificateIT, parsePemEncodedPlatformCAPckCert)
{
    // GIVEN
    const auto expected = ExpectedPlatformPckCertData();

    // WHEN
    const auto& pckCertificate = x509::PckCertificate::parse(PlatformPEM);

    // THEN
    ASSERT_EQ(pckCertificate.getVersion(), 3);

    ASSERT_THAT(pckCertificate.getSerialNumber(), ElementsAreArray(expected.SERIAL_NUMBER));
    ASSERT_THAT(pckCertificate.getPubKey(), ElementsAreArray(expected.PUBLIC_KEY));

    const auto& subject = pckCertificate.getSubject();
    ASSERT_EQ(subject.getRaw(), expected.RAW_SUBJECT);
    ASSERT_EQ(subject.getCommonName(), expected.COMMON_NAME_ISSUER);
    ASSERT_EQ(subject.getCountryName(), expected.COUNTRY_NAME_ISSUER);
    ASSERT_EQ(subject.getOrganizationName(), expected.ORGANIZATION_NAME_ISSUER);
    ASSERT_EQ(subject.getLocationName(), expected.LOCATION_NAME_ISSUER);
    ASSERT_EQ(subject.getStateName(), expected.STATE_NAME_ISSUER);

    const auto& issuer = pckCertificate.getIssuer();
    ASSERT_EQ(issuer.getRaw(), expected.RAW_ISSUER);
    ASSERT_EQ(issuer.getCommonName(), expected.COMMON_NAME_SUBJECT);
    ASSERT_EQ(issuer.getCountryName(), expected.COUNTRY_NAME_SUBJECT);
    ASSERT_EQ(issuer.getOrganizationName(), expected.ORGANIZATION_NAME_SUBJECT);
    ASSERT_EQ(issuer.getLocationName(), expected.LOCATION_NAME_SUBJECT);
    ASSERT_EQ(issuer.getStateName(), expected.STATE_NAME_SUBJECT);

    ASSERT_NE(issuer, subject); // they may be equal if certificate is self-signed

    const auto& validity = pckCertificate.getValidity();
    ASSERT_EQ(validity.getNotBeforeTime(), expected.NOT_BEFORE_TIME);
    ASSERT_EQ(validity.getNotAfterTime(), expected.NOT_AFTER_TIME);

    const auto& signature = pckCertificate.getSignature();
    ASSERT_THAT(signature.getRawDer(), ElementsAreArray(expected.RAW_DER));
    ASSERT_THAT(signature.getR(), ElementsAreArray(expected.R));
    ASSERT_THAT(signature.getS(), ElementsAreArray(expected.S));

    const auto& ppid = pckCertificate.getPpid();
    ASSERT_THAT(ppid, ElementsAreArray(expected.PPID));

    const auto& tcb = pckCertificate.getTcb();
    ASSERT_THAT(tcb.getCpuSvn(), ElementsAreArray(expected.CPUSVN));
    ASSERT_THAT(tcb.getSgxTcbComponents(), ElementsAreArray(expected.CPUSVN));
    ASSERT_EQ(tcb.getPceSvn(), expected.PCESVN);
    ASSERT_EQ(tcb, x509::Tcb(expected.CPUSVN, expected.CPUSVN, expected.PCESVN));

    const auto& pceId = pckCertificate.getPceId();
    ASSERT_THAT(pceId, ElementsAreArray(expected.PCE_ID));

    const auto& fmspc = pckCertificate.getFmspc();
    ASSERT_THAT(fmspc, ElementsAreArray(expected.FMSPC));

    const auto& sgxType = pckCertificate.getSgxType();
    ASSERT_EQ(sgxType, expected.SGX_TYPE);
}

TEST_F(ParseX509CertificateIT, parsePemEncodedPlatformCAPckCertAsPlatformPckCertificateClass)
{
    // GIVEN
    const auto expected = ExpectedPlatformPckCertData();

    // WHEN
    const auto& pckCertificate = x509::PlatformPckCertificate::parse(PlatformPEM);

    // THEN
    ASSERT_EQ(pckCertificate.getVersion(), 3);

    ASSERT_THAT(pckCertificate.getSerialNumber(), ElementsAreArray(expected.SERIAL_NUMBER));
    ASSERT_THAT(pckCertificate.getPubKey(), ElementsAreArray(expected.PUBLIC_KEY));

    const auto& subject = pckCertificate.getSubject();
    ASSERT_EQ(subject.getRaw(), expected.RAW_SUBJECT);
    ASSERT_EQ(subject.getCommonName(), expected.COMMON_NAME_ISSUER);
    ASSERT_EQ(subject.getCountryName(), expected.COUNTRY_NAME_ISSUER);
    ASSERT_EQ(subject.getOrganizationName(), expected.ORGANIZATION_NAME_ISSUER);
    ASSERT_EQ(subject.getLocationName(), expected.LOCATION_NAME_ISSUER);
    ASSERT_EQ(subject.getStateName(), expected.STATE_NAME_ISSUER);

    const auto& issuer = pckCertificate.getIssuer();
    ASSERT_EQ(issuer.getRaw(), expected.RAW_ISSUER);
    ASSERT_EQ(issuer.getCommonName(), expected.COMMON_NAME_SUBJECT);
    ASSERT_EQ(issuer.getCountryName(), expected.COUNTRY_NAME_SUBJECT);
    ASSERT_EQ(issuer.getOrganizationName(), expected.ORGANIZATION_NAME_SUBJECT);
    ASSERT_EQ(issuer.getLocationName(), expected.LOCATION_NAME_SUBJECT);
    ASSERT_EQ(issuer.getStateName(), expected.STATE_NAME_SUBJECT);

    ASSERT_NE(issuer, subject); // they may be equal if certificate is self-signed

    const auto& validity = pckCertificate.getValidity();
    ASSERT_EQ(validity.getNotBeforeTime(), expected.NOT_BEFORE_TIME);
    ASSERT_EQ(validity.getNotAfterTime(), expected.NOT_AFTER_TIME);

    const auto& signature = pckCertificate.getSignature();
    ASSERT_THAT(signature.getRawDer(), ElementsAreArray(expected.RAW_DER));
    ASSERT_THAT(signature.getR(), ElementsAreArray(expected.R));
    ASSERT_THAT(signature.getS(), ElementsAreArray(expected.S));

    const auto& ppid = pckCertificate.getPpid();
    ASSERT_THAT(ppid, ElementsAreArray(expected.PPID));

    const auto& tcb = pckCertificate.getTcb();
    ASSERT_THAT(tcb.getCpuSvn(), ElementsAreArray(expected.CPUSVN));
    ASSERT_THAT(tcb.getSgxTcbComponents(), ElementsAreArray(expected.CPUSVN));
    ASSERT_EQ(tcb.getPceSvn(), expected.PCESVN);
    ASSERT_EQ(tcb, x509::Tcb(expected.CPUSVN, expected.CPUSVN, expected.PCESVN));

    const auto& pceId = pckCertificate.getPceId();
    ASSERT_THAT(pceId, ElementsAreArray(expected.PCE_ID));

    const auto& fmspc = pckCertificate.getFmspc();
    ASSERT_THAT(fmspc, ElementsAreArray(expected.FMSPC));

    const auto& sgxType = pckCertificate.getSgxType();
    ASSERT_EQ(sgxType, expected.SGX_TYPE);

    const auto& platformInstanceId = pckCertificate.getPlatformInstanceId();
    ASSERT_THAT(platformInstanceId, ElementsAreArray(expected.PLATFORM_INSTANCE_ID));

    const auto& sgxConfiguration = pckCertificate.getConfiguration();
    ASSERT_TRUE(sgxConfiguration.isCachedKeys());
    ASSERT_TRUE(sgxConfiguration.isDynamicPlatform());
    ASSERT_FALSE(sgxConfiguration.isSmtEnabled());
}

// Parsing certificate issued by Platform CA (SgxType::ScalableWithIntegrity)

std::string PlatformWithIntegrityPEM =
        "-----BEGIN CERTIFICATE-----\n"
        "MIIE8zCCBJmgAwIBAgIVAILO2URCMFbw6xfrP0TTrc679g1zMAoGCCqGSM49BAMCMHAxIjAgBgNV\n"
        "BAMMGUludGVsIFNHWCBQQ0sgUGxhdGZvcm0gQ0ExGjAYBgNVBAoMEUludGVsIENvcnBvcmF0aW9u\n"
        "MRQwEgYDVQQHDAtTYW50YSBDbGFyYTELMAkGA1UECAwCQ0ExCzAJBgNVBAYTAlVTMB4XDTIyMDIx\n"
        "NjEyNDcxMFoXDTI5MDIxNjEyNDcxMFowcDEiMCAGA1UEAwwZSW50ZWwgU0dYIFBDSyBDZXJ0aWZp\n"
        "Y2F0ZTEaMBgGA1UECgwRSW50ZWwgQ29ycG9yYXRpb24xFDASBgNVBAcMC1NhbnRhIENsYXJhMQsw\n"
        "CQYDVQQIDAJDQTELMAkGA1UEBhMCVVMwWTATBgcqhkjOPQIBBggqhkjOPQMBBwNCAAT30CUukJ0l\n"
        "3VCyNNLVptN2xU2bXBCwzNzeZR9jmS2H8W88pGPYsPYv7pnWrhNSotBC+JPA5IJc9TQXjhZjm/6c\n"
        "o4IDDjCCAwowHwYDVR0jBBgwFoAU7bmCA3TzblbsRZSTub7BGnDEPbQwYgYDVR0fBFswWTBXoFWg\n"
        "U4ZRaHR0cHM6Ly9wcmUxMy1ncmVlbi1wY3Muc2d4bnAuYWRzZGNzcC5jb20vc2d4L2NlcnRpZmlj\n"
        "YXRpb24vdjMvcGNrY3JsP2NhPXBsYXRmb3JtMB0GA1UdDgQWBBSyyGVppvpNBzw0bcltaNErdJhP\n"
        "EDAOBgNVHQ8BAf8EBAMCBsAwDAYDVR0TAQH/BAIwADCCAkQGCSqGSIb4TQENAQSCAjUwggIxMB4G\n"
        "CiqGSIb4TQENAQEEEOHh+upTZjEol7iwtEhm/OswggFuBgoqhkiG+E0BDQECMIIBXjAQBgsqhkiG\n"
        "+E0BDQECAQIBAjARBgsqhkiG+E0BDQECAgICAMEwEQYLKoZIhvhNAQ0BAgMCAgDZMBAGCyqGSIb4\n"
        "TQENAQIEAgFsMBEGCyqGSIb4TQENAQIFAgIAsjARBgsqhkiG+E0BDQECBgICALswEQYLKoZIhvhN\n"
        "AQ0BAgcCAgCwMBEGCyqGSIb4TQENAQIIAgIA4zARBgsqhkiG+E0BDQECCQICAIswEAYLKoZIhvhN\n"
        "AQ0BAgoCASQwEAYLKoZIhvhNAQ0BAgsCARowEQYLKoZIhvhNAQ0BAgwCAgCyMBEGCyqGSIb4TQEN\n"
        "AQINAgIA+zAQBgsqhkiG+E0BDQECDgIBUzAQBgsqhkiG+E0BDQECDwIBfzARBgsqhkiG+E0BDQEC\n"
        "EAICALkwEQYLKoZIhvhNAQ0BAhECAmaLMB8GCyqGSIb4TQENAQISBBACwdlssruw44skGrL7U3+5\n"
        "MBAGCiqGSIb4TQENAQMEAswTMBQGCiqGSIb4TQENAQQEBtpm5J90FDAPBgoqhkiG+E0BDQEFCgEC\n"
        "MB4GCiqGSIb4TQENAQYEEJ61rAGzJ3zKdBntGHRNJjEwRAYKKoZIhvhNAQ0BBzA2MBAGCyqGSIb4\n"
        "TQENAQcBAQEAMBAGCyqGSIb4TQENAQcCAQH/MBAGCyqGSIb4TQENAQcDAQEAMAoGCCqGSM49BAMC\n"
        "A0gAMEUCIENqWQp0J3uZPdDk7hbrbQkhXIrDKWT4TLmuPNb3qinQAiEAiQZ4YXASqhvjpS5fsYEJ\n"
        "XGpyDjBR6DSctPvC1skMkwc=\n"
        "-----END CERTIFICATE-----";

struct ExpectedPlatformWithIntegrityPckCertData
{
    /// Example values for subject
    const std::string RAW_SUBJECT = "C=US,ST=CA,L=Santa Clara,O=Intel Corporation,CN=Intel SGX PCK Certificate";
    const std::string COMMON_NAME_ISSUER = "Intel SGX PCK Certificate";
    const std::string COUNTRY_NAME_ISSUER = "US";
    const std::string ORGANIZATION_NAME_ISSUER = "Intel Corporation";
    const std::string LOCATION_NAME_ISSUER = "Santa Clara";
    const std::string STATE_NAME_ISSUER = "CA";

    /// Example values for issuer
    const std::string RAW_ISSUER = "C=US,ST=CA,L=Santa Clara,O=Intel Corporation,CN=Intel SGX PCK Platform CA";
    const std::string COMMON_NAME_SUBJECT = "Intel SGX PCK Platform CA";
    const std::string COUNTRY_NAME_SUBJECT = "US";
    const std::string ORGANIZATION_NAME_SUBJECT = "Intel Corporation";
    const std::string LOCATION_NAME_SUBJECT = "Santa Clara";
    const std::string STATE_NAME_SUBJECT = "CA";

    const std::vector<uint8_t> RAW_DER = { 0x30, 0x45, 0x02, 0x20, // header
                                           0x43, 0x6A, 0x59, 0x0A, 0x74, 0x27, 0x7B, 0x99, // R
                                           0x3D, 0xD0, 0xE4, 0xEE, 0x16, 0xEB, 0x6D, 0x09,
                                           0x21, 0x5C, 0x8A, 0xC3, 0x29, 0x64, 0xF8, 0x4C,
                                           0xB9, 0xAE, 0x3C, 0xD6, 0xF7, 0xAA, 0x29, 0xD0,
                                           0x02, 0x21, 0x00, // marker
                                           0x89, 0x06, 0x78, 0x61, 0x70, 0x12, 0xAA, 0x1B, // S
                                           0xE3, 0xA5, 0x2E, 0x5F, 0xB1, 0x81, 0x09, 0x5C,
                                           0x6A, 0x72, 0x0E, 0x30, 0x51, 0xE8, 0x34, 0x9C,
                                           0xB4, 0xFB, 0xC2, 0xD6, 0xC9, 0x0C, 0x93, 0x07 };
    const std::vector<uint8_t> R = { 0x43, 0x6A, 0x59, 0x0A, 0x74, 0x27, 0x7B, 0x99,
                                     0x3D, 0xD0, 0xE4, 0xEE, 0x16, 0xEB, 0x6D, 0x09,
                                     0x21, 0x5C, 0x8A, 0xC3, 0x29, 0x64, 0xF8, 0x4C,
                                     0xB9, 0xAE, 0x3C, 0xD6, 0xF7, 0xAA, 0x29, 0xD0 };
    const std::vector<uint8_t> S = { 0x89, 0x06, 0x78, 0x61, 0x70, 0x12, 0xAA, 0x1B,
                                     0xE3, 0xA5, 0x2E, 0x5F, 0xB1, 0x81, 0x09, 0x5C,
                                     0x6A, 0x72, 0x0E, 0x30, 0x51, 0xE8, 0x34, 0x9C,
                                     0xB4, 0xFB, 0xC2, 0xD6, 0xC9, 0x0C, 0x93, 0x07 };

    const std::time_t NOT_BEFORE_TIME = 1645015630;
    const std::time_t NOT_AFTER_TIME = 1865940430;

    const std::vector<uint8_t> SERIAL_NUMBER { 0x82, 0xCE, 0xD9, 0x44, 0x42, 0x30, 0x56, 0xF0, 0xEB, 0x17,
                                               0xEB, 0x3F, 0x44, 0xD3, 0xAD, 0xCE, 0xBB, 0xF6, 0x0D, 0x73 };

    const std::vector<uint8_t> PUBLIC_KEY { 0x04, // header
                                            0xF7, 0xD0, 0x25, 0x2E, 0x90, 0x9D, 0x25, 0xDD, // X
                                            0x50, 0xB2, 0x34, 0xD2, 0xD5, 0xA6, 0xD3, 0x76,
                                            0xC5, 0x4D, 0x9B, 0x5C, 0x10, 0xB0, 0xCC, 0xDC,
                                            0xDE, 0x65, 0x1F, 0x63, 0x99, 0x2D, 0x87, 0xF1,
                                            0x6F, 0x3C, 0xA4, 0x63, 0xD8, 0xB0, 0xF6, 0x2F, // Y
                                            0xEE, 0x99, 0xD6, 0xAE, 0x13, 0x52, 0xA2, 0xD0,
                                            0x42, 0xF8, 0x93, 0xC0, 0xE4, 0x82, 0x5C, 0xF5,
                                            0x34, 0x17, 0x8E, 0x16, 0x63, 0x9B, 0xFE, 0x9C };

    const std::vector<uint8_t> PPID { 0xE1, 0xE1, 0xFA, 0xEA, 0x53, 0x66, 0x31, 0x28,
                                      0x97, 0xB8, 0xB0, 0xB4, 0x48, 0x66, 0xFC, 0xEB };

    const std::vector<uint8_t> CPUSVN { 0x02, 0xC1, 0xD9, 0x6C, 0xB2, 0xBB, 0xB0, 0xE3,
                                        0x8B, 0x24, 0x1A, 0xB2, 0xFB, 0x53, 0x7F, 0xB9 };

    const uint32_t PCESVN = 26251;

    const std::vector<uint8_t> PCE_ID { 0xCC, 0x13 };

    const std::vector<uint8_t> FMSPC { 0xDA, 0x66, 0xE4, 0x9F, 0x74, 0x14 };

    const x509::SgxType SGX_TYPE = x509::SgxType::ScalableWithIntegrity;

    const std::vector<uint8_t> PLATFORM_INSTANCE_ID { 0x9E, 0xB5, 0xAC, 0x01, 0xB3, 0x27, 0x7C, 0xCA,
                                                      0x74, 0x19, 0xED, 0x18, 0x74, 0x4D, 0x26, 0x31 };
};

TEST_F(ParseX509CertificateIT, parsePemEncodedPlatformWithIntegrityCAPckCert)
{
    // GIVEN
    const auto expected = ExpectedPlatformWithIntegrityPckCertData();

    // WHEN
    const auto& pckCertificate = x509::PckCertificate::parse(PlatformWithIntegrityPEM);

    // THEN
    ASSERT_EQ(pckCertificate.getVersion(), 3);

    ASSERT_THAT(pckCertificate.getSerialNumber(), ElementsAreArray(expected.SERIAL_NUMBER));
    ASSERT_THAT(pckCertificate.getPubKey(), ElementsAreArray(expected.PUBLIC_KEY));

    const auto& subject = pckCertificate.getSubject();
    ASSERT_EQ(subject.getRaw(), expected.RAW_SUBJECT);
    ASSERT_EQ(subject.getCommonName(), expected.COMMON_NAME_ISSUER);
    ASSERT_EQ(subject.getCountryName(), expected.COUNTRY_NAME_ISSUER);
    ASSERT_EQ(subject.getOrganizationName(), expected.ORGANIZATION_NAME_ISSUER);
    ASSERT_EQ(subject.getLocationName(), expected.LOCATION_NAME_ISSUER);
    ASSERT_EQ(subject.getStateName(), expected.STATE_NAME_ISSUER);

    const auto& issuer = pckCertificate.getIssuer();
    ASSERT_EQ(issuer.getRaw(), expected.RAW_ISSUER);
    ASSERT_EQ(issuer.getCommonName(), expected.COMMON_NAME_SUBJECT);
    ASSERT_EQ(issuer.getCountryName(), expected.COUNTRY_NAME_SUBJECT);
    ASSERT_EQ(issuer.getOrganizationName(), expected.ORGANIZATION_NAME_SUBJECT);
    ASSERT_EQ(issuer.getLocationName(), expected.LOCATION_NAME_SUBJECT);
    ASSERT_EQ(issuer.getStateName(), expected.STATE_NAME_SUBJECT);

    ASSERT_NE(issuer, subject); // they may be equal if certificate is self-signed

    const auto& validity = pckCertificate.getValidity();
    ASSERT_EQ(validity.getNotBeforeTime(), expected.NOT_BEFORE_TIME);
    ASSERT_EQ(validity.getNotAfterTime(), expected.NOT_AFTER_TIME);

    const auto& signature = pckCertificate.getSignature();
    ASSERT_THAT(signature.getRawDer(), ElementsAreArray(expected.RAW_DER));
    ASSERT_THAT(signature.getR(), ElementsAreArray(expected.R));
    ASSERT_THAT(signature.getS(), ElementsAreArray(expected.S));

    const auto& ppid = pckCertificate.getPpid();
    ASSERT_THAT(ppid, ElementsAreArray(expected.PPID));

    const auto& tcb = pckCertificate.getTcb();
    ASSERT_THAT(tcb.getCpuSvn(), ElementsAreArray(expected.CPUSVN));
    ASSERT_THAT(tcb.getSgxTcbComponents(), ElementsAreArray(expected.CPUSVN));
    ASSERT_EQ(tcb.getPceSvn(), expected.PCESVN);
    ASSERT_EQ(tcb, x509::Tcb(expected.CPUSVN, expected.CPUSVN, expected.PCESVN));

    const auto& pceId = pckCertificate.getPceId();
    ASSERT_THAT(pceId, ElementsAreArray(expected.PCE_ID));

    const auto& fmspc = pckCertificate.getFmspc();
    ASSERT_THAT(fmspc, ElementsAreArray(expected.FMSPC));

    const auto& sgxType = pckCertificate.getSgxType();
    ASSERT_EQ(sgxType, expected.SGX_TYPE);
}

TEST_F(ParseX509CertificateIT, parsePemEncodedPlatformCAWithIntegrityPckCertAsPlatformPckCertificateClass)
{
    // GIVEN
    const auto expected = ExpectedPlatformWithIntegrityPckCertData();

    // WHEN
    const auto& pckCertificate = x509::PlatformPckCertificate::parse(PlatformWithIntegrityPEM);

    // THEN
    ASSERT_EQ(pckCertificate.getVersion(), 3);

    ASSERT_THAT(pckCertificate.getSerialNumber(), ElementsAreArray(expected.SERIAL_NUMBER));
    ASSERT_THAT(pckCertificate.getPubKey(), ElementsAreArray(expected.PUBLIC_KEY));

    const auto& subject = pckCertificate.getSubject();
    ASSERT_EQ(subject.getRaw(), expected.RAW_SUBJECT);
    ASSERT_EQ(subject.getCommonName(), expected.COMMON_NAME_ISSUER);
    ASSERT_EQ(subject.getCountryName(), expected.COUNTRY_NAME_ISSUER);
    ASSERT_EQ(subject.getOrganizationName(), expected.ORGANIZATION_NAME_ISSUER);
    ASSERT_EQ(subject.getLocationName(), expected.LOCATION_NAME_ISSUER);
    ASSERT_EQ(subject.getStateName(), expected.STATE_NAME_ISSUER);

    const auto& issuer = pckCertificate.getIssuer();
    ASSERT_EQ(issuer.getRaw(), expected.RAW_ISSUER);
    ASSERT_EQ(issuer.getCommonName(), expected.COMMON_NAME_SUBJECT);
    ASSERT_EQ(issuer.getCountryName(), expected.COUNTRY_NAME_SUBJECT);
    ASSERT_EQ(issuer.getOrganizationName(), expected.ORGANIZATION_NAME_SUBJECT);
    ASSERT_EQ(issuer.getLocationName(), expected.LOCATION_NAME_SUBJECT);
    ASSERT_EQ(issuer.getStateName(), expected.STATE_NAME_SUBJECT);

    ASSERT_NE(issuer, subject); // they may be equal if certificate is self-signed

    const auto& validity = pckCertificate.getValidity();
    ASSERT_EQ(validity.getNotBeforeTime(), expected.NOT_BEFORE_TIME);
    ASSERT_EQ(validity.getNotAfterTime(), expected.NOT_AFTER_TIME);

    const auto& signature = pckCertificate.getSignature();
    ASSERT_THAT(signature.getRawDer(), ElementsAreArray(expected.RAW_DER));
    ASSERT_THAT(signature.getR(), ElementsAreArray(expected.R));
    ASSERT_THAT(signature.getS(), ElementsAreArray(expected.S));

    const auto& ppid = pckCertificate.getPpid();
    ASSERT_THAT(ppid, ElementsAreArray(expected.PPID));

    const auto& tcb = pckCertificate.getTcb();
    ASSERT_THAT(tcb.getCpuSvn(), ElementsAreArray(expected.CPUSVN));
    ASSERT_THAT(tcb.getSgxTcbComponents(), ElementsAreArray(expected.CPUSVN));
    ASSERT_EQ(tcb.getPceSvn(), expected.PCESVN);
    ASSERT_EQ(tcb, x509::Tcb(expected.CPUSVN, expected.CPUSVN, expected.PCESVN));

    const auto& pceId = pckCertificate.getPceId();
    ASSERT_THAT(pceId, ElementsAreArray(expected.PCE_ID));

    const auto& fmspc = pckCertificate.getFmspc();
    ASSERT_THAT(fmspc, ElementsAreArray(expected.FMSPC));

    const auto& sgxType = pckCertificate.getSgxType();
    ASSERT_EQ(sgxType, expected.SGX_TYPE);

    const auto& platformInstanceId = pckCertificate.getPlatformInstanceId();
    ASSERT_THAT(platformInstanceId, ElementsAreArray(expected.PLATFORM_INSTANCE_ID));

    const auto& sgxConfiguration = pckCertificate.getConfiguration();
    ASSERT_TRUE(sgxConfiguration.isCachedKeys());
    ASSERT_FALSE(sgxConfiguration.isDynamicPlatform());
    ASSERT_FALSE(sgxConfiguration.isSmtEnabled());
}
