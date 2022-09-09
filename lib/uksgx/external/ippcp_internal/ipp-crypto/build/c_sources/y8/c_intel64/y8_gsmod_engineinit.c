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
//        gsModEngineInit()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpbnumisc.h"
#include "pcpbnuarith.h"
#include "gsmodstuff.h"
#include "pcptool.h"

/*F*
// Name: gsModEngineInit
//
// Purpose: Initialization of the ModEngine context (Montgomery)
//
// Returns:                Reason:
//      ippStsLengthErr     modulusBitSize < 1
//                          numpe < MOD_ENGINE_MIN_POOL_SIZE
//      ippStsBadModulusErr (pModulus) && (pModulus[0] & 1) == 0
//      ippStsNoErr         no errors
//
// Parameters:
//      pME             pointer to ModEngine
//      pModulus        modulus
//      numpe           length of pool
//      modulusBitSize  max modulus length (in bits)
//      method          ModMethod
//
*F*/

IPP_OWN_DEFN (IppStatus, gsModEngineInit, (gsModEngine* pME, const Ipp32u* pModulus, int modulusBitSize, int numpe, const gsModMethod* method))
{
   IPP_BADARG_RET(modulusBitSize<1, ippStsLengthErr);
   IPP_BADARG_RET((pModulus) && (pModulus[0] & 1) == 0, ippStsBadModulusErr);
   IPP_BADARG_RET(numpe<MOD_ENGINE_MIN_POOL_SIZE, ippStsLengthErr);

/* convert bitsize nbits into  the number of BNU_CHUNK_T */
#define BITS_BNU_IPP32U(nbits) (((nbits)+31)/32)

   {
      int pelmLen = BITS_BNU_CHUNK(modulusBitSize);
      int modLen   = BITS_BNU_CHUNK(modulusBitSize);
      int modLen32 = BITS_BNU_IPP32U(modulusBitSize);
      Ipp8u* ptr = (Ipp8u*)pME;

      /* clear whole context */
      PadBlock(0, pME, sizeof(gsModEngine));

      MOD_PARENT(pME)   = NULL;
      MOD_EXTDEG(pME)   = 1;
      MOD_BITSIZE(pME)  = modulusBitSize;
      MOD_LEN(pME)      = modLen;
      MOD_PELEN(pME)    = pelmLen;
      MOD_METHOD(pME)   = method;
      MOD_MODULUS(pME)  = (BNU_CHUNK_T*)(ptr += sizeof(gsModEngine));
      MOD_MNT_R(pME)    = (BNU_CHUNK_T*)(ptr += modLen*(Ipp32s)sizeof(BNU_CHUNK_T));
      MOD_MNT_R2(pME)   = (BNU_CHUNK_T*)(ptr += modLen*(Ipp32s)sizeof(BNU_CHUNK_T));
      MOD_POOL_BUF(pME) = (BNU_CHUNK_T*)(ptr += modLen*(Ipp32s)sizeof(BNU_CHUNK_T));
      MOD_MAXPOOL(pME)  = numpe;
      MOD_USEDPOOL(pME) = 0;

      if (pModulus) {
         /* store modulus */
         ZEXPAND_COPY_BNU((Ipp32u*)MOD_MODULUS(pME), modLen * (cpSize)(sizeof(BNU_CHUNK_T) / sizeof(Ipp32u)), pModulus, modLen32);

         /* montgomery factor */
         MOD_MNT_FACTOR(pME) = gsMontFactor(MOD_MODULUS(pME)[0]);

         /* montgomery identity (R) */
         ZEXPAND_BNU(MOD_MNT_R(pME), 0, modLen);
         MOD_MNT_R(pME)[modLen] = 1;
         cpMod_BNU(MOD_MNT_R(pME), modLen+1, MOD_MODULUS(pME), modLen);

         /* montgomery domain converter (RR) */
         ZEXPAND_BNU(MOD_MNT_R2(pME), 0, modLen);
         COPY_BNU(MOD_MNT_R2(pME)+modLen, MOD_MNT_R(pME), modLen);
         cpMod_BNU(MOD_MNT_R2(pME), 2*modLen, MOD_MODULUS(pME), modLen);
      }
   }

#undef BITS_BNU_IPP32U

   return ippStsNoErr;
}
