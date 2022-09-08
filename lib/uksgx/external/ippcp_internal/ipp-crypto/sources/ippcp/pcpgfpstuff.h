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
//  Purpose:
//     Intel(R) Integrated Performance Primitives
//     Cryptographic Primitives
//     Internal GF(p) basic Definitions & Function Prototypes
//
*/

#if !defined(_PCP_GFP_H_)
#define _PCP_GFP_H_

#include "owncp.h"
#include "pcpgfpmethod.h"
#include "pcpmontgomery.h"
#include "pcpmask_ct.h"

/* GF element */
typedef struct _cpGFpElement {
   Ipp32u      idCtx;   /* GF() element ident */
   int         length;  /* length of element (in BNU_CHUNK_T) */
   BNU_CHUNK_T*  pData;
} cpGFpElement;

#define GFPE_SET_ID(pCtx)  ((pCtx)->idCtx = (Ipp32u)idCtxGFPE ^ (Ipp32u)IPP_UINT_PTR(pCtx))
#define GFPE_ROOM(pCtx)    ((pCtx)->length)
#define GFPE_DATA(pCtx)    ((pCtx)->pData)

#define GFPE_VALID_ID(pCtx) ((((pCtx)->idCtx) ^ (Ipp32u)IPP_UINT_PTR(pCtx)) == idCtxGFPE)


/* GF(p) context */
typedef struct _cpGFp {
   Ipp32u         idCtx;   /* GFp spec ident     */
   gsModEngine*   pGFE;    /* arithmethic engine */
} cpGFp;

#define GFP_ALIGNMENT   ((int)(sizeof(void*)))

/* Local definitions */
#define GFP_MAX_BITSIZE       (IPP_MAX_GF_BITSIZE)      /* max bitsize for GF element */
#define GFP_POOL_SIZE         (16)//(IPP_MAX_EXPONENT_NUM+3)  /* num of elements into the pool */
#define GFP_RAND_ADD_BITS     (128)                     /* parameter of random element generation ?? == febits/2 */

#define GFP_SET_ID(pCtx)      ((pCtx)->idCtx = (Ipp32u)idCtxGFP ^ (Ipp32u)IPP_UINT_PTR(pCtx))
#define GFP_PMA(pCtx)         ((pCtx)->pGFE)

#define GFP_PARENT(pCtx)      MOD_PARENT((pCtx))
#define GFP_EXTDEGREE(pCtx)   MOD_EXTDEG((pCtx))
#define GFP_FEBITLEN(pCtx)    MOD_BITSIZE((pCtx))
#define GFP_FELEN(pCtx)       MOD_LEN((pCtx))
#define GFP_FELEN32(pCtx)     MOD_LEN32((pCtx))
#define GFP_PELEN(pCtx)       MOD_PELEN((pCtx))
#define GFP_METHOD(pCtx)      MOD_METHOD((pCtx))
#define GFP_MODULUS(pCtx)     MOD_MODULUS((pCtx))
#define GFP_MNT_FACTOR(pCtx)  MOD_MNT_FACTOR((pCtx))
#define GFP_MNT_R(pCtx)       MOD_MNT_R((pCtx))
#define GFP_MNT_RR(pCtx)      MOD_MNT_R2((pCtx))
#define GFP_HMODULUS(pCtx)    MOD_HMODULUS((pCtx))
#define GFP_QNR(pCtx)         MOD_QNR((pCtx))
#define GFP_POOL(pCtx)        MOD_POOL_BUF((pCtx))
#define GFP_MAXPOOL(pCtx)     MOD_MAXPOOL((pCtx))
#define GFP_USEDPOOL(pCtx)    MOD_USEDPOOL((pCtx))

#define GFP_IS_BASIC(pCtx)    (GFP_PARENT((pCtx))==NULL)
#define GFP_VALID_ID(pCtx)    ((((pCtx)->idCtx) ^ (Ipp32u)IPP_UINT_PTR(pCtx)) == idCtxGFP)

/*
// get/release n element from/to the pool
*/
#define cpGFpGetPool(n, gfe)     gsModPoolAlloc((gfe), (n))
#define cpGFpReleasePool(n, gfe) gsModPoolFree((gfe), (n))


__INLINE int cpGFpElementLen(const BNU_CHUNK_T* pE, int nsE)
{
   for(; nsE>1 && 0==pE[nsE-1]; nsE--) ;
   return nsE;
}
__INLINE BNU_CHUNK_T* cpGFpElementCopy(BNU_CHUNK_T* pR, const BNU_CHUNK_T* pE, int nsE)
{
   int n;
   for(n=0; n<nsE; n++) pR[n] = pE[n];
   return pR;
}
__INLINE BNU_CHUNK_T* cpGFpElementPad(BNU_CHUNK_T* pE, int nsE, BNU_CHUNK_T filler)
{
   int n;
   for(n=0; n<nsE; n++) pE[n] = filler;
   return pE;
}
__INLINE BNU_CHUNK_T* cpGFpElementCopyPad(BNU_CHUNK_T* pR, int nsR, const BNU_CHUNK_T* pE, int nsE)
{
   int n;
   for(n=0; n<nsE; n++) pR[n] = pE[n];
   for(; n<nsR; n++) pR[n] = 0;
   return pR;
}
__INLINE int cpGFpElementCmp(const BNU_CHUNK_T* pE, const BNU_CHUNK_T* pX, int nsE)
{
   return cpCmp_BNU(pE, nsE, pX, nsE);
}

__INLINE int cpGFpElementIsEquChunk(const BNU_CHUNK_T* pE, int nsE, BNU_CHUNK_T x)
{
   BNU_CHUNK_T res = pE[0] ^ x;
   int n;
   for(n=1; n<nsE; n++)
      res |= pE[n];
   return cpIsZero_ct(res) & 1;
}

__INLINE BNU_CHUNK_T* cpGFpElementSetChunk(BNU_CHUNK_T* pR, int nsR, BNU_CHUNK_T x)
{
   return cpGFpElementCopyPad(pR, nsR, &x, 1);
}

__INLINE BNU_CHUNK_T* cpGFpAdd(BNU_CHUNK_T* pR, const BNU_CHUNK_T* pA, const BNU_CHUNK_T* pB, gsModEngine* pGFE)
{
   return GFP_METHOD(pGFE)->add(pR, pA, pB, pGFE);
}

__INLINE BNU_CHUNK_T* cpGFpSub(BNU_CHUNK_T* pR, const BNU_CHUNK_T* pA, const BNU_CHUNK_T* pB, gsModEngine* pGFE)
{
   return GFP_METHOD(pGFE)->sub(pR, pA, pB, pGFE);
}

__INLINE BNU_CHUNK_T* cpGFpNeg(BNU_CHUNK_T* pR, const BNU_CHUNK_T* pA, gsModEngine* pGFE)
{
   return GFP_METHOD(pGFE)->neg(pR, pA, pGFE);
}

__INLINE BNU_CHUNK_T* cpGFpMul(BNU_CHUNK_T* pR, const BNU_CHUNK_T* pA, const BNU_CHUNK_T* pB, gsModEngine* pGFE)
{
   return GFP_METHOD(pGFE)->mul(pR, pA, pB, pGFE);
}

__INLINE BNU_CHUNK_T* cpGFpSqr(BNU_CHUNK_T* pR, const BNU_CHUNK_T* pA, gsModEngine* pGFE)
{
   return GFP_METHOD(pGFE)->sqr(pR, pA, pGFE);
}

__INLINE BNU_CHUNK_T* cpGFpHalve(BNU_CHUNK_T* pR, const BNU_CHUNK_T* pA, gsModEngine* pGFE)
{
   return GFP_METHOD(pGFE)->div2(pR, pA, pGFE);
}


#define GFP_LT(a,b,size)  (-1==cpGFpElementCmp((a),(b),(size)))
#define GFP_EQ(a,b,size)  ( 0==cpGFpElementCmp((a),(b),(size)))
#define GFP_GT(a,b,size)  ( 1==cpGFpElementCmp((a),(b),(size)))

#define GFP_IS_ZERO(a,size)  cpGFpElementIsEquChunk((a),(size), 0)
#define GFP_IS_ONE(a,size)   cpGFpElementIsEquChunk((a),(size), 1)

#define GFP_ZERO(a,size)      cpGFpElementSetChunk((a),(size), 0)
#define GFP_ONE(a,size)       cpGFpElementSetChunk((a),(size), 1)

#define GFP_IS_EVEN(a)  (0==((a)[0]&1))
#define GFP_IS_ODD(a)   (1==((a)[0]&1))


/* construct GF element */
__INLINE IppsGFpElement* cpGFpElementConstruct(IppsGFpElement* pR, BNU_CHUNK_T* pDataBufer, int ns)
{
   GFPE_SET_ID(pR);
   GFPE_ROOM(pR) = ns;
   GFPE_DATA(pR) = pDataBufer;
   return pR;
}


/* size of GFp context, init and setup */
#define cpGFpGetSize OWNAPI(cpGFpGetSize)
   IPP_OWN_DECL (int, cpGFpGetSize, (int feBitSize, int peBitSize, int numpe))
#define cpGFpInitGFp OWNAPI(cpGFpInitGFp)
   IPP_OWN_DECL (IppStatus, cpGFpInitGFp, (int primeBitSize, IppsGFpState* pGF))
#define cpGFpSetGFp OWNAPI(cpGFpSetGFp)
   IPP_OWN_DECL (IppStatus, cpGFpSetGFp, (const BNU_CHUNK_T* pPrime, int primeBitSize, const IppsGFpMethod* method, IppsGFpState* pGF))

/* operations */
#define cpGFpRand OWNAPI(cpGFpRand)
   IPP_OWN_DECL (BNU_CHUNK_T*, cpGFpRand, (BNU_CHUNK_T* pR, gsModEngine* pGFE, IppBitSupplier rndFunc, void* pRndParam))
#define cpGFpSet  OWNAPI(cpGFpSet)
   IPP_OWN_DECL (BNU_CHUNK_T*, cpGFpSet, (BNU_CHUNK_T* pR, const BNU_CHUNK_T* pDataA, int nsA, gsModEngine* pGFE))
#define cpGFpGet  OWNAPI(cpGFpGet)
   IPP_OWN_DECL (BNU_CHUNK_T*, cpGFpGet, (BNU_CHUNK_T* pDataA, int nsA, const BNU_CHUNK_T* pR, gsModEngine* pGFE))
#define cpGFpSetOctString OWNAPI(cpGFpSetOctString)
   IPP_OWN_DECL (BNU_CHUNK_T*, cpGFpSetOctString, (BNU_CHUNK_T* pR, const Ipp8u* pStr, int strSize, gsModEngine* pGFE))
#define cpGFpGetOctString OWNAPI(cpGFpGetOctString)
   IPP_OWN_DECL (Ipp8u*,       cpGFpGetOctString, (Ipp8u* pStr, int strSize, const BNU_CHUNK_T* pA, gsModEngine* pGFE))
#define cpGFpInv   OWNAPI(cpGFpInv)
   IPP_OWN_DECL (BNU_CHUNK_T*, cpGFpInv, (BNU_CHUNK_T* pR, const BNU_CHUNK_T* pA, gsModEngine* pGFE))
#define cpGFpExp   OWNAPI(cpGFpExp)
   IPP_OWN_DECL (BNU_CHUNK_T*, cpGFpExp, (BNU_CHUNK_T* pR, const BNU_CHUNK_T* pA, const BNU_CHUNK_T* pE, int nsE, gsModEngine* pGFE))
#define cpGFpSqrt OWNAPI(cpGFpSqrt)
   IPP_OWN_DECL (int,         cpGFpSqrt, (BNU_CHUNK_T* pR, const BNU_CHUNK_T* pA, gsModEngine* pGFE))
#define cpGFEqnr OWNAPI(cpGFEqnr)
   IPP_OWN_DECL (void,        cpGFEqnr, (gsModEngine* pGFE))

#endif /* _PCP_GFP_H_ */
