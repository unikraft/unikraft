/*******************************************************************************
* Copyright 2020-2021 Intel Corporation
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
  *  \brief SMS4 Cipher Block Chaining mode of operation (CBC) example
  *
  *  This example demonstrates usage of SMS4 block cipher
  *  run with CBC mode of operation. Encryption scheme.
  *
  *  The CBC mode of operation is implemented according to the
  *  "NIST Special Publication 800-38A: Recommendation for Block Cipher Modes of
  *  Operation" document:
  *
  *  https://csrc.nist.gov/publications/detail/sp/800-38a/final
  *
  */

#include <string.h>

#include "ippcp.h"
#include "examples_common.h"

/*! SMS4 block size in bytes */
static const int SMS4_BLOCK_SIZE = 16;

/*! Key size in bytes */
static const int KEY_SIZE = 16;

/*! Message size in bytes */
static const int SRC_LEN = 16;

/*! Plain text */
static Ipp8u plainText[SRC_LEN] = {
    0xAA,0xAA,0xAA,0xAA,0xBB,0xBB,0xBB,0xBB,
    0xCC,0xCC,0xCC,0xCC,0xDD,0xDD,0xDD,0xDD
};

/*! Cipher text */
static Ipp8u cipherText[SRC_LEN] = {
    0x78,0xEB,0xB1,0x1C,0xC4,0x0B,0x0A,0x48,
    0x31,0x2A,0xAE,0xB2,0x04,0x02,0x44,0xCB
};

/*! 128-bit secret key */
static Ipp8u key[KEY_SIZE] = {
    0x01,0x23,0x45,0x67,0x89,0xAB,0xCD,0xEF,
    0xFE,0xDC,0xBA,0x98,0x76,0x54,0x32,0x10
};

/*! Initialization vector for CBC mode.
 *  Size of initialization vector for SMS4-CBC shall be equal to the size of SMS4 block (16 bytes).
 */
static Ipp8u iv[SMS4_BLOCK_SIZE] = {
    0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
    0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F
};

/*! Main function  */
int main(void)
{
    /* Size of SMS4 context structure. It will be set up in ippsSMS4GetSize(). */
    int ctxSize = 0;

    Ipp8u pOut[SRC_LEN] = {};

    /* Internal function status */
    IppStatus status = ippStsNoErr;

    /* Pointer to SMS4 context structure */
    IppsSMS4Spec* pSMS4 = 0;

    do {
        /* 1. Get size needed for SMS4 context structure */
        status = ippsSMS4GetSize(&ctxSize);
        if (!checkStatus("ippsSMS4GetSize", ippStsNoErr, status))
            return status;

        /* 2. Allocate memory for SMS4 context structure */
        pSMS4 = (IppsSMS4Spec*)(new Ipp8u[ctxSize]);
        if (NULL == pSMS4) {
            printf("ERROR: Cannot allocate memory (%d bytes) for SMS4 context\n", ctxSize);
            return -1;
        }

        /* 3. Initialize SMS4 context */
        status = ippsSMS4Init(key, sizeof(key), pSMS4, ctxSize);
        if (!checkStatus("ippsSMS4Init", ippStsNoErr, status))
            break;

        /* 4. Encryption */
        status = ippsSMS4EncryptCBC(plainText, pOut, sizeof(plainText), pSMS4, iv);
        if (!checkStatus("ippsSMS4EncryptCBC", ippStsNoErr, status))
            break;

        /* Compare encrypted message and reference text */
        if (0 != memcmp(pOut, cipherText, sizeof(cipherText))) {
            printf("ERROR: Encrypted and reference messages do not match\n");
            break;
        }
    } while (0);

    /* 5. Remove secret and release resources */
    ippsSMS4Init(0, KEY_SIZE, pSMS4, ctxSize);
    if (pSMS4) delete [] (Ipp8u*)pSMS4;

    PRINT_EXAMPLE_STATUS("ippsSMS4EncryptCBC", "SMS4-CBC Encryption", !status)

    return status;
}
