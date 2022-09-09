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
//     RSA Functions
// 
//  Contents:
//        ippsRSA_Decrypt()
//
*/

#include "owncp.h"
#include "pcpbn.h"
#include "pcpngrsa.h"
#include "pcpngrsamethod.h"

/*F*
// Name: ippsRSA_Decrypt
//
// Purpose: Performs RSA Decryprion
//
// Returns:                   Reason:
//    ippStsNullPtrErr           NULL == pKey
//                               NULL == pCtxt
//                               NULL == pPtxt
//                               NULL == pBuffer
//
//    ippStsContextMatchErr     !RSA_PUB_KEY_VALID_ID()
//                              !BN_VALID_ID(pCtxt)
//                              !BN_VALID_ID(pPtxt)
//
//    ippStsIncompleteContextErr private key is not set up
//
//    ippStsOutOfRangeErr        pCtxt >= modulus
//                               pCtxt <0
//
//    ippStsSizeErr              BN_ROOM(pPtxt) is not enough
//
//    ippStsNoErr                no error
//
// Parameters:
//    pCtxt          pointer to the ciphertext
//    pPtxt          pointer to the plaintext
//    pKey           pointer to the key context
//    pScratchBuffer pointer to the temporary buffer
*F*/
IPPFUN(IppStatus, ippsRSA_Decrypt,(const IppsBigNumState* pCtxt,
                                         IppsBigNumState* pPtxt,
                                   const IppsRSAPrivateKeyState* pKey,
                                         Ipp8u* pBuffer))
{
   IPP_BAD_PTR2_RET(pKey, pBuffer);
   IPP_BADARG_RET(!RSA_PRV_KEY_VALID_ID(pKey), ippStsContextMatchErr);
   IPP_BADARG_RET(!RSA_PRV_KEY_IS_SET(pKey), ippStsIncompleteContextErr);

   IPP_BAD_PTR1_RET(pCtxt);
   IPP_BADARG_RET(!BN_VALID_ID(pCtxt), ippStsContextMatchErr);
   IPP_BADARG_RET(BN_NEGATIVE(pCtxt), ippStsOutOfRangeErr);
   IPP_BADARG_RET(0 <= cpCmp_BNU(BN_NUMBER(pCtxt), BN_SIZE(pCtxt),
                                 MOD_MODULUS(RSA_PRV_KEY_NMONT(pKey)), MOD_LEN(RSA_PRV_KEY_NMONT(pKey))), ippStsOutOfRangeErr);

   IPP_BAD_PTR1_RET(pPtxt);
   IPP_BADARG_RET(!BN_VALID_ID(pPtxt), ippStsContextMatchErr);
   IPP_BADARG_RET(BN_ROOM(pPtxt) < BITS_BNU_CHUNK(RSA_PRV_KEY_BITSIZE_N(pKey)), ippStsSizeErr);

   {
      BNU_CHUNK_T* pScratchBuffer = (BNU_CHUNK_T*)( IPP_ALIGNED_PTR(pBuffer, (int)sizeof(BNU_CHUNK_T)) );

      if(RSA_PRV_KEY1_VALID_ID(pKey))
         gsRSAprv_cipher(pPtxt, pCtxt, pKey, pScratchBuffer);
      else
         gsRSAprv_cipher_crt(pPtxt, pCtxt, pKey, pScratchBuffer);

      return ippStsNoErr;
   }
}
