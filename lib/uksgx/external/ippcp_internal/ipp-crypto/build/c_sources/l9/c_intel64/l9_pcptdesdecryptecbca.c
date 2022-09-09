/*******************************************************************************
* Copyright 2002-2021 Intel Corporation
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
//     Encrypt byte data stream according to TDES (ECB mode)
// 
//  Contents:
//     ippsTDESDecryptECB()
// 
// 
*/


#include "owndefs.h"

#include "owncp.h"
#include "pcpdes.h"
#include "pcptool.h"


/*F*
//    Name: ippsTDESDecryptECB
//
// Purpose: Decrypt byte data stream according to TDES in EBC mode.
//
// Returns:                Reason:
//    ippStsNullPtrErr        pCtx1 == NULL
//                            pCtx2 == NULL
//                            pCtx3 == NULL
//                            pSrc == NULL
//                            pDst == NULL
//    ippStsContextMatchErr   pCtx1->idCtx != idCtxDES
//                            pCtx2->idCtx != idCtxDES
//                            pCtx3->idCtx != idCtxDES
//    ippStsLengthErr         length < 1
//    ippStsUnderRunErr       (length & 7)
//    ippStsNoErr             no errors
//
// Parameters:
//    pSrc        pointer to the source byte data stream
//    pDst        pointer to the target byte data stream
//    length      plaintext stream length (bytes)
//    pCtx1-3     pointers to the DES context
//    padding     the padding scheme indicator
//
*F*/
IPPFUN(IppStatus, ippsTDESDecryptECB,(const Ipp8u* pSrc, Ipp8u* pDst, int len,
                                      const IppsDESSpec* pCtx1,
                                      const IppsDESSpec* pCtx2,
                                      const IppsDESSpec* pCtx3,
                                      IppsPadding padding))
{
   /* test contexts */
   IPP_BAD_PTR3_RET(pCtx1, pCtx2, pCtx3);

   IPP_BADARG_RET(!VALID_DES_ID(pCtx1), ippStsContextMatchErr);
   IPP_BADARG_RET(!VALID_DES_ID(pCtx2), ippStsContextMatchErr);
   IPP_BADARG_RET(!VALID_DES_ID(pCtx3), ippStsContextMatchErr);
   /* test source and destination pointers */
   IPP_BAD_PTR2_RET(pSrc, pDst);
   /* test stream length */
   IPP_BADARG_RET((len<1), ippStsLengthErr);

   /* force ippPaddingNONE padding */
   if(ippPaddingNONE!=padding)
      padding = ippPaddingNONE;
   /* test stream integrity */
   IPP_BADARG_RET((len&7), ippStsUnderRunErr);

   {
      int nBlocks = len/MBS_DES;

      /* misaligned source and/or target */
      if( (IPP_UINT_PTR(pSrc) & 0x7) || (IPP_UINT_PTR(pDst) & 0x7) ) {
         int n;
         for(n=0; n<nBlocks; n++) {
            Ipp64u block;
            CopyBlock8(pSrc, &block);
            block = Cipher_DES(block, DES_DKEYS(pCtx3), DESspbox);
            block = Cipher_DES(block, DES_EKEYS(pCtx2), DESspbox);
            block = Cipher_DES(block, DES_DKEYS(pCtx1), DESspbox);
            CopyBlock8(&block, pDst);
            pSrc += MBS_DES;
            pDst += MBS_DES;
         }
      }

      /* aligned source and/or target */
      else {
         const RoundKeyDES* pRKey[3];
         pRKey[0] = DES_DKEYS(pCtx3);
         pRKey[1] = DES_EKEYS(pCtx2);
         pRKey[2] = DES_DKEYS(pCtx1);

         ECB_TDES((const Ipp64u*)pSrc, (Ipp64u*)pDst, nBlocks, pRKey, DESspbox);
         //pSrc += nBlocks*MBS_DES;
         //pDst += nBlocks*MBS_DES;
      }

      return ippStsNoErr;
   }
}
