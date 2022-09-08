/*******************************************************************************
* Copyright 2015-2021 Intel Corporation
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
//     Message Authentication Algorithm
//     Internal Definitions and Internal Functions Prototypes
// 
// 
*/

#if !defined(_CP_AESAUTH_GCM_H)
#define _CP_AESAUTH_GCM_H

#include "owncp.h"
#include "pcpaesm.h"

#if(_IPP32E < _IPP32E_K0)

#define BLOCK_SIZE (MBS_RIJ128)

/* GCM Hash prototype: GHash = GHash*HKey mod G() */
IPP_OWN_FUNPTR (void, MulGcm_, (Ipp8u* pGHash, const Ipp8u* pHKey, const void* pParam))

/* GCM Authentication prototype: GHash = (GHash^src[])*HKey mod G() */
IPP_OWN_FUNPTR (void, Auth_, (Ipp8u* pHash, const Ipp8u* pSrc, int len, const Ipp8u* pHKey, const void* pParam))

/* GCM Encrypt_Authentication prototype */
IPP_OWN_FUNPTR (void, Encrypt_, (Ipp8u* pDst, const Ipp8u* pSrc, int len, IppsAES_GCMState* pCtx))

/* GCM Authentication_Decrypt prototype */
IPP_OWN_FUNPTR (void, Decrypt_, (Ipp8u* pDst, const Ipp8u* pSrc, int len, IppsAES_GCMState* pCtx))

typedef enum {
   GcmInit,
   GcmIVprocessing,
   GcmAADprocessing,
   GcmTXTprocessing
} GcmState;

struct _cpAES_GCM {
   
   Ipp32u   idCtx;                  /* AES-GCM id                    */
   GcmState state;                  /* GCM state: Init, IV|AAD|TXT processing */
   Ipp64u   ivLen;                  /* IV length (bytes)             */
   Ipp64u   aadLen;                 /* header length (bytes)         */
   Ipp64u   txtLen;                 /* text length (bytes)           */

   int      bufLen;                 /* staff buffer length           */
   __ALIGN16                        /* aligned buffers               */
   Ipp8u    counter[BLOCK_SIZE];    /* counter                       */
   Ipp8u    ecounter0[BLOCK_SIZE];  /* encrypted initial counter     */
   Ipp8u    ecounter[BLOCK_SIZE];   /* encrypted counter             */
   Ipp8u    ghash[BLOCK_SIZE];      /* ghash accumulator             */

   MulGcm_  hashFun;                /* AES-GCM mul function          */
   Auth_    authFun;                /* authentication function       */
   Encrypt_ encFun;                 /* encryption & authentication   */
   Decrypt_ decFun;                 /* authentication & decryption   */

   __ALIGN16                        /* aligned AES context           */
   IppsAESSpec cipher;

   __ALIGN16                        /* aligned pre-computed data:    */
   Ipp8u multiplier[BLOCK_SIZE];    /* - (default) hKey                             */
                                    /* - (aes_ni)  hKey*t, (hKey*t)^2, (hKey*t)^4   */
                                    /* - (vaes_ni) 8 reverted ordered vectors by 4 128-bit values.
                                      hKeys derivations in the multiplier[] array in order of appearance
                                      (zero-index starts from the left):
                                      hKey^4<<1, hKey^3<<1,   hKey^2<<1,  hKey<<1,
                                      hKey^8<<1, hKey^7<<1,   hKey^6<<1,  hKey^5<<1,
                                      hKey^12<<1, hKey^11<<1, hKey^10<<1, hKey^9<<1,
                                      hKey^16<<1, hKey^15<<1, hKey^14<<1, hKey^13<<1,
                                      ... <same 4 vectors for Karatsuba partial products> ...
                                     */
                                     /* - (safe) hKey*(t^i), i=0,...,127             */
};

#define CTR_POS         12

/* alignment */
#define AESGCM_ALIGNMENT   (16)

#define PRECOMP_DATA_SIZE_AES_NI_AESGCM   (BLOCK_SIZE*4)
#define PRECOMP_DATA_SIZE_VAES_NI_AESGCM  (BLOCK_SIZE*16*2)
#define PRECOMP_DATA_SIZE_FAST2K          (BLOCK_SIZE*128)

/*
// Useful macros
*/
#define AESGCM_SET_ID(context)       ((context)->idCtx = (Ipp32u)idCtxAESGCM ^ (Ipp32u)IPP_UINT_PTR(context))
#define AESGCM_STATE(context)        ((context)->state)

#define AESGCM_IV_LEN(context)       ((context)->ivLen)
#define AESGCM_AAD_LEN(context)      ((context)->aadLen)
#define AESGCM_TXT_LEN(context)      ((context)->txtLen)

#define AESGCM_BUFLEN(context)       ((context)->bufLen)
#define AESGCM_COUNTER(context)      ((context)->counter)
#define AESGCM_ECOUNTER0(context)    ((context)->ecounter0)
#define AESGCM_ECOUNTER(context)     ((context)->ecounter)
#define AESGCM_GHASH(context)        ((context)->ghash)

#define AESGCM_HASH(context)         ((context)->hashFun)
#define AESGCM_AUTH(context)         ((context)->authFun)
#define AESGCM_ENC(context)          ((context)->encFun)
#define AESGCM_DEC(context)          ((context)->decFun)

#define AESGCM_CIPHER(context)       (IppsAESSpec*)(&((context)->cipher))

#define AESGCM_HKEY(context)         ((context)->multiplier)
#define AESGCM_CPWR(context)         ((context)->multiplier)
#define AES_GCM_MTBL(context)        ((context)->multiplier)

#define AESGCM_VALID_ID(context)     ((((context)->idCtx) ^ (Ipp32u)IPP_UINT_PTR((context))) == (Ipp32u)idCtxAESGCM)

#if 0
__INLINE void IncrementCounter32(Ipp8u* pCtr)
{
   int i;
   for(i=BLOCK_SIZE-1; i>=CTR_POS && 0==(Ipp8u)(++pCtr[i]); i--) ;
}
#endif
__INLINE void IncrementCounter32(Ipp8u* pCtr)
{
   Ipp32u* pCtr32 = (Ipp32u*)pCtr;
   Ipp32u ctrVal = pCtr32[3];
   ctrVal = ENDIANNESS32(ctrVal);
   ctrVal++;
   ctrVal = ENDIANNESS32(ctrVal);
   pCtr32[3] = ctrVal;
}

#if (_IPP>=_IPP_P8) || (_IPP32E>=_IPP32E_Y8)
#define AesGcmPrecompute_avx OWNAPI(AesGcmPrecompute_avx)
   IPP_OWN_DECL (void, AesGcmPrecompute_avx, (Ipp8u* pPrecomputeData, const Ipp8u* pHKey))
#define AesGcmMulGcm_avx OWNAPI(AesGcmMulGcm_avx)
   IPP_OWN_DECL (void, AesGcmMulGcm_avx, (Ipp8u* pGhash, const Ipp8u* pHkey, const void* pParam))
#define AesGcmAuth_avx OWNAPI(AesGcmAuth_avx)
   IPP_OWN_DECL (void, AesGcmAuth_avx, (Ipp8u* pGhash, const Ipp8u* pSrc, int len, const Ipp8u* pHkey, const void* pParam))
#define wrpAesGcmEnc_avx OWNAPI(wrpAesGcmEnc_avx)
   IPP_OWN_DECL (void, wrpAesGcmEnc_avx, (Ipp8u* pDst, const Ipp8u* pSrc, int len, IppsAES_GCMState* pCtx))
#define wrpAesGcmDec_avx OWNAPI(wrpAesGcmDec_avx)
   IPP_OWN_DECL (void, wrpAesGcmDec_avx, (Ipp8u* pDst, const Ipp8u* pSrc, int len, IppsAES_GCMState* pCtx))
#define AesGcmEnc_avx OWNAPI(AesGcmEnc_avx)
   IPP_OWN_DECL (void, AesGcmEnc_avx, (Ipp8u* pDst, const Ipp8u* pSrc, int len, RijnCipher cipher, int nr, const Ipp8u* pKeys, Ipp8u* pGhash, Ipp8u* pCnt, Ipp8u* pECnt, const Ipp8u* pMuls))
#define AesGcmDec_avx OWNAPI(AesGcmDec_avx)
   IPP_OWN_DECL (void, AesGcmDec_avx, (Ipp8u* pDst, const Ipp8u* pSrc, int len, RijnCipher cipher, int nr, const Ipp8u* pKeys, Ipp8u* pGhash, Ipp8u* pCnt, Ipp8u* pECnt, const Ipp8u* pMuls))
#endif

#if(_IPP32E>=_IPP32E_K0)
#define AesGcmPrecompute_vaes OWNAPI(AesGcmPrecompute_vaes)
   IPP_OWN_DECL (void, AesGcmPrecompute_vaes, (Ipp8u* const pPrecomputeData, const Ipp8u* const pHKey))
#define AesGcmMulGcm_vaes OWNAPI(AesGcmMulGcm_vaes)
   IPP_OWN_DECL (void, AesGcmMulGcm_vaes, (Ipp8u* pGhash, const Ipp8u* pHkey, const void* pParam))
#define AesGcmAuth_vaes OWNAPI(AesGcmAuth_vaes)
   IPP_OWN_DECL (void, AesGcmAuth_vaes, (Ipp8u* pGhash, const Ipp8u* pSrc, int len, const Ipp8u* pHkey, const void* pParam))
#define AesGcmEnc_vaes OWNAPI(AesGcmEnc_vaes)
   IPP_OWN_DECL (void, AesGcmEnc_vaes, (Ipp8u* pDst, const Ipp8u* pSrc, int len, IppsAES_GCMState* pCtx))
#define AesGcmDec_vaes OWNAPI(AesGcmDec_vaes)
   IPP_OWN_DECL (void, AesGcmDec_vaes, (Ipp8u* pDst, const Ipp8u* pSrc, int len, IppsAES_GCMState* pCtx))
#endif /* _IPP32E>=_IPP32E_K0 */

#define AesGcmPrecompute_table2K OWNAPI(AesGcmPrecompute_table2K)
   IPP_OWN_DECL (void, AesGcmPrecompute_table2K, (Ipp8u* pPrecomputeData, const Ipp8u* pHKey))

/* #define AesGcmMulGcm_table2K OWNAPI(AesGcmMulGcm_table2K) */
/*    IPP_OWN_DECL (void, AesGcmMulGcm_table2K, (Ipp8u* pGhash, const Ipp8u* pHkey, const void* pParam)) */
#define AesGcmMulGcm_table2K_ct OWNAPI(AesGcmMulGcm_table2K_ct)
   IPP_OWN_DECL (void, AesGcmMulGcm_table2K_ct, (Ipp8u* pGhash, const Ipp8u* pHkey, const void* pParam))

/* #define AesGcmAuth_table2K OWNAPI(AesGcmAuth_table2K) */
/*    IPP_OWN_DECL (void, AesGcmAuth_table2K, (Ipp8u* pGhash, const Ipp8u* pSrc, int len, const Ipp8u* pHkey, const void* pParam)) */
#define AesGcmAuth_table2K_ct OWNAPI(AesGcmAuth_table2K_ct)
   IPP_OWN_DECL (void, AesGcmAuth_table2K_ct, (Ipp8u* pGhash, const Ipp8u* pSrc, int len, const Ipp8u* pHkey, const void* pParam))

#define wrpAesGcmEnc_table2K OWNAPI(wrpAesGcmEnc_table2K)
   IPP_OWN_DECL (void, wrpAesGcmEnc_table2K, (Ipp8u* pDst, const Ipp8u* pSrc, int len, IppsAES_GCMState* pCtx))
#define wrpAesGcmDec_table2K OWNAPI(wrpAesGcmDec_table2K)
   IPP_OWN_DECL (void, wrpAesGcmDec_table2K, (Ipp8u* pDst, const Ipp8u* pSrc, int len, IppsAES_GCMState* pCtx))

extern const Ipp16u AesGcmConst_table[256];            /* precomputed reduction table */

static int cpSizeofCtx_AESGCM(void)
{
   int precomp_size;

   #if (_IPP>=_IPP_P8) || (_IPP32E>=_IPP32E_Y8)
   if( IsFeatureEnabled(ippCPUID_AES|ippCPUID_CLMUL) )
      precomp_size = PRECOMP_DATA_SIZE_AES_NI_AESGCM;
   else
   #endif
      precomp_size = PRECOMP_DATA_SIZE_FAST2K;

   /* decrease precomp_size as soon as BLOCK_SIZE bytes already reserved in context */
   precomp_size -= BLOCK_SIZE;

   return (Ipp32s)sizeof(IppsAES_GCMState)
         +precomp_size
         +AESGCM_ALIGNMENT-1;
}

#endif // (_IPP32E < _IPP32E_K0)

#endif /* _CP_AESAUTH_GCM_H*/
