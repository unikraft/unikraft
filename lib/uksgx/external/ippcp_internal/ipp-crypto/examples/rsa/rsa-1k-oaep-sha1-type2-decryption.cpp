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
 *  \brief RSA-OAEP decryption scheme usage example.
 *
 *  This example demonstrates message decryption according to
 *  RSA-OAEP scheme with 1024-bit RSA modulus and SHA-1 hash function.
 *  The private key of Type2 (to be able to apply Chinese Reminder Theorem) is used in this example.
 *  It uses Reduced Memory Footprint (_rmf) version of the function.
 *
 *  The RSASSA-OAEP scheme is implemented according to the PKCS#1 v2.1: RSA Cryptography Standard (June 2002),
 *  available at:
 *
 *  https://tools.ietf.org/html/rfc3447.
 *
 */

#include <string.h>

#include "ippcp.h"
#include "examples_common.h"
#include "bignum.h"

/*! Prime P factor */
static BigNumber P("0xEECFAE81B1B9B3C908810B10A1B5600199EB9F44AEF4FDA493B81A9E3D84F632"
                   "124EF0236E5D1E3B7E28FAE7AA040A2D5B252176459D1F397541BA2A58FB6599");

/*! Prime Q factor */
static BigNumber Q("0xC97FB1F027F453F6341233EAAAD1D9353F6C42D08866B1D05A0F2035028B9D86"
                   "9840B41666B42E92EA0DA3B43204B5CFCE3352524D0416A5A441E700AF461503");

/*! D mod (p-1) factor */
static BigNumber DP("0x54494CA63EBA0337E4E24023FCD69A5AEB07DDDC0183A4D0AC9B54B051F2B13E"
                    "D9490975EAB77414FF59C1F7692E9A2E202B38FC910A474174ADC93C1F67C981");

/*! D mod (q-1) factor */
static BigNumber DQ("0x471E0290FF0AF0750351B7F878864CA961ADBD3A8A7E991C5C0556A94C3146A7"
                    "F9803F8F6F8AE342E931FD8AE47A220D1B99A495849807FE39F9245A9836DA3D");

/*! Q^-1 mod p factor */
static BigNumber InvQ("0xB06C4FDABB6301198D265BDBAE9423B380F271F73453885093077FCD39E2119F"
                      "C98632154F5883B167A967BF402B4E9E2E0F9656E698EA3666EDFB25798039F7");

/*! Plain text */
static Ipp8u sourceMessageRef[] =
      "\xd4\x36\xe9\x95\x69\xfd\x32\xa7"
      "\xc8\xa0\x5b\xbc\x90\xd3\x2c\x49";

/*! Cipher text to decrypt. */
static Ipp8u cipherText[] =
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

    /* Size in bits of P factor */
    const int bitSizeP = P.BitSize();
    /* Size in bits of Q factor */
    const int bitSizeQ = Q.BitSize();

    /* Allocate memory for private key.
     * There are two types of private keys that are supported: Type1 and Type2.
     * You can choose any of them, depending on your private key representation.
     * This example uses Type2 key.
     * For more information, see https://software.intel.com/en-us/ipp-crypto-reference-2019-rsa-getsizepublickey-rsa-getsizeprivatekeytype1-rsa-getsizeprivatekeytype2
     */
    int keySize = 0;
    ippsRSA_GetSizePrivateKeyType2(bitSizeP, bitSizeQ, &keySize);
    IppsRSAPrivateKeyState* pPrvKeyType2 = (IppsRSAPrivateKeyState*)(new Ipp8u [keySize]);
    ippsRSA_InitPrivateKeyType2(bitSizeP, bitSizeQ, pPrvKeyType2, keySize);

    /* Allocate memory for decrypted plain text, not less than RSA modulus size. */
    int plainTextLen = bitSizeInBytes(bitSizeP + bitSizeQ);
    Ipp8u* pPlainText = new Ipp8u[plainTextLen];

    do {
        /* Set private key */
        status = ippsRSA_SetPrivateKeyType2(P, Q, DP, DQ, InvQ, pPrvKeyType2);
        if (!checkStatus("ippsRSA_SetPrivateKeyType2", ippStsNoErr, status))
            break;

        /* Calculate temporary buffer size */
        int bufSize = 0;
        status = ippsRSA_GetBufferSizePrivateKey(&bufSize, pPrvKeyType2);
        if (!checkStatus("ippsRSA_GetBufferSizePrivateKey", ippStsNoErr, status))
            break;

        /* Allocate memory for temporary buffer */
        Ipp8u* pScratchBuffer = new Ipp8u[bufSize];

        /* Decrypt message */
       status = ippsRSADecrypt_OAEP_rmf(cipherText,
                                        0  /* optional label to be assotiated with the message */,
                                        0, /* label length */
                                        pPlainText, &plainTextLen,
                                        pPrvKeyType2,
                                        ippsHashMethod_SHA1(),
                                        pScratchBuffer);

        if (pScratchBuffer) delete [] pScratchBuffer;

        if (!checkStatus("ippsRSADecrypt_OAEP_rmf", ippStsNoErr, status))
            break;

        if (0 != memcmp(sourceMessageRef, pPlainText, sizeof(sourceMessageRef)-1)) {
            printf("ERROR: Decrypted and plain text messages do not match\n");
            status = ippStsErr;
        }
    } while (0);

    PRINT_EXAMPLE_STATUS("ippsRSADecrypt_OAEP_rmf", "RSA-OAEP 1024 (SHA1) Type2 decryption", ippStsNoErr == status)

    if (pPlainText) delete [] pPlainText;
    if (pPrvKeyType2) delete [] (Ipp8u*)pPrvKeyType2;

    return status;
}

