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

/*
//
//  Purpose:
//     Cryptography Primitive.
//     AES Multi Buffer Encryption (CFB mode)
//
//  Contents:
//        ippsAES_EncryptCFB16_MB()
//
*/

#include "owncp.h"
#include "pcpaesm.h"
#include "aes_cfb_vaes_mb.h"
#include "aes_cfb_aesni_mb.h"


/*!
 *  \brief ippsAES_EncryptCFB16_MB
 *
 *  Name:         ippsAES_EncryptCFB16_MB
 *
 *  Purpose:      AES-CFB16 Multi Buffer Encryption
 *
 *  Parameters:
 *    \param[in]   pSrc                 Pointer to the array of source data
 *    \param[out]  pDst                 Pointer to the array of target data
 *    \param[in]   len                  Pointer to the array of input buffer lengths (in bytes)
 *    \param[in]   pCtx                 Pointer to the array of AES contexts
 *    \param[in]   pIV                  Pointer to the array of initialization vectors (IV)
 *    \param[out]  status               Pointer to the IppStatus array that contains status 
 *                                      for each processed buffer in encryption operation
 *    \param[in]   numBuffers           Number of buffers to be processed
 *
 *  Returns:                          Reason:
 *    \return ippStsNullPtrErr            Indicates an error condition if any of the specified pointers is NULL:
 *                                        NULL == pSrc
 *                                        NULL == pDst
 *                                        NULL == len
 *                                        NULL == pCtx
 *                                        NULL == pIV
 *                                        NULL == status
 *    \return ippStsContextMatchErr       Indicates an error condition if input buffers have different key sizes
 *    \return ippStsLengthErr             Indicates an error condition if numBuffers < 1
 *    \return ippStsErr                   One or more of performed operation executed with error
 *                                        Check status array for details
 *    \return ippStsNoErr                 No error
 */

IPPFUN(IppStatus, ippsAES_EncryptCFB16_MB, (const Ipp8u* pSrc[], Ipp8u* pDst[], int len[], const IppsAESSpec* pCtx[], 
                                            const Ipp8u* pIV[], IppStatus status[], int numBuffers))
{  
    int i;

    // Check input pointers
    IPP_BAD_PTR2_RET(pCtx, pIV);
    IPP_BAD_PTR4_RET(pSrc, pDst, len, status);

    // Check number of buffers to be processed
    IPP_BADARG_RET((numBuffers < 1), ippStsLengthErr);

    // Sequential check of all input buffers
    int isAllBuffersValid = 1;
    int numRounds = 0;
    for (i = 0; i < numBuffers; i++) {
        // Test source, target buffers and initialization pointers
        if (pSrc[i] == NULL || pDst[i] == NULL || pIV[i] == NULL || pCtx[i] == NULL) {
            status[i] = ippStsNullPtrErr;
            isAllBuffersValid = 0;
            continue;
        }

        // Test the context ID
        if(!VALID_AES_ID(pCtx[i])) {
            status[i] = ippStsContextMatchErr;
            isAllBuffersValid = 0;
            continue;
        }

        // Test stream length
        if (len[i] < 1) {
            status[i] = ippStsLengthErr;
            isAllBuffersValid = 0;
            continue;
        }

        // Test stream integrity
        if ((len[i] % CFB16_BLOCK_SIZE)) {
            status[i] = ippStsUnderRunErr;
            isAllBuffersValid = 0;
            continue;
        }

        numRounds = RIJ_NR(pCtx[i]);
        status[i] = ippStsNoErr;
    }

    // If any of the input buffer is not valid stop the processig
    IPP_BADARG_RET(!isAllBuffersValid, ippStsErr)

    // Check compatibility of the keys
    int referenceKeySize = RIJ_NK(pCtx[0]);
    for (i = 0; i < numBuffers; i++) {
        IPP_BADARG_RET((RIJ_NK(pCtx[i]) != referenceKeySize), ippStsContextMatchErr);
    }

    #if (_IPP32E>=_IPP32E_Y8)
    Ipp32u* enc_keys[AES_MB_MAX_KERNEL_SIZE];
    int tmp_len[AES_MB_MAX_KERNEL_SIZE];
    int buffersProcessed = 0;
    #endif

    #if(_IPP32E>=_IPP32E_K1)
    if (IsFeatureEnabled(ippCPUID_AVX512VAES)) {
        while(numBuffers > 0) {
            if (numBuffers > AES_MB_MAX_KERNEL_SIZE / 2) {
                for (i = 0; i < AES_MB_MAX_KERNEL_SIZE; i++) {
                    if (i >= numBuffers) {
                        tmp_len[i] = 0;
                        continue;
                    }

                    enc_keys[i] = (Ipp32u*)RIJ_EKEYS(pCtx[i + buffersProcessed]);
                    tmp_len[i] = len[i + buffersProcessed];
                }

                aes_cfb16_enc_vaes_mb16((const Ipp8u* const*)(pSrc + buffersProcessed), (pDst + buffersProcessed), tmp_len, numRounds, (const Ipp32u **)enc_keys, (pIV + buffersProcessed));
                numBuffers -= AES_MB_MAX_KERNEL_SIZE;
                buffersProcessed += AES_MB_MAX_KERNEL_SIZE;
            }
            else if (numBuffers > AES_MB_MAX_KERNEL_SIZE / 4 && numBuffers <= AES_MB_MAX_KERNEL_SIZE / 2) {
                for (i = 0; i < AES_MB_MAX_KERNEL_SIZE / 2; i++) {
                    if (i >= numBuffers) {
                        tmp_len[i] = 0;
                        continue;
                    }

                    enc_keys[i] = (Ipp32u*)RIJ_EKEYS(pCtx[i + buffersProcessed]);
                    tmp_len[i] = len[i + buffersProcessed];
                }

                aes_cfb16_enc_vaes_mb8((const Ipp8u* const*)(pSrc + buffersProcessed), (pDst + buffersProcessed), tmp_len, numRounds, (const Ipp32u **)enc_keys, (pIV + buffersProcessed));
                numBuffers -= AES_MB_MAX_KERNEL_SIZE / 2;
                buffersProcessed += AES_MB_MAX_KERNEL_SIZE / 2;
            }
            else if (numBuffers > 0 && numBuffers <= AES_MB_MAX_KERNEL_SIZE / 4) {
                for (i = 0; i < AES_MB_MAX_KERNEL_SIZE / 4; i++) {
                    if (i >= numBuffers) {
                        tmp_len[i] = 0;
                        continue;
                    }

                    enc_keys[i] = (Ipp32u*)RIJ_EKEYS(pCtx[i + buffersProcessed]);
                    tmp_len[i] = len[i + buffersProcessed];
                }

                aes_cfb16_enc_vaes_mb4((const Ipp8u* const*)(pSrc + buffersProcessed), (pDst + buffersProcessed), tmp_len, numRounds, (const Ipp32u **)enc_keys, (pIV + buffersProcessed));
                numBuffers -= AES_MB_MAX_KERNEL_SIZE / 4;
                buffersProcessed += AES_MB_MAX_KERNEL_SIZE / 4;
            }
            else {
                break;
            }
        }
    }
    #endif // if(_IPP32E>=_IPP32E_K1)

    #if (_IPP32E>=_IPP32E_Y8)
    if( IsFeatureEnabled(ippCPUID_AES) ) {
        while(numBuffers > 0) {
            for (i = 0; i < AES_MB_MAX_KERNEL_SIZE / 4; i++) {
                if (i >= numBuffers) {
                    tmp_len[i] = 0;
                    continue;
                }

                enc_keys[i] = (Ipp32u*)RIJ_EKEYS(pCtx[i + buffersProcessed]);
                tmp_len[i] = len[i + buffersProcessed];
            }

            aes_cfb16_enc_aesni_mb4((const Ipp8u* const*)(pSrc + buffersProcessed), (pDst + buffersProcessed), tmp_len, numRounds, (const Ipp32u **)enc_keys, (pIV + buffersProcessed));
            numBuffers -= AES_MB_MAX_KERNEL_SIZE / 4;
            buffersProcessed += AES_MB_MAX_KERNEL_SIZE / 4;
        }
    }
    #endif // (_IPP32E>=_IPP32E_Y8)

    for (i = 0; i < numBuffers; i++) {
        status[i] = ippsAESEncryptCFB(pSrc[i], pDst[i], len[i], CFB16_BLOCK_SIZE, pCtx[i], pIV[i]);
    }

    for (i = 0; i < numBuffers; i++) {
        if (status[i] != ippStsNoErr) {
            return ippStsErr;
        }
    }

    return ippStsNoErr;
}
