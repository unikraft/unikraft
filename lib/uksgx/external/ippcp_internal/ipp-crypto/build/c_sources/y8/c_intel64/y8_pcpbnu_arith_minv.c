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
//  Purpose:
//     Intel(R) Integrated Performance Primitives. Cryptography Primitives.
//     Internal Unsigned arithmetic
// 
//  Contents:
//     cpModInv_BNU()
// 
*/

#include "owncp.h"
#include "pcpbnuarith.h"
#include "pcpbnumisc.h"


/*
// cpMAC_BNU
//
// Multiply with ACcumulation
// Computes r <- r + a * b, returns real size of the r in the size_r variable
// Returns 0 if there are no enought buffer size to write to r[MAX(size_r + 1, size_a + size_b) - 1]
// Returns 1 if no error
//
// Note:
//  DO NOT run in inplace mode
//  The minimum buffer size for the r must be (size_a + size_b - 1)
//      the maximum buffer size for the r is MAX(size_r + 1, size_a + size_b)
*/
static int cpMac_BNU(BNU_CHUNK_T* pR, cpSize nsR,
        const BNU_CHUNK_T* pA, cpSize nsA,
        const BNU_CHUNK_T* pB, cpSize nsB)
{
   /* cleanup the rest of destination buffer */
   ZEXPAND_BNU(pR, nsR, nsA+nsB-1);
   //nsR = IPP_MAX(nsR, nsA+nsB);

   {
      BNU_CHUNK_T expansion = 0;
      cpSize i;
      for(i=0; i<nsB && !expansion; i++) {
         expansion = cpAddMulDgt_BNU(pR+i, pA, nsA, pB[i]);
         if(expansion)
            expansion = cpInc_BNU(pR+i+nsA, pR+i+nsA, nsR-i-nsA, expansion);
      }

      if(expansion)
         return 0;
      else {   /* compute real size */
         FIX_BNU(pR, nsR);
         return nsR;
      }
   }
}

/*F*
//    Name: cpModInv_BNU
//
// Purpose: Multiplicative Inversion BigNum.
//
// Returns:                Reason:
//
// Parameters:
//    pA     source (value) BigNum A
//    nsA    size of A
//    pM     source (modulus) BigNum M
//    nsM    size of M
//    pInv   result BigNum
//    bufInv buffer of Inv
//    bufA   buffer of A
//    bufM   buffer of M
//
*F*/
IPP_OWN_DEFN (int, cpModInv_BNU, (BNU_CHUNK_T* pInv, const BNU_CHUNK_T* pA, cpSize nsA, const BNU_CHUNK_T* pM, cpSize nsM, BNU_CHUNK_T* bufInv, BNU_CHUNK_T* bufA, BNU_CHUNK_T* bufM))
{
    FIX_BNU(pA, nsA);
    FIX_BNU(pM, nsM);

   /* inv(1) = 1 */
   if(nsA==1 && pA[0]==1) {
      pInv[0] = 1;
      return 1;
   }

   {
      cpSize moduloSize = nsM;

      BNU_CHUNK_T* X1 = pInv;
      BNU_CHUNK_T* X2 = bufM;
      BNU_CHUNK_T* Q = bufInv;
      cpSize nsX1 = 1;
      cpSize nsX2 = 1;
      cpSize nsQ;

      COPY_BNU(bufA, pA, nsA);

      ZEXPAND_BNU(X1, 0, moduloSize);
      ZEXPAND_BNU(X2, 0, moduloSize);
      X2[0] = 1;

      for(;;) {
         nsM = cpDiv_BNU(Q, &nsQ, (BNU_CHUNK_T*)pM, nsM, bufA, nsA);
         nsX1 = cpMac_BNU(X1,moduloSize, Q,nsQ, X2,nsX2);

         if (nsM==1 && pM[0]==1) {
            ////ZEXPAND_BNU(X2, nsX2, moduloSize);
            nsX2 = cpMac_BNU(X2,moduloSize, X1,nsX1, bufA, nsA);
            COPY_BNU((BNU_CHUNK_T*)pM, X2, moduloSize);
            cpSub_BNU(pInv, pM, X1, moduloSize);
            FIX_BNU(pInv, moduloSize);
            return moduloSize;
         }
         else if (nsM==1 && pM[0]==0) {
            cpMul_BNU_school((BNU_CHUNK_T*)pM, X1,nsX1, bufA, nsA);
            /* gcd = buf_a */
            return 0;
         }

         nsA = cpDiv_BNU(Q, &nsQ, bufA, nsA, (BNU_CHUNK_T*)pM, nsM);
         nsX2 = cpMac_BNU(X2,moduloSize, Q,nsQ, X1,nsX1);

         if(nsA==1 && bufA[0]==1) {
            ////ZEXPAND_BNU(X1, nsX1, moduloSize);
            nsX1 = cpMac_BNU(X1, moduloSize, X2, nsX2, pM, nsM);
            COPY_BNU((BNU_CHUNK_T*)pM, X1, moduloSize);
            COPY_BNU(pInv, X2, nsX2);
            return nsX2;
         }
         else if (nsA==1 && bufA[0]==0) {
            /* gcd = m */
            COPY_BNU(X1, pM, nsM);
            cpMul_BNU_school((BNU_CHUNK_T*)pM, X2, nsX2, X1, nsM);
            return 0;
         }
      }
   }
}
