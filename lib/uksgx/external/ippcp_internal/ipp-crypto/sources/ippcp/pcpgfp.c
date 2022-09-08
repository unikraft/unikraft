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
//     Operations over GF(p).
// 
//     Context:
//        cpGFpGetSize()
//        cpGFpInitGFp()
// 
// 
*/
#include "owndefs.h"
#include "owncp.h"

#include "pcpgfpstuff.h"
#include "pcpgfpxstuff.h"
#include "pcptool.h"


/*
// size of GFp engine context (Montgomery)
*/
IPP_OWN_DEFN (int, cpGFpGetSize, (int feBitSize, int peBitSize, int numpe))
{
   int ctxSize = 0;
   int elemLen = BITS_BNU_CHUNK(feBitSize);
   int pelmLen = BITS_BNU_CHUNK(peBitSize);
   
   /* size of GFp engine */
   ctxSize = (Ipp32s)sizeof(gsModEngine)
            + elemLen*(Ipp32s)sizeof(BNU_CHUNK_T)    /* modulus  */
            + elemLen*(Ipp32s)sizeof(BNU_CHUNK_T)    /* mont_R   */
            + elemLen*(Ipp32s)sizeof(BNU_CHUNK_T)    /* mont_R^2 */
            + elemLen*(Ipp32s)sizeof(BNU_CHUNK_T)    /* half of modulus */
            + elemLen*(Ipp32s)sizeof(BNU_CHUNK_T)    /* quadratic non-residue */
            + pelmLen*(Ipp32s)sizeof(BNU_CHUNK_T)*numpe; /* pool */

   ctxSize += sizeof(IppsGFpState);   /* size of IppsGFPState */
   return ctxSize;
}

/*
// init GFp engine context (Montgomery)
*/
static void cpGFEInit(gsModEngine* pGFE, int modulusBitSize, int peBitSize, int numpe)
{
   int modLen  = BITS_BNU_CHUNK(modulusBitSize);
   int pelmLen = BITS_BNU_CHUNK(peBitSize);

   Ipp8u* ptr = (Ipp8u*)pGFE;

   /* clear whole context */
   PadBlock(0, ptr, sizeof(gsModEngine));
   ptr += sizeof(gsModEngine);

   GFP_PARENT(pGFE)    = NULL;
   GFP_EXTDEGREE(pGFE) = 1;
   GFP_FEBITLEN(pGFE)  = modulusBitSize;
   GFP_FELEN(pGFE)     = modLen;
   GFP_FELEN32(pGFE)   = BITS2WORD32_SIZE(modulusBitSize);
   GFP_PELEN(pGFE)     = pelmLen;
 //GFP_METHOD(pGFE)    = method;
   GFP_MODULUS(pGFE)   = (BNU_CHUNK_T*)(ptr);   ptr += modLen*(Ipp32s)sizeof(BNU_CHUNK_T);
   GFP_MNT_R(pGFE)     = (BNU_CHUNK_T*)(ptr);   ptr += modLen*(Ipp32s)sizeof(BNU_CHUNK_T);
   GFP_MNT_RR(pGFE)    = (BNU_CHUNK_T*)(ptr);   ptr += modLen*(Ipp32s)sizeof(BNU_CHUNK_T);
   GFP_HMODULUS(pGFE)  = (BNU_CHUNK_T*)(ptr);   ptr += modLen*(Ipp32s)sizeof(BNU_CHUNK_T);
   GFP_QNR(pGFE)       = (BNU_CHUNK_T*)(ptr);   ptr += modLen*(Ipp32s)sizeof(BNU_CHUNK_T);
   GFP_POOL(pGFE)      = (BNU_CHUNK_T*)(ptr);/* ptr += modLen*(Ipp32s)sizeof(BNU_CHUNK_T);*/
   GFP_MAXPOOL(pGFE)   = numpe;
   GFP_USEDPOOL(pGFE)  = 0;

   cpGFpElementPad(GFP_MODULUS(pGFE), modLen, 0);
   cpGFpElementPad(GFP_MNT_R(pGFE), modLen, 0);
   cpGFpElementPad(GFP_MNT_RR(pGFE), modLen, 0);
   cpGFpElementPad(GFP_HMODULUS(pGFE), modLen, 0);
   cpGFpElementPad(GFP_QNR(pGFE), modLen, 0);
}

IPP_OWN_DEFN (IppStatus, cpGFpInitGFp, (int primeBitSize, IppsGFpState* pGF))
{
   IPP_BADARG_RET((primeBitSize< IPP_MIN_GF_BITSIZE) || (primeBitSize> IPP_MAX_GF_BITSIZE), ippStsSizeErr);
   IPP_BAD_PTR1_RET(pGF);

   {
      Ipp8u* ptr = (Ipp8u*)pGF;

      GFP_SET_ID(pGF);
      GFP_PMA(pGF) = (gsModEngine*)(ptr+sizeof(IppsGFpState));
      cpGFEInit(GFP_PMA(pGF), primeBitSize, primeBitSize+BITSIZE(BNU_CHUNK_T), GFP_POOL_SIZE);

      return ippStsNoErr;
   }
}
