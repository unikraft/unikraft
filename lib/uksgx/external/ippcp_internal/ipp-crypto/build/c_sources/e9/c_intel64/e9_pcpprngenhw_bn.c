/*******************************************************************************
* Copyright 2015-2021 Intel Corporation
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
//     PRNG Functions
// 
//  Contents:
//        ippsPRNGenRDRAND_BN()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpbn.h"
#include "pcptool.h"
#include "pcpprng_genhw.h"

/*F*
// Name: ippsPRNGenRDRAND_BN
//
// Purpose: Generates a pseudorandom big number
//          based on RDRAND instruction of the specified nBits length.
//
// Returns:                   Reason:
//    ippStsNullPtrErr           NULL == pBuffer
//
//    ippStsLengthErr            1 > nBits
//                               nBits > BN_ROOM(pRand)
//
//    ippStsNotSupportedModeErr  unsupported rdrand instruction
//
//    ippStsErr                  random big number can't be generated
//
//    ippStsNoErr                no error
//
// Parameters:
//    pRand  pointer to the big number
//    nBits    number of bits be requested
//    pCtx     pointer to the context
*F*/
IPPFUN(IppStatus, ippsPRNGenRDRAND_BN,(IppsBigNumState* pRand, int nBits, void* pCtx))
{
   /* test random BN */
   IPP_BAD_PTR1_RET(pRand);
   IPP_BADARG_RET(!BN_VALID_ID(pRand), ippStsContextMatchErr);

   /* test sizes */
   IPP_BADARG_RET(nBits< 1, ippStsLengthErr);
   IPP_BADARG_RET(nBits> BN_ROOM(pRand)*BNU_CHUNK_BITS, ippStsLengthErr);

   IPP_UNREFERENCED_PARAMETER(pCtx);

   #if ((_IPP>=_IPP_G9) || (_IPP32E>=_IPP32E_E9))
   if( IsFeatureEnabled(ippCPUID_RDRAND) ) {
      BNU_CHUNK_T* pRandBN = BN_NUMBER(pRand);
      cpSize rndSize = BITS_BNU_CHUNK(nBits);
      BNU_CHUNK_T rndMask = MASK_BNU_CHUNK(nBits);

      if(cpRandHW_buffer((Ipp32u*)pRandBN, rndSize*(Ipp32s)(sizeof(BNU_CHUNK_T)/sizeof(Ipp32u)))) {
         pRandBN[rndSize-1] &= rndMask;

         FIX_BNU(pRandBN, rndSize);
         BN_SIZE(pRand) = rndSize;
         BN_SIGN(pRand) = ippBigNumPOS;

         return ippStsNoErr;
      }
      else
         return ippStsErr;
   }

   /* unsupported rdrand instruction */
   else
   #endif
      IPP_ERROR_RET(ippStsNotSupportedModeErr);
}
