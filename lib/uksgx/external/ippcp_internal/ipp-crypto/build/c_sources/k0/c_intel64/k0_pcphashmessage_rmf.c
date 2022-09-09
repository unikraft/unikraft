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
// 
//  Purpose:
//     Cryptography Primitive.
//     Security Hash Standard
//     Generalized Functionality
// 
//  Contents:
//        ippsHashMessage_rmf()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcphash_rmf.h"
#include "pcptool.h"

/*F*
//    Name: ippsHashMessage_rmf
//
// Purpose: Hash of the whole message.
//
// Returns:                Reason:
//    ippStsNullPtrErr           pMD == NULL
//                               pMsg == NULL but len!=0
//    ippStsLengthErr            len <0
//    ippStsNoErr                no errors
//
// Parameters:
//    pMsg        pointer to the input message
//    len         input message length
//    pMD         address of the output digest
//    pMethod     hash methods
//
*F*/
IPPFUN(IppStatus, ippsHashMessage_rmf,(const Ipp8u* pMsg, int len, Ipp8u* pMD, const IppsHashMethod* pMethod))
{
   /* test method pointer */
   IPP_BAD_PTR1_RET(pMethod);
   /* test digest pointer */
   IPP_BAD_PTR1_RET(pMD);
   /* test message length */
   IPP_BADARG_RET(0>len, ippStsLengthErr);
   IPP_BADARG_RET((len && !pMsg), ippStsNullPtrErr);

   {
      /* message length in the multiple MBS and the rest */
      int msgLenBlks = len &(-pMethod->msgBlkSize);
      int msgLenRest = len - msgLenBlks;

      /* init hash */
      DigestSHA512 hash;
      pMethod->hashInit(hash);

      /* process main part of the message */
      if(msgLenBlks) {
         pMethod->hashUpdate(hash, pMsg, msgLenBlks);
         pMsg += msgLenBlks;
      }
      cpFinalize_rmf(hash,
                     pMsg, msgLenRest,
                     (Ipp64u)len, 0,
                     pMethod);

      pMethod->hashOctStr(pMD, hash);

      return ippStsNoErr;
   }
}
