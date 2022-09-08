/*******************************************************************************
* Copyright 2013-2021 Intel Corporation
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
//     AES encryption/decryption (ECB mode)
// 
//  Contents:
//     ippsAESEncryptECB()
// 
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpaesm.h"
#include "pcptool.h"

#if (_ALG_AES_SAFE_==_ALG_AES_SAFE_COMPACT_SBOX_)
#  include "pcprijtables.h"
#endif


/*
// AES-ECB ecnryption
//
// Parameters:
//    pSrc        pointer to the source data buffer
//    pDst        pointer to the target data buffer
//    nBlocks     number of ecnrypted data blocks
//    pCtx        pointer to the AES context
*/
static
void cpEncryptAES_ecb(const Ipp8u* pSrc, Ipp8u* pDst, int nBlocks, const IppsAESSpec* pCtx)
{
#if (_IPP>=_IPP_P8) || (_IPP32E>=_IPP32E_Y8)
   /* use pipelined version is possible */
   if(AES_NI_ENABLED==RIJ_AESNI(pCtx)) {
      EncryptECB_RIJ128pipe_AES_NI(pSrc, pDst, RIJ_NR(pCtx), RIJ_EKEYS(pCtx), nBlocks*MBS_RIJ128);
   }
   else
#endif
   {
      /* block-by-block encryption */
      RijnCipher encoder = RIJ_ENCODER(pCtx);

      while(nBlocks) {
         //encoder((const Ipp32u*)pSrc, (Ipp32u*)pDst, RIJ_NR(pCtx), RIJ_EKEYS(pCtx), (const Ipp32u (*)[256])RIJ_ENC_SBOX(pCtx));
         #if (_ALG_AES_SAFE_==_ALG_AES_SAFE_COMPACT_SBOX_)
         encoder(pSrc, pDst, RIJ_NR(pCtx), RIJ_EKEYS(pCtx), RijEncSbox/*NULL*/);
         #else
         encoder(pSrc, pDst, RIJ_NR(pCtx), RIJ_EKEYS(pCtx), NULL);
         #endif

         pSrc += MBS_RIJ128;
         pDst += MBS_RIJ128;
         nBlocks--;
      }
   }
}

/*F*
//    Name: ippsAESEncryptECB
//
// Purpose: AES-ECB encryption.
//
// Returns:                Reason:
//    ippStsNullPtrErr        pCtx == NULL
//                            pSrc == NULL
//                            pDst == NULL
//    ippStsContextMatchErr   !VALID_AES_ID()
//    ippStsLengthErr         dataLen <1
//    ippStsUnderRunErr       0!=(dataLen%MBS_RIJ128)
//    ippStsNoErr             no errors
//
// Parameters:
//    pSrc        pointer to the source data buffer
//    pDst        pointer to the target data buffer
//    len         input/output buffer length (in bytes)
//    pCtx        pointer to the AES context
//
*F*/
IPPFUN(IppStatus, ippsAESEncryptECB,(const Ipp8u* pSrc, Ipp8u* pDst, int len,
                                     const IppsAESSpec* pCtx))
{
   /* test context */
   IPP_BAD_PTR1_RET(pCtx);
   /* test the context ID */
   IPP_BADARG_RET(!VALID_AES_ID(pCtx), ippStsContextMatchErr);

   /* test source and target buffer pointers */
   IPP_BAD_PTR2_RET(pSrc, pDst);
   /* test stream length */
   IPP_BADARG_RET((len<1), ippStsLengthErr);
   /* test stream integrity */
   IPP_BADARG_RET((len&(MBS_RIJ128-1)), ippStsUnderRunErr);

   /* do encryption */
   {
      int nBlocks = len / MBS_RIJ128;

      #if(_IPP32E>=_IPP32E_K1)
      if (IsFeatureEnabled(ippCPUID_AVX512VAES))
         EncryptECB_RIJ128pipe_VAES_NI(pSrc, pDst, len, pCtx);
      else
      #endif
      cpEncryptAES_ecb(pSrc, pDst, nBlocks, pCtx);


      return ippStsNoErr;
   }
}
