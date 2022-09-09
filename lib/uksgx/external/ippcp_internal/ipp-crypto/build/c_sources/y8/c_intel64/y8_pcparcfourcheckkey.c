/*******************************************************************************
* Copyright 2005-2021 Intel Corporation
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
//     RC4 implementation
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcparcfour.h"
#include "pcptool.h"

/*F*
//    Name: ippsARCFourCheckKey
//
// Purpose: Checks key for weakness.
//
// Returns:                Reason:
//    ippStsNullPtrErr        pKey== NULL
//    ippStsNullPtrErr        pIsWeak== NULL
//    ippStsLengthErr         1 > keyLen
//                            keyLen > IPP_ARCFOUR_KEYMAX_SIZE
//    ippStsNoErr             no errors
//
// Parameters:
//    key         security key
//    keyLen      length of key (bytes)
//    pIsWeak     pointer to the result
//
// Note:
//    See ANDREW ROOS "A CLASS OF WEAK KEYS IN THE RC4 STREAM CIPHER"
//    (http://marcel.wanda.ch/Archive/WeakKeys)
*F*/
IPPFUN(IppStatus, ippsARCFourCheckKey, (const Ipp8u *pKey, int keyLen, IppBool* pIsWeak))
{
   /* test key */
   IPP_BAD_PTR1_RET(pKey);
   IPP_BADARG_RET(((1>keyLen)||(IPP_ARCFOUR_KEYMAX_SIZE< keyLen)), ippStsLengthErr);

   /* test result*/
   IPP_BAD_PTR1_RET(pIsWeak);

   if(1==keyLen)
      *pIsWeak = (pKey[0]==128)? ippTrue : ippFalse;
   else
      *pIsWeak = (pKey[0] + pKey[1])%256 == 0 ? ippTrue : ippFalse;

   return ippStsNoErr;
}
