/*******************************************************************************
* Copyright 2017-2021 Intel Corporation
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
//     Intel(R) Integrated Performance Primitives. Cryptography Primitives.
// 
//     Context:
//        ippsSMS4_CCMEncrypt()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpsms4authccm.h"
#include "pcptool.h"

/*F*
//    Name: ippsSMS4_CCMEncrypt
//
// Purpose: Encrypts data and updates authentication tag.
//
// Returns:                Reason:
//    ippStsNullPtrErr        pCtx== NULL
//                            pSrc == NULL
//                            pDst == NULL
//    ippStsContextMatchErr   !VALID_SMS4CCM_ID()
//    ippStsLengthErr         if exceed overall length of message is being processed
//    ippStsNoErr             no errors
//
// Parameters:
//    pSrc        pointer to the plane text beffer
//    pDst        pointer to the cipher text bubber
//    len         length of the buffer
//    pCtx        pointer to the CCM context
//
*F*/

/*
//
// NOTE
//
// We consider to not spend time for further optimizing of this algoritm because it is not widely using.
// There is two ways for further optimization:
// - Add parallel processing of CTR cipher. Performance advantages can be achieved by parallel processing of a big number of blocks.
// - Try to decreace the memory reading/writing operation number, eg combine two calls of one block encryption 
// function to single call that procees two blocks with the same key. Performance advantages can be achieved by 
// reducing of reading/writing operations number, because we do not need to read the key twice in single loop.
//
*/

IPPFUN(IppStatus, ippsSMS4_CCMEncrypt,(const Ipp8u* pSrc, Ipp8u* pDst, int len, IppsSMS4_CCMState* pCtx))
{
   /* test pCtx pointer */
   IPP_BAD_PTR1_RET(pCtx);
   IPP_BADARG_RET(!VALID_SMS4CCM_ID(pCtx), ippStsContextMatchErr);

   /* test source/destination data */
   IPP_BAD_PTR2_RET(pSrc, pDst);

   /* test message length */
   IPP_BADARG_RET(len<0 || SMS4CCM_LENPRO(pCtx)+(Ipp64u)len >SMS4CCM_MSGLEN(pCtx), ippStsLengthErr);

   /*
   // enctypt payload and update MAC
   */
   if(len) {
      /* SMS4 context */
      IppsSMS4Spec* pSMS4 = SMS4CCM_CIPHER(pCtx);

      /* buffer for secret data */
      __ALIGN16 Ipp32u TMP[3*(MBS_SMS4/sizeof(Ipp32u))+6];

      /*
         MAC          size = MBS_SMS4/sizeof(Ipp32u)
         CTR          size = MBS_SMS4/sizeof(Ipp32u)
         S            size = MBS_SMS4/sizeof(Ipp32u)
         flag         size = 1
         qLen         size = 1
         tmpLen       size = 1
         counterVal   size = 1
         counterEnc   size = 2
      */

      Ipp32u*          MAC = TMP;
      Ipp32u*          CTR = TMP + MBS_SMS4/sizeof(Ipp32u);          
      Ipp32u*            S = TMP + 2*(MBS_SMS4/sizeof(Ipp32u));     
      Ipp32u*         flag = TMP + 3*(MBS_SMS4/sizeof(Ipp32u));      
      Ipp32u*         qLen = TMP + 3*(MBS_SMS4/sizeof(Ipp32u)) + 1;
      Ipp32u*       tmpLen = TMP + 3*(MBS_SMS4/sizeof(Ipp32u)) + 2;
      Ipp32u*   counterVal = TMP + 3*(MBS_SMS4/sizeof(Ipp32u)) + 3;
      Ipp32u*   counterEnc = TMP + 3*(MBS_SMS4/sizeof(Ipp32u)) + 4;

      *flag = (Ipp32u)( SMS4CCM_LENPRO(pCtx) &(MBS_SMS4-1) );

      /* extract from the state */
      CopyBlock16(SMS4CCM_MAC(pCtx), MAC);
      CopyBlock16(SMS4CCM_CTR0(pCtx), CTR);
      CopyBlock16(SMS4CCM_Si(pCtx), S);
      *counterVal = SMS4CCM_COUNTER(pCtx);

      /* extract qLen */
      *qLen = (((Ipp8u*)CTR)[0] &0x7) +1; /* &0x7 just to fix KW issue */

      if(*flag) {
         *tmpLen = IPP_MIN((Ipp32u)len, MBS_SMS4-1);

         /* copy as much input as possible into the internal buffer*/
         CopyBlock(pSrc, SMS4CCM_BLK(pCtx)+*flag, (cpSize)(*tmpLen));

         XorBlock(pSrc, (Ipp8u*)S+*flag, pDst, (cpSize)(*tmpLen));

         /* update MAC */
         if(*flag+*tmpLen == MBS_SMS4) {
            XorBlock16(MAC, SMS4CCM_BLK(pCtx), MAC);
            cpSMS4_Cipher((Ipp8u*)MAC, (Ipp8u*)MAC, SMS4_RK(pSMS4));
         }

         SMS4CCM_LENPRO(pCtx) += (Ipp32u)*tmpLen;
         pSrc += *tmpLen;
         pDst += *tmpLen;
         len  -= *tmpLen;
      }

      while(len >= MBS_SMS4) {
         /* update MAC */
         XorBlock16(MAC, pSrc, MAC);
         cpSMS4_Cipher((Ipp8u*)MAC, (Ipp8u*)MAC, SMS4_RK(pSMS4));

         /* increment counter and format counter block */
         *counterVal+=1;
         CopyBlock(CounterEnc(counterEnc, (Ipp32s)(*qLen), *counterVal), ((Ipp8u*)CTR)+MBS_SMS4-*qLen, (Ipp32s)(*qLen));
         /* encode counter block */
         cpSMS4_Cipher((Ipp8u*)S, (Ipp8u*)CTR, SMS4_RK(pSMS4));

         /* store cipher text */
         XorBlock16(pSrc, S, pDst);

         SMS4CCM_LENPRO(pCtx) += MBS_SMS4;
         pSrc += MBS_SMS4;
         pDst += MBS_SMS4;
         len  -= MBS_SMS4;
      }

      if(len) {
         /* store partial data block */
         CopyBlock_safe(pSrc, len, SMS4CCM_BLK(pCtx), MBS_SMS4);

         /* increment counter and format counter block */
         *counterVal+=1;
         CopyBlock(CounterEnc(counterEnc, (Ipp32s)(*qLen), *counterVal), ((Ipp8u*)CTR)+MBS_SMS4-*qLen, (Ipp32s)(*qLen));
         /* encode counter block */
         cpSMS4_Cipher((Ipp8u*)S, (Ipp8u*)CTR, SMS4_RK(pSMS4));

         /* store cipher text */
         XorBlock(pSrc, S, pDst, len);

         SMS4CCM_LENPRO(pCtx) += (Ipp64u)len;
      }

      /* update state */
      CopyBlock16(MAC, SMS4CCM_MAC(pCtx));
      CopyBlock16(S, SMS4CCM_Si(pCtx));
      SMS4CCM_COUNTER(pCtx) = *counterVal;

      /* clear secret data */
      PurgeBlock(TMP, sizeof(TMP));
   }

   return ippStsNoErr;
}
