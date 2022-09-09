/*******************************************************************************
* Copyright 2016-2021 Intel Corporation
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

#include "owncp.h"

#if (_IPP32E>=_IPP32E_L9)

#include "pcpbnuimpl.h"
#include "pcpbnumisc.h"

#include "pcpngmontexpstuff.h"
#include "pcpngmontexpstuff_avx2.h"
#include "gsscramble.h"
#include "pcpmask_ct.h"


//tbcd: temporary excluded: #include <assert.h>
/*
   converts regular (base = 2^32) representation (pRegular, regLen)
   into "redundant" (base = 2^DIGIT_SIZE) represenrartion (pRep, repLen)

   return 1

   note:
   1) repLen >= (bitsize +DIGIT_SIZE-1)/DIGIT_SIZE for complete conversion
   2) regular representation should expanded by at least one zero value,
      pre-requisite: pRegular[regLen] == 0 to make conversion correct
   3) caller must provide suitable lengths of regular and redundant respresentations
      so, conversion does correct
*/
static int regular_dig27(Ipp64u* pRep27, int repLen, const Ipp32u* pRegular, int regLen)
{
   /* expected number of digit in redundant representation */
   int n = cpDigitNum_avx2(regLen*BITSIZE(Ipp32u), EXP_DIGIT_SIZE_AVX2);

   //tbcd: temporary excluded: assert(pRegular[regLen]==0);  /* test pre-requisite */

   {
      int redBit;    /* output representatin bit */
      int i;
      for(i=0, redBit=0; i<n; i++, redBit+=EXP_DIGIT_SIZE_AVX2) {
         int idx   = redBit /NORM_DIGSIZE_AVX2;   /* input digit number */
         int shift = redBit %NORM_DIGSIZE_AVX2;   /* output digit position inside input one */
         Ipp64u x = ((Ipp64u*)(pRegular+idx))[0];
         x >>= shift;
         pRep27[i] = x & EXP_DIGIT_MASK_AVX2;
      }

      /* expands by zeros if necessary */
      for(; i<repLen; i++) {
         pRep27[i] = 0;
      }

      return 1;
   }
}

/*
   converts "redundant" (base = 2^DIGIT_SIZE) representation (pRep27, repLen)
   into regular (base = 2^32) representation (pRegular, regLen)

   note:
   caller must provide suitable lengths of regular and redundant respresentations
   so, conversion does correct
*/
static int dig27_regular(Ipp32u* pRegular, int regLen, const Ipp64u* pRep27, int repLen)
{
   int shift = EXP_DIGIT_SIZE_AVX2;
   Ipp64u x = pRep27[0];

   int idx, i;
   for(i=1, idx=0; i<repLen && idx<regLen; i++) {
      x += pRep27[i] <<shift;
      shift +=EXP_DIGIT_SIZE_AVX2;
      if(shift>=NORM_DIGSIZE_AVX2) {
         pRegular[idx++] = (Ipp32u)(x & NORM_MASK_AVX2);
         x >>= NORM_DIGSIZE_AVX2;
         shift -= NORM_DIGSIZE_AVX2;
      }
   }

   if(idx<regLen)
      pRegular[idx++] = (Ipp32u)x;

   return idx;
}

/* mont_mul wraper */
__INLINE void cpMontMul_avx2(Ipp64u* pR, const Ipp64u* pA, const Ipp64u* pB, const Ipp64u* pModulus, int mLen, Ipp64u k0, Ipp64u* pBuffer)
{
   if(mLen==38)  /* corresponds to 1024-bit regular representation */
      cpMontMul1024_avx2(pR, pA, pB, pModulus, mLen, k0);
   else {
      int caseFlag = mLen%4;
      switch(caseFlag) {
      case 1: cpMontMul4n1_avx2(pR, pA, pB, pModulus, mLen, k0, pBuffer); break;
      case 2: cpMontMul4n2_avx2(pR, pA, pB, pModulus, mLen, k0, pBuffer); break;
      case 3: cpMontMul4n3_avx2(pR, pA, pB, pModulus, mLen, k0, pBuffer); break;
      default: cpMontMul4n_avx2(pR, pA, pB, pModulus, mLen, k0, pBuffer); break;
      }
   }
}

/* mont_sqr wraper */
__INLINE void cpMontSqr_avx2(Ipp64u* pR, const Ipp64u* pA, const Ipp64u* pModulus, int mLen, Ipp64u k0, Ipp64u* pBuffer)
{
   if(mLen==38) /* corresponds to 1024-bit regular representation */
      cpMontSqr1024_avx2(pR, pA, pModulus, mLen, k0, pBuffer);
   else {
      cpSqr_avx2(pBuffer, pA, mLen, pBuffer);
      cpMontRed_avx2(pR, pBuffer, pModulus, mLen, k0);
   }
}


/* ======= degugging section =========================================*/
//#define _EXP_AVX2_DEBUG_
#ifdef _EXP_AVX2_DEBUG_
#include "pcpmontred.h"
void debugToConvMontDomain(BNU_CHUNK_T* pR,
                     const BNU_CHUNK_T* redInp, const BNU_CHUNK_T* redM, int redLen,
                     const BNU_CHUNK_T* pM, const BNU_CHUNK_T* pRR, int nsM, BNU_CHUNK_T k0,
                     BNU_CHUNK_T* pBuffer)
{
   Ipp64u one[152] = {
      1,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,
      0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,
      0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,
      0,0,0,0,0,0,0,0
   };
   Ipp64u redT[152];
   cpMontMul_avx2(redT, redInp, one, redM, redLen, k0, pBuffer);
   dig27_regular((Ipp32u*)redT, nsM*sizeof(BNU_CHUNK_T)/sizeof(Ipp32u), redT, redLen);

   cpMul_BNU(pBuffer, redT,nsM, pRR,nsM, 0);
   cpMontRed_BNU_opt(pR, pBuffer, pM, nsM, k0);
}
#endif
/* ===================================================================*/

#if !defined(_USE_WINDOW_EXP_)
IPP_OWN_DEFN (cpSize, gsMontExpBinBuffer_avx2, (int modulusBits))
{
   cpSize redNum = numofVariable_avx2(modulusBits);      /* "sizeof" variable */
   cpSize redBufferNum = numofVariableBuff_avx2(redNum); /* "sizeof" variable  buffer */
   return redBufferNum *8;
}
#endif /* !_USE_WINDOW_EXP_ */

#if defined(_USE_WINDOW_EXP_)
IPP_OWN_DEFN (cpSize, gsMontExpWinBuffer_avx2, (int modulusBits))
{
   cpSize w = gsMontExp_WinSize(modulusBits);

   cpSize redNum = numofVariable_avx2(modulusBits);      /* "sizeof" variable */
   cpSize redBufferNum = numofVariableBuff_avx2(redNum); /* "sizeof" variable  buffer */

   cpSize bufferNum = CACHE_LINE_SIZE/(Ipp32s)sizeof(BNU_CHUNK_T)
                    + gsGetScrambleBufferSize(redNum, w) /* pre-computed table */
                    + redBufferNum *7;                   /* addition 7 variables */
   return bufferNum;
}
#endif /* _USE_WINDOW_EXP_ */


#if !defined(_USE_WINDOW_EXP_)
/*
// "fast" binary montgomery exponentiation
//
// scratch buffer structure:
//    redX[redBufferLen]
//    redT[redBufferLen]
//    redY[redBufferLen]
//    redM[redBufferLen]
//    redBuffer[redBufferLen*3]
*/
IPP_OWN_DEFN (cpSize, gsMontExpBin_BNU_avx2, (BNU_CHUNK_T* dataY, const BNU_CHUNK_T* dataX, cpSize nsX, const BNU_CHUNK_T* dataE, cpSize bitsizeE, gsModEngine* pMont, BNU_CHUNK_T* pBuffer))
{
   const BNU_CHUNK_T* dataM = MOD_MODULUS(pMont);
   const BNU_CHUNK_T* dataRR= MOD_MNT_R2(pMont);
   cpSize nsM = MOD_LEN(pMont);
   BNU_CHUNK_T k0 = MOD_MNT_FACTOR(pMont);

   cpSize nsE = BITS_BNU_CHUNK(bitsizeE);

   int modulusBitSize = BITSIZE_BNU(dataM, nsM);
   int convModulusBitSize = cpDigitNum_avx2(modulusBitSize, BITSIZE(BNU_CHUNK_T)) * BITSIZE(BNU_CHUNK_T);
   int modulusLen32 = BITS2WORD32_SIZE(modulusBitSize);
   int redLen = cpDigitNum_avx2(convModulusBitSize+2, EXP_DIGIT_SIZE_AVX2);
   int redBufferLen = numofVariableBuff_avx2(redLen);

   /* allocate buffers */
   BNU_CHUNK_T* redX = pBuffer;
   BNU_CHUNK_T* redT = redX+redBufferLen;
   BNU_CHUNK_T* redY = redT+redBufferLen;
   BNU_CHUNK_T* redM = redY+redBufferLen;
   BNU_CHUNK_T* redBuffer = redM+redBufferLen;
   #ifdef _EXP_AVX2_DEBUG_
   BNU_CHUNK_T dbgValue[152];
   #endif

   /* convert modulus into reduced domain */
   ZEXPAND_COPY_BNU(redT, nsM+1, dataM, nsM);
   regular_dig27(redM, redBufferLen, (Ipp32u*)redT,  modulusLen32);

   /* compute taget domain Montgomery converter RR' */
   ZEXPAND_BNU(redT, 0, redBufferLen);
   SET_BIT(redT, (4*redLen*EXP_DIGIT_SIZE_AVX2 - 4*convModulusBitSize));
   regular_dig27(redY, redBufferLen, (Ipp32u*)redT,  modulusLen32);

   ZEXPAND_COPY_BNU(redX, nsM+1, dataRR, nsM);
   regular_dig27(redT, redBufferLen, (Ipp32u*)redX,  modulusLen32);
   cpMontSqr_avx2(redT, redT, redM, redLen, k0, redBuffer);
   cpMontMul_avx2(redT, redT, redY, redM, redLen, k0, redBuffer);

   /* convert base to Montgomery domain */
   ZEXPAND_COPY_BNU(redY, redBufferLen/*nsX+1*/, dataX, nsX);
   regular_dig27(redX, redBufferLen, (Ipp32u*)redY,  nsX*sizeof(BNU_CHUNK_T)/sizeof(Ipp32u));
   cpMontMul_avx2(redX, redX, redT, redM, redLen, k0, redBuffer);
   #ifdef _EXP_AVX2_DEBUG_
   debugToConvMontDomain(dbgValue, redX, redM, redLen, dataM, dataRR, nsM, k0, redBuffer);
   #endif

   /* init result */
   COPY_BNU(redY, redX, redLen);

   FIX_BNU(dataE, nsE);
   {
      /* execute most significant part pE */
      BNU_CHUNK_T eValue = dataE[nsE-1];
      int n = cpNLZ_BNU(eValue)+1;

      eValue <<= n;
      for(; n<BNU_CHUNK_BITS; n++, eValue<<=1) {
         /* squaring/multiplication: Y = Y*Y */
         cpMontSqr_avx2(redY, redY, redM, redLen, k0, redBuffer);
         #ifdef _EXP_AVX2_DEBUG_
         debugToConvMontDomain(dbgValue, redY, redM, redLen, dataM, dataRR, nsM, k0, redBuffer);
         #endif

         /* and multiply Y = Y*X */
         if(eValue & ((BNU_CHUNK_T)1<<(BNU_CHUNK_BITS-1)))
            cpMontMul_avx2(redY, redY, redX, redM, redLen, k0, redBuffer);
      }

      /* execute rest bits of E */
      for(--nsE; nsE>0; nsE--) {
         eValue = dataE[nsE-1];

         for(n=0; n<BNU_CHUNK_BITS; n++, eValue<<=1) {
            /* squaring: Y = Y*Y */
            cpMontSqr_avx2(redY, redY, redM, redLen, k0, redBuffer);
            #ifdef _EXP_AVX2_DEBUG_
            debugToConvMontDomain(dbgValue, redY, redM, redLen, dataM, dataRR, nsM, k0, redBuffer);
            #endif

            /* and multiply: Y = Y*X */
            if(eValue & ((BNU_CHUNK_T)1<<(BNU_CHUNK_BITS-1)))
               cpMontMul_avx2(redY, redY, redX, redM, redLen, k0, redBuffer);
         }
      }
   }

   /* convert result back to regular domain */
   ZEXPAND_BNU(redT, 0, redBufferLen);
   redT[0] = 1;
   cpMontMul_avx2(redY, redY, redT, redM, redLen, k0, redBuffer);
   dig27_regular((Ipp32u*)dataY, nsM*sizeof(BNU_CHUNK_T)/sizeof(ipp32u), redY, redLen);

   return nsM;
}

/*
// "safe" binary montgomery exponentiation
//
// scratch buffer structure:
//    redX[redBufferLen]
//    redT[redBufferLen]
//    redY[redBufferLen]
//    redM[redBufferLen]
//    redBuffer[redBufferLen*3]
*/
IPP_OWN_DEFN (cpSize, gsMontExpBin_BNU_sscm_avx2, (BNU_CHUNK_T* dataY, const BNU_CHUNK_T* dataX, cpSize nsX, const BNU_CHUNK_T* dataE, cpSize bitsizeE, gsModEngine* pMont, BNU_CHUNK_T* pBuffer))
{
   const BNU_CHUNK_T* dataM = MOD_MODULUS(pMont);
   const BNU_CHUNK_T* dataRR= MOD_MNT_R2(pMont);
   cpSize nsM = MOD_LEN(pMont);
   cpSize nsE = BITS_BNU_CHUNK(bitsizeE);
   BNU_CHUNK_T k0 = MOD_MNT_FACTOR(pMont);

   int modulusBitSize = BITSIZE_BNU(dataM, nsM);
   int convModulusBitSize = cpDigitNum_avx2(modulusBitSize, BITSIZE(BNU_CHUNK_T)) * BITSIZE(BNU_CHUNK_T);
   int modulusLen32 = BITS2WORD32_SIZE(modulusBitSize);
   int redLen = cpDigitNum_avx2(convModulusBitSize+2, EXP_DIGIT_SIZE_AVX2);
   int redBufferLen = numofVariableBuff_avx2(redLen);

   /* allocate buffers */
   BNU_CHUNK_T* redX = pBuffer;
   BNU_CHUNK_T* redM = redX+redBufferLen;
   BNU_CHUNK_T* redR = redM+redBufferLen;
   BNU_CHUNK_T* redT = redR+redBufferLen;
   BNU_CHUNK_T* redY = redT+redBufferLen;
   BNU_CHUNK_T* redBuffer = redY+redBufferLen;
   #ifdef _EXP_AVX2_DEBUG_
   BNU_CHUNK_T dbgValue[152];
   #endif

   /* convert modulus into reduced domain */
   ZEXPAND_COPY_BNU(redT, nsM+1, dataM, nsM);
   regular_dig27(redM, redBufferLen, (Ipp32u*)redT,  modulusLen32);

   /* compute taget domain Montgomery converter RR' */
   ZEXPAND_BNU(redT, 0, redBufferLen);
   SET_BIT(redT, (4*redLen*EXP_DIGIT_SIZE_AVX2 - 4*convModulusBitSize));
   regular_dig27(redY, redBufferLen, (Ipp32u*)redT,  modulusLen32);

   ZEXPAND_COPY_BNU(redX, nsM+1, dataRR, nsM);
   regular_dig27(redT, redBufferLen, (Ipp32u*)redX,  modulusLen32);
   cpMontSqr_avx2(redT, redT, redM, redLen, k0, redBuffer);
   cpMontMul_avx2(redT, redT, redY, redM, redLen, k0, redBuffer);

   /* convert base to Montgomery domain */
   ZEXPAND_COPY_BNU(redY, redBUfferLen/*nsX+1*/, dataX, nsX);
   regular_dig27(redX, redBufferLen, (Ipp32u*)redY,  nsX*sizeof(BNU_CHUNK_T)/sizeof(Ipp32u));
   cpMontMul_avx2(redX, redX, redT, redM, redLen, k0, redBuffer);
   #ifdef _EXP_AVX2_DEBUG_
   debugToConvMontDomain(dbgValue, redX, redM, redLen, dataM, dataRR, nsM, k0, redBuffer);
   #endif

   /* init result */
   ZEXPAND_BNU(redR, 0, redBufferLen);
   redR[0] = 1;
   cpMontMul_avx2(redR, redR, redT, redM, redLen, k0, redBuffer);
   COPY_BNU(redY, redR, redBufferLen);

   /* execute bits of E */
   for(; nsE>0; nsE--) {
      BNU_CHUNK_T eValue = dataE[nsE-1];

      int n;
      for(n=BNU_CHUNK_BITS; n>0; n--) {
         /* T = ( msb(eValue) )? X : mont(1) */
         BNU_CHUNK_T mask = cpIsMsb_ct(eValue);
         eValue <<= 1;
         cpMaskedCopyBNU_ct(redT, mask, redX, redR, redLen);

         /* squaring: Y = Y^2 */
         cpMontSqr_avx2(redY, redY, redM, redLen, k0, redBuffer);
         #ifdef _EXP_AVX2_DEBUG_
         debugToConvMontDomain(dbgValue, redY, redM, redLen, dataM, dataRR, nsM, k0, redBuffer);
         #endif
         /* and multiply: Y = Y * T */
         cpMontMul_avx2(redY, redY, redT, redM, redLen, k0, redBuffer);
      }
   }

   /* convert result back to regular domain */
   ZEXPAND_BNU(redT, 0, redBufferLen);
   redT[0] = 1;
   cpMontMul_avx2(redY, redY, redT, redM, redLen, k0, redBuffer);
   dig27_regular((Ipp32u*)dataY, nsM*sizeof(BNU_CHUNK_T)/sizeof(ipp32u), redY, redLen);

   return nsM;
}

#endif /* !_USE_WINDOW_EXP_ */


#if defined(_USE_WINDOW_EXP_)
/*
// "fast" fixed-size window montgomery exponentiation
//
// scratch buffer structure:
//    precomuted table of multipliers[(1<<w)*redLen]
//    redM[redBufferLen]
//    redY[redBufferLen]
//    redT[redBufferLen]
//    redBuffer[redBufferLen*3]
//    redE[redBufferLen]
*/
IPP_OWN_DEFN (cpSize, gsMontExpWin_BNU_avx2, (BNU_CHUNK_T* dataY, const BNU_CHUNK_T* dataX, cpSize nsX, const BNU_CHUNK_T* dataE, cpSize bitsizeE, gsModEngine* pMont, BNU_CHUNK_T* pBuffer))
{
   const BNU_CHUNK_T* dataM = MOD_MODULUS(pMont);
   const BNU_CHUNK_T* dataRR= MOD_MNT_R2(pMont);
   cpSize nsM = MOD_LEN(pMont);
   BNU_CHUNK_T k0 = MOD_MNT_FACTOR(pMont);

   cpSize nsE = BITS_BNU_CHUNK(bitsizeE);

   int modulusBitSize = BITSIZE_BNU(dataM, nsM);
   int convModulusBitSize = cpDigitNum_avx2(modulusBitSize, BITSIZE(BNU_CHUNK_T)) * BITSIZE(BNU_CHUNK_T);
   int modulusLen32 = BITS2WORD32_SIZE(modulusBitSize);
   int redLen = cpDigitNum_avx2(convModulusBitSize+2, EXP_DIGIT_SIZE_AVX2);
   int redBufferLen = numofVariableBuff_avx2(redLen);

   cpSize window = gsMontExp_WinSize(bitsizeE);
   BNU_CHUNK_T wmask = (1<<window) -1;
   cpSize nPrecomute= 1<<window;
   int n;

   #ifdef _EXP_AVX2_DEBUG_
   BNU_CHUNK_T dbgValue[152];
   #endif

   BNU_CHUNK_T* redTable = pBuffer;
   BNU_CHUNK_T* redM = redTable + gsGetScrambleBufferSize(redLen, window);
   BNU_CHUNK_T* redY = redM + redBufferLen;
   BNU_CHUNK_T* redT = redY + redBufferLen;
   BNU_CHUNK_T* redBuffer = redT + redBufferLen;
   BNU_CHUNK_T* redE = redBuffer + redBufferLen*3;

   /* convert modulus into reduced domain */
   ZEXPAND_COPY_BNU(redE, nsM+1, dataM, nsM);
   regular_dig27(redM, redBufferLen, (Ipp32u*)redE,  modulusLen32);

   /* compute taget domain Montgomery converter RR' */
   ZEXPAND_BNU(redT, 0, redBufferLen);
   SET_BIT(redT, (4*redLen*EXP_DIGIT_SIZE_AVX2 - 4*convModulusBitSize));
   regular_dig27(redY, redBufferLen, (Ipp32u*)redT,  modulusLen32);

   ZEXPAND_COPY_BNU(redE, nsM+1, dataRR, nsM);
   regular_dig27(redT, redBufferLen, (Ipp32u*)redE,  modulusLen32);
   cpMontSqr_avx2(redT, redT, redM, redLen, k0, redBuffer);
   cpMontMul_avx2(redT, redT, redY, redM, redLen, k0, redBuffer);

   /*
      pre-compute T[i] = X^i, i=0,.., 2^w-1
   */
   ZEXPAND_BNU(redY, 0, redBufferLen);
   redY[0] = 1;
   cpMontMul_avx2(redY, redY, redT, redM, redLen, k0, redBuffer);
   COPY_BNU(redTable+0, redY, redLen);
   #ifdef _EXP_AVX2_DEBUG_
   debugToConvMontDomain(dbgValue, redY, redM, redLen, dataM, dataRR, nsM, k0, redBuffer);
   #endif

   ZEXPAND_COPY_BNU(redE, redBufferLen/*nsX+1*/, dataX, nsX);
   regular_dig27(redY, redBufferLen, (Ipp32u*)redE,  nsX*(Ipp32s)(sizeof(BNU_CHUNK_T)/sizeof(Ipp32u)));
   cpMontMul_avx2(redY, redY, redT, redM, redLen, k0, redBuffer);
   COPY_BNU(redTable+redLen, redY, redLen);
   #ifdef _EXP_AVX2_DEBUG_
   debugToConvMontDomain(dbgValue, redY, redM, redLen, dataM, dataRR, nsM, k0, redBuffer);
   #endif

   cpMontSqr_avx2(redT, redY, redM, redLen, k0, redBuffer);
   COPY_BNU(redTable+redLen*2, redT, redLen);
   #ifdef _EXP_AVX2_DEBUG_
   debugToConvMontDomain(dbgValue, redT, redM, redLen, dataM, dataRR, nsM, k0, redBuffer);
   #endif

   for(n=3; n<nPrecomute; n++) {
      cpMontMul_avx2(redT, redT, redY, redM, redLen, k0, redBuffer);
      COPY_BNU(redTable+redLen*n, redT, redLen);
      #ifdef _EXP_AVX2_DEBUG_
      debugToConvMontDomain(dbgValue, redT, redM, redLen, dataM, dataRR, nsM, k0, redBuffer);
      #endif
   }

   /* expand exponent */
   ZEXPAND_COPY_BNU(redE, nsE+1, dataE, nsE);
   bitsizeE = ((bitsizeE+window-1)/window) *window;

   /* exponentiation */
   {
      /* position of the 1-st (left) window */
      int eBit = bitsizeE - window;

      /* Note:  Static analysis can generate error/warning on the expression below.
      
      The value of "bitSizeE" is limited, (modulusBitSize > bitSizeE > 0),
      it is checked in initialization phase by (ippsRSA_GetSizePublickey() and ippsRSA_InitPublicKey).
      Buffer "redE" assigned for copy of dataE, is 1 (64-bit) chunk longer than size of RSA modulus,
      therefore the access "*((Ipp32u*)((Ipp16u*)redE+ eBit/BITSIZE(Ipp16u)))" is always inside the boundary.
      */
      /* extract 1-st window value */
      Ipp32u eChunk = *((Ipp32u*)((Ipp16u*)redE+ eBit/BITSIZE(Ipp16u)));
      int shift = eBit & 0xF;
      cpSize windowVal = (cpSize)((eChunk>>shift) &wmask);

      /* initialize result */
      COPY_BNU(redY, redTable+windowVal*redLen, redLen);
      #ifdef _EXP_AVX2_DEBUG_
      debugToConvMontDomain(dbgValue, redY, redM, redLen, dataM, dataRR, nsM, k0, redBuffer);
      #endif

      for(eBit-=window; eBit>=0; eBit-=window) {
         /* do squaring window-times */
         for(n=0; n<window; n++) {
            cpMontSqr_avx2(redY, redY, redM, redLen, k0, redBuffer);
         }
         #ifdef _EXP_AVX2_DEBUG_
         debugToConvMontDomain(dbgValue, redY, redM, redLen, dataM, dataRR, nsM, k0, redBuffer);
         #endif

         /* extract next window value */
         eChunk = *((Ipp32u*)((Ipp16u*)redE+ eBit/BITSIZE(Ipp16u)));
         shift = eBit & 0xF;
         windowVal = (cpSize)((eChunk>>shift) &wmask);

         /* precomputed value muptiplication */
         if(windowVal) {
            cpMontMul_avx2(redY, redY, redTable+windowVal*redLen, redM, redLen, k0, redBuffer);
            #ifdef _EXP_AVX2_DEBUG_
            COPY_BNU(redT, redTable+windowVal*redBufferLen, redBufferLen);
            cpMontMul_avx2(redY, redY, redT, redM, redLen, k0, redBuffer);
            debugToConvMontDomain(dbgValue, redY, redM, redLen, dataM, dataRR, nsM, k0, redBuffer);
            #endif
         }
      }
   }
   #ifdef _EXP_AVX2_DEBUG_
   debugToConvMontDomain(dbgValue, redY, redM, redLen, dataM, dataRR, nsM, k0, redBuffer);
   #endif

   /* convert result back */
   ZEXPAND_BNU(redT, 0, redBufferLen);
   redT[0] = 1;
   cpMontMul_avx2(redY, redY, redT, redM, redLen, k0, redBuffer);
   dig27_regular((Ipp32u*)dataY, nsM*(Ipp32s)(sizeof(BNU_CHUNK_T)/sizeof(ipp32u)), redY, redLen);

   return nsM;
}

/*
// "safe" fixed-size window montgomery exponentiation
//
// scratch buffer structure:
//    precomuted table of multipliers[(1<<w)*redLen]
//    redM[redBufferLen]
//    redY[redBufferLen]
//    redT[redBufferLen]
//    redBuffer[redBufferLen*3]
//    redE[redBufferLen]
*/
IPP_OWN_DEFN (cpSize, gsMontExpWin_BNU_sscm_avx2, (BNU_CHUNK_T* dataY, const BNU_CHUNK_T* dataX, cpSize nsX, const BNU_CHUNK_T* dataE, cpSize bitsizeE, gsModEngine* pMont, BNU_CHUNK_T* pBuffer))
{
   const BNU_CHUNK_T* dataM = MOD_MODULUS(pMont);
   const BNU_CHUNK_T* dataRR= MOD_MNT_R2(pMont);
   cpSize nsM = MOD_LEN(pMont);
   BNU_CHUNK_T k0 = MOD_MNT_FACTOR(pMont);

   cpSize nsE = BITS_BNU_CHUNK(bitsizeE);

   int modulusBitSize = MOD_BITSIZE(pMont);
   int convModulusBitSize = cpDigitNum_avx2(modulusBitSize, BITSIZE(BNU_CHUNK_T)) * BITSIZE(BNU_CHUNK_T);
   int modulusLen32 = BITS2WORD32_SIZE(modulusBitSize);
   int redLen = cpDigitNum_avx2(convModulusBitSize+2, EXP_DIGIT_SIZE_AVX2);
   int redBufferLen = numofVariableBuff_avx2(redLen);

   cpSize window = gsMontExp_WinSize(bitsizeE);
   cpSize nPrecomute= 1<<window;
   BNU_CHUNK_T wmask = (BNU_CHUNK_T)(nPrecomute -1);
   int n;

   #ifdef _EXP_AVX2_DEBUG_
   BNU_CHUNK_T dbgValue[152];
   #endif

   BNU_CHUNK_T* redTable = (BNU_CHUNK_T*)(IPP_ALIGNED_PTR((pBuffer), CACHE_LINE_SIZE));
   BNU_CHUNK_T* redM = redTable + gsGetScrambleBufferSize(redLen, window);
   BNU_CHUNK_T* redY = redM + redBufferLen;
   BNU_CHUNK_T* redT = redY + redBufferLen;
   BNU_CHUNK_T* redBuffer = redT + redBufferLen;
   BNU_CHUNK_T* redE = redBuffer + redBufferLen*3;

   /* convert modulus into reduced domain */
   ZEXPAND_COPY_BNU(redE, nsM+1, dataM, nsM);
   regular_dig27(redM, redBufferLen, (Ipp32u*)redE,  modulusLen32);

   /* compute taget domain Montgomery converter RR' */
   ZEXPAND_BNU(redT, 0, redBufferLen);
   SET_BIT(redT, (4*redLen*EXP_DIGIT_SIZE_AVX2 - 4*convModulusBitSize));
   regular_dig27(redY, redBufferLen, (Ipp32u*)redT,  modulusLen32);

   ZEXPAND_COPY_BNU(redE, nsM+1, dataRR, nsM);
   regular_dig27(redT, redBufferLen, (Ipp32u*)redE,  modulusLen32);
   cpMontSqr_avx2(redT, redT, redM, redLen, k0, redBuffer);
   cpMontMul_avx2(redT, redT, redY, redM, redLen, k0, redBuffer);

   /*
      pre-compute T[i] = X^i, i=0,.., 2^w-1
   */
   ZEXPAND_BNU(redY, 0, redBufferLen);
   redY[0] = 1;
   cpMontMul_avx2(redY, redY, redT, redM, redLen, k0, redBuffer);
   gsScramblePut(redTable, 0, redY, redLen, window);
   #ifdef _EXP_AVX2_DEBUG_
   debugToConvMontDomain(dbgValue, redY, redM, redLen, dataM, dataRR, nsM, k0, redBuffer);
   #endif

   ZEXPAND_COPY_BNU(redE, redBufferLen/*nsX+1*/, dataX, nsX);
   regular_dig27(redY, redBufferLen, (Ipp32u*)redE,  nsX*(Ipp32s)(sizeof(BNU_CHUNK_T)/sizeof(Ipp32u)));
   cpMontMul_avx2(redY, redY, redT, redM, redLen, k0, redBuffer);
   gsScramblePut(redTable, 1, redY, redLen, window);
   #ifdef _EXP_AVX2_DEBUG_
   debugToConvMontDomain(dbgValue, redY, redM, redLen, dataM, dataRR, nsM, k0, redBuffer);
   #endif

   cpMontSqr_avx2(redT, redY, redM, redLen, k0, redBuffer);
   gsScramblePut(redTable, 2, redT, redLen, window);
   #ifdef _EXP_AVX2_DEBUG_
   debugToConvMontDomain(dbgValue, redT, redM, redLen, dataM, dataRR, nsM, k0, redBuffer);
   #endif

   for(n=3; n<nPrecomute; n++) {
      cpMontMul_avx2(redT, redT, redY, redM, redLen, k0, redBuffer);
      gsScramblePut(redTable, n, redT, redLen, window);
      #ifdef _EXP_AVX2_DEBUG_
      debugToConvMontDomain(dbgValue, redT, redM, redLen, dataM, dataRR, nsM, k0, redBuffer);
      #endif
   }

   /* expand exponent */
   ZEXPAND_COPY_BNU(redE, nsM+1, dataE, nsE);
   bitsizeE = ((bitsizeE+window-1)/window) *window;

   /* exponentiation */
   {
      /* position of the 1-st (left) window */
      int eBit = bitsizeE - window;

      /* extract 1-st window value */
      Ipp32u eChunk = *((Ipp32u*)((Ipp16u*)redE+ eBit/BITSIZE(Ipp16u)));
      int shift = eBit & 0xF;
      cpSize windowVal = (cpSize)((eChunk>>shift) &wmask);

      /* initialize result */
      gsScrambleGet_sscm(redY, redLen, redTable, windowVal, window);
      #ifdef _EXP_AVX2_DEBUG_
      debugToConvMontDomain(dbgValue, redY, redM, redLen, dataM, dataRR, nsM, k0, redBuffer);
      #endif

      for(eBit-=window; eBit>=0; eBit-=window) {
         /* do squaring window-times */
         for(n=0; n<window; n++) {
            cpMontSqr_avx2(redY, redY, redM, redLen, k0, redBuffer);
         }
         #ifdef _EXP_AVX2_DEBUG_
         debugToConvMontDomain(dbgValue, redY, redM, redLen, dataM, dataRR, nsM, k0, redBuffer);
         #endif

         /* extract next window value */
         eChunk = *((Ipp32u*)((Ipp16u*)redE+ eBit/BITSIZE(Ipp16u)));
         shift = eBit & 0xF;
         windowVal = (cpSize)((eChunk>>shift) &wmask);
         /* exptact precomputed value and muptiply */
         gsScrambleGet_sscm(redT, redLen, redTable, windowVal, window);
         cpMontMul_avx2(redY, redY, redT, redM, redLen, k0, redBuffer);
         #ifdef _EXP_AVX2_DEBUG_
         debugToConvMontDomain(dbgValue, redY, redM, redLen, dataM, dataRR, nsM, k0, redBuffer);
         #endif
      }
   }
   #ifdef _EXP_AVX2_DEBUG_
   debugToConvMontDomain(dbgValue, redY, redM, redLen, dataM, dataRR, nsM, k0, redBuffer);
   #endif

   /* convert result back */
   ZEXPAND_BNU(redT, 0, redBufferLen);
   redT[0] = 1;
   cpMontMul_avx2(redY, redY, redT, redM, redLen, k0, redBuffer);
   dig27_regular((Ipp32u*)dataY, nsM*(Ipp32s)(sizeof(BNU_CHUNK_T)/sizeof(ipp32u)), redY, redLen);

   return nsM;
}
#endif /* _USE_WINDOW_EXP_ */

#endif /* _IPP32E_L9 */
