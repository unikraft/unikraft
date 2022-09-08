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

#include <CertVerification/CertificateChain.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <cstring>
#include <ctime>

static const std::string CORRECT_BEG_CERT = "-----BEGIN CERTIFICATE-----";
static const std::string CORRECT_END_CERT = "-----END CERTIFICATE-----";

static const std::string CORRECT_ROOT_CERT_BODY = R"cert(
MIICkDCCAjWgAwIBAgIVALf/CXgnn0m9mkeN7uLmdD8NVr5mMAoGCCqGSM49BAMC
MGgxGjAYBgNVBAMMEUludGVsIFNHWCBSb290IENBMRowGAYDVQQKDBFJbnRlbCBD
b3Jwb3JhdGlvbjEUMBIGA1UEBwwLU2FudGEgQ2xhcmExCzAJBgNVBAgMAkNBMQsw
CQYDVQQGEwJVUzAeFw0xODAzMjkxMDA3MTFaFw00OTEyMzEyMjU5NTlaMGgxGjAY
BgNVBAMMEUludGVsIFNHWCBSb290IENBMRowGAYDVQQKDBFJbnRlbCBDb3Jwb3Jh
dGlvbjEUMBIGA1UEBwwLU2FudGEgQ2xhcmExCzAJBgNVBAgMAkNBMQswCQYDVQQG
EwJVUzBZMBMGByqGSM49AgEGCCqGSM49AwEHA0IABOg+J8Dbw8l21h+KbjyFt8qH
9R6jRTx/t+EdBkgbknxB3rEVOW0xKIe68ILaGFQXzuc/V9KYULriAQ8EV84W7Ouj
gbswgbgwHwYDVR0jBBgwFoAUt/8JeCefSb2aR43u4uZ0Pw1WvmYwUgYDVR0fBEsw
STBHoEWgQ4ZBaHR0cHM6Ly9jZXJ0aWZpY2F0ZXMudHJ1c3RlZHNlcnZpY2VzLmlu
dGVsLmNvbS9JbnRlbFNHWFJvb3RDQS5jcmwwHQYDVR0OBBYEFLf/CXgnn0m9mkeN
7uLmdD8NVr5mMA4GA1UdDwEB/wQEAwIBBjASBgNVHRMBAf8ECDAGAQH/AgEBMAoG
CCqGSM49BAMCA0kAMEYCIQDNrdFnV6RpzcxPeU4buq6XWJuVwWQJiYuhzAEtpn7p
bgIhAJbRv7dhz/yzCD14FmJuZnwI/3aDSvkc6MSD/K6m5ot0
)cert";
static const std::string CORRECT_INTERMEDIATE_CERT_BODY = R"cert(
MIICmDCCAj2gAwIBAgIVANKBpQn32peLOTmU/PvTnUKQ1s0YMAoGCCqGSM49BAMC
MGgxGjAYBgNVBAMMEUludGVsIFNHWCBSb290IENBMRowGAYDVQQKDBFJbnRlbCBD
b3Jwb3JhdGlvbjEUMBIGA1UEBwwLU2FudGEgQ2xhcmExCzAJBgNVBAgMAkNBMQsw
CQYDVQQGEwJVUzAeFw0xODAzMjkxMDA3MTFaFw0zMzAzMjkxMDA3MTFaMHAxIjAg
BgNVBAMMGUludGVsIFNHWCBQQ0sgUGxhdGZvcm0gQ0ExGjAYBgNVBAoMEUludGVs
IENvcnBvcmF0aW9uMRQwEgYDVQQHDAtTYW50YSBDbGFyYTELMAkGA1UECAwCQ0Ex
CzAJBgNVBAYTAlVTMFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAEmvma6z1+yoMn
MfrAP+hy2OAiB7Ouqd8Mf17KhbRMGaUf1hLiZJiInnis74pBSbHa9rs01Rf+LEee
K2NSnzs0vaOBuzCBuDAfBgNVHSMEGDAWgBS3/wl4J59JvZpHje7i5nQ/DVa+ZjBS
BgNVHR8ESzBJMEegRaBDhkFodHRwczovL2NlcnRpZmljYXRlcy50cnVzdGVkc2Vy
dmljZXMuaW50ZWwuY29tL0ludGVsU0dYUm9vdENBLmNybDAdBgNVHQ4EFgQU0oGl
Cffal4s5OZT8+9OdQpDWzRgwDgYDVR0PAQH/BAQDAgEGMBIGA1UdEwEB/wQIMAYB
Af8CAQAwCgYIKoZIzj0EAwIDSQAwRgIhAJCOqB6hf+SbNgr5EWv/8GxfJ7u5ObvB
f7Wb9ZUZDxu3AiEArbbiUaWTFlZAwCxnjfkDJxN4YIbus9H1IfaW+QOMir0=
)cert";
static const std::string CORRECT_PCK_CERT_BODY = R"cert(
MIIEfDCCBCKgAwIBAgIULDeIhjaXZQa97bX3uVF8CjsXlwUwCgYIKoZIzj0EAwIw
cDEiMCAGA1UEAwwZSW50ZWwgU0dYIFBDSyBQbGF0Zm9ybSBDQTEaMBgGA1UECgwR
SW50ZWwgQ29ycG9yYXRpb24xFDASBgNVBAcMC1NhbnRhIENsYXJhMQswCQYDVQQI
DAJDQTELMAkGA1UEBhMCVVMwHhcNMTgwMzI5MTAwNzExWhcNMjUwMzI5MTEwNzEx
WjBwMSIwIAYDVQQDDBlJbnRlbCBTR1ggUENLIENlcnRpZmljYXRlMRowGAYDVQQK
DBFJbnRlbCBDb3Jwb3JhdGlvbjEUMBIGA1UEBwwLU2FudGEgQ2xhcmExCzAJBgNV
BAgMAkNBMQswCQYDVQQGEwJVUzBZMBMGByqGSM49AgEGCCqGSM49AwEHA0IABN5D
XuIQ/ru9r20nKpdHixsCRXLrx1GSDTU/RDk2DodO6FJLmrmJXJOOm02JuWbdAacV
taKSZvkx2QF6MSeCK56jggKYMIIClDAfBgNVHSMEGDAWgBTSgaUJ99qXizk5lPz7
051CkNbNGDBYBgNVHR8EUTBPME2gS6BJhkdodHRwczovL2NlcnRpZmljYXRlcy50
cnVzdGVkc2VydmljZXMuaW50ZWwuY29tL0ludGVsU0dYUENLUHJvY2Vzc29yLmNy
bDAdBgNVHQ4EFgQUuQ26eIKSacFhYF8baYdQYMm46iYwDgYDVR0PAQH/BAQDAgbA
MAwGA1UdEwEB/wQCMAAwggHYBgkqhkiG+E0BDQEBAf8EggHGMIIBwjAeBgoqhkiG
+E0BDQEBBBD1sZGHcbCWskmg+DFPhfsOMIIBZQYKKoZIhvhNAQ0BAjCCAVUwEAYL
KoZIhvhNAQ0BAgECAfYwEAYLKoZIhvhNAQ0BAgICAXcwEAYLKoZIhvhNAQ0BAgMC
AQcwEAYLKoZIhvhNAQ0BAgQCAQ4wEAYLKoZIhvhNAQ0BAgUCAfkwEAYLKoZIhvhN
AQ0BAgYCAaMwEAYLKoZIhvhNAQ0BAgcCAQcwEAYLKoZIhvhNAQ0BAggCAd4wEAYL
KoZIhvhNAQ0BAgkCAf4wEAYLKoZIhvhNAQ0BAgoCAbAwEAYLKoZIhvhNAQ0BAgsC
Af4wEAYLKoZIhvhNAQ0BAgwCATMwEAYLKoZIhvhNAQ0BAg0CAS8wEAYLKoZIhvhN
AQ0BAg4CAWIwEAYLKoZIhvhNAQ0BAg8CAd0wEAYLKoZIhvhNAQ0BAhACATQwEgYL
KoZIhvhNAQ0BAhECAwDaxDAfBgsqhkiG+E0BDQECEgQQ9ncHDvmjB97+sP4zL2Ld
NDAQBgoqhkiG+E0BDQEDBAIe1TAUBgoqhkiG+E0BDQEEBAbwSmVrjXAwDwYKKoZI
hvhNAQ0BBQoBADAKBggqhkjOPQQDAgNIADBFAiEAmj9y/KmgcvH/0CsUUO/BnAyk
SruGvu3DJ+zdTvm/0EgCIChm9INugpWJOqwLz/qFnK6nBKl3joq2VKFrj71kUguQ
)cert";

using namespace intel::sgx::dcap;
using namespace intel::sgx;
using namespace testing;

struct PckCertChainParserTests : public Test
{
    CertificateChain certChain;
    dcap::parser::x509::Certificate rootCert, intermediateCert;
    dcap::parser::x509::PckCertificate finalCert;
};

TEST_F(PckCertChainParserTests, simpleParsing)
{
    // GIVEN
    const std::string txt = CORRECT_BEG_CERT + CORRECT_ROOT_CERT_BODY + CORRECT_END_CERT;
    rootCert = dcap::parser::x509::Certificate::parse(txt);
    
    // WHEN
    ASSERT_EQ(certChain.parse(txt.c_str()), STATUS_OK);
   
    // THEN 
    EXPECT_EQ(1, certChain.length());
    EXPECT_EQ(*certChain.getRootCert(), rootCert);
    EXPECT_EQ(*certChain.getTopmostCert(), rootCert);
    EXPECT_EQ(*certChain.get(rootCert.getSubject()), rootCert);

    auto certs = certChain.getCerts();
    EXPECT_EQ(*certs.front(), rootCert);
    EXPECT_EQ(1, certs.size());
}

TEST_F(PckCertChainParserTests, parsingTwoElements)
{
    // GIVEN
    const std::string txt1 = CORRECT_BEG_CERT + CORRECT_ROOT_CERT_BODY + CORRECT_END_CERT;
    const std::string txt2 = CORRECT_BEG_CERT + CORRECT_INTERMEDIATE_CERT_BODY + CORRECT_END_CERT;
    const auto toParse = txt1 + txt2;
    rootCert = dcap::parser::x509::Certificate::parse(txt1);
    intermediateCert = dcap::parser::x509::Certificate::parse(txt2);

    // WHEN
    ASSERT_EQ(certChain.parse(toParse.c_str()), STATUS_OK);

    // THEN
    EXPECT_EQ(2, certChain.length());
    EXPECT_EQ(*certChain.getRootCert(), rootCert);
    EXPECT_EQ(*certChain.getTopmostCert(), intermediateCert);
    EXPECT_EQ(*certChain.get(rootCert.getSubject()), rootCert);
    EXPECT_EQ(*certChain.get(intermediateCert.getSubject()), intermediateCert);
    auto certs = certChain.getCerts();
    EXPECT_EQ(2, certs.size());
    EXPECT_EQ(*certs[0], rootCert);
    EXPECT_EQ(*certs[1], intermediateCert);
}

TEST_F(PckCertChainParserTests, parsingThreeElements)
{
    // GIVEN
    const std::string txt1 = CORRECT_BEG_CERT + CORRECT_ROOT_CERT_BODY + CORRECT_END_CERT;
    const std::string txt2 = CORRECT_BEG_CERT + CORRECT_INTERMEDIATE_CERT_BODY + CORRECT_END_CERT;
    const std::string txt3 = CORRECT_BEG_CERT + CORRECT_PCK_CERT_BODY + CORRECT_END_CERT;
    const auto toParse = txt1 + txt2 + txt3;
    rootCert = dcap::parser::x509::Certificate::parse(txt1);
    intermediateCert = dcap::parser::x509::Certificate::parse(txt2);
    finalCert = dcap::parser::x509::PckCertificate::parse(txt3);

    // WHEN
    ASSERT_EQ(certChain.parse(toParse.c_str()), STATUS_OK);

    // THEN
    EXPECT_EQ(3, certChain.length());
    EXPECT_EQ(*certChain.getRootCert(), rootCert);
    EXPECT_EQ(*certChain.getTopmostCert(), finalCert);
    EXPECT_EQ(*certChain.get(rootCert.getSubject()), rootCert);
    EXPECT_EQ(*certChain.get(intermediateCert.getSubject()), intermediateCert);
    EXPECT_EQ(*certChain.get(finalCert.getSubject()), finalCert);

    auto certs = certChain.getCerts();
    EXPECT_EQ(3, certs.size());
    EXPECT_EQ(*certs[0], rootCert);
    EXPECT_EQ(*certs[1], intermediateCert);
    EXPECT_EQ(*certs[2], finalCert);
}

TEST_F(PckCertChainParserTests, parsingTwoElementsWithNewLines)
{
    // GIVEN
    const std::string txt1 = CORRECT_BEG_CERT + CORRECT_ROOT_CERT_BODY + CORRECT_END_CERT;
    const std::string txt2 = CORRECT_BEG_CERT + CORRECT_INTERMEDIATE_CERT_BODY + CORRECT_END_CERT;
    // new lines at the end of txt1 and txt2 are expected to be gone
    const auto toParse = txt1 + "\n" + txt2 + "\n";

    // WHEN
    ASSERT_EQ(certChain.parse(toParse.c_str()), STATUS_OK);

    // THEN
    EXPECT_EQ(2, certChain.length());
}

TEST_F(PckCertChainParserTests, parsingTwoElementsWithNewLinesWithMixedNewLinesTypes)
{
    // GIVEN
    const std::string txt1 = CORRECT_BEG_CERT + CORRECT_ROOT_CERT_BODY + CORRECT_END_CERT;
    const std::string txt2 = CORRECT_BEG_CERT + CORRECT_INTERMEDIATE_CERT_BODY + CORRECT_END_CERT;
    // new lines at the end of txt1 and txt2 are expected to be gone
    const auto toParse = txt1 + "\r\n" + txt2 + "\n";

    // WHEN
    ASSERT_EQ(certChain.parse(toParse.c_str()), STATUS_OK);

    // THEN
    EXPECT_EQ(2, certChain.length());
}

TEST_F(PckCertChainParserTests, parsingTwoElementsSecondIncorrectBegFormat)
{
    // GIVEN
    const std::string txt1 = CORRECT_BEG_CERT + CORRECT_ROOT_CERT_BODY + CORRECT_END_CERT;
    // missing dash at the end of BE CERT
    const std::string txt2 = std::string("-----BEGIN CERTIFICATE----") + CORRECT_INTERMEDIATE_CERT_BODY + CORRECT_END_CERT;
    const auto toParse = txt1 + txt2;

    // WHEN
    ASSERT_EQ(certChain.parse(toParse.c_str()), STATUS_OK);

    // THEN
    EXPECT_EQ(1, certChain.length());
}

TEST_F(PckCertChainParserTests, parsingTwoElementsMissingSecondBegFormat)
{
    // GIVEN
    const std::string txt1 = CORRECT_BEG_CERT + CORRECT_ROOT_CERT_BODY + CORRECT_END_CERT;
    const std::string txt2 = CORRECT_INTERMEDIATE_CERT_BODY + CORRECT_END_CERT;
    const auto toParse = txt1 + txt2;

    // WHEN
    ASSERT_EQ(certChain.parse(toParse.c_str()), STATUS_OK);

    // THEN
    EXPECT_EQ(1, certChain.length());
}

TEST_F(PckCertChainParserTests, parsingThreeElementsMissingSecondBegFormat)
{
    // GIVEN
    const std::string txt1 = CORRECT_BEG_CERT + CORRECT_ROOT_CERT_BODY + CORRECT_END_CERT;
    const std::string txt2 = CORRECT_INTERMEDIATE_CERT_BODY + CORRECT_END_CERT;
    const std::string txt3 = CORRECT_BEG_CERT + CORRECT_PCK_CERT_BODY + CORRECT_END_CERT;

    const auto toParse = txt1 + txt2 + txt3;

    // WHEN
    ASSERT_EQ(certChain.parse(toParse.c_str()), STATUS_OK);

    // THEN
    EXPECT_EQ(2, certChain.length());
}

TEST_F(PckCertChainParserTests, endCertificateHasMissingDashShouldReturnNothing)
{
    // GIVEN
    // END CERT has missing dash
    const std::string txt = CORRECT_BEG_CERT + CORRECT_ROOT_CERT_BODY + "-----END CERTIFICATE----";

    // WHEN
    ASSERT_EQ(certChain.parse(txt.c_str()), STATUS_UNSUPPORTED_CERT_FORMAT);

    // THEN
    EXPECT_EQ(0, certChain.length());
    EXPECT_EQ(certChain.getRootCert(), nullptr);
    EXPECT_EQ(certChain.getTopmostCert(), nullptr);
}

TEST_F(PckCertChainParserTests, begCertHasMissngDashShouldReturnNothing)
{
    // GIVEN
    // BEG CERT has missing dash
    const std::string txt = std::string("-----BEGIN CERTIFICATE----") + CORRECT_ROOT_CERT_BODY + CORRECT_END_CERT;

    // WHEN
    ASSERT_EQ(certChain.parse(txt.c_str()), STATUS_UNSUPPORTED_CERT_FORMAT);

    // THEN
    EXPECT_EQ(0, certChain.length());
    EXPECT_EQ(certChain.getRootCert(), nullptr);
    EXPECT_EQ(certChain.getTopmostCert(), nullptr);
}

TEST_F(PckCertChainParserTests, emptyString)
{
    // GIVEN
    // WHEN
    ASSERT_EQ(certChain.parse(""), STATUS_UNSUPPORTED_CERT_FORMAT);

    // THEN
    EXPECT_EQ(0, certChain.length());
    EXPECT_EQ(certChain.getRootCert(), nullptr);
    EXPECT_EQ(certChain.getTopmostCert(), nullptr);
}

TEST_F(PckCertChainParserTests, parsingOneCertificateWithBegAndEndCertInverted)
{
    // GIVEN
    const std::string txt = CORRECT_END_CERT + CORRECT_ROOT_CERT_BODY + CORRECT_BEG_CERT;

    // WHEN
    ASSERT_EQ(certChain.parse(txt.c_str()), STATUS_UNSUPPORTED_CERT_FORMAT);

    // THEN
    EXPECT_EQ(0, certChain.length());
    EXPECT_EQ(certChain.getRootCert(), nullptr);
    EXPECT_EQ(certChain.getTopmostCert(), nullptr);
}

TEST_F(PckCertChainParserTests, parsingTwoCertificatesWithFirstBegAndEndCertInverted)
{
    // GIVEN
    const std::string txt = CORRECT_END_CERT + CORRECT_ROOT_CERT_BODY + CORRECT_BEG_CERT
        + CORRECT_BEG_CERT + CORRECT_ROOT_CERT_BODY + CORRECT_END_CERT;

    // WHEN
    ASSERT_EQ(certChain.parse(txt.c_str()), STATUS_UNSUPPORTED_CERT_FORMAT);

    // THEN
    EXPECT_EQ(0, certChain.length());
    EXPECT_EQ(certChain.getRootCert(), nullptr);
    EXPECT_EQ(certChain.getTopmostCert(), nullptr);
}

TEST_F(PckCertChainParserTests, parsingTwoCertificatesWithSecondBegAndEndCertInverted)
{
    // GIVEN
    const std::string txt = CORRECT_BEG_CERT + CORRECT_ROOT_CERT_BODY + CORRECT_END_CERT
            + CORRECT_END_CERT + CORRECT_ROOT_CERT_BODY + CORRECT_BEG_CERT;

    // WHEN
    ASSERT_EQ(certChain.parse(txt.c_str()), STATUS_UNSUPPORTED_CERT_FORMAT);

    // THEN
    EXPECT_EQ(0, certChain.length());
    EXPECT_EQ(certChain.getRootCert(), nullptr);
    EXPECT_EQ(certChain.getTopmostCert(), nullptr);
}

TEST_F(PckCertChainParserTests, parsingThreeCertificatesWithLastBegAndEndCertInverted)
{
    // GIVEN
    const std::string txt = CORRECT_BEG_CERT + CORRECT_ROOT_CERT_BODY + CORRECT_END_CERT
            + CORRECT_BEG_CERT + CORRECT_ROOT_CERT_BODY + CORRECT_END_CERT
            + CORRECT_END_CERT + CORRECT_ROOT_CERT_BODY + CORRECT_BEG_CERT;

    // WHEN
    ASSERT_EQ(certChain.parse(txt.c_str()), STATUS_UNSUPPORTED_CERT_FORMAT);

    // THEN
    EXPECT_EQ(0, certChain.length());
    EXPECT_EQ(certChain.getRootCert(), nullptr);
    EXPECT_EQ(certChain.getTopmostCert(), nullptr);
}

TEST_F(PckCertChainParserTests, parsingThreeCertificatesWithMiddleBegAndEndCertInverted)
{
    // GIVEN
    const std::string txt = CORRECT_BEG_CERT + CORRECT_ROOT_CERT_BODY + CORRECT_END_CERT
            + CORRECT_END_CERT + CORRECT_ROOT_CERT_BODY + CORRECT_BEG_CERT
            + CORRECT_BEG_CERT + CORRECT_ROOT_CERT_BODY + CORRECT_END_CERT;

    // WHEN
    ASSERT_EQ(certChain.parse(txt.c_str()), STATUS_UNSUPPORTED_CERT_FORMAT);

    // THEN
    EXPECT_EQ(0, certChain.length());
    EXPECT_EQ(certChain.getRootCert(), nullptr);
    EXPECT_EQ(certChain.getTopmostCert(), nullptr);
}