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
//     Intel(R) Integrated Performance Primitives. Cryptography Primitives.
// 
//     Context:
//        ippsAES_CCMGetTag()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpaesauthccm.h"
#include "pcptool.h"

#if (_ALG_AES_SAFE_==_ALG_AES_SAFE_COMPACT_SBOX_)
#  include "pcprijtables.h"
#endif

/*F*
//    Name: ippsAES_CCMGetTag
//
// Purpose: Compute message auth tag and return one.
//          Note, that futher encryption/decryption and auth tag update is possible
//
// Returns:                Reason:
//    ippStsNullPtrErr        pTag == NULL
//                            pState == NULL
//    ippStsContextMatchErr   !VALID_AESCCM_ID()
//    ippStsLengthErr         MBS_RIJ128 < tagLen
//                            1 > tagLen
//    ippStsNoErr             no errors
//
// Parameters:
//    pTag        pointer to the output authenticated tag
//    tagLen      requested length of the tag
//    pState      pointer to the CCM context
//
*F*/
IPPFUN(IppStatus, ippsAES_CCMGetTag,(Ipp8u* pTag, int tagLen, const IppsAES_CCMState* pState))
{
   /* test pState pointer */
   IPP_BAD_PTR1_RET(pState);

   /* test state ID */
   IPP_BADARG_RET(!VALID_AESCCM_ID(pState), ippStsContextMatchErr);

   /* test tag (pointer and length) */
   IPP_BAD_PTR1_RET(pTag);
   IPP_BADARG_RET((Ipp32u)tagLen>AESCCM_TAGLEN(pState) || tagLen<1, ippStsLengthErr);

   {
      Ipp32u flag = (Ipp32u)( AESCCM_LENPRO(pState) &(MBS_RIJ128-1) );

      Ipp32u MAC[NB(128)];
      CopyBlock16(AESCCM_MAC(pState), MAC);

      if(flag) {
         IppsAESSpec* pAES = AESCCM_CIPHER(pState);
         RijnCipher encoder = RIJ_ENCODER(pAES);

         Ipp8u  BLK[MBS_RIJ128];
         FillBlock16(0, NULL,BLK, 0);
         CopyBlock(AESCCM_BLK(pState), BLK, (cpSize)flag);

         XorBlock16(MAC, BLK, MAC);
         #if (_ALG_AES_SAFE_==_ALG_AES_SAFE_COMPACT_SBOX_)
         encoder((Ipp8u*)MAC, (Ipp8u*)MAC, RIJ_NR(pAES), RIJ_EKEYS(pAES), RijEncSbox/*NULL*/);
         #else
         encoder((Ipp8u*)MAC, (Ipp8u*)MAC, RIJ_NR(pAES), RIJ_EKEYS(pAES), NULL);
         #endif
      }

      XorBlock(MAC, AESCCM_S0(pState), pTag, (cpSize)tagLen);
      return ippStsNoErr;
   }
}
