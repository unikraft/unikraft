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
//     AES-GCM
// 
//  Contents:
//        ippsAES_GCMStart()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpaesm.h"
#include "pcptool.h"

#if(_IPP32E>=_IPP32E_K0)
#include "pcpaesauthgcm_avx512.h"
#else
#include "pcpaesauthgcm.h"
#endif /* #if(_IPP32E>=_IPP32E_K1) */

/*F*
//    Name: ippsAES_GCMStart
//
// Purpose: Start the process of encryption or decryption and authentication tag generation.
//
// Returns:                Reason:
//    ippStsNullPtrErr        pState == NULL
//                            pIV == NULL, ivLen>0
//                            pAAD == NULL, aadLen>0
//    ippStsContextMatchErr   !AESGCM_VALID_ID()
//    ippStsLengthErr         ivLen < 0
//                            aadLen < 0
//    ippStsNoErr             no errors
//
// Parameters:
//    pIV         pointer to the IV (nonce)
//    ivLen       length of the IV in bytes
//    pAAD        pointer to the Addition Authenticated Data (header)
//    aadLen      length of the AAD in bytes
//    pState      pointer to the AES-GCM state
//
*F*/
IPPFUN(IppStatus, ippsAES_GCMStart,(const Ipp8u* pIV,  int ivLen,
                                    const Ipp8u* pAAD, int aadLen,
                                    IppsAES_GCMState* pState))
{
   IppStatus sts = ippsAES_GCMReset(pState);
   if(ippStsNoErr==sts)
      sts = ippsAES_GCMProcessIV(pIV, ivLen, pState);
   if(ippStsNoErr==sts)
      sts = ippsAES_GCMProcessAAD(pAAD, aadLen, pState);
   return sts;
}
