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
//     Digesting message according to MD5
//     (derived from the RSA Data Security, Inc. MD5 Message-Digest Algorithm)
// 
//     Equivalent code is available from RFC 1321.
// 
//  Contents:
//        ippsMD5MessageDigest()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcphash.h"
#include "pcphash_rmf.h"
#include "pcptool.h"
#include "pcpmd5stuff.h"

/*F*
//    Name: ippsMD5MessageDigest
//
// Purpose: Ddigest of the whole message.
//
// Returns:                Reason:
//    ippStsNullPtrErr        pMsg == NULL
//                            pMD == NULL
//    ippStsLengthErr         len <0
//    ippStsNoErr             no errors
//
// Parameters:
//    pMsg        pointer to the input message
//    len         input message length
//    pMD         address of the output digest
//
*F*/
IPPFUN(IppStatus, ippsMD5MessageDigest,(const Ipp8u* pMsg, int len, Ipp8u* pMD))
{
   /* test digest pointer */
   IPP_BAD_PTR1_RET(pMD);
   /* test message length */
   IPP_BADARG_RET((len<0), ippStsLengthErr);
   /* test message pointer */
   IPP_BADARG_RET((len && !pMsg), ippStsNullPtrErr);

   {
      /* message length in the multiple MBS and the rest */
      int msgLenBlks = len & (-MBS_MD5);
      int msgLenRest = len - msgLenBlks;

      /* init hash */
      /* init hash */
      ((Ipp32u*)(pMD))[0] = md5_iv[0];
      ((Ipp32u*)(pMD))[1] = md5_iv[1];
      ((Ipp32u*)(pMD))[2] = md5_iv[2];
      ((Ipp32u*)(pMD))[3] = md5_iv[3];

      /* process main part of the message */
      if(msgLenBlks) {
         UpdateMD5((Ipp32u*)pMD, pMsg, msgLenBlks, MD5_cnt);
         pMsg += msgLenBlks;
      }

      cpFinalizeMD5((Ipp32u*)pMD, pMsg, msgLenRest, (Ipp64u)len);

      return ippStsNoErr;
   }
}
