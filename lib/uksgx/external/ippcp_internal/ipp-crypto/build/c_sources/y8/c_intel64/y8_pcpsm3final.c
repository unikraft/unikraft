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
//     Digesting message according to SM3
// 
//  Contents:
//        ippsSM3Final()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcphash.h"
#include "pcphash_rmf.h"
#include "pcptool.h"
#include "pcpsm3stuff.h"

/*F*
//    Name: ippsSM3Final
//
// Purpose: Stop message digesting and return digest.
//
// Returns:                Reason:
//    ippStsNullPtrErr        pState == NULL
//                            pMD == NULL
//    ippStsContextMatchErr   pState->idCtx != idCtxSM3
//    ippStsNoErr             no errors
//
// Parameters:
//    pMD         address of the output digest
//    pState      pointer to the SM3 state
//
*F*/
IPPFUN(IppStatus, ippsSM3Final,(Ipp8u* pMD, IppsSM3State* pState))
{
   /* test state pointer and ID */
   IPP_BAD_PTR1_RET(pState);
   IPP_BADARG_RET(!HASH_VALID_ID(pState, idCtxSM3), ippStsContextMatchErr);

   /* test digest pointer */
   IPP_BAD_PTR1_RET(pMD);

   cpFinalizeSM3(HASH_VALUE(pState), HASH_BUFF(pState), HAHS_BUFFIDX(pState), HASH_LENLO(pState));
   /* convert hash into big endian */
   ((Ipp32u*)pMD)[0] = ENDIANNESS32(HASH_VALUE(pState)[0]);
   ((Ipp32u*)pMD)[1] = ENDIANNESS32(HASH_VALUE(pState)[1]);
   ((Ipp32u*)pMD)[2] = ENDIANNESS32(HASH_VALUE(pState)[2]);
   ((Ipp32u*)pMD)[3] = ENDIANNESS32(HASH_VALUE(pState)[3]);
   ((Ipp32u*)pMD)[4] = ENDIANNESS32(HASH_VALUE(pState)[4]);
   ((Ipp32u*)pMD)[5] = ENDIANNESS32(HASH_VALUE(pState)[5]);
   ((Ipp32u*)pMD)[6] = ENDIANNESS32(HASH_VALUE(pState)[6]);
   ((Ipp32u*)pMD)[7] = ENDIANNESS32(HASH_VALUE(pState)[7]);

   /* re-init hash value */
   HAHS_BUFFIDX(pState) = 0;
   HASH_LENLO(pState) = 0;
   sm3_hashInit(HASH_VALUE(pState));

   return ippStsNoErr;
}
