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
//        pcpgfpec_initgfpxctx.c()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpgfpstuff.h"
#include "pcpgfpxstuff.h"
#include "pcptool.h"


/* the "static" specificator removed because of incorrect result under Linux-32, p8
   what's wrong? not know maybe compiler (icl 2017)
   need to check after switchng on icl 2018
   */
/*static*/ 
IPP_OWN_DEFN (void, InitGFpxCtx, (const IppsGFpState* pGroundGF, int extDeg, const IppsGFpMethod* method, IppsGFpState* pGFpx))
{
   gsModEngine* pGFEp = GFP_PMA(pGroundGF);
   int elemLen = extDeg * GFP_FELEN(pGFEp);
   int elemLen32 = extDeg * GFP_FELEN32(pGFEp);

   Ipp8u* ptr = (Ipp8u*)pGFpx + sizeof(IppsGFpState);

   /* context identifier */
   GFP_SET_ID(pGFpx);
   GFP_PMA(pGFpx) = (gsModEngine*)ptr;
   {
      gsModEngine* pGFEx = GFP_PMA(pGFpx);

      /* clear whole context */
      PadBlock(0, ptr, sizeof(gsModEngine));
      ptr += sizeof(gsModEngine);

      GFP_PARENT(pGFEx)    = pGFEp;
      GFP_EXTDEGREE(pGFEx) = extDeg;
      GFP_FEBITLEN(pGFEx)  = 0;//elemBitLen;
      GFP_FELEN(pGFEx)     = elemLen;
      GFP_FELEN32(pGFEx)   = elemLen32;
      GFP_PELEN(pGFEx)     = elemLen;
      GFP_METHOD(pGFEx)    = method->arith;
      GFP_MODULUS(pGFEx)   = (BNU_CHUNK_T*)(ptr);  ptr += elemLen * (Ipp32s)sizeof(BNU_CHUNK_T);  /* field polynomial */
      GFP_POOL(pGFEx)      = (BNU_CHUNK_T*)(ptr);                                         /* pool */
      GFP_MAXPOOL(pGFEx)   = GFPX_POOL_SIZE;
      GFP_USEDPOOL(pGFEx)  = 0;

      cpGFpElementPad(GFP_MODULUS(pGFEx), elemLen, 0);
   }
}
