/*******************************************************************************
* Copyright 2005-2021 Intel Corporation
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
//     HASH based Mask Generation Functions
// 
//  Contents:
//     ippsMGF_SHA1()
//     ippsMGF_SHA224()
//     ippsMGF_SHA256()
//     ippsMGF_SHA384()
//     ippsMGF_SHA512()
//     ippsMGF_MD5()
// 
// 
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcphash.h"
#include "pcptool.h"


/*F*
//    Name: ippsMGF_SHA1
//          ippsMGF_SHA224
//          ippsMGF_SHA256
//          ippsMGF_SHA384
//          ippsMGF_SHA512
//          ippsMGF_MD5
//
// Purpose: Mask Generation Functions.
//
// Returns:                Reason:
//    ippStsNullPtrErr           pMask == NULL
//    ippStsLengthErr            seedLen <0
//                               maskLen <0
//    ippStsNotSupportedModeErr  if algID is not match to supported hash alg
//    ippStsNoErr                no errors
//
// Parameters:
//    pSeed       pointer to the input stream
//    seedLen     input stream length (bytes)
//    pMask       pointer to the ouput mask
//    maskLen     desired length of mask (bytes)
//    hashAlg     identifier of the hash algorithm
//
*F*/
IPPFUN(IppStatus, ippsMGF, (const Ipp8u* pSeed, int seedLen, Ipp8u* pMask, int maskLen, IppHashAlgId hashAlg))
{
   /* get algorithm id */
   hashAlg = cpValidHashAlg(hashAlg);
   /* test hash alg */
   IPP_BADARG_RET(ippHashAlg_Unknown==hashAlg, ippStsNotSupportedModeErr);

   IPP_BAD_PTR1_RET(pMask);
   IPP_BADARG_RET((seedLen<0)||(maskLen<0), ippStsLengthErr);

   {
      /* hash specific */
      int hashSize = cpHashSize(hashAlg);

      int i, outLen;

      IppsHashState hashCtx;
      ippsHashInit(&hashCtx, hashAlg);

      if(!pSeed)
         seedLen = 0;

      for(i=0,outLen=0; outLen<maskLen; i++) {
         Ipp8u cnt[4];
         cnt[0] = (Ipp8u)((i>>24) & 0xFF);
         cnt[1] = (Ipp8u)((i>>16) & 0xFF);
         cnt[2] = (Ipp8u)((i>>8)  & 0xFF);
         cnt[3] = (Ipp8u)(i & 0xFF);

         cpReInitHash(&hashCtx, hashAlg);
         ippsHashUpdate(pSeed, seedLen, &hashCtx);
         ippsHashUpdate(cnt,   4,       &hashCtx);

         if((outLen + hashSize) <= maskLen) {
            ippsHashFinal(pMask+outLen, &hashCtx);
            outLen += hashSize;
         }
         else {
            Ipp8u md[MAX_HASH_SIZE];
            ippsHashFinal(md, &hashCtx);
            CopyBlock(md, pMask+outLen, maskLen-outLen);
            outLen = maskLen;
         }
      }

      return ippStsNoErr;
   }
}
