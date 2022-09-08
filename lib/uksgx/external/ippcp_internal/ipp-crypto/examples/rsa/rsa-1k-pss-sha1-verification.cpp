/*******************************************************************************
* Copyright 2019-2021 Intel Corporation
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*******************************************************************************/

/*!
 *
 *  \file
 *
 *  \brief RSASSA-PSS verification scheme usage example.
 *
 *  This example demonstrates message verification according to
 *  RSASSA-PSS scheme with 1024-bit RSA modulus and SHA-1 hash function.
 *  It uses Reduced Memory Footprint (_rmf) version of the function.
 *
 *  The RSASSA-PSS scheme is implemented according to the PKCS#1 v2.1: RSA Cryptography Standard (June 2002),
 *  available at:
 *
 *  https://tools.ietf.org/html/rfc3447.
 *
 */
#include "ippcp.h"
#include "examples_common.h"
#include "bignum.h"

/*! 1024-bit RSA Modulus N = P*Q */
static BigNumber N("0xA2BA40EE07E3B2BD2F02CE227F36A195024486E49C19CB41BBBDFBBA98B22B0E577C2EEAFFA20D883A76E65E394C69D4"
                   "B3C05A1E8FADDA27EDB2A42BC000FE888B9B32C22D15ADD0CD76B3E7936E19955B220DD17D4EA904B1EC102B2E4DE775"
                   "1222AA99151024C7CB41CC5EA21D00EEB41F7C800834D2C6E06BCE3BCE7EA9A5");

/*! Public exponent */
static BigNumber E("0x10001");

/*! Message to be signed */
static
Ipp8u sourceMessage[]  = "\x85\x9e\xef\x2f\xd7\x8a\xca\x00\x30\x8b\xdc\x47\x11\x93\xbf\x55"
                         "\xbf\x9d\x78\xdb\x8f\x8a\x67\x2b\x48\x46\x34\xf3\xc9\xc2\x6e\x64"
                         "\x78\xae\x10\x26\x0f\xe0\xdd\x8c\x08\x2e\x53\xa5\x29\x3a\xf2\x17"
                         "\x3c\xd5\x0c\x6d\x5d\x35\x4f\xeb\xf7\x8b\x26\x02\x1c\x25\xc0\x27"
                         "\x12\xe7\x8c\xd4\x69\x4c\x9f\x46\x97\x77\xe4\x51\xe7\xf8\xe9\xe0"
                         "\x4c\xd3\x73\x9c\x6b\xbf\xed\xae\x48\x7f\xb5\x56\x44\xe9\xca\x74"
                         "\xff\x77\xa5\x3c\xb7\x29\x80\x2f\x6e\xd4\xa5\xff\xa8\xba\x15\x98"
                         "\x90\xfc";

/*! Signature to verify */
static
Ipp8u signatureRef[] = "\x8d\xaa\x62\x7d\x3d\xe7\x59\x5d\x63\x05\x6c\x7e\xc6\x59\xe5\x44"
                       "\x06\xf1\x06\x10\x12\x8b\xaa\xe8\x21\xc8\xb2\xa0\xf3\x93\x6d\x54"
                       "\xdc\x3b\xdc\xe4\x66\x89\xf6\xb7\x95\x1b\xb1\x8e\x84\x05\x42\x76"
                       "\x97\x18\xd5\x71\x5d\x21\x0d\x85\xef\xbb\x59\x61\x92\x03\x2c\x42"
                       "\xbe\x4c\x29\x97\x2c\x85\x62\x75\xeb\x6d\x5a\x45\xf0\x5f\x51\x87"
                       "\x6f\xc6\x74\x3d\xed\xdd\x28\xca\xec\x9b\xb3\x0e\xa9\x9e\x02\xc3"
                       "\x48\x82\x69\x60\x4f\xe4\x97\xf7\x4c\xcd\x7c\x7f\xca\x16\x71\x89"
                       "\x71\x23\xcb\xd3\x0d\xef\x5d\x54\xa2\xb5\x53\x6a\xd9\x0a\x74\x7e";

/*! Main function  */
int main(void)
{
    /* Internal function status */
    IppStatus status = ippStsNoErr;

    /* Size in bits of RSA modulus */
    const int bitSizeN = N.BitSize();
    /* Size in bits of RSA public exponent */
    const int bitSizeE = E.BitSize();

    /* Allocate memory for public key. */
    int keySize = 0;
    ippsRSA_GetSizePublicKey(bitSizeN, bitSizeE, &keySize);
    IppsRSAPublicKeyState* pPubKey = (IppsRSAPublicKeyState*)(new Ipp8u[keySize]);
    ippsRSA_InitPublicKey(bitSizeN, bitSizeE, pPubKey, keySize);

    if (pPubKey) {
        do {
            /* Set public key */
            status = ippsRSA_SetPublicKey(N, E, pPubKey);
            if (!checkStatus("ippsRSA_SetPublicKey", ippStsNoErr, status))
                break;

            /* Calculate temporary buffer size */
            int pubBufSize = 0;
            status = ippsRSA_GetBufferSizePublicKey(&pubBufSize, pPubKey);
            if (!checkStatus("ippsRSA_GetBufferSizePublicKey", ippStsNoErr, status))
                break;

            Ipp8u* pScratchBuffer = new Ipp8u[pubBufSize];

            /* Verify message with use of SHA-1 hash function. The verification result will be placed
             * into isValid variable. */
            int isValid = 0;
            status = ippsRSAVerify_PSS_rmf(sourceMessage, sizeof(sourceMessage)-1,
                                           signatureRef, &isValid,
                                           pPubKey,
                                           ippsHashMethod_SHA1(),
                                           pScratchBuffer);

            if (pScratchBuffer) delete [] pScratchBuffer;

            if (!checkStatus("ippsRSAVerify_PSS_rmf", ippStsNoErr, status))
                break;

            /* If isValid is zero, then verification fails */
            status = isValid ? ippStsNoErr : ippStsErr;
        } while (0);
    }

    if (pPubKey) delete [] (Ipp8u*)pPubKey;

    PRINT_EXAMPLE_STATUS("ippsRSAVerify_PSS_rmf", "RSA-PSS 1024 (SHA1) Verification", ippStsNoErr == status);

    return status;
}
