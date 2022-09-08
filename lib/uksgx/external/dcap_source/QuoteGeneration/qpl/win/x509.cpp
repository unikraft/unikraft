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
/**
 * File: x509.cpp
 *  
 * Description: X.509 parser
 *
 */
#include <Windows.h>
#include <Wincrypt.h>
#include <codecvt>
#include <iostream>
#include <locale>

std::string get_cdp_url_from_pem_cert(const char *p_cert) {
    BYTE derPubKey[2048];
    DWORD derPubKeyLen = 2048;
    PCERT_INFO publicKeyInfo;
    DWORD keyLength;

    /*
     * Convert from PEM format to DER format - removes header and footer and decodes from base64
     */
    if (!CryptStringToBinaryA(p_cert, 0, CRYPT_STRING_ANY, NULL, &derPubKeyLen, NULL, NULL)) {
        return "";
    } else {
        if (!CryptStringToBinaryA(p_cert, 0, CRYPT_STRING_ANY, derPubKey, &derPubKeyLen, NULL, NULL)) {
            return "";
        }
    }

    /*
     * Decode from DER format to CERT_PUBLIC_KEY_INFO
     */
    if (!CryptDecodeObjectEx(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, X509_CERT_TO_BE_SIGNED, derPubKey, derPubKeyLen,
                             CRYPT_ENCODE_ALLOC_FLAG, NULL, &publicKeyInfo, &keyLength)) {
        return "";
    }

    const char *OID_CRL_DISTRIBUTION_POINT = "2.5.29.31";
    PCERT_EXTENSION pExtensionCrlCdp = CertFindExtension(OID_CRL_DISTRIBUTION_POINT, publicKeyInfo->cExtension, publicKeyInfo->rgExtension);
    if (!pExtensionCrlCdp) {
        LocalFree(publicKeyInfo);
        return "";
    }

    PCRL_DIST_POINTS_INFO pCrlDistPoints = NULL;
    DWORD dwCrlDistPoints = 0;
    if (!CryptDecodeObjectEx(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, X509_CRL_DIST_POINTS, pExtensionCrlCdp->Value.pbData,
                             pExtensionCrlCdp->Value.cbData, CRYPT_DECODE_ALLOC_FLAG, (PCRYPT_DECODE_PARA)NULL, &pCrlDistPoints, &dwCrlDistPoints)) {
        LocalFree(publicKeyInfo);
        return "";
    }

    std::wstring pwszURL = pCrlDistPoints->rgDistPoint[0].DistPointName.FullName.rgAltEntry->pwszURL;
    std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
    std::string pszURL = converter.to_bytes(pwszURL);

    LocalFree(pCrlDistPoints);
    LocalFree(publicKeyInfo);

    return pszURL;
}
