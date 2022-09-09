/*******************************************************************************
* Copyright 2003-2021 Intel Corporation
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
//     Modular Exponentiation (binary version)
// 
//  Contents:
//        cpSafeMontExp_Binary()
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpbn.h"
#include "pcpmontgomery.h"

//tbcd: temporary excluded: #include <assert.h>

/*F*
// Name: cpSafeMontExp_Binary
//
// Purpose: Binary method of Exponentiation
//
//
// Parameters:
//      pX           big number integer of Montgomery form within the
//                      range [0,m-1]
//      pE           big number exponent
//      pMont        Montgomery modulus of IppsMontState.
/       pY           the Montgomery exponentation result.
//
*F*/

#if !defined(_USE_VERSION1_CBA_MITIGATION_) && !defined(_USE_IPP_OWN_CBA_MITIGATION_) // unsafe version
void cpSafeMontExp_Binary(IppsBigNumState* pY,
                const IppsBigNumState* pX, const IppsBigNumState* pE,
                      IppsMontState* pMont)
{
   int k;

   /* if E==0 then Y=R mod m */
   if (pE->size == 1 && pE->number[0] == 0) {
      int len = IPP_MULTIPLE_OF(pMont->n->size, BNUBASE_TYPE_SIZE);
      cpMemset32u(pMont->wb->number, 0, len);
      pMont->wb->number[len] = 1;

      cpMod_BNU(pMont->wb->number, len + 1, pMont->n->number, pMont->n->size, &pY->size);

      cpMemcpy32u(pY->number, pMont->wb->number, pY->size);
      pY->sgn = ippBigNumPOS;
      return;
   }

   else {
      Ipp32u* r_number = pY->workBuffer;
      int r_size = pY->size;

      int flag=1;
      Ipp32u power = pE->number[pE->size-1];

      for( k = 31; k >= 0; k-- ) {
         Ipp32u powd = power & 0x80000000;/* from top to bottom*/
         power <<= 1;

         if((flag == 1) && (powd == 0))
            continue;

         else if (flag == 0) {
            #if defined(_USE_NN_MONTMUL_)
            cpMontMul(r_number, r_size, r_number,r_size,
                      pMont->n->number, pMont->n->size,
                      r_number,&r_size, pMont->n0, pMont->wb->number);
            #else
            cpMontMul(r_number, r_size, r_number,r_size,
                      pMont->n->number, pMont->n->size,
                      r_number,&r_size, pMont->n0, pMont->wb->number, pMont->pBuffer);
            #endif
            if (powd)
               #if defined(_USE_NN_MONTMUL_)
               cpMontMul(r_number, r_size, pX->number,pX->size,
                         pMont->n->number, pMont->n->size,
                         r_number,&r_size, pMont->n0, pMont->wb->number);
               #else
               cpMontMul(r_number, r_size, pX->number,pX->size,
                         pMont->n->number, pMont->n->size,
                         r_number,&r_size, pMont->n0, pMont->wb->number, pMont->pBuffer);
               #endif
         }

         else {
            int i;
            flag = 0;
            r_size = pMont->n->size;
            if( pX->size < pMont->n->size )
               for(i = r_size - 1; i >= pX->size; i-- )
                  r_number[i] = 0;

            for( i = pX->size - 1; i >= 0; i-- )
               r_number[i] = pX->number[i];
         }
      }

      if (pE->size > 1) {
         struct BNU {
            Ipp32u *number;
            int    *size;
         } BNUs[2];
         BNUs[0].number = r_number;
         BNUs[0].size   = &r_size;
         BNUs[1].number = pX->number;
         BNUs[1].size   = &(((IppsBigNumState*)pX)->size);

         for( k = pE->size - 2; k >= 0; k-- ) {
            int j;
            Ipp32u powd = 0;
            power = pE->number[k];

            for( j = 31; j >= 0; j-- ) {
               #if defined(_USE_NN_MONTMUL_)
               cpMontMul(r_number, r_size, BNUs[powd].number, *(BNUs[powd].size),
                         pMont->n->number, pMont->n->size,
                         r_number,&r_size, pMont->n0, pMont->wb->number);
               #else
               cpMontMul(r_number, r_size, BNUs[powd].number, *(BNUs[powd].size),
                         pMont->n->number, pMont->n->size,
                         r_number,&r_size, pMont->n0, pMont->wb->number, pMont->pBuffer);
               #endif
               powd = ((power >> j) & 0x1) & (powd ^ 1);
               j += powd;
            }
         }
      }

      for(k=r_size-1; k>= 0; k--)
         pY->number[k] = r_number[k];

      pY->sgn = ippBigNumPOS;
      pY->size = r_size;

      while((pY->size > 1) && (pY->number[pY->size-1] == 0))
         pY->size--;

      return;
   }
}
#endif /* _USE_VERSION1_CBA_MITIGATION_,  _xUSE_IPP_OWN_CBA_MITIGATION_ */


#if defined(_USE_VERSION1_CBA_MITIGATION_)
/*
// The version below was designed according to recommendation
// from Crypto experts.
// The reason was to mitigate "cache monitoring" attack on RSA
// Note: this version slower than pre-mitigated version ~ 30-40%
*/


#define SET_BNU(dst,val,len) \
{ \
   int n; \
   for(n=0; n<(len); n++) (dst)[n] = (val); \
}

#define AND_BNU(dst,src1,src2,len) \
{ \
   int n; \
   for(n=0; n<(len); n++) (dst)[n] = (src1)[n] & (src2)[n]; \
}

/*F*
// Name: cpSafeMontExp_Binary
//
// Purpose: Binary method of Exponentiation
//
//
// Parameters:
//      pX           big number integer of Montgomery form within the
//                      range [0,m-1]
//      pE           big number exponent
//      pMont        Montgomery modulus of IppsMontState.
/       pY           the Montgomery exponentation result.
//
*F*/

void cpSafeMontExp_Binary(IppsBigNumState* pY,
                const IppsBigNumState* pX, const IppsBigNumState* pE,
                      IppsMontState* pMont)
{
   Ipp32u* eData = BN_NUMBER(pE);
   int eSize = BN_SIZE(pE);

   /*
   // if e==0 then r=R mod m (i.e MontEnc(1))
   */
   if (eSize == 1 && eData[0] == 0) {
      cpBN_copy(MNT_1(pMont), pY);
      return;
   }

   /*
   // modulo exponentiation
   */
   if(pY!=pX) /* init result */
      cpBN_copy(pX, pY);

   {
      Ipp32u eValue;
      int nBits;

      Ipp32u* pModulus = BN_NUMBER(MNT_MODULO(pMont));
      int mSize = BN_SIZE(MNT_MODULO(pMont));
      Ipp32u* pHelper = MNT_HELPER(pMont);

      Ipp32u* yData = BN_NUMBER(pY);
      Ipp32u* xData = BN_BUFFER(pY);
      int ySize = BN_SIZE(pY);

      Ipp32u* tData   = BN_NUMBER(MNT_PRODUCT(pMont));
      Ipp32u* pBuffer = BN_BUFFER(MNT_PRODUCT(pMont));
      Ipp32u* pMontOne= BN_NUMBER(MNT_1(pMont));

      /* expand Mont(1) */
      ZEXPAND_BNU(pMontOne, BN_SIZE(MNT_1(pMont)), mSize);

      /* copy base */
      ZEXPAND_COPY_BNU(yData,ySize, xData,mSize);


      /* execute most significant part pE */
      eValue = eData[eSize-1];
      nBits = 32-NLZ32u(eValue);
      eValue <<= (32-nBits);

      nBits--;
      eValue <<=1;
      for(; nBits>0; nBits--, eValue<<=1) {
         Ipp32u carry;

         /* squaring: R^2 mod Modulus */
         #if defined(_USE_NN_MONTMUL_)
         cpMontMul(yData,     ySize,
                   yData,     ySize,
                   pModulus,  mSize,
                   yData,    &ySize,
                   pHelper,   pBuffer);
         #else
         cpMontMul(yData,     ySize,
                   yData,     ySize,
                   pModulus,  mSize,
                   yData,    &ySize,
                   pHelper,   pBuffer, MNT_BUFFER(pMont));
         #endif

         /* T = (X-1)*bitof(E,j) + 1 */
         SET_BNU(pBuffer, ((Ipp32s)eValue)>>31, mSize);
         carry = cpSub_BNU(tData, xData, pMontOne, mSize);
         AND_BNU(tData, tData, pBuffer, mSize);
         carry = cpAdd_BNU(tData, tData, pMontOne, mSize);

         /* multiply: Y*T mod Modulus */
         #if defined(_USE_NN_MONTMUL_)
         cpMontMul(yData,    ySize,
                   tData,    mSize,
                   pModulus, mSize,
                   yData,   &ySize,
                   pHelper, pBuffer);
         #else
         cpMontMul(yData,    ySize,
                   tData,    mSize,
                   pModulus, mSize,
                   yData,   &ySize,
                   pHelper, pBuffer, MNT_BUFFER(pMont));
         #endif
      }

      /* execute rest bits of E */
      eSize--;
      for(; eSize>0; eSize--) {
         eValue = eData[eSize-1];

         for(nBits=32; nBits>0; nBits--, eValue<<=1) {
            Ipp32u carry;

            /* squaring: R^2 mod Modulus */
            #if defined(_USE_NN_MONTMUL_)
            cpMontMul(yData,     ySize,
                      yData,     ySize,
                      pModulus,  mSize,
                      yData,    &ySize,
                      pHelper,   pBuffer);
            #else
            cpMontMul(yData,     ySize,
                      yData,     ySize,
                      pModulus,  mSize,
                      yData,    &ySize,
                      pHelper,   pBuffer, MNT_BUFFER(pMont));
            #endif

            /* T = (X-1)*bitof(E,j) + 1 */
            SET_BNU(pBuffer, ((Ipp32s)eValue)>>31, mSize);
            carry = cpSub_BNU(tData, xData, pMontOne, mSize);
            AND_BNU(tData, tData, pBuffer, mSize);
            carry = cpAdd_BNU(tData, tData, pMontOne, mSize);

            /* multiply: R*T mod Modulus */
            #if defined(_USE_NN_MONTMUL_)
            cpMontMul(yData,    ySize,
                      tData,    mSize,
                      pModulus, mSize,
                      yData,   &ySize,
                      pHelper, pBuffer);
            #else
            cpMontMul(yData,    ySize,
                      tData,    mSize,
                      pModulus, mSize,
                      yData,   &ySize,
                      pHelper, pBuffer, MNT_BUFFER(pMont));
            #endif
         }
      }

      BN_SIZE(pY) = ySize;
   }
}
#endif /* _USE_VERSION1_CBA_MITIGATION_ */