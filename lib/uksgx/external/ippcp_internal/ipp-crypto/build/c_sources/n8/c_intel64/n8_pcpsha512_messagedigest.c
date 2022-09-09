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
//     SHA512 message digest
// 
//  Contents:
//        cpSHA512MessageDigest()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcphash.h"
#include "pcphash_rmf.h"
#include "pcptool.h"
#include "pcpsha512stuff.h"

IPP_OWN_DEFN (IppStatus, cpSHA512MessageDigest, (DigestSHA512 hash, const Ipp8u* pMsg, int msgLen, const DigestSHA512 IV))
{
   /* test digest pointer */
   IPP_BAD_PTR1_RET(hash);
   /* test message length */
   IPP_BADARG_RET((msgLen<0), ippStsLengthErr);
   /* test message pointer */
   IPP_BADARG_RET((msgLen && !pMsg), ippStsNullPtrErr);

   {
      /* message length in the multiple MBS and the rest */
      int msgLenBlks = msgLen & (-MBS_SHA512);
      int msgLenRest = msgLen - msgLenBlks;

      /* init hash */
      hashInit(hash, IV);

      /* process main part of the message */
      if(msgLenBlks) {
         UpdateSHA512(hash, pMsg, msgLenBlks, sha512_cnt);
         pMsg += msgLenBlks;
      }

      cpFinalizeSHA512(hash, pMsg, msgLenRest, (Ipp64u)msgLen, 0);
      hash[0] = ENDIANNESS64(hash[0]);
      hash[1] = ENDIANNESS64(hash[1]);
      hash[2] = ENDIANNESS64(hash[2]);
      hash[3] = ENDIANNESS64(hash[3]);
      hash[4] = ENDIANNESS64(hash[4]);
      hash[5] = ENDIANNESS64(hash[5]);
      hash[6] = ENDIANNESS64(hash[6]);
      hash[7] = ENDIANNESS64(hash[7]);

      return ippStsNoErr;
   }
}
