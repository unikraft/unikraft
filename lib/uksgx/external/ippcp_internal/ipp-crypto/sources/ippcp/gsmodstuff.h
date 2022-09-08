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

#if !defined(_GS_MOD_STUFF_H)
#define _GS_MOD_STUFF_H

//#define MONTMUL_ONESTAGE

#include "owncp.h"

#include "pcpbnuimpl.h"
#include "gsmodmethod.h"

#define MOD_ENGINE_MIN_POOL_SIZE 1

typedef struct _gsModEngine gsModEngine_T;

typedef struct _gsModEngine
{
   gsModEngine_T*       pParentME;     /* pointer to parent stuff          */
   int                  extdegree;     /* parent modulus extension (deg)   */
   int                  modBitLen;     /* length of modulus in bits        */
   int                  modLen;        /* length of modulus  (BNU_CHUNK_T) */
   int                  modLen32;      /* length of modulus  (Ipp32u)      */
   int                  peLen;         /* length of pool element (BNU_CHUNK_T) */
   const gsModMethod*   method;        /* modular arithmetic methods       */
   BNU_CHUNK_T*         pModulus;      /* modulus                          */
   BNU_CHUNK_T          k0;            /* low word of (1/modulus) mod R    */
   BNU_CHUNK_T*         pMontR;        /* mont_enc(1)                      */
   BNU_CHUNK_T*         pMontR2;       /* mont_enc(1)^2                    */
   BNU_CHUNK_T*         pHalfModulus;  /* modulus/2                        */
   BNU_CHUNK_T*         pQnr;          /* quadratic non-residue            */
   int                  poolLenUsed;   /* number of reserved temporary BNU */
   int                  poolLen;       /* max number of temporary BNU      */
   BNU_CHUNK_T*         pBuffer;       /* buffer of modLen*nBuffers length */
} gsModEngine;

/* accessory macros */
#define MOD_PARENT(eng)      ((eng)->pParentME)
#define MOD_EXTDEG(eng)      ((eng)->extdegree)
#define MOD_BITSIZE(eng)     ((eng)->modBitLen)
#define MOD_LEN(eng)         ((eng)->modLen)
#define MOD_LEN32(eng)       ((eng)->modLen32)
#define MOD_PELEN(eng)       ((eng)->peLen)
#define MOD_METHOD(eng)      ((eng)->method)
#define MOD_MODULUS(eng)     ((eng)->pModulus)
#define MOD_MNT_FACTOR(eng)  ((eng)->k0)
#define MOD_MNT_R(eng)       ((eng)->pMontR)
#define MOD_MNT_R2(eng)      ((eng)->pMontR2)
#define MOD_HMODULUS(eng)    ((eng)->pHalfModulus)
#define MOD_QNR(eng)         ((eng)->pQnr)
#define MOD_POOL_BUF(eng)    ((eng)->pBuffer)
#define MOD_MAXPOOL(eng)     ((eng)->poolLen)
#define MOD_USEDPOOL(eng)    ((eng)->poolLenUsed)

#define MOD_BUFFER(eng,n)    ((eng)->pBuffer+(MOD_PELEN(eng))*(n))

#define MOD_ENGINE_ALIGNMENT ((int)sizeof(void*))

/*
// size of context and it initialization
*/
#define gsModEngineInit OWNAPI(gsModEngineInit)
   IPP_OWN_DECL (IppStatus, gsModEngineInit, (gsModEngine* pME, const Ipp32u* pModulus, int modulusBitSize, int numpe, const gsModMethod* method))
#define gsModEngineGetSize OWNAPI(gsModEngineGetSize)
   IPP_OWN_DECL (IppStatus, gsModEngineGetSize, (int modulusBitSIze, int numpe, int* pSize))
#define gsMontFactor OWNAPI(gsMontFactor)
   IPP_OWN_DECL (BNU_CHUNK_T, gsMontFactor, (BNU_CHUNK_T m0))


/*
// pool management methods
*/

/*F*
// Name: gsModPoolAlloc
//
// Purpose: Allocation pool.
//
// Returns:                        Reason:
//       pointer to allocate Pool       enough of pool
//       NULL                           required pool more than pME have
//
// Parameters:
//    pME       ModEngine
//    poolReq   Required pool
*F*/

__INLINE BNU_CHUNK_T* gsModPoolAlloc(gsModEngine* pME, int poolReq)
{
   BNU_CHUNK_T* pPool = MOD_BUFFER(pME, pME->poolLenUsed);

   if(pME->poolLenUsed + poolReq > pME->poolLen)
      pPool = NULL;
   else
      pME->poolLenUsed += poolReq;

   return pPool;
}

/*F*
// Name: gsModPoolFree
//
// Purpose: Delete pool.
//
// Returns:
//    nothing
//
// Parameters:
//    pME       ModEngine
//    poolReq   Required pool
*F*/

__INLINE void gsModPoolFree(gsModEngine* pME, int poolReq)
{
   if(pME->poolLenUsed < poolReq)
      poolReq = pME->poolLenUsed;
   pME->poolLenUsed -= poolReq;
}

/* return pointer to the top pool buffer */
#define gsModGetPool OWNAPI(gsModGetPool)
   IPP_OWN_DECL (BNU_CHUNK_T*, gsModGetPool, (gsModEngine* pME))
/*
// advanced operations
*/
IPP_OWN_FUNPTR (int, alm_inv, (BNU_CHUNK_T* pr, const BNU_CHUNK_T* pa, gsModEngine* pMA))

#define alm_mont_inv OWNAPI(alm_mont_inv)
   IPP_OWN_DECL (int, alm_mont_inv, (BNU_CHUNK_T* pr, const BNU_CHUNK_T* pa, gsModEngine* pMA))
#define alm_mont_inv_ct OWNAPI(alm_mont_inv_ct)
   IPP_OWN_DECL (int, alm_mont_inv_ct, (BNU_CHUNK_T* pr, const BNU_CHUNK_T* pa, gsModEngine* pMA))
#define gs_mont_inv OWNAPI(gs_mont_inv)
   IPP_OWN_DECL (BNU_CHUNK_T*, gs_mont_inv, (BNU_CHUNK_T* pr, const BNU_CHUNK_T* pa, gsModEngine* pMA, alm_inv invf))
#define gs_inv OWNAPI(gs_inv)
   IPP_OWN_DECL (BNU_CHUNK_T*, gs_inv, (BNU_CHUNK_T* pr, const BNU_CHUNK_T* pa, gsModEngine* pMA, alm_inv invf))

/*
// Pack/Unpack methods
*/
#define gsPackModEngineCtx OWNAPI(gsPackModEngineCtx)
   IPP_OWN_DECL (void, gsPackModEngineCtx, (const gsModEngine* pCtx, Ipp8u* pBuffer))
#define gsUnpackModEngineCtx OWNAPI(gsUnpackModEngineCtx)
   IPP_OWN_DECL (void, gsUnpackModEngineCtx, (const Ipp8u* pBuffer, gsModEngine* pCtx))

#endif /* _GS_MOD_STUFF_H */
