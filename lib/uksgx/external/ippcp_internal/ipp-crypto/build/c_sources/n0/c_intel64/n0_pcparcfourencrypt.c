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
//    Name: ippsARCFourEncrypt
//
// Purpose: Encrypt data stream.
//
// Returns:                Reason:
//    ippStsNullPtrErr        pCtx == NULL
//                            pSrc == NULL
//                            pDst == NULL
//    ippStsContextMatchErr   pCtx->idCtx != idCtxARCFOUR
//    ippStsLengthErr         length<1
//    ippStsNoErr             no errors
//
// Parameters:
//    pSrc        pointer to the source byte data block stream
//    pDst        pointer to the destination byte data block stream
//    length      stream length (bytes)
//    pCtx        ponter to the ARCFOUR context
//
// Note:
//    Convenience function only
*F*/
IPPFUN(IppStatus, ippsARCFourEncrypt, (const Ipp8u *pSrc, Ipp8u *pDst, int length,
                  IppsARCFourState *pCtx))
{
   /* test context */
   IPP_BAD_PTR1_RET(pCtx);
   IPP_BADARG_RET(!RC4_VALID_ID(pCtx), ippStsContextMatchErr);

   /* test source and destination pointers */
   IPP_BAD_PTR2_RET(pSrc, pDst);
   /* test stream length */
   IPP_BADARG_RET((length<1), ippStsLengthErr);

   /* process data */
   ARCFourProcessData(pSrc, pDst, length, pCtx);

   return ippStsNoErr;
}
