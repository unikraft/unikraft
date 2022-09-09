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
//        ippsSHA384Final()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcphash.h"
#include "pcphash_rmf.h"
#include "pcptool.h"
#include "pcpsha512stuff.h"

/*F*
//    Name: ippsSHA384Final
//
// Purpose: Stop message digesting and return digest.
//
// Returns:                Reason:
//    ippStsNullPtrErr        pMD == NULL
//                            pState == NULL
//    ippStsContextMatchErr   pState->idCtx != idCtxSHA512
//    ippStsNoErr             no errors
//
// Parameters:
//    pMD         address of the output digest
//    pState      pointer to the SHA384 state
//
*F*/

IPPFUN(IppStatus, ippsSHA384Final,(Ipp8u* pMD, IppsSHA384State* pState))
{
   /* test state pointer and ID */
   IPP_BAD_PTR1_RET(pState);
   IPP_BADARG_RET(!HASH_VALID_ID(pState, idCtxSHA512), ippStsContextMatchErr);

   /* test digest pointer */
   IPP_BAD_PTR1_RET(pMD);

   cpFinalizeSHA512(HASH_VALUE(pState),
                    HASH_BUFF(pState), HAHS_BUFFIDX(pState),
                    HASH_LENLO(pState), HASH_LENHI(pState));
   /* convert hash into big endian */
   ((Ipp64u*)pMD)[0] = ENDIANNESS64(HASH_VALUE(pState)[0]);
   ((Ipp64u*)pMD)[1] = ENDIANNESS64(HASH_VALUE(pState)[1]);
   ((Ipp64u*)pMD)[2] = ENDIANNESS64(HASH_VALUE(pState)[2]);
   ((Ipp64u*)pMD)[3] = ENDIANNESS64(HASH_VALUE(pState)[3]);
   ((Ipp64u*)pMD)[4] = ENDIANNESS64(HASH_VALUE(pState)[4]);
   ((Ipp64u*)pMD)[5] = ENDIANNESS64(HASH_VALUE(pState)[5]);

   /* re-init hash value */
   HAHS_BUFFIDX(pState) = 0;
   HASH_LENLO(pState) = 0;
   HASH_LENHI(pState) = 0;
   sha512_384_hashInit(HASH_VALUE(pState));

   return ippStsNoErr;
}
