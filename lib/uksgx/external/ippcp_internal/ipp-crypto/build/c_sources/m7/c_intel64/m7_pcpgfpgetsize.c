/*******************************************************************************
* Copyright 2018-2021 Intel Corporation
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
//     Operations over GF(p).
// 
//     Context:
//        ippsGFpGetSize()
// 
*/
#include "owndefs.h"
#include "owncp.h"

#include "pcpgfpstuff.h"
#include "pcpgfpxstuff.h"
#include "pcptool.h"


/*F*
// Name: ippsGFpGetSize
//
// Purpose: Gets the size of the context of a GF(q) field
//
// Returns:                   Reason:               
//     ippStsNullPtrErr        pSize == NULL.                   
//     ippStsSizeErr           bitSize is less than 2 or greater than 1024.
//     ippStsNoErr             no error
//
// Parameters:
//     feBitSize        Size, in bytes, of the odd prime number q (modulus of GF(q)).
//     pSize            Pointer to the buffer size, in bytes, needed for the IppsGFpState
//                      context.
//     
*F*/

IPPFUN(IppStatus, ippsGFpGetSize,(int feBitSize, int* pSize))
{
   IPP_BAD_PTR1_RET(pSize);
   IPP_BADARG_RET((feBitSize < 2) || (feBitSize > GFP_MAX_BITSIZE), ippStsSizeErr);

   *pSize = cpGFpGetSize(feBitSize, feBitSize+BITSIZE(BNU_CHUNK_T), GFP_POOL_SIZE);
   return ippStsNoErr;
}
