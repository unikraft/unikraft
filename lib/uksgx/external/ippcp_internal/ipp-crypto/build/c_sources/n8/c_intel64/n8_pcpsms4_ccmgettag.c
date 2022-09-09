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
//  Purpose:
//     Cryptography Primitive.
//     SMS4-CCM implementation.
// 
//     Content:
//        ippsSMS4_CCMGetTag()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpsms4authccm.h"
#include "pcptool.h"

/*F*
//    Name: ippsSMS4_CCMGetTag
//
// Purpose: Compute message auth tag and return one.
//          Note, that futher encryption/decryption and auth tag update is possible
//
// Returns:                Reason:
//    ippStsNullPtrErr        pTag == NULL
//                            pCtx == NULL
//    ippStsContextMatchErr   !VALID_SMS4CCM_ID()
//    ippStsLengthErr         MBS_SMS4 < tagLen
//                            1 > tagLen
//    ippStsNoErr             no errors
//
// Parameters:
//    pTag        pointer to the output authenticated tag
//    tagLen      requested length of the tag
//    pCtx        pointer to the CCM context
//
*F*/
IPPFUN(IppStatus, ippsSMS4_CCMGetTag,(Ipp8u* pTag, int tagLen, const IppsSMS4_CCMState* pCtx))
{
   /* test pCtx pointer */
   IPP_BAD_PTR1_RET(pCtx);

   /* test state ID */
   IPP_BADARG_RET(!VALID_SMS4CCM_ID(pCtx), ippStsContextMatchErr);

   /* test tag (pointer and length) */
   IPP_BAD_PTR1_RET(pTag);
   IPP_BADARG_RET((Ipp32u)tagLen>SMS4CCM_TAGLEN(pCtx) || tagLen<1, ippStsLengthErr);

   {  
      __ALIGN16 Ipp32u TMP[2*(MBS_SMS4/sizeof(Ipp32u)) + 1];
      /*
         MAC          size = MBS_SMS4/sizeof(Ipp32u)
         BLK          size = MBS_SMS4/sizeof(Ipp32u)
         flag         size = 1
      */
      
      Ipp32u*          MAC = TMP;
      Ipp32u*          BLK = TMP + (MBS_SMS4/sizeof(Ipp32u));
      Ipp32u*         flag = TMP + 2*(MBS_SMS4/sizeof(Ipp32u));

      *flag = (Ipp32u)( SMS4CCM_LENPRO(pCtx) &(MBS_SMS4-1) );

      CopyBlock16(SMS4CCM_MAC(pCtx), MAC);

      if(*flag) {
         /* SMS4 context */
         IppsSMS4Spec* pSMS4 = SMS4CCM_CIPHER(pCtx);

         FillBlock16(0, NULL,BLK, 0);
         CopyBlock(SMS4CCM_BLK(pCtx), (Ipp8u*)BLK, (cpSize)(*flag));

         XorBlock16(MAC, BLK, MAC);
         cpSMS4_Cipher((Ipp8u*)MAC, (Ipp8u*)MAC, SMS4_RK(pSMS4));

      }

      XorBlock(MAC, SMS4CCM_S0(pCtx), pTag, tagLen);

      /* clear secret data */
      PurgeBlock(TMP, sizeof(TMP));

      return ippStsNoErr;
   }
}
