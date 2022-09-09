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
//     Digesting message according to SHA256
// 
//  Contents:
//        cpSHA256MessageDigest()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcphash.h"
#include "pcphash_rmf.h"
#include "pcptool.h"
#include "pcpsha256stuff.h"

IPP_OWN_DEFN (IppStatus, cpSHA256MessageDigest, (DigestSHA256 hash, const Ipp8u* pMsg, int msgLen, const DigestSHA256 IV))
{
   /* test digest pointer */
   IPP_BAD_PTR1_RET(hash);
   /* test message length */
   IPP_BADARG_RET((msgLen<0), ippStsLengthErr);
   /* test message pointer */
   IPP_BADARG_RET((msgLen && !pMsg), ippStsNullPtrErr);

   {
      /* select processing function */
      #if (_SHA_NI_ENABLING_==_FEATURE_ON_)
      cpHashProc updateFunc = UpdateSHA256ni;
      #elif (_SHA_NI_ENABLING_==_FEATURE_TICKTOCK_)
      cpHashProc updateFunc = IsFeatureEnabled(ippCPUID_SHA)? UpdateSHA256ni : UpdateSHA256;
      #else
      cpHashProc updateFunc = UpdateSHA256;
      #endif

      /* message length in the multiple MBS and the rest */
      int msgLenBlks = msgLen & (-MBS_SHA256);
      int msgLenRest = msgLen - msgLenBlks;

      /* init hash */
      hash[0] = IV[0];
      hash[1] = IV[1];
      hash[2] = IV[2];
      hash[3] = IV[3];
      hash[4] = IV[4];
      hash[5] = IV[5];
      hash[6] = IV[6];
      hash[7] = IV[7];

      /* process main part of the message */
      if(msgLenBlks) {
         updateFunc(hash, pMsg, msgLenBlks, sha256_cnt);
         pMsg += msgLenBlks;
      }

      cpFinalizeSHA256(hash, pMsg, msgLenRest, (Ipp64u)msgLen);
      hash[0] = ENDIANNESS32(hash[0]);
      hash[1] = ENDIANNESS32(hash[1]);
      hash[2] = ENDIANNESS32(hash[2]);
      hash[3] = ENDIANNESS32(hash[3]);
      hash[4] = ENDIANNESS32(hash[4]);
      hash[5] = ENDIANNESS32(hash[5]);
      hash[6] = ENDIANNESS32(hash[6]);
      hash[7] = ENDIANNESS32(hash[7]);

      return ippStsNoErr;
   }
}
