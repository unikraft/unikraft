/*******************************************************************************
* Copyright 2010-2021 Intel Corporation
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
//     Intel(R) Integrated Performance Primitives. Cryptography Primitives.
//     Operations over GF(p) ectension.
// 
//     Context:
//        pcpgfpxgetsize.c()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpgfpstuff.h"
#include "pcpgfpxstuff.h"
#include "pcptool.h"

/* Get context size */
static int cpGFExGetSize(int elemLen, int pelmLen, int numpe)
{
   int ctxSize = 0;

   /* size of GFp engine */
   ctxSize = (Ipp32s)sizeof(gsModEngine)
            + elemLen*(Ipp32s)sizeof(BNU_CHUNK_T)    /* modulus  */
            + pelmLen*(Ipp32s)sizeof(BNU_CHUNK_T)*numpe; /* pool */

   ctxSize = (Ipp32s)sizeof(IppsGFpState)   /* size of IppsGFPState*/
           + ctxSize;               /* GFpx engine */
   return ctxSize;
}

/*F*
// Name: ippsGFpxGetSize
//
// Purpose: Gets the size of the context of a GF(p^d) field.
//
// Returns:                   Reason:
//     ippStsNullPtrErr        pSize == NULL.
//     ippStsContextMatchErr   !GFP_VALID_ID(pGroundGF)
//     ippStsBadArgErr         degree is greater than or equal to 9 or is less than 2.
//     ippStsNoErr             no error
//
// Parameters:
//     pGroundGF      Pointer to the context of the finite field GF(p) being extended.
//     degree         Degree of the extension.
//     pSize          Pointer to the buffer size, in bytes, needed for the IppsGFpState
//                    context.
//
*F*/

IPPFUN(IppStatus, ippsGFpxGetSize, (const IppsGFpState* pGroundGF, int degree, int* pSize))
{
   IPP_BAD_PTR2_RET(pGroundGF, pSize);
   IPP_BADARG_RET( degree<IPP_MIN_GF_EXTDEG || degree >IPP_MAX_GF_EXTDEG, ippStsBadArgErr);
   IPP_BADARG_RET( !GFP_VALID_ID(pGroundGF), ippStsContextMatchErr );

   #define MAX_GFx_SIZE     (1<<15)  /* max size (bytes) of GF element (32KB) */
   {
      int groundElmLen = GFP_FELEN(GFP_PMA(pGroundGF));
      Ipp64u elmLen64 = (Ipp64u)groundElmLen * (Ipp64u)sizeof(BNU_CHUNK_T) * (Ipp64u)degree;
      int elemLen = (int)IPP_LODWORD(elmLen64);
      *pSize = 0;
      IPP_BADARG_RET(elmLen64> MAX_GFx_SIZE, ippStsBadArgErr);

      *pSize = cpGFExGetSize(elemLen, elemLen, GFPX_POOL_SIZE);
      return ippStsNoErr;
   }
   #undef MAX_GFx_SIZE
}
