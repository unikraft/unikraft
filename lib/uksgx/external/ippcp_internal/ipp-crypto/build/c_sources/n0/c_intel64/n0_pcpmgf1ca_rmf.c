/*******************************************************************************
* Copyright 2016-2021 Intel Corporation
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
//  Purpose:
//     Cryptography Primitive.
//     HASH based Mask Generation Functions
//
//  Contents:
//        ippsMGF1_rmf()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcphash_rmf.h"
#include "pcptool.h"

/*F*
//    Name: ippsMGF1
//
// Purpose: Mask Generation Functios.
//
// Returns:                Reason:
//    ippStsNullPtrErr           pMask == NULL
//                               pMethod ==NULL
//    ippStsLengthErr            seedLen <0
//                               maskLen <0
//    ippStsNoErr                no errors
//
// Parameters:
//    pSeed       pointer to the input stream
//    seedLen     input stream length (bytes)
//    pMaske      pointer to the ouput mask
//    maskLen     desired length of mask (bytes)
//    pMethod     hash method
//
//
// Note.
//    MGF1 defined in the IEEE P1363 standard.
//    MGF1 defined in the ANSI X9.63 standard and frequently called KDF (key Generation Function).
//    The fifference between MGF1 and MGF2 is negligible - counter i runs from 0 (in MGF1) and from 1 (in MGF2)
*F*/
IPPFUN(IppStatus, ippsMGF1_rmf,(const Ipp8u* pSeed, int seedLen, Ipp8u* pMask, int maskLen, const IppsHashMethod* pMethod))
{
   IPP_BAD_PTR2_RET(pMask, pMethod);
   IPP_BADARG_RET((seedLen<0)||(maskLen<0), ippStsLengthErr);

   {
      /* hash specific */
      int hashSize = pMethod->hashLen;

      int i, outLen;

      __ALIGN8 IppsHashState_rmf hashCtx;
      ippsHashInit_rmf(&hashCtx, pMethod);

      if(!pSeed)
         seedLen = 0;

      for(i=0,outLen=0; outLen<maskLen; i++) { /* counter i runs from 0 */
         Ipp8u cnt[4];
         cnt[0] = (Ipp8u)((i>>24) & 0xFF);
         cnt[1] = (Ipp8u)((i>>16) & 0xFF);
         cnt[2] = (Ipp8u)((i>>8)  & 0xFF);
         cnt[3] = (Ipp8u)(i & 0xFF);

         ippsHashUpdate_rmf(pSeed, seedLen, &hashCtx);
         ippsHashUpdate_rmf(cnt,   4,       &hashCtx);

         if((outLen + hashSize) <= maskLen) {
            ippsHashFinal_rmf(pMask+outLen, &hashCtx);
            outLen += hashSize;
         }
         else {
            Ipp8u md[MAX_HASH_SIZE];
            ippsHashFinal_rmf(md, &hashCtx);
            CopyBlock(md, pMask+outLen, maskLen-outLen);
            outLen = maskLen;
         }
      }

      return ippStsNoErr;
   }
}
