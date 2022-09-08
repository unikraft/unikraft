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
// 
//  Purpose:
//     Intel(R) Integrated Performance Primitives. Cryptography Primitives.
//     Internal EC over GF(p^m) basic Definitions & Function Prototypes
// 
// 
*/

#if !defined(_CP_ECGFP_H_)
#define _CP_ECGFP_H_

#include "pcpgfpstuff.h"
#include "pcpgfpxstuff.h"
#include "pcpmask_ct.h"

#define _LEGACY_ECCP_SUPPORT_

/*
// EC over GF(p) Point context
*/
typedef struct _cpGFpECPoint {
   Ipp32u       idCtx;  /* EC Point identifier     */
   int          flags;  /* flags: affine           */
   int    elementSize;  /* size of each coordinate */
   BNU_CHUNK_T* pData;  /* coordinate X, Y, Z      */
} cpGFPECPoint;

/*
// Contet Access Macros
*/
#define ECP_POINT_SET_ID(ctx)   ((ctx)->idCtx = (Ipp32u)idCtxGFPPoint ^ (Ipp32u)IPP_UINT_PTR(ctx))
#define ECP_POINT_FLAGS(ctx)    ((ctx)->flags)
#define ECP_POINT_FELEN(ctx)    ((ctx)->elementSize)
#define ECP_POINT_DATA(ctx)     ((ctx)->pData)
#define ECP_POINT_X(ctx)        ((ctx)->pData)
#define ECP_POINT_Y(ctx)        ((ctx)->pData+(ctx)->elementSize)
#define ECP_POINT_Z(ctx)        ((ctx)->pData+(ctx)->elementSize*2)
#define ECP_POINT_VALID_ID(ctx)  ((((ctx)->idCtx) ^ (Ipp32u)IPP_UINT_PTR(ctx)) == (Ipp32u)idCtxGFPPoint)

/* point flags */
#define ECP_AFFINE_POINT   (1)
#define ECP_FINITE_POINT   (2)

#define  IS_ECP_AFFINE_POINT(ctx)      (ECP_POINT_FLAGS((ctx))&ECP_AFFINE_POINT)
#define SET_ECP_AFFINE_POINT(ctx)      (ECP_POINT_FLAGS((ctx))|ECP_AFFINE_POINT)
#define SET_ECP_PROJECTIVE_POINT(ctx)  (ECP_POINT_FLAGS((ctx))&~ECP_AFFINE_POINT)

#define  IS_ECP_FINITE_POINT(ctx)      (ECP_POINT_FLAGS((ctx))&ECP_FINITE_POINT)
#define SET_ECP_FINITE_POINT(ctx)      (ECP_POINT_FLAGS((ctx))|ECP_FINITE_POINT)
#define SET_ECP_INFINITE_POINT(ctx)    (ECP_POINT_FLAGS((ctx))&~ECP_FINITE_POINT)

/*
// define using projective coordinates
*/
#define JACOBIAN        (0)
#define HOMOGENEOUS     (1)
#define ECP_PROJECTIVE_COORD  JACOBIAN
//#define ECP_PROJECTIVE_COORD  HOMOGENEOUS

#if (ECP_PROJECTIVE_COORD != JACOBIAN && ECP_PROJECTIVE_COORD != HOMOGENEOUS)
   #error ECP_PROJECTIVE_COORD should be either JACOBIAN or HOMOGENEOUS type
#endif


/*
// pre-computed Base Point descriptor
*/
IPP_OWN_FUNPTR (void, selectAP,  (BNU_CHUNK_T* pAP, const BNU_CHUNK_T* pAPtbl, int index))

typedef struct _cpPrecompAP {
   int      w;                   /* scalar's window bitsize */
   selectAP select_affine_point; /* get affine point function */
   const BNU_CHUNK_T* pTbl;      /* pre-computed table */
} cpPrecompAP;


/* EC over GF(p) context */
typedef struct _cpGFpEC {
   Ipp32u       idCtx;  /* EC identifier */

   IppsGFpState*  pGF;  /* arbitrary GF(p^d)*/

   int          subgroup;  /* set up subgroup */
   int       elementSize;  /* length of EC point */
   int      orderBitSize;  /* base_point order bitsize */
   BNU_CHUNK_T*       pA;  /*   EC parameter A */
   BNU_CHUNK_T*       pB;  /*                B */
   BNU_CHUNK_T*       pG;  /*       base_point */
   BNU_CHUNK_T* cofactor;  /* cofactor = #E/base_point order */
   int          parmAspc;  /* NIST's, EPIDv2.0 A-parameter specific */
   int          infinity;  /* 0/1 if B !=0/==0 */
   const cpPrecompAP* pBaseTbl;  /* address of pre-computed [n]G tabble */
   gsModEngine*    pMontR; /* EC order montgomery engine */

   BNU_CHUNK_T*    pPool;  /* pool of points   */
   #if defined(_LEGACY_ECCP_SUPPORT_)
   BNU_CHUNK_T*  pPublic;  /* regular   public key */
   BNU_CHUNK_T*  pPublicE; /* ephemeral public key */
   BNU_CHUNK_T*  pPrivat;  /* regular   private key */
   BNU_CHUNK_T*  pPrivatE; /* ephemeral private key */
   BNU_CHUNK_T*  pBuffer;  /* pointer to scaratch buffer (for legacy ECCP only) */
   #endif
} cpGFPEC;

#define ECGFP_ALIGNMENT   ((int)(sizeof(void*)))

/* Local definitions */
#define EC_POOL_SIZE       (10)  /* num of points into the pool */

#define EC_MONT_POOL_SIZE   (4)  /* num of temp values for modular arithmetic */

#define ECP_SET_ID(pCtx)      ((pCtx)->idCtx = (Ipp32u)idCtxGFPEC ^ (Ipp32u)IPP_UINT_PTR(pCtx))
#define ECP_GFP(pCtx)         ((pCtx)->pGF)
#define ECP_SUBGROUP(pCtx)    ((pCtx)->subgroup)
#define ECP_POINTLEN(pCtx)    ((pCtx)->elementSize)
#define ECP_ORDBITSIZE(pCtx)  ((pCtx)->orderBitSize)
#define ECP_COFACTOR(pCtx)    ((pCtx)->cofactor)
#define ECP_SPECIFIC(pCtx)    ((pCtx)->parmAspc)
#define ECP_INFINITY(pCtx)    ((pCtx)->infinity)
#define ECP_A(pCtx)           ((pCtx)->pA)
#define ECP_B(pCtx)           ((pCtx)->pB)
#define ECP_G(pCtx)           ((pCtx)->pG)
#define ECP_PREMULBP(pCtx)    ((pCtx)->pBaseTbl)
#define ECP_MONT_R(pCtx)      ((pCtx)->pMontR)
#define ECP_POOL(pCtx)        ((pCtx)->pPool)
#if defined(_LEGACY_ECCP_SUPPORT_)
   #define ECP_PUBLIC(pCtx)   ((pCtx)->pPublic)
   #define ECP_PUBLIC_E(pCtx) ((pCtx)->pPublicE)
   #define ECP_PRIVAT(pCtx)   ((pCtx)->pPrivat)
   #define ECP_PRIVAT_E(pCtx) ((pCtx)->pPrivatE)
   #define ECP_SBUFFER(pCtx)  ((pCtx)->pBuffer)
#endif

#define VALID_ECP_ID(pCtx)     ((((pCtx)->idCtx) ^ (Ipp32u)IPP_UINT_PTR((pCtx))) == (Ipp32u)idCtxGFPEC)

/* EC curve specific (a-parameter) */
#define ECP_Acom    (0) /* commont case */
#define ECP_Ami3    (1) /* a=-3 NIST's and SM2 curve */
#define ECP_Aeq0    (2) /* a=0  EPIDv2.0 curve */

#define ECP_ARB     ECP_Acom
#define ECP_STD     ECP_Ami3
#define ECP_EPID2   ECP_Aeq0

/* std ec pre-computed tables */
#define gfpec_precom_nistP192r1_fun OWNAPI(gfpec_precom_nistP192r1_fun)
#define gfpec_precom_nistP224r1_fun OWNAPI(gfpec_precom_nistP224r1_fun)
#define gfpec_precom_nistP256r1_fun OWNAPI(gfpec_precom_nistP256r1_fun)
#define gfpec_precom_nistP384r1_fun OWNAPI(gfpec_precom_nistP384r1_fun)
#define gfpec_precom_nistP521r1_fun OWNAPI(gfpec_precom_nistP521r1_fun)
#define gfpec_precom_sm2_fun        OWNAPI(gfpec_precom_sm2_fun)

IPP_OWN_DECL (const cpPrecompAP*, gfpec_precom_nistP192r1_fun, (void))
IPP_OWN_DECL (const cpPrecompAP*, gfpec_precom_nistP224r1_fun, (void))
IPP_OWN_DECL (const cpPrecompAP*, gfpec_precom_nistP256r1_fun, (void))
IPP_OWN_DECL (const cpPrecompAP*, gfpec_precom_nistP384r1_fun, (void))
IPP_OWN_DECL (const cpPrecompAP*, gfpec_precom_nistP521r1_fun, (void))
IPP_OWN_DECL (const cpPrecompAP*, gfpec_precom_sm2_fun, (void))

/*
// get/release n points from/to the pool
*/
__INLINE BNU_CHUNK_T* cpEcGFpGetPool(int n, IppsGFpECState* pEC)
{
   BNU_CHUNK_T* pPool = ECP_POOL(pEC);
   ECP_POOL(pEC) += n*GFP_FELEN(GFP_PMA(ECP_GFP(pEC)))*3;
   return pPool;
}
__INLINE void cpEcGFpReleasePool(int n, IppsGFpECState* pEC)
{
   ECP_POOL(pEC) -= n*GFP_FELEN(GFP_PMA(ECP_GFP(pEC)))*3;
}

__INLINE IppsGFpECPoint* cpEcGFpInitPoint(IppsGFpECPoint* pPoint, BNU_CHUNK_T* pData, int flags, const IppsGFpECState* pEC)
{
   ECP_POINT_SET_ID(pPoint);
   ECP_POINT_FLAGS(pPoint) = flags;
   ECP_POINT_FELEN(pPoint) = GFP_FELEN(GFP_PMA(ECP_GFP(pEC)));
   ECP_POINT_DATA(pPoint) = pData;
   return pPoint;
}

/* copy one point into another */
__INLINE IppsGFpECPoint* gfec_CopyPoint(IppsGFpECPoint* pPointR, const IppsGFpECPoint* pPointA, int elemLen)
{
   cpGFpElementCopy(ECP_POINT_DATA(pPointR), ECP_POINT_DATA(pPointA), 3*elemLen);
   ECP_POINT_FLAGS(pPointR) = ECP_POINT_FLAGS(pPointA);
   return pPointR;
}


__INLINE IppsGFpECPoint* gfec_SetPointAtInfinity(IppsGFpECPoint* pPoint)
{
   int elemLen = ECP_POINT_FELEN(pPoint);
   cpGFpElementPad(ECP_POINT_X(pPoint), elemLen, 0);
   cpGFpElementPad(ECP_POINT_Y(pPoint), elemLen, 0);
   cpGFpElementPad(ECP_POINT_Z(pPoint), elemLen, 0);
   ECP_POINT_FLAGS(pPoint) = 0;
   return pPoint;
}

/*
// test infinity:
//    IsProjectivePointAtInfinity
*/
__INLINE int gfec_IsPointAtInfinity(const IppsGFpECPoint* pPoint)
{
   return GFP_IS_ZERO( ECP_POINT_Z(pPoint), ECP_POINT_FELEN(pPoint));
}



/* signed encode */
__INLINE void booth_recode(Ipp8u* sign, Ipp8u* digit, Ipp8u in, int w)
{
   Ipp8u s = (Ipp8u)(~((in >> w) - 1));
   int d = (1 << (w+1)) - in - 1;
   d = (d & s) | (in & ~s);
   d = (d >> 1) + (d & 1);
   *sign = s & 1;
   *digit = (Ipp8u)d;
}


#define gfec_point_add OWNAPI(gfec_point_add)
#define gfec_affine_point_add OWNAPI(gfec_affine_point_add)
#define gfec_point_double OWNAPI(gfec_point_double)
#define gfec_point_mul OWNAPI(gfec_point_mul)
#define gfec_point_prod OWNAPI(gfec_point_prod)
#define gfec_base_point_mul OWNAPI(gfec_base_point_mul)
#define setupTable OWNAPI(setupTable)

IPP_OWN_DECL (void, gfec_point_add, (BNU_CHUNK_T* pRdata, const BNU_CHUNK_T* pPdata, const BNU_CHUNK_T* pQdata, IppsGFpECState* pEC))
IPP_OWN_DECL (void, gfec_affine_point_add, (BNU_CHUNK_T* pRdata, const BNU_CHUNK_T* pPdata, const BNU_CHUNK_T* pAdata, IppsGFpECState* pEC))
IPP_OWN_DECL (void, gfec_point_double, (BNU_CHUNK_T* pRdata, const BNU_CHUNK_T* pPdata, IppsGFpECState* pEC))
IPP_OWN_DECL (void, gfec_point_mul, (BNU_CHUNK_T* pRdata, const BNU_CHUNK_T* pPdata, const Ipp8u* pScalar8, int scalarBitSize, IppsGFpECState* pEC, Ipp8u* pScratchBuffer))
IPP_OWN_DECL (void, gfec_point_prod, (BNU_CHUNK_T* pointR, const BNU_CHUNK_T* pointA, const Ipp8u* pScalarA, const BNU_CHUNK_T* pointB, const Ipp8u* pScalarB, int scalarBitSize, IppsGFpECState* pEC, Ipp8u* pScratchBuffer))
IPP_OWN_DECL (void, gfec_base_point_mul, (BNU_CHUNK_T* pRdata, const Ipp8u* pScalarB, int scalarBitSize, IppsGFpECState* pEC))
IPP_OWN_DECL (void, setupTable, (BNU_CHUNK_T* pTbl, const BNU_CHUNK_T* pPdata, IppsGFpECState* pEC))


/* size of context */
#define cpGFpECGetSize OWNAPI(cpGFpECGetSize)
IPP_OWN_DECL (int, cpGFpECGetSize, (int deg, int basicElmBitSize))

/* point operations */
#define gfec_GetPoint OWNAPI(gfec_GetPoint)
#define gfec_SetPoint OWNAPI(gfec_SetPoint)
#define gfec_MakePoint OWNAPI(gfec_MakePoint)
#define gfec_ComparePoint OWNAPI(gfec_ComparePoint)
#define gfec_IsPointOnCurve OWNAPI(gfec_IsPointOnCurve)

IPP_OWN_DECL (int, gfec_GetPoint, (BNU_CHUNK_T* pX, BNU_CHUNK_T* pY, const IppsGFpECPoint* pPoint, IppsGFpECState* pEC))
IPP_OWN_DECL (int, gfec_SetPoint, (BNU_CHUNK_T* pP, const BNU_CHUNK_T* pX, const BNU_CHUNK_T* pY, IppsGFpECState* pEC))
IPP_OWN_DECL (int, gfec_MakePoint, (IppsGFpECPoint* pPoint, const BNU_CHUNK_T* pElm, IppsGFpECState* pEC))
IPP_OWN_DECL (int, gfec_ComparePoint, (const IppsGFpECPoint* pP, const IppsGFpECPoint* pQ, IppsGFpECState* pEC))
IPP_OWN_DECL (int, gfec_IsPointOnCurve, (const IppsGFpECPoint* pP, IppsGFpECState* pEC))

__INLINE IppsGFpECPoint* gfec_DblPoint(IppsGFpECPoint* pR,
                        const IppsGFpECPoint* pP, IppsGFpECState* pEC)
{
   gfec_point_double(ECP_POINT_X(pR), ECP_POINT_X(pP), pEC);
   ECP_POINT_FLAGS(pR) = gfec_IsPointAtInfinity(pR)? 0 : ECP_FINITE_POINT;
   return pR;
}

__INLINE IppsGFpECPoint* gfec_AddPoint(IppsGFpECPoint* pR,
                        const IppsGFpECPoint* pP, const IppsGFpECPoint* pQ,
                        IppsGFpECState* pEC)
{
   gfec_point_add(ECP_POINT_X(pR), ECP_POINT_X(pP), ECP_POINT_X(pQ), pEC);
   ECP_POINT_FLAGS(pR) = gfec_IsPointAtInfinity(pR)? 0 : ECP_FINITE_POINT;
   return pR;
}


#define gfec_NegPoint OWNAPI(gfec_NegPoint)
#define gfec_MulPoint OWNAPI(gfec_MulPoint)
#define gfec_MulBasePoint OWNAPI(gfec_MulBasePoint)
/* #define gfec_PointProduct OWNAPI(gfec_PointProduct) */
#define gfec_BasePointProduct OWNAPI(gfec_BasePointProduct)
#define p192r1_select_ap_w7 OWNAPI(p192r1_select_ap_w7)
#define p224r1_select_ap_w7 OWNAPI(p224r1_select_ap_w7)
#define p256r1_select_ap_w7 OWNAPI(p256r1_select_ap_w7)
#define p384r1_select_ap_w5 OWNAPI(p384r1_select_ap_w5)
#define p521r1_select_ap_w5 OWNAPI(p521r1_select_ap_w5)

IPP_OWN_DECL (IppsGFpECPoint*, gfec_NegPoint, (IppsGFpECPoint* pR, const IppsGFpECPoint* pP, IppsGFpECState* pEC))
IPP_OWN_DECL (IppsGFpECPoint*, gfec_MulPoint, (IppsGFpECPoint* pR, const IppsGFpECPoint* pP, const BNU_CHUNK_T* pScalar, int scalarLen, IppsGFpECState* pEC, Ipp8u* pScratchBuffer))
IPP_OWN_DECL (IppsGFpECPoint*, gfec_MulBasePoint, (IppsGFpECPoint* pR, const BNU_CHUNK_T* pScalar, int scalarLen, IppsGFpECState* pEC, Ipp8u* pScratchBuffer))
/* IPP_OWN_DECL (IppsGFpECPoint*, gfec_PointProduct, (IppsGFpECPoint* pR, const IppsGFpECPoint* pP, const BNU_CHUNK_T* pScalarP, int scalarPlen, const IppsGFpECPoint* pQ, const BNU_CHUNK_T* pScalarQ, int scalarQlen, IppsGFpECState* pEC, Ipp8u* pScratchBuffer)) */
IPP_OWN_DECL (IppsGFpECPoint*, gfec_BasePointProduct, (IppsGFpECPoint* pR, const BNU_CHUNK_T* pScalarG, int scalarGlen, const IppsGFpECPoint* pP, const BNU_CHUNK_T* pScalarP, int scalarPlen, IppsGFpECState* pEC, Ipp8u* pScratchBuffer))

IPP_OWN_DECL (void, p192r1_select_ap_w7, (BNU_CHUNK_T* pAffinePoint, const BNU_CHUNK_T* pTable, int index))
IPP_OWN_DECL (void, p224r1_select_ap_w7, (BNU_CHUNK_T* pAffinePoint, const BNU_CHUNK_T* pTable, int index))
IPP_OWN_DECL (void, p256r1_select_ap_w7, (BNU_CHUNK_T* pAffinePoint, const BNU_CHUNK_T* pTable, int index))
IPP_OWN_DECL (void, p384r1_select_ap_w5, (BNU_CHUNK_T* pAffinePoint, const BNU_CHUNK_T* pTable, int index))
IPP_OWN_DECL (void, p521r1_select_ap_w5, (BNU_CHUNK_T* pAffinePoint, const BNU_CHUNK_T* pTable, int index))

#endif /* _CP_ECGFP_H_ */
