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
//     ippsGcd_BN()
// 
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpbn.h"
#include "pcptool.h"


/*F*
//    Name: ippsGcd_BN
//
// Purpose: compute GCD value.
//
// Returns:                Reason:
//    ippStsNullPtrErr        pA  == NULL
//                            pB  == NULL
//                            pGCD  == NULL
//    ippStsContextMatchErr   !BN_VALID_ID(pA)
//                            !BN_VALID_ID(pB)
//                            !BN_VALID_ID(pGCD)
//    ippStsBadArgErr         A==B==0
//    ippStsOutOfRangeErr     pGCD can not hold result
//    ippStsNoErr             no errors
//
// Parameters:
//    pA    source BigNum
//    pB    source BigNum
//    pGCD    GCD value
//
*F*/
IPPFUN(IppStatus, ippsGcd_BN, (IppsBigNumState* pA, IppsBigNumState* pB, IppsBigNumState* pGCD))
{
   IPP_BAD_PTR3_RET(pA, pB, pGCD);

   IPP_BADARG_RET(!BN_VALID_ID(pA), ippStsContextMatchErr);
   IPP_BADARG_RET(!BN_VALID_ID(pB), ippStsContextMatchErr);
   IPP_BADARG_RET(!BN_VALID_ID(pGCD), ippStsContextMatchErr);

   IPP_BADARG_RET(BN_ROOM(pGCD) < IPP_MIN(BN_SIZE(pA), BN_SIZE(pB)), ippStsOutOfRangeErr);

   {
      IppsBigNumState* x = pA;
      IppsBigNumState* y = pB;
      IppsBigNumState* g = pGCD;

      int aIsZero = BN_SIZE(pA)==1 && BN_NUMBER(pA)[0]==0;
      int bIsZero = BN_SIZE(pB)==1 && BN_NUMBER(pB)[0]==0;

      if(aIsZero && bIsZero)
         return ippStsBadArgErr;
      if(aIsZero && !bIsZero) {
         COPY_BNU(BN_NUMBER(g), BN_NUMBER(pB), BN_SIZE(pB));
         BN_SIZE(g) = BN_SIZE(pB);
         BN_SIGN(g) = ippBigNumPOS;
         return ippStsNoErr;
      }
      if(bIsZero && !aIsZero) {
         COPY_BNU(BN_NUMBER(g), BN_NUMBER(pA), BN_SIZE(pA));
         BN_SIZE(g) = BN_SIZE(pA);
         BN_SIGN(g) = ippBigNumPOS;
         return ippStsNoErr;
      }

      /*
      // Lehmer's algorithm requres that first number must be greater than second
      // x is the first, y is the second
      */
      {
         int cmpRes = cpCmp_BNU(BN_NUMBER(x), BN_SIZE(x), BN_NUMBER(y), BN_SIZE(y));
         if(0>cmpRes)
            SWAP_PTR(IppsBigNumState, x, y);
         if(0==cmpRes) {
            COPY_BNU(BN_NUMBER(g), BN_NUMBER(x), BN_SIZE(x));
            BN_SIGN(g) = ippBigNumPOS;
            BN_SIZE(g) = BN_SIZE(x);
            return ippStsNoErr;
         }
         if(BN_SIZE(x)==1) {
            BNU_CHUNK_T gcd = cpGcd_BNU(BN_NUMBER(x)[0], BN_NUMBER(y)[0]);
            BN_NUMBER(g)[0] = gcd;
            BN_SIZE(g) = 1;
            return ippStsNoErr;
         }
      }

      {
         Ipp32u* xBuffer = (Ipp32u*)BN_BUFFER(x);
         Ipp32u* yBuffer = (Ipp32u*)BN_BUFFER(y);
         Ipp32u* gBuffer = (Ipp32u*)BN_BUFFER(g);
         Ipp32u* xData = (Ipp32u*)BN_NUMBER(x);
         Ipp32u* yData = (Ipp32u*)BN_NUMBER(y);
         Ipp32u* gData = (Ipp32u*)BN_NUMBER(g);
         cpSize nsXmax = BN_ROOM(x)*((Ipp32s)(sizeof(BNU_CHUNK_T)/sizeof(Ipp32u)));
         cpSize nsYmax = BN_ROOM(y)*((Ipp32s)(sizeof(BNU_CHUNK_T)/sizeof(Ipp32u)));
         cpSize nsGmax = BN_ROOM(g)*((Ipp32s)(sizeof(BNU_CHUNK_T)/sizeof(Ipp32u)));
         cpSize nsX = BN_SIZE(x)*((Ipp32s)(sizeof(BNU_CHUNK_T)/sizeof(Ipp32u)));
         cpSize nsY = BN_SIZE(y)*((Ipp32s)(sizeof(BNU_CHUNK_T)/sizeof(Ipp32u)));

         Ipp32u* T;
         Ipp32u* u;

         FIX_BNU32(xData, nsX);
         FIX_BNU32(yData, nsY);

         /* init buffers */
         ZEXPAND_COPY_BNU(xBuffer, nsXmax, xData, nsX);
         ZEXPAND_COPY_BNU(yBuffer, nsYmax, yData, nsY);

         T = gBuffer;
         u = gData;
         ZEXPAND_BNU(T, 0, nsGmax);
         ZEXPAND_BNU(u, 0, nsGmax);

         while(nsX > (cpSize)(sizeof(BNU_CHUNK_T)/sizeof(Ipp32u))) {
            /* xx and yy is the high-order digits of x and y (yy could be 0) */

            Ipp64u xx = (Ipp64u)(xBuffer[nsX-1]);
            Ipp64u yy = (nsY < nsX)? 0 : (Ipp64u)(yBuffer[nsY-1]);

            Ipp64s AA = 1;
            Ipp64s BB = 0;
            Ipp64s CC = 0;
            Ipp64s DD = 1;
            Ipp64s t;

            while((yy+(Ipp64u)CC)!=0 && (yy+(Ipp64u)DD)!=0) {
               Ipp64u q  = ( xx + (Ipp64u)AA ) / ( yy + (Ipp64u)CC );
               Ipp64u q1 = ( xx + (Ipp64u)BB ) / ( yy + (Ipp64u)DD );
               if(q!=q1)
                  break;
               t = AA - (Ipp64s)q*CC;
               AA = CC;
               CC = t;
               t = BB - (Ipp64s)q*DD;
               BB = DD;
               DD = t;
               t = (Ipp64s)(xx - q*yy);
               xx = yy;
               yy = (Ipp64u)t;
            }

            if(BB == 0) {
               /* T = x mod y */
               cpSize nsT = cpMod_BNU32(xBuffer, nsX, yBuffer, nsY);
               ZEXPAND_BNU(T, 0, nsGmax);
               COPY_BNU(T, xBuffer, nsT);
               /* a = b; b = T; */
               ZEXPAND_BNU(xBuffer, 0, nsXmax);
               COPY_BNU(xBuffer, yBuffer, nsY);
               ZEXPAND_BNU(yBuffer, 0, nsYmax);
               COPY_BNU(yBuffer, T, nsY);
            }

            else {
               Ipp32u carry;
               /*
               // T = AA*x + BB*y;
               // u = CC*x + DD*y;
               // b = u; a = T;
               */
               if((AA <= 0)&&(BB>=0)) {
                  Ipp32u a1 = (Ipp32u)(-AA);
                  carry = cpMulDgt_BNU32(T, yBuffer, nsY, (Ipp32u)BB);
                  carry = cpMulDgt_BNU32(u, xBuffer, nsY, a1);
                  /* T = BB*y - AA*x; */
                  carry = cpSub_BNU32(T, T, u, nsY);
               }
               else {
                  if((AA >= 0)&&(BB<=0)) {
                     Ipp32u b1 = (Ipp32u)(-BB);
                     carry = cpMulDgt_BNU32(T, xBuffer, nsY, (Ipp32u)AA);
                     carry = cpMulDgt_BNU32(u, yBuffer, nsY, b1);
                     /* T = AA*x - BB*y; */
                     carry = cpSub_BNU32(T, T, u, nsY);
                  }
                  else {
                     /*AA*BB>=0 */
                     carry = cpMulDgt_BNU32(T, xBuffer, nsY, (Ipp32u)AA);
                     carry = cpMulDgt_BNU32(u, yBuffer, nsY, (Ipp32u)BB);
                     /* T = AA*x + BB*y; */
                     carry = cpAdd_BNU32(T, T, u, nsY);
                  }
               }

               /* Now T is reserved. We use only u for intermediate results. */
               if((CC <= 0)&&(DD>=0)){
                  Ipp32u c1 = (Ipp32u)(-CC);
                  /* u = x*CC; x = u; */
                  carry = cpMulDgt_BNU32(u, xBuffer, nsY, c1);
                  COPY_BNU(xBuffer, u, nsY);
                  /* u = y*DD; */
                  carry = cpMulDgt_BNU32(u, yBuffer, nsY, (Ipp32u)DD);
                  /* u = DD*y - CC*x; */
                  carry = cpSub_BNU32(u, u, xBuffer, nsY);
               }
               else {
                  if((CC >= 0)&&(DD<=0)){
                     Ipp32u d1 = (Ipp32u)(-DD);
                     /* u = y*DD; y = u */
                     carry = cpMulDgt_BNU32(u, yBuffer, nsY, d1);
                     COPY_BNU(yBuffer, u, nsY);
                     /* u = CC*x; */
                     carry = cpMulDgt_BNU32(u, xBuffer, nsY, (Ipp32u)CC);
                     /* u = CC*x - DD*y; */
                     carry = cpSub_BNU32(u, u, yBuffer, nsY);
                  }
                  else {
                     /*CC*DD>=0 */
                     /* y = y*DD */
                     carry = cpMulDgt_BNU32(u,  yBuffer, nsY, (Ipp32u)DD);
                     COPY_BNU(yBuffer, u, nsY);
                     /* u = x*CC */
                     carry = cpMulDgt_BNU32(u, xBuffer, nsY, (Ipp32u)CC);
                     /* u = x*CC + y*DD */
                     carry = cpAdd_BNU32(u, u, yBuffer, nsY);
                  }
               }

               /* y = u; x = T; */
               COPY_BNU(yBuffer, u, nsY);
               COPY_BNU(xBuffer, T, nsY);
            }

            FIX_BNU32(xBuffer, nsX);
            FIX_BNU32(yBuffer, nsY);

            if (nsY > nsX) {
               SWAP_PTR(IppsBigNumState, x, y);
               SWAP(nsX, nsY);
            }

            if (nsY==1 && yBuffer[nsY-1]==0) {
               /* End evaluation */
               ZEXPAND_BNU(gData, 0, nsGmax);
               COPY_BNU(gData, xBuffer, nsX);
               BN_SIZE(g) = INTERNAL_BNU_LENGTH(nsX);
               BN_SIGN(g) = ippBigNumPOS;
               return ippStsNoErr;
            }
         }

         BN_NUMBER(g)[0] = cpGcd_BNU(((BNU_CHUNK_T*)xBuffer)[0], ((BNU_CHUNK_T*)yBuffer)[0]);
         BN_SIZE(g) = 1;
         BN_SIGN(g) = ippBigNumPOS;
         return ippStsNoErr;
      }
   }
}
