/*******************************************************************************
* Copyright 2013-2021 Intel Corporation
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
//     RSA Functions
// 
//  Contents:
//        gsRSAprv_cipher_crt()
//
*/

#include "owncp.h"
#include "pcpbn.h"
#include "pcpngrsa.h"
#include "pcpngrsamethod.h"

#include "pcprsa_getdefmeth_priv.h"

/* CTE version of CRT based RSA decrypt */
IPP_OWN_DEFN (void, gsRSAprv_cipher_crt, (IppsBigNumState* pY, const IppsBigNumState* pX, const IppsRSAPrivateKeyState* pKey, BNU_CHUNK_T* pBuffer))
{
   const BNU_CHUNK_T* dataX = BN_NUMBER(pX);
   cpSize nsX = BN_SIZE(pX);

   BNU_CHUNK_T* dataY = BN_NUMBER(pY);

   BNU_CHUNK_T* dataXp = BN_NUMBER(pY);
   BNU_CHUNK_T* dataXq = BN_BUFFER(pY);

   /* P- and Q- montgometry engines */
   gsModEngine* pMontP = RSA_PRV_KEY_PMONT(pKey);
   gsModEngine* pMontQ = RSA_PRV_KEY_QMONT(pKey);
   cpSize nsP = MOD_LEN(pMontP);
   cpSize nsQ = MOD_LEN(pMontQ);
   cpSize bitSizeP = RSA_PRV_KEY_BITSIZE_P(pKey);
   cpSize bitSizeQ = RSA_PRV_KEY_BITSIZE_Q(pKey);
   cpSize bitSizeDP = bitSizeP; //BITSIZE_BNU(RSA_PRV_KEY_DP(pKey), nsP); /* bitsize of dP exp */
   cpSize bitSizeDQ = bitSizeQ; //BITSIZE_BNU(RSA_PRV_KEY_DQ(pKey), nsQ); /* bitsize of dQ exp */

   /* Prefer dual exponentiation method if available */
   gsMethod_RSA* m = getDualExpMethod_RSA_private(bitSizeDP, bitSizeDQ);
   if (m) {
      ZEXPAND_COPY_BNU(pBuffer, nsQ+nsQ, dataX, nsX);
      MOD_METHOD(pMontQ)->red(dataXq, pBuffer, pMontQ);
      MOD_METHOD(pMontQ)->mul(dataXq, dataXq, MOD_MNT_R2(pMontQ), pMontQ);

      ZEXPAND_COPY_BNU(pBuffer, nsP+nsP, dataX, nsX);
      MOD_METHOD(pMontP)->red(dataXp, pBuffer, pMontP);
      MOD_METHOD(pMontP)->mul(dataXp, dataXp, MOD_MNT_R2(pMontP), pMontP);

      BNU_CHUNK_T* pDataX[2] = {0};
      pDataX[0] = dataXq;
      pDataX[1] = dataXp;

      cpSize pSize[2] = {0};
      pSize[0] = nsQ;
      pSize[1] = nsP;

      BNU_CHUNK_T* pPrvExp[2] = {0};
      pPrvExp[0] = RSA_PRV_KEY_DQ(pKey);
      pPrvExp[1] = RSA_PRV_KEY_DP(pKey);

      gsModEngine* pMont[2] = {0};
      pMont[0] = pMontQ;
      pMont[1] = pMontP;

      m->dualExpFun(pDataX, (const BNU_CHUNK_T**)pDataX, pSize, (const BNU_CHUNK_T**)pPrvExp, pMont, pBuffer);
   } else {
      /* compute xq = x^dQ mod Q */
      if (bitSizeP== bitSizeQ) { /* believe it's enough conditions for correct Mont application */
         ZEXPAND_COPY_BNU(pBuffer, nsQ+nsQ, dataX, nsX);
         MOD_METHOD(pMontQ)->red(dataXq, pBuffer, pMontQ);
         MOD_METHOD(pMontQ)->mul(dataXq, dataXq, MOD_MNT_R2(pMontQ), pMontQ);
      }
      else {
         COPY_BNU(dataXq, dataX, nsX);
         cpMod_BNU(dataXq, nsX, MOD_MODULUS(pMontQ), nsQ);
      }

      m = getDefaultMethod_RSA_private(bitSizeDQ);
      m->expFun(dataXq, dataXq, nsQ, RSA_PRV_KEY_DQ(pKey), bitSizeDQ, pMontQ, pBuffer);

      /* compute xp = x^dP mod P */
      if (bitSizeP== bitSizeQ) { /* believe it's enough conditions for correct Mont application */
         ZEXPAND_COPY_BNU(pBuffer, nsP+nsP, dataX, nsX);
         MOD_METHOD(pMontP)->red(dataXp, pBuffer, pMontP);
         MOD_METHOD(pMontP)->mul(dataXp, dataXp, MOD_MNT_R2(pMontP), pMontP);
      }
      else {
         COPY_BNU(dataXp, dataX, nsX);
         cpMod_BNU(dataXp, nsX, MOD_MODULUS(pMontP), nsP);
      }

      m = getDefaultMethod_RSA_private(bitSizeDP);
      m->expFun(dataXp, dataXp, nsP, RSA_PRV_KEY_DP(pKey), bitSizeDP, pMontP, pBuffer);
   }

   /*
   // recombination
   */
   /* xq = xq mod P
      must be sure that xq in the same residue domain as xp
      because of following (xp-xq) mod P operation
   */
   if (bitSizeP == bitSizeQ) { /* believe it's enough conditions for correct Mont application */
      ZEXPAND_COPY_BNU(pBuffer, nsP+nsP, dataXq, nsQ);
      //MOD_METHOD(pMontP)->red(pBuffer, pBuffer, pMontP);
      //MOD_METHOD(pMontP)->mul(pBuffer, pBuffer, MOD_MNT_R2(pMontP), pMontP);
      MOD_METHOD(pMontP)->sub(pBuffer, pBuffer, MOD_MODULUS(pMontP), pMontP);
      /* xp = (xp - xq) mod P */
      MOD_METHOD(pMontP)->sub(dataXp, dataXp, pBuffer, pMontP);
   }
   else {
      COPY_BNU(pBuffer, dataXq, nsQ);
      {
         cpSize nsQP = cpMod_BNU(pBuffer, nsQ, MOD_MODULUS(pMontP), nsP);
         BNU_CHUNK_T cf = cpSub_BNU(dataXp, dataXp, pBuffer, nsQP);
         if(nsP-nsQP)
            cf = cpDec_BNU(dataXp+nsQP, dataXp + nsQP, (nsP-nsQP), cf);
         if (cf)
            cpAdd_BNU(dataXp, dataXp, MOD_MODULUS(pMontP), nsP);
      }
   }

   /* xp = xp*qInv mod P */
   /* convert invQ into Montgomery domain */
   MOD_METHOD(pMontP)->encode(pBuffer, RSA_PRV_KEY_INVQ(pKey), (gsModEngine*)pMontP);
   /* and multiply xp *= mont(invQ) mod P */
   MOD_METHOD(pMontP)->mul(dataXp, dataXp, pBuffer, pMontP);

   /* Y = xq + xp*Q */
   cpMul_BNU_school(pBuffer, dataXp, nsP, MOD_MODULUS(pMontQ), nsQ);
   {
      BNU_CHUNK_T cf = cpAdd_BNU(dataY, pBuffer, dataXq, nsQ);
      cpInc_BNU(dataY + nsQ, pBuffer + nsQ, nsP, cf);
   }

   nsX = nsP + nsQ;
   FIX_BNU(dataY, nsX);
   BN_SIZE(pY) = nsX;
   BN_SIGN(pY) = ippBigNumPOS;
}
