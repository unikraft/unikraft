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
// 
*/

#if !defined(_CP_MONTGOMETRY_H)
#define _CP_MONTGOMETRY_H

#include "pcpbn.h"
#include "gsmodstuff.h"

//tbcd: temporary excluded: #include <assert.h>

#define MONT_DEFAULT_POOL_LENGTH (6)

/*
// Montgomery spec structure
*/
struct _cpMontgomery
{
   Ipp32u         idCtx;      /* Montgomery spec identifier             */
   cpSize         maxLen;     /* Maximum length of modulus being stored */
   gsModEngine*   pEngine;    /* Modular arith engine structure         */
};

/* accessory macros */
#define MNT_SET_ID(eng)   ((eng)->idCtx = (Ipp32u)idCtxMontgomery ^ (Ipp32u)IPP_UINT_PTR(eng))
#define MNT_ROOM(eng)     ((eng)->maxLen)
#define MNT_ENGINE(eng)   ((eng)->pEngine)

#define MNT_VALID_ID(eng) ((((eng)->idCtx) ^ (Ipp32u)IPP_UINT_PTR((eng))) == (Ipp32u)idCtxMontgomery)

/* default methos */
#define EXPONENT_METHOD    (ippBinaryMethod)

/* alignment */
#define MONT_ALIGNMENT  ((int)(sizeof(void*)))


/*
// Pacp/unpack Montgomery context
*/
#define cpPackMontCtx OWNAPI(cpPackMontCtx)
   IPP_OWN_DECL (void, cpPackMontCtx, (const IppsMontState* pCtx, Ipp8u* pBuffer))
#define cpUnpackMontCtx OWNAPI(cpUnpackMontCtx)
   IPP_OWN_DECL (void, cpUnpackMontCtx, (const Ipp8u* pBuffer, IppsMontState* pCtx))

/*
// Montgomery reduction, multiplication and squaring
*/
__INLINE void cpMontRed_BNU(BNU_CHUNK_T* pR,
                            BNU_CHUNK_T* pProduct,
                            gsModEngine* pModEngine)
{
   MOD_METHOD( pModEngine )->red(pR, pProduct, pModEngine);
}

__INLINE void cpMontMul_BNU(BNU_CHUNK_T* pR,
                     const BNU_CHUNK_T* pA,
                     const BNU_CHUNK_T* pB,
                           gsModEngine* pModEngine)
{
   MOD_METHOD( pModEngine )->mul(pR, pA, pB, pModEngine);
}

__INLINE cpSize cpMontMul_BNU_EX(BNU_CHUNK_T* pR,
                           const BNU_CHUNK_T* pA, cpSize nsA,
                           const BNU_CHUNK_T* pB, cpSize nsB,
                                 gsModEngine* pModEngine)
{
   const int usedPoolLen = 1;
   cpSize nsM = MOD_LEN( pModEngine );
   BNU_CHUNK_T* pDataR  = pR;
   BNU_CHUNK_T* pDataA  = gsModPoolAlloc(pModEngine, usedPoolLen);
   //tbcd: temporary excluded: assert(NULL!=pDataA);

   ZEXPAND_COPY_BNU(pDataA, nsM, pA, nsA);
   ZEXPAND_COPY_BNU(pDataR, nsM, pB, nsB);

   MOD_METHOD( pModEngine )->mul(pDataR, pDataA, pDataR, pModEngine);

   gsModPoolFree(pModEngine, usedPoolLen);
   return nsM;
}

__INLINE void cpMontSqr_BNU(BNU_CHUNK_T* pR,
                      const BNU_CHUNK_T* pA,
                            gsModEngine* pModEngine)
{
   MOD_METHOD( pModEngine )->sqr(pR, pA, pModEngine);
}

__INLINE void cpMontSqr_BNU_EX(BNU_CHUNK_T* pR,
                         const BNU_CHUNK_T* pA, cpSize nsA,
                               gsModEngine* pModEngine)
{
   cpSize nsM = MOD_LEN( pModEngine );
   ZEXPAND_COPY_BNU(pR, nsM, pA, nsA);

   MOD_METHOD( pModEngine )->sqr(pR, pR, pModEngine);
}

/*
// Montgomery encoding/decoding
*/
__INLINE cpSize cpMontEnc_BNU(BNU_CHUNK_T* pR,
                        const BNU_CHUNK_T* pXreg,
                              gsModEngine* pModEngine)
{
   cpSize nsM = MOD_LEN(pModEngine);

   MOD_METHOD( pModEngine )->encode(pR, pXreg, pModEngine);

   FIX_BNU(pR, nsM);
   return nsM;
}

__INLINE cpSize cpMontEnc_BNU_EX(BNU_CHUNK_T* pR,
                           const BNU_CHUNK_T* pXreg, cpSize nsX,
                                 gsModEngine* pModEngine)
{
   cpSize nsM = MOD_LEN(pModEngine);

   ZEXPAND_COPY_BNU(pR, nsM, pXreg, nsX);

   MOD_METHOD( pModEngine )->encode(pR, pR, pModEngine);

   FIX_BNU(pR, nsM);

   return nsM;
}

__INLINE cpSize cpMontDec_BNU(BNU_CHUNK_T* pR,
                        const BNU_CHUNK_T* pXmont, cpSize nsX,
                              gsModEngine* pModEngine)
{
   cpSize nsM = MOD_LEN( pModEngine );

   ZEXPAND_COPY_BNU(pR, nsM, pXmont, nsX);

   MOD_METHOD( pModEngine )->decode(pR, pR, pModEngine);

   FIX_BNU(pR, nsM);
   return nsM;
}

__INLINE void cpMontMul_BN(IppsBigNumState* pRbn,
                     const IppsBigNumState* pXbn,
                     const IppsBigNumState* pYbn,
                           gsModEngine*     pModEngine)
{
   cpSize nsM = cpMontMul_BNU_EX(BN_NUMBER(pRbn),
                                 BN_NUMBER(pXbn), BN_SIZE(pXbn),
                                 BN_NUMBER(pYbn), BN_SIZE(pYbn),
                                 pModEngine);

   FIX_BNU(BN_NUMBER(pRbn), nsM);
   BN_SIZE(pRbn) = nsM;
   BN_SIGN(pRbn) = ippBigNumPOS;
}

__INLINE void cpMontEnc_BN(IppsBigNumState* pRbn,
                     const IppsBigNumState* pXbn,
                           gsModEngine*     pModEngine)
{
   cpSize nsM = cpMontEnc_BNU_EX(BN_NUMBER(pRbn),
                                 BN_NUMBER(pXbn), BN_SIZE(pXbn),
                                 pModEngine);

   BN_SIZE(pRbn) = nsM;
   BN_SIGN(pRbn) = ippBigNumPOS;
}

__INLINE void cpMontDec_BN(IppsBigNumState* pRbn,
                     const IppsBigNumState* pXbn,
                           gsModEngine*     pModEngine)
{
   cpSize nsM = MOD_LEN(pModEngine);
   cpMontDec_BNU(BN_NUMBER(pRbn), BN_NUMBER(pXbn), BN_SIZE(pXbn), pModEngine);

   BN_SIZE(pRbn) = nsM;
   BN_SIGN(pRbn) = ippBigNumPOS;
}

/*
// Montgomery exponentiation (binary) "fast" and "safe" versions
*/
#define cpMontExpBin_BNU OWNAPI(cpMontExpBin_BNU)
   IPP_OWN_DECL (cpSize, cpMontExpBin_BNU, (BNU_CHUNK_T* pY, const BNU_CHUNK_T* pX, cpSize nsX, const BNU_CHUNK_T* pE, cpSize nsE, gsModEngine* pModEngine))
#define cpMontExpBin_BNU_sscm OWNAPI(cpMontExpBin_BNU_sscm)
   IPP_OWN_DECL (cpSize, cpMontExpBin_BNU_sscm, (BNU_CHUNK_T* pY, const BNU_CHUNK_T* pX, cpSize nsX, const BNU_CHUNK_T* pE, cpSize nsE, gsModEngine* pModEngine))

__INLINE void cpMontExpBin_BN_sscm(IppsBigNumState* pYbn,
                             const IppsBigNumState* pXbn,
                             const IppsBigNumState* pEbn,
                                   gsModEngine*     pMont)
{
   BNU_CHUNK_T* pX = BN_NUMBER(pXbn);
   cpSize nsX = BN_SIZE(pXbn);
   BNU_CHUNK_T* pE = BN_NUMBER(pEbn);
   cpSize nsE = BN_SIZE(pEbn);
   BNU_CHUNK_T* pY = BN_NUMBER(pYbn);
   cpSize nsY = cpMontExpBin_BNU_sscm(pY, pX,nsX, pE,nsE, pMont);
   FIX_BNU(pY, nsY);
   BN_SIZE(pYbn) = nsY;
   BN_SIGN(pYbn) = ippBigNumPOS;
}

__INLINE void cpMontExpBin_BN(IppsBigNumState* pYbn,
                        const IppsBigNumState* pXbn,
                        const IppsBigNumState* pEbn,
                              gsModEngine* pModEngine)
{
   BNU_CHUNK_T* pX = BN_NUMBER(pXbn);
   cpSize nsX = BN_SIZE(pXbn);
   BNU_CHUNK_T* pE = BN_NUMBER(pEbn);
   cpSize nsE = BN_SIZE(pEbn);
   BNU_CHUNK_T* pY = BN_NUMBER(pYbn);
   cpSize nsY = cpMontExpBin_BNU(pY, pX,nsX, pE,nsE, pModEngine);
   FIX_BNU(pY, nsY);
   BN_SIZE(pYbn) = nsY;
   BN_SIGN(pYbn) = ippBigNumPOS;
}


/*
// Montgomery exponentiation (fixed window)
*/
#define cpMontExp_WinSize OWNAPI(cpMontExp_WinSize)
   IPP_OWN_DECL (cpSize, cpMontExp_WinSize, (int bitsize))

#if defined(_USE_WINDOW_EXP_)
#define cpMontExpWin_BN_sscm OWNAPI(cpMontExpWin_BN_sscm)
   IPP_OWN_DECL (void, cpMontExpWin_BN_sscm, (IppsBigNumState* pY, const IppsBigNumState* pX, const IppsBigNumState* pE, gsModEngine* pMont, BNU_CHUNK_T* pPrecompResource))
#define cpMontExpWin_BN OWNAPI(cpMontExpWin_BN)
   IPP_OWN_DECL (void, cpMontExpWin_BN, (IppsBigNumState* pY, const IppsBigNumState* pX, const IppsBigNumState* pE, gsModEngine* pMont, BNU_CHUNK_T* pPrecompResource))
#endif

/*
// Montgomery multi-exponentiation
*/
/* precompute table for multi-exponentiation */
#define cpMontMultiExpInitArray OWNAPI(cpMontMultiExpInitArray)
   IPP_OWN_DECL (void, cpMontMultiExpInitArray, (BNU_CHUNK_T* pPrecomTbl, const BNU_CHUNK_T** ppX, cpSize xItemBitSize, cpSize numItems, gsModEngine* pMont))

/* multi-exponentiation */
#define cpFastMontMultiExp OWNAPI(cpFastMontMultiExp)
   IPP_OWN_DECL (void, cpFastMontMultiExp, (BNU_CHUNK_T* pY, const BNU_CHUNK_T* pPrecomTbl, const Ipp8u** ppE, cpSize eItemBitSize, cpSize numItems, gsModEngine* pMont))
/*
// Montgomery inversion
*/
#define cpMontInv_BNU OWNAPI(cpMontInv_BNU)
   IPP_OWN_DECL (BNU_CHUNK_T*, cpMontInv_BNU, (BNU_CHUNK_T* pR, const BNU_CHUNK_T* pA, IppsMontState* pMont))
#define cpRegInv_BNU OWNAPI(cpRegInv_BNU)
   IPP_OWN_DECL (BNU_CHUNK_T*, cpRegInv_BNU, (BNU_CHUNK_T* pR, const BNU_CHUNK_T* pA, IppsMontState* pMont))


/*
// Montgomery internal GetSize/Init functions
*/
#define cpMontGetSize OWNAPI(cpMontGetSize)
   IPP_OWN_DECL (IppStatus, cpMontGetSize, (cpSize maxLen32, int poolLength, cpSize* pCtxSize))
#define cpMontInit OWNAPI(cpMontInit)
   IPP_OWN_DECL (IppStatus, cpMontInit, (int maxLen32, int poolLength, IppsMontState* pMont))
#define cpMontSet OWNAPI(cpMontSet)
   IPP_OWN_DECL (IppStatus, cpMontSet, (const Ipp32u* pModulus, cpSize len32, IppsMontState* pMont))

#endif /* _CP_MONTGOMETRY_H */
