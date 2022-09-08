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
 *  \brief RSA-OAEP encryption scheme usage example.
 *
 *  This example demonstrates message encryption according to
 *  RSA-OAEP scheme with 1024-bit RSA modulus and SHA-1 hash function.
 *  It uses Reduced Memory Footprint (_rmf) version of the function.
 *
 *  The RSASSA-OAEP scheme is implemented according to the PKCS#1 v2.1: RSA Cryptography Standard (June 2002),
 *  available at:
 *
 *  https://tools.ietf.org/html/rfc3447.
 *
 */

#include <cstring>

#include "ippcp.h"
#include "examples_common.h"
#include "bignum.h"

/*! 1024-bit RSA Modulus N = P*Q */
static BigNumber N("0xBBF82F090682CE9C2338AC2B9DA871F7368D07EED41043A440D6B6F07454F51FB8DFBAAF035C02AB61EA48CEEB6FCD4876ED520D60E1E"
                   "C4619719D8A5B8B807FAFB8E0A3DFC737723EE6B4B7D93A2584EE6A649D060953748834B2454598394EE0AAB12D7B61A51F527A9A41F6C1"
                   "687FE2537298CA2A8F5946F8E5FD091DBDCB");

/*! Public exponent */
static BigNumber E("0x11");

/*! Plain text source message */
static Ipp8u sourceMessage[] =
      "\xd4\x36\xe9\x95\x69\xfd\x32\xa7"
      "\xc8\xa0\x5b\xbc\x90\xd3\x2c\x49";

/*! Seed string of hash size */
static Ipp8u seed[] = "\xaa\xfd\x12\xf6\x59\xca\xe6\x34\x89\xb4"
                      "\x79\xe5\x07\x6d\xde\xc2\xf0\x6c\xb5\x8f";

/*! Reference cipher text. Length is equal to RSA modulus size */
static Ipp8u cipherTextRef[] =
      "\x12\x53\xE0\x4D\xC0\xA5\x39\x7B\xB4\x4A\x7A\xB8\x7E\x9B\xF2\xA0"
      "\x39\xA3\x3D\x1E\x99\x6F\xC8\x2A\x94\xCC\xD3\x00\x74\xC9\x5D\xF7"
      "\x63\x72\x20\x17\x06\x9E\x52\x68\xDA\x5D\x1C\x0B\x4F\x87\x2C\xF6"
      "\x53\xC1\x1D\xF8\x23\x14\xA6\x79\x68\xDF\xEA\xE2\x8D\xEF\x04\xBB"
      "\x6D\x84\xB1\xC3\x1D\x65\x4A\x19\x70\xE5\x78\x3B\xD6\xEB\x96\xA0"
      "\x24\xC2\xCA\x2F\x4A\x90\xFE\x9F\x2E\xF5\xC9\xC1\x40\xE5\xBB\x48"
      "\xDA\x95\x36\xAD\x87\x00\xC8\x4F\xC9\x13\x0A\xDE\xA7\x4E\x55\x8D"
      "\x51\xA7\x4D\xDF\x85\xD8\xB5\x0D\xE9\x68\x38\xD6\x06\x3E\x09\x55";

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

    /* Allocate memory for cipher text, not less than RSA modulus size. */
    int cipherTextLen = bitSizeInBytes(bitSizeN);
    Ipp8u* pCipherText = new Ipp8u[cipherTextLen];

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

        /* Allocate memory for temporary buffer */
        Ipp8u* pScratchBuffer = new Ipp8u[pubBufSize];

        /* Encrypt message */
        status = ippsRSAEncrypt_OAEP_rmf(sourceMessage, sizeof(sourceMessage)-1,
                                         0  /* optional label assotiated with the sourceMessage */,
                                         0, /* label length */
                                         seed, pCipherText, pPubKey,
                                         ippsHashMethod_SHA1(),
                                         pScratchBuffer);

        if (pScratchBuffer) delete [] pScratchBuffer;

        if (!checkStatus("ippsRSAEncrypt_OAEP_rmf", ippStsNoErr, status))
            break;

        if (0 != memcmp(cipherTextRef, pCipherText, sizeof(cipherTextRef)-1)) {
            printf("ERROR: Encrypted and reference messages do not match\n");
            status = ippStsErr;
        }
    } while (0);

    PRINT_EXAMPLE_STATUS("ippsRSAEncrypt_OAEP_rmf", "RSA-OAEP 1024 (SHA1) Encryption", ippStsNoErr == status);

    if (pCipherText) delete [] pCipherText;
    if (pPubKey) delete [] (Ipp8u*)pPubKey;

    return status;
}

