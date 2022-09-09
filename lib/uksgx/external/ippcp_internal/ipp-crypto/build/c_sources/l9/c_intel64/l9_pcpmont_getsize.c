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
//        cpMontGetSize()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpbn.h"
#include "pcpmontgomery.h"

#include "pcptool.h"

/*F*
// Name: cpMontGetSize
//
// Purpose: Specifies size of buffer in bytes.
//
// Returns:                Reason:
//      ippStsNoErr         no errors
//
// Parameters:
//      poolLength   length of pool
//      maxLen32     max modulus length (in Ipp32u chunks)
//      pCtxSize     pointer to size of context
//
*F*/

IPP_OWN_DEFN (IppStatus, cpMontGetSize, (cpSize maxLen32, int poolLength, cpSize* pCtxSize))
{
   {
      int size = 0;
      int maxBitSize = maxLen32 << 5;
      gsModEngineGetSize(maxBitSize, poolLength, &size);

      *pCtxSize = (Ipp32s)sizeof(IppsMontState)
               + (cpSize)size;

      return ippStsNoErr;
   }
}
