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
//        cpPackMontCtx()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpbn.h"
#include "pcpmontgomery.h"
#include "pcptool.h"

/*F*
//    Name: cpPackMontCtx
//
// Purpose: Serialize mont context
//
// Parameters:
//    pCtx    context
//    pBuffer buffer
*F*/

IPP_OWN_DEFN (void, cpPackMontCtx, (const IppsMontState* pCtx, Ipp8u* pBuffer))
{
   /* size of context (bytes) */
   int ctxSize = sizeof(IppsMontState);
   CopyBlock(pCtx, pBuffer, ctxSize);

   pBuffer = (Ipp8u*)pBuffer + sizeof(IppsMontState);

   gsPackModEngineCtx(MNT_ENGINE(pCtx), pBuffer);
}
