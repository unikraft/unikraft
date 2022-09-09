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
//     SMS4 encryption/decryption
// 
//  Contents:
//        ippsSMS4DecryptCBC_CS1()
//
*/

#include "owncp.h"
#include "pcpsms4.h"
#include "pcptool.h"
#include "pcpsms4_decrypt_cbc.h"

/*F*
//    Name: ippsSMS4DecryptCBC_CS1
//
// Purpose: SMS4-CBC_CS1 decryption.
//
// Returns:                Reason:
//    ippStsNullPtrErr        pCtx == NULL
//                            pSrc == NULL
//                            pDst == NULL
//                            pIV  == NULL
//    ippStsContextMatchErr   !VALID_SMS4_ID()
//    ippStsLengthErr         len <MBS_SMS4
//    ippStsNoErr             no errors
//
// Parameters:
//    pSrc        pointer to the source data buffer
//    pDst        pointer to the target data buffer
//    len         input/output buffer length (in bytes)
//    pCtx        pointer to the SMS4 context
//    pIV         pointer to the initialization vector
//
*F*/
IPPFUN(IppStatus, ippsSMS4DecryptCBC_CS1,(const Ipp8u* pSrc, Ipp8u* pDst, int len,
                                          const IppsSMS4Spec* pCtx,
                                          const Ipp8u* pIV))
{
   /* test context */
   IPP_BAD_PTR1_RET(pCtx);
   /* test the context ID */
   IPP_BADARG_RET(!VALID_SMS4_ID(pCtx), ippStsContextMatchErr);

   /* test source, target buffers and initialization pointers */
   IPP_BAD_PTR3_RET(pSrc, pIV, pDst);
   /* test stream length */
   IPP_BADARG_RET((len<MBS_SMS4), ippStsLengthErr);

   {
      int tail = len & (MBS_SMS4-1); /* length of the last partial block */

      if(tail) {
         __ALIGN16 Ipp8u TMP[3*MBS_SMS4+1];

         /*
            lastIV     size = MBS_SMS4
            lastDecBlk size = 2*MBS_SMS4
            c          size = 1
         */

         Ipp8u*     lastIV = TMP;
         Ipp8u* lastDecBlk = TMP + MBS_SMS4;
         Ipp8u*          c = TMP + 3*MBS_SMS4;

         int n;

         len -= MBS_SMS4+tail;

         if(len) {
            CopyBlock16(pSrc+len-MBS_SMS4, lastIV);
            cpDecryptSMS4_cbc(pIV, pSrc, pDst, len, pCtx);
            pSrc += len;
            pDst += len;
         }
         else
            CopyBlock16(pIV, lastIV);

         /* decrypt last  block */
         cpSMS4_Cipher(lastDecBlk+MBS_SMS4, pSrc+tail,  SMS4_DRK(pCtx));

         CopyBlock16(lastDecBlk+MBS_SMS4, lastDecBlk);
         for(n=0; n<tail; n++) lastDecBlk[n] = pSrc[n];
         /* decrypt penultimate  block */
         cpSMS4_Cipher(lastDecBlk, lastDecBlk, SMS4_DRK(pCtx));

         for(n=0; n<MBS_SMS4; n++) {
            *c = pSrc[n];
            pDst[n] = lastDecBlk[n] ^ lastIV[n];
            lastIV[n] = pSrc[n+tail];
            lastDecBlk[n] = *c;
         }
         for(tail+=MBS_SMS4; n<tail; n++)
            pDst[n] = lastDecBlk[n] ^ lastDecBlk[n-MBS_SMS4];

         /* clear secret data */
         PurgeBlock(TMP, sizeof(TMP));
      }

      else
         cpDecryptSMS4_cbc(pIV, pSrc, pDst, len, pCtx);

      return ippStsNoErr;
   }
}
