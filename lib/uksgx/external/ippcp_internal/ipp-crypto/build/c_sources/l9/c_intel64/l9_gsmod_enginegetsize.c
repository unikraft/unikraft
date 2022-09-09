/*******************************************************************************
* Copyright 2017-2021 Intel Corporation
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
//     Cryptography Primitive. Modular Arithmetic Engine. General Functionality
// 
//  Contents:
//        gsModEngineGetSize()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpbnumisc.h"
#include "pcpbnuarith.h"
#include "gsmodstuff.h"
#include "pcptool.h"

/*F*
// Name: gsModEngineGetSize
//
// Purpose: Specifies size of size of ModEngine context (Montgomery).
//
// Returns:                Reason:
//      ippStsLengthErr     modulusBitSize < 1
//                          numpe < MOD_ENGINE_MIN_POOL_SIZE
//      ippStsNoErr         no errors
//
// Parameters:
//      numpe           length of pool
//      modulusBitSize  max modulus length (in bits)
//      pSize           pointer to size
//
*F*/

IPP_OWN_DEFN (IppStatus, gsModEngineGetSize, (int modulusBitSize, int numpe, int* pSize))
{
   int modLen  = BITS_BNU_CHUNK(modulusBitSize);
   int pelmLen = BITS_BNU_CHUNK(modulusBitSize);

   IPP_BADARG_RET(modulusBitSize<1, ippStsLengthErr);
   IPP_BADARG_RET(numpe<MOD_ENGINE_MIN_POOL_SIZE, ippStsLengthErr);

   /* allocates mimimal necessary to Montgomery based methods */
   *pSize = (Ipp32s)sizeof(gsModEngine)
           + modLen*(Ipp32s)(sizeof(BNU_CHUNK_T))        /* modulus  */
           + modLen*(Ipp32s)(sizeof(BNU_CHUNK_T))         /* mont_R   */
           + modLen*(Ipp32s)(sizeof(BNU_CHUNK_T))         /* mont_R^2 */
           + pelmLen*(Ipp32s)(sizeof(BNU_CHUNK_T))*numpe; /* buffers  */

   return ippStsNoErr;
}
