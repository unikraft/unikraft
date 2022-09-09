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
//               Intel(R) Integrated Performance Primitives
//                   Cryptographic Primitives (ippcp)
// 
//  Contents:
//        ippsMontInit()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpbn.h"
#include "pcpmontgomery.h"
#include "pcptool.h"

/*F*
// Name: ippsMontInit
//
// Purpose: Initializes the symbolic data structure and partitions the
//      specified buffer space.
//
// Returns:                Reason:
//      ippStsNullPtrErr    pCtx == NULL
//      ippStsLengthErr     length < 1
//                          length > BITS2WORD32_SIZE(BN_MAXBITSIZE)
//      ippStsNoErr         no errors
//
// Parameters:
//      method    selected exponential method (unused parameter)
//      length    max modulus length (in Ipp32u chunks)
//      pCtx      pointer to Montgomery context
*F*/
IPPFUN(IppStatus, ippsMontInit,(IppsExpMethod method, int length, IppsMontState* pCtx))
{
    IPP_BADARG_RET(length<1 || length>BITS2WORD32_SIZE(BN_MAXBITSIZE), ippStsLengthErr);

    IPP_BAD_PTR1_RET(pCtx);

    IPP_UNREFERENCED_PARAMETER(method);

    {
        return cpMontInit(length, MONT_DEFAULT_POOL_LENGTH, pCtx);
    }
}
