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
//     AES-CMAC Functions
// 
//  Contents:
//        ippsAES_CMACGetTag()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpcmac.h"
#include "pcpaesm.h"
#include "pcptool.h"

#if (_ALG_AES_SAFE_==_ALG_AES_SAFE_COMPACT_SBOX_)
#  include "pcprijtables.h"
#endif

/*F*
//    Name: ippsAES_CMACGetTag
//
// Purpose: computes MD value and could contunue process.
//
// Returns:                Reason:
//    ippStsNullPtrErr        pMD == NULL
//                            pState == NULL
//    ippStsContextMatchErr   !VALID_AESCMAC_ID()
//    ippStsLengthErr         MBS_RIJ128 < mdLen <1
//    ippStsNoErr             no errors
//
// Parameters:
//    pMD      pointer to the output message digest
//    mdLen    requested length of the message digest
//    pState   pointer to the CMAC context
//
*F*/
IPPFUN(IppStatus, ippsAES_CMACGetTag,(Ipp8u* pMD, int mdLen, const IppsAES_CMACState* pState))
{
   /* test context pointer and ID */
   IPP_BAD_PTR1_RET(pState);

   IPP_BADARG_RET(!VALID_AESCMAC_ID(pState), ippStsContextMatchErr);
   /* test DAC pointer */
   IPP_BAD_PTR1_RET(pMD);
   IPP_BADARG_RET((mdLen<1)||(MBS_RIJ128<mdLen), ippStsLengthErr);

   {
      const IppsAESSpec* pAES = &CMAC_CIPHER(pState);
      /* setup encoder method */
      RijnCipher encoder = RIJ_ENCODER(pAES);

      Ipp8u locBuffer[MBS_RIJ128];
      Ipp8u locMac[MBS_RIJ128];
      CopyBlock16(CMAC_BUFF(pState), locBuffer);
      CopyBlock16(CMAC_MAC(pState), locMac);

      /* message length is divided by MBS_RIJ128 */
      if(MBS_RIJ128==CMAC_INDX(pState)) {
         XorBlock16(locBuffer, CMAC_K1(pState), locBuffer);
      }
      /* message length isn't divided by MBS_RIJ128 */
      else {
         PadBlock(0, locBuffer+CMAC_INDX(pState), MBS_RIJ128-CMAC_INDX(pState));
         locBuffer[CMAC_INDX(pState)] = 0x80;
         XorBlock16(locBuffer, CMAC_K2(pState), locBuffer);
      }

      XorBlock16(locBuffer, locMac, locMac);

      #if (_ALG_AES_SAFE_==_ALG_AES_SAFE_COMPACT_SBOX_)
      encoder(locMac, locMac, RIJ_NR(pAES), RIJ_EKEYS(pAES), RijEncSbox/*NULL*/);
      #else
      encoder(locMac, locMac, RIJ_NR(pAES), RIJ_EKEYS(pAES), NULL);
      #endif

      /* return truncated DAC */
      CopyBlock(locMac, pMD, mdLen);

      return ippStsNoErr;
   }
}
