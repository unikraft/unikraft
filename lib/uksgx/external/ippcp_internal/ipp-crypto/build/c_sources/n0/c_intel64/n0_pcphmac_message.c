/*******************************************************************************
* Copyright 2014-2021 Intel Corporation
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
//     HMAC General Functionality
// 
//  Contents:
//        ippsHMAC_Message()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcphmac.h"
#include "pcptool.h"

/*F*
//    Name: ippsHMAC_Message
//
// Purpose: MAC (MD5) of the whole message.
//
// Returns:                Reason:
//    ippStsNullPtrErr           pMsg == NULL
//                               pKey == NULL
//                               pMD == NULL
//    ippStsLengthErr            msgLen <0
//                               keyLen <0
//                               size_of_digest < mdLen <1
//    ippStsNotSupportedModeErr  if algID is not match to supported hash alg
//    ippStsNoErr                no errors
//
// Parameters:
//    pMsg        pointer to the input message
//    msgLen      input message length
//    pKey        pointer to the secret key
//    keyLen      secret key length
//    pMD         pointer to message digest
//    mdLen       MD length
//    hashAlg     hash alg ID
//
*F*/
IPPFUN(IppStatus, ippsHMAC_Message,(const Ipp8u* pMsg, int msgLen,
                                   const Ipp8u* pKey, int keyLen,
                                   Ipp8u* pMD, int mdLen,
                                   IppHashAlgId hashAlg))
{
   /* get algorithm id */
   hashAlg = cpValidHashAlg(hashAlg);
   /* test hash alg */
   IPP_BADARG_RET(ippHashAlg_Unknown==hashAlg, ippStsNotSupportedModeErr);

   /* test secret key pointer and length */
   IPP_BAD_PTR1_RET(pKey);
   IPP_BADARG_RET((keyLen<0), ippStsLengthErr);

   /* test input message pointer and length */
   IPP_BADARG_RET((msgLen<0), ippStsLengthErr);
   IPP_BADARG_RET((msgLen && !pMsg), ippStsNullPtrErr);

   /* test MD pointer and length */
   IPP_BAD_PTR1_RET(pMD);
   IPP_BADARG_RET(0>=mdLen || mdLen>cpHashSize(hashAlg), ippStsLengthErr);

   {
      __ALIGN8 IppsHMACState ctx;
      IppStatus sts = ippsHMAC_Init(pKey, keyLen, &ctx, hashAlg);
      if(ippStsNoErr!=sts) goto exit;

      sts = ippsHashUpdate(pMsg,msgLen, &HASH_CTX(&ctx));
      if(ippStsNoErr!=sts) goto exit;

      sts = ippsHMAC_Final(pMD, mdLen, &ctx);

      exit:
      PurgeBlock(&ctx, sizeof(IppsHMACState));
      return sts;
   }
}
