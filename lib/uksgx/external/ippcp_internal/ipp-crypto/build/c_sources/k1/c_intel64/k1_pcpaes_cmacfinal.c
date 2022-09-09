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
//        ippsAES_CMACFinal()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpcmac.h"
#include "pcpaesm.h"
#include "pcptool.h"
#include "pcpaes_cmac_stuff.h"

#if (_ALG_AES_SAFE_==_ALG_AES_SAFE_COMPACT_SBOX_)
#  include "pcprijtables.h"
#endif


/*F*
//    Name: ippsAES_CMACFinal
//
// Purpose: Stop message digesting and return MD.
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
IPPFUN(IppStatus, ippsAES_CMACFinal,(Ipp8u* pMD, int mdLen, IppsAES_CMACState* pState))
{
   /* test context pointer and ID */
   IPP_BAD_PTR1_RET(pState);
   /* test DAC pointer */
   IPP_BAD_PTR1_RET(pMD);
   IPP_BADARG_RET((mdLen<1)||(MBS_RIJ128<mdLen), ippStsLengthErr);

   {
      Ipp8u localMD[MBS_RIJ128];
      IppStatus sts = ippsAES_CMACGetTag(localMD, MBS_RIJ128, pState);

      if(ippStsNoErr==sts) {
         /* return truncated DAC */
         CopyBlock(localMD, pMD, mdLen);
         /* re-init context */
         init(pState);
      }

      return sts;
   }
}
