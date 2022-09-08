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
//     Internal Definitions and
//     Internal SMS4 Function Prototypes
// 
// 
*/

#if !defined(_PCP_SMS4_H)
#define _PCP_SMS4_H

#include "owncp.h"

/* SMS4 round keys number */
#define SMS4_ROUND_KEYS_NUM (32)

#if SMS4_ROUND_KEYS_NUM != 32
   #error SMS4_ROUND_KEYS_NUM must be equal 32
#endif

struct _cpSMS4 {
   Ipp32u      idCtx;                              /* SMS4 spec identifier */
   Ipp32u      enc_rkeys[SMS4_ROUND_KEYS_NUM];     /* enc round keys       */
   Ipp32u      dec_rkeys[SMS4_ROUND_KEYS_NUM];     /* dec round keys       */
};

/*
// access macros
*/
#define SMS4_SET_ID(ctx)   ((ctx)->idCtx = (Ipp32u)idCtxSMS4 ^ (Ipp32u)IPP_UINT_PTR(ctx))
#define SMS4_RK(ctx)       ((ctx)->enc_rkeys)
#define SMS4_ERK(ctx)      ((ctx)->enc_rkeys)
#define SMS4_DRK(ctx)      ((ctx)->dec_rkeys)

/* SMS4 data block size (bytes) */
#define MBS_SMS4  (16)

#if MBS_SMS4 != 16
   #error MBS_SMS4 must be equal 16
#endif

/* valid SMS4 context ID */
#define VALID_SMS4_ID(ctx)   ((((ctx)->idCtx) ^ (Ipp32u)IPP_UINT_PTR((ctx))) == (Ipp32u)idCtxSMS4)

/* alignment of AES context */
#define SMS4_ALIGNMENT   (4)

/* size of SMS4 context */
__INLINE int cpSizeofCtx_SMS4(void)
{
   return sizeof(IppsSMS4Spec);
}

/* SMS4 constants */
extern const __ALIGN64 Ipp8u  SMS4_Sbox[16*16];
extern const Ipp32u SMS4_FK[4];
extern const Ipp32u SMS4_CK[32];

//////////////////////////////////////////////////////////////////////////////////////////////////////
/* S-box substitution (endian dependent!) */

#include "pcpbnuimpl.h"
#define SELECTION_BITS  ((sizeof(BNU_CHUNK_T)/sizeof(Ipp8u)) -1)

#if defined(__INTEL_COMPILER)
__INLINE Ipp8u getSboxValue(Ipp8u x)
{
   BNU_CHUNK_T selection = 0;
   const BNU_CHUNK_T* SboxEntry = (BNU_CHUNK_T*)SMS4_Sbox;

   BNU_CHUNK_T i_sel = x/sizeof(BNU_CHUNK_T);  /* selection index */
   BNU_CHUNK_T i;
   for(i=0; i<sizeof(SMS4_Sbox)/sizeof(BNU_CHUNK_T); i++) {
      BNU_CHUNK_T mask = (i==i_sel)? (BNU_CHUNK_T)(-1) : 0;  
      /* ipp and IPP build specific avoid jump instruction here */
      /* Intel(R) C++ Compiler compile this code with movcc instruction */
      selection |= SboxEntry[i] & mask;
   }
   selection >>= (x & SELECTION_BITS)*8;
   return (Ipp8u)(selection & 0xFF);
}
#else
#include "pcpmask_ct.h"
__INLINE Ipp8u getSboxValue(Ipp8u x)
{
   BNU_CHUNK_T selection = 0;
   const BNU_CHUNK_T* SboxEntry = (BNU_CHUNK_T*)SMS4_Sbox;

   Ipp32u _x = x/sizeof(BNU_CHUNK_T);
   Ipp32u i;
   for(i=0; i<sizeof(SMS4_Sbox)/(sizeof(BNU_CHUNK_T)); i++) {
      BNS_CHUNK_T mask = (BNS_CHUNK_T)cpIsEqu_ct(_x, i);
      selection |= SboxEntry[i] & (BNU_CHUNK_T)mask;
   }
   selection >>= (x & SELECTION_BITS)*8;
   return (Ipp8u)(selection & 0xFF);
}
#endif

__INLINE Ipp32u cpSboxT_SMS4(Ipp32u x)
{
   Ipp32u y = getSboxValue(x & 0xFF);
   y |= (Ipp32u)(getSboxValue((x>> 8) & 0xFF) <<8);
   y |= (Ipp32u)(getSboxValue((x>>16) & 0xFF) <<16);
   y |= (Ipp32u)(getSboxValue((x>>24) & 0xFF) <<24);
   return y;
}

/* key expansion transformation:
   - linear Linear
   - mixer Mix (permutation T in the SMS4 standart phraseology)
*/
__INLINE Ipp32u cpExpKeyLinear_SMS4(Ipp32u x)
{
   return x^ROL32(x,13)^ROL32(x,23);
}

__INLINE Ipp32u cpExpKeyMix_SMS4(Ipp32u x)
{
   return cpExpKeyLinear_SMS4( cpSboxT_SMS4(x) );
}

/* cipher transformations:
   - linear Linear
   - mixer Mix (permutation T in the SMS4 standart phraseology)
*/
__INLINE Ipp32u cpCipherLinear_SMS4(Ipp32u x)
{
   return x^ROL32(x,2)^ROL32(x,10)^ROL32(x,18)^ROL32(x,24);
}

__INLINE Ipp32u cpCipherMix_SMS4(Ipp32u x)
{
   return cpCipherLinear_SMS4( cpSboxT_SMS4(x) );
}
//////////////////////////////////////////////////////////////////////////////////////////////


#define cpSMS4_Cipher OWNAPI(cpSMS4_Cipher)
   IPP_OWN_DECL (void, cpSMS4_Cipher, (Ipp8u* otxt, const Ipp8u* itxt, const Ipp32u* pRoundKeys))

#if (_IPP>=_IPP_P8) || (_IPP32E>=_IPP32E_Y8)
#define cpSMS4_SetRoundKeys_aesni OWNAPI(cpSMS4_SetRoundKeys_aesni)
   IPP_OWN_DECL (void, cpSMS4_SetRoundKeys_aesni, (Ipp32u* pRounKey, const Ipp8u* pSecretKey))

#define cpSMS4_ECB_aesni_x1 OWNAPI(cpSMS4_ECB_aesni_x1)
   IPP_OWN_DECL (void, cpSMS4_ECB_aesni_x1, (Ipp8u* pOut, const Ipp8u* pInp, const Ipp32u* pRKey))
#define cpSMS4_ECB_aesni OWNAPI(cpSMS4_ECB_aesni)
   IPP_OWN_DECL (int, cpSMS4_ECB_aesni, (Ipp8u* pDst, const Ipp8u* pSrc, int nLen, const Ipp32u* pRKey))
#define cpSMS4_CBC_dec_aesni OWNAPI(cpSMS4_CBC_dec_aesni)
   IPP_OWN_DECL (int, cpSMS4_CBC_dec_aesni, (Ipp8u* pDst, const Ipp8u* pSrc, int nLen, const Ipp32u* pRKey, Ipp8u* pIV))
#define cpSMS4_CTR_aesni OWNAPI(cpSMS4_CTR_aesni)
   IPP_OWN_DECL (int, cpSMS4_CTR_aesni, (Ipp8u* pOut, const Ipp8u* pInp, int len, const Ipp32u* pRKey, const Ipp8u* pCtrMask, Ipp8u* pCtr))

#if (_IPP>=_IPP_H9) || (_IPP32E>=_IPP32E_L9)
#define cpSMS4_ECB_aesni_x12 OWNAPI(cpSMS4_ECB_aesni_x12)
   IPP_OWN_DECL (int, cpSMS4_ECB_aesni_x12, (Ipp8u* pOut, const Ipp8u* pInp, int len, const Ipp32u* pRKey))
#define cpSMS4_CBC_dec_aesni_x12 OWNAPI(cpSMS4_CBC_dec_aesni_x12)
   IPP_OWN_DECL (int, cpSMS4_CBC_dec_aesni_x12, (Ipp8u* pOut, const Ipp8u* pInp, int len, const Ipp32u* pRKey, Ipp8u* pIV))
#define cpSMS4_CTR_aesni_x4 OWNAPI(cpSMS4_CTR_aesni_x4)
   IPP_OWN_DECL (int, cpSMS4_CTR_aesni_x4, (Ipp8u* pOut, const Ipp8u* pInp, int len, const Ipp32u* pRKey, const Ipp8u* pCtrMask, Ipp8u* pCtr))

#if (_IPP32E>=_IPP32E_K1)
#if defined (__INTEL_COMPILER) || !defined (_MSC_VER) || (_MSC_VER >= 1920)

#define cpSMS4_ECB_gfni_x1 OWNAPI(cpSMS4_ECB_gfni_x1)
   IPP_OWN_DECL (void, cpSMS4_ECB_gfni_x1, (Ipp8u* pOut, const Ipp8u* pInp, const Ipp32u* pRKey))
#define cpSMS4_ECB_gfni512 OWNAPI(cpSMS4_ECB_gfni512)
   IPP_OWN_DECL (int, cpSMS4_ECB_gfni512, (Ipp8u* pOut, const Ipp8u* pInp, int len, const Ipp32u* pRKey))
#define cpSMS4_CBC_dec_gfni512 OWNAPI(cpSMS4_CBC_dec_gfni512)
   IPP_OWN_DECL (int, cpSMS4_CBC_dec_gfni512, (Ipp8u* pOut, const Ipp8u* pInp, int len, const Ipp32u* pRKey, Ipp8u* pIV))
#define cpSMS4_CTR_gfni512 OWNAPI(cpSMS4_CTR_gfni512)
   IPP_OWN_DECL (int, cpSMS4_CTR_gfni512, (Ipp8u* pOut, const Ipp8u* pInp, int len, const Ipp32u* pRKey, const Ipp8u* pCtrMask, Ipp8u* pCtr))
#define cpSMS4_CFB_dec_gfni512 OWNAPI(cpSMS4_CFB_dec_gfni512)
   IPP_OWN_DECL (void, cpSMS4_CFB_dec_gfni512, (Ipp8u* pOut, const Ipp8u* pInp, int len, int cfbBlkSize, const Ipp32u* pRKey, Ipp8u* pIV))

#endif /* #if defined (__INTEL_COMPILER) || !defined (_MSC_VER) || (_MSC_VER >= 1920) */
#endif /* (_IPP32E>=_IPP32E_K1) */

#endif /* (_IPP>=_IPP_H9) || (_IPP32E>=_IPP32E_L9) */

#endif

#define cpProcessSMS4_ctr OWNAPI(cpProcessSMS4_ctr)
   IPP_OWN_DECL (IppStatus, cpProcessSMS4_ctr, (const Ipp8u* pSrc, Ipp8u* pDst, int dataLen, const IppsSMS4Spec* pCtx, Ipp8u* pCtrValue, int ctrNumBitSize))

#endif /* _PCP_SMS4_H */
