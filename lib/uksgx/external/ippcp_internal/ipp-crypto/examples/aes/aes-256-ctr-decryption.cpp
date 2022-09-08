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
  *  \brief AES Counter mode of operation (CTR) example
  *
  *  This example demonstrates usage of AES block cipher with 256-bit key
  *  run with CTR mode of operation. Decryption scheme.
  *
  *  The CTR mode of operation is implemented according to the
  *  "NIST Special Publication 800-38A: Recommendation for Block Cipher Modes of
  *  Operation" document:
  *
  *  https://csrc.nist.gov/publications/detail/sp/800-38a/final
  *
  */

#include <string.h>

#include "ippcp.h"
#include "examples_common.h"

/*! AES block size in bytes */
static const int AES_BLOCK_SIZE = 16;

/*! Key size in bytes */
static const int KEY_SIZE = 32;

/*! Message size in bytes */
static const int SRC_LEN = 16;

/*! Plain text */
static Ipp8u plainText[SRC_LEN] = {
    0x6b,0xc1,0xbe,0xe2,0x2e,0x40,0x9f,0x96,
    0xe9,0x3d,0x7e,0x11,0x73,0x93,0x17,0x2a
};

/*! Cipher text */
static Ipp8u cipherText[SRC_LEN] = {
    0x60,0x1e,0xc3,0x13,0x77,0x57,0x89,0xa5,
    0xb7,0xa7,0xf5,0x04,0xbb,0xf3,0xd2,0x28
};

/*! 256-bit secret key */
static Ipp8u key256[KEY_SIZE] = {
    0x60,0x3d,0xeb,0x10,0x15,0xca,0x71,0xbe,
    0x2b,0x73,0xae,0xf0,0x85,0x7d,0x77,0x81,
    0x1f,0x35,0x2c,0x07,0x3b,0x61,0x08,0xd7,
    0x2d,0x98,0x10,0xa3,0x09,0x14,0xdf,0xf4
};

/*! Initial counter for CTR mode.
 *  Size of counter for AES-CTR shall be equal to the size of AES block (16 bytes).
 */
static Ipp8u initialCounter[AES_BLOCK_SIZE] = {
    0xf0,0xf1,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,
    0xf8,0xf9,0xfa,0xfb,0xfc,0xfd,0xfe,0xff
};

/*! Main function  */
int main(void)
{
    /* Length of changeable bits in a counter (can be value starting from 1 till block size 128) */
    const Ipp32u counterLen = 128;

    /* Size of AES context structure. It will be set up in ippsAESGetSize(). */
    int ctxSize = 0;

    Ipp8u pOut[SRC_LEN]            = {};
    Ipp8u pCounter[AES_BLOCK_SIZE] = {};

    /* Internal function status */
    IppStatus status = ippStsNoErr;

    /* Pointer to AES context structure */
    IppsAESSpec* pAES = 0;

    do {
        /* 1. Get size needed for AES context structure */
        status = ippsAESGetSize(&ctxSize);
        if (!checkStatus("ippsAESGetSize", ippStsNoErr, status))
            return status;

        /* 2. Allocate memory for AES context structure */
        pAES = (IppsAESSpec*)(new Ipp8u[ctxSize]);
        if (NULL == pAES) {
            printf("ERROR: Cannot allocate memory (%d bytes) for AES context\n", ctxSize);
            return -1;
        }

        /* 3. Initialize AES context */
        status = ippsAESInit(key256, sizeof(key256), pAES, ctxSize);
        if (!checkStatus("ippsAESInit", ippStsNoErr, status))
            break;

        /* Initialize counter before decryption.
         * An updated counter value will be stored here after ippsAESDecryptCTR finishes.
         */
        memcpy(pCounter, initialCounter, sizeof(initialCounter));

        /* 4. Decryption */
        status = ippsAESDecryptCTR(cipherText, pOut, sizeof(cipherText), pAES, pCounter, counterLen);
        if (!checkStatus("ippsAESDecryptCTR", ippStsNoErr, status))
            break;

        /* Compare decrypted message and original text */
        if (0 != memcmp(pOut, plainText, sizeof(plainText))) {
            printf("ERROR: Decrypted and plain text messages do not match\n");
            break;
        }
    } while (0);

    /* 5. Remove secret and release resources */
    ippsAESInit(0, KEY_SIZE, pAES, ctxSize);
    if (pAES) delete [] (Ipp8u*)pAES;

    PRINT_EXAMPLE_STATUS("ippsAESDecryptCTR", "AES-CTR 256 Decryption", !status);

    return status;
}
