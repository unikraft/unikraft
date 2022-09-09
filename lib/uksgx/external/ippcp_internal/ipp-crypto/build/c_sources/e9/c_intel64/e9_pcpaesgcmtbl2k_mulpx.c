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
//     Cryptography Primitive.
//     Encrypt/Decrypt byte data stream according to Rijndael128 (GCM mode)
// 
//     "fast" stuff
// 
//  Contents:
//      AesGcmMulGcm_table2K()
// 
*/


#include "owndefs.h"
#include "owncp.h"

#include "pcpaesauthgcm.h"
#include "pcptool.h"
#include "pcpmask_ct.h"

#if (_ALG_AES_SAFE_==_ALG_AES_SAFE_COMPACT_SBOX_)
#  include "pcprijtables.h"
#endif

#if(_IPP32E<_IPP32E_K0)

typedef struct{
      Ipp8u b[16];
} AesGcmPrecompute_GF;


#if !((_IPP==_IPP_V8) || (_IPP==_IPP_P8) || \
      (_IPP==_IPP_S8) || (_IPP>=_IPP_G9) || \
      (_IPP32E==_IPP32E_U8) || (_IPP32E==_IPP32E_Y8) || \
      (_IPP32E==_IPP32E_N8) || (_IPP32E>=_IPP32E_E9))
/*
// AesGcmMulGcm_def|safe(Ipp8u* pGhash, const Ipp8u* pHKey)
//
// Ghash = Ghash * HKey mod G()
*/
__INLINE Ipp16u getAesGcmConst_table_ct(int idx)
{
   #define TBL_SLOTS_REP_READ  (Ipp32s)(sizeof(BNU_CHUNK_T)/sizeof(AesGcmConst_table[0]))
   const BNU_CHUNK_T* TblEntry = (BNU_CHUNK_T*)AesGcmConst_table;

   BNU_CHUNK_T idx_sel = (BNU_CHUNK_T)(idx / TBL_SLOTS_REP_READ);  /* selection index */
   BNU_CHUNK_T i;
   BNU_CHUNK_T selection = 0;
   for (i = 0; i<sizeof(AesGcmConst_table) / (sizeof(BNU_CHUNK_T)); i++) {
      BNU_CHUNK_T mask = cpIsEqu_ct(i, idx_sel);
      selection |= TblEntry[i] & mask;
   }
   selection >>= (idx & (TBL_SLOTS_REP_READ-1)) * sizeof(Ipp16u)*8;
   return (Ipp16u)(selection & 0xFFffFF);
   #undef TBL_SLOTS_REP_READ
}

#if 0
void AesGcmMulGcm_table2K(Ipp8u* pGhash, const Ipp8u* pPrecomputeData, const void* pParam)
{
   __ALIGN16 Ipp8u t5[BLOCK_SIZE];
   __ALIGN16 Ipp8u t4[BLOCK_SIZE];
   __ALIGN16 Ipp8u t3[BLOCK_SIZE];
   __ALIGN16 Ipp8u t2[BLOCK_SIZE];

   int nw;
   Ipp32u a;

   IPP_UNREFERENCED_PARAMETER(pParam);

   #if 0
   XorBlock16(t5, t5, t5);
   XorBlock16(t4, t4, t4);
   XorBlock16(t3, t3, t3);
   XorBlock16(t2, t2, t2);
   #endif
   PadBlock(0, t5, sizeof(t5));
   PadBlock(0, t4, sizeof(t4));
   PadBlock(0, t3, sizeof(t3));
   PadBlock(0, t2, sizeof(t2));

   for(nw=0; nw<4; nw++) {
      Ipp32u hashdw = ((Ipp32u*)pGhash)[nw];

      a = hashdw & 0xf0f0f0f0;
      XorBlock16(t5, pPrecomputeData+1024+EBYTE(a,1)+256*nw, t5);
      XorBlock16(t4, pPrecomputeData+1024+EBYTE(a,0)+256*nw, t4);
      XorBlock16(t3, pPrecomputeData+1024+EBYTE(a,3)+256*nw, t3);
      XorBlock16(t2, pPrecomputeData+1024+EBYTE(a,2)+256*nw, t2);

      a = (hashdw<<4) & 0xf0f0f0f0;
      XorBlock16(t5, pPrecomputeData+EBYTE(a,1)+256*nw, t5);
      XorBlock16(t4, pPrecomputeData+EBYTE(a,0)+256*nw, t4);
      XorBlock16(t3, pPrecomputeData+EBYTE(a,3)+256*nw, t3);
      XorBlock16(t2, pPrecomputeData+EBYTE(a,2)+256*nw, t2);
   }

   XorBlock(t2+1, t3, t2+1, BLOCK_SIZE-1);
   XorBlock(t5+1, t2, t5+1, BLOCK_SIZE-1);
   XorBlock(t4+1, t5, t4+1, BLOCK_SIZE-1);

   nw = t3[BLOCK_SIZE-1];
   //a = (Ipp32u)AesGcmConst_table[nw];
   a = (Ipp32u)getAesGcmConst_table_ct(nw);
   a <<= 8;
   nw = t2[BLOCK_SIZE-1];
   //a ^= (Ipp32u)AesGcmConst_table[nw];
   a ^= (Ipp32u)getAesGcmConst_table_ct(nw);
   a <<= 8;
   nw = t5[BLOCK_SIZE-1];
   //a ^= (Ipp32u)AesGcmConst_table[nw];
   a ^= (Ipp32u)getAesGcmConst_table_ct(nw);

   XorBlock(t4, &a, t4, sizeof(Ipp32u));
   CopyBlock16(t4, pGhash);
}
#endif

/*
// CTE version of AesGcmMulGcm_table2K()
*/
#if (_IPP_ARCH ==_IPP_ARCH_EM64T)
__INLINE void MaskedXorBlock16(const Ipp8u* pSrc1, const Ipp8u* pSrc2, Ipp8u* pDst, Ipp64u src2mask)
{
   ((Ipp64u*)pDst)[0] = ((Ipp64u*)pSrc1)[0] ^ (((Ipp64u*)pSrc2)[0] & src2mask);
   ((Ipp64u*)pDst)[1] = ((Ipp64u*)pSrc1)[1] ^ (((Ipp64u*)pSrc2)[1] & src2mask);
}
#else /* IPP_ARCH == IPP_ARCH_IA32 */
__INLINE void MaskedXorBlock16(const Ipp8u* pSrc1, const Ipp8u* pSrc2, Ipp8u* pDst, Ipp32u src2mask)
{
   ((Ipp32u*)pDst)[0] = ((Ipp32u*)pSrc1)[0] ^ (((Ipp32u*)pSrc2)[0] & src2mask);
   ((Ipp32u*)pDst)[1] = ((Ipp32u*)pSrc1)[1] ^ (((Ipp32u*)pSrc2)[1] & src2mask);
   ((Ipp32u*)pDst)[2] = ((Ipp32u*)pSrc1)[2] ^ (((Ipp32u*)pSrc2)[2] & src2mask);
   ((Ipp32u*)pDst)[3] = ((Ipp32u*)pSrc1)[3] ^ (((Ipp32u*)pSrc2)[3] & src2mask);
}
#endif

IPP_OWN_DEFN (void, AesGcmMulGcm_table2K_ct, (Ipp8u* pGhash, const Ipp8u* pPrecomputeData, const void* pParam))
{
   __ALIGN16 Ipp8u t5[BLOCK_SIZE];
   __ALIGN16 Ipp8u t4[BLOCK_SIZE];
   __ALIGN16 Ipp8u t3[BLOCK_SIZE];
   __ALIGN16 Ipp8u t2[BLOCK_SIZE];

   int nw;

   IPP_UNREFERENCED_PARAMETER(pParam);

#if 0
   XorBlock16(t5, t5, t5);
   XorBlock16(t4, t4, t4);
   XorBlock16(t3, t3, t3);
   XorBlock16(t2, t2, t2);
#endif
   PadBlock(0, t5, sizeof(t5));
   PadBlock(0, t4, sizeof(t4));
   PadBlock(0, t3, sizeof(t3));
   PadBlock(0, t2, sizeof(t2));

   for(nw=0; nw<4; nw++) {
      Ipp32u hashdw = ((Ipp32u*)pGhash)[nw];
      Ipp32u a = hashdw & 0xf0f0f0f0;

      Ipp32u a0 = EBYTE(a,0);
      Ipp32u a1 = EBYTE(a,1);
      Ipp32u a2 = EBYTE(a,2);
      Ipp32u a3 = EBYTE(a,3);

      int idx;
      for(idx=0; idx<256; idx+=16) {
         BNU_CHUNK_T mask0 = cpIsEqu_ct(a0, (BNU_CHUNK_T)idx);
         BNU_CHUNK_T mask1 = cpIsEqu_ct(a1, (BNU_CHUNK_T)idx);
         BNU_CHUNK_T mask2 = cpIsEqu_ct(a2, (BNU_CHUNK_T)idx);
         BNU_CHUNK_T mask3 = cpIsEqu_ct(a3, (BNU_CHUNK_T)idx);
         MaskedXorBlock16(t5, pPrecomputeData+1024 +256*nw +idx, t5, mask1);
         MaskedXorBlock16(t4, pPrecomputeData+1024 +256*nw +idx, t4, mask0);
         MaskedXorBlock16(t3, pPrecomputeData+1024 +256*nw +idx, t3, mask3);
         MaskedXorBlock16(t2, pPrecomputeData+1024 +256*nw +idx, t2, mask2);
      }

      a = (hashdw << 4) & 0xf0f0f0f0;
      a0 = EBYTE(a, 0);
      a1 = EBYTE(a, 1);
      a2 = EBYTE(a, 2);
      a3 = EBYTE(a, 3);
      for (idx = 0; idx < 256; idx += 16) {
         BNU_CHUNK_T mask0 = cpIsEqu_ct(a0, (BNU_CHUNK_T)idx);
         BNU_CHUNK_T mask1 = cpIsEqu_ct(a1, (BNU_CHUNK_T)idx);
         BNU_CHUNK_T mask2 = cpIsEqu_ct(a2, (BNU_CHUNK_T)idx);
         BNU_CHUNK_T mask3 = cpIsEqu_ct(a3, (BNU_CHUNK_T)idx);
         MaskedXorBlock16(t5, pPrecomputeData +256*nw +idx, t5, mask1);
         MaskedXorBlock16(t4, pPrecomputeData +256*nw +idx, t4, mask0);
         MaskedXorBlock16(t3, pPrecomputeData +256*nw +idx, t3, mask3);
         MaskedXorBlock16(t2, pPrecomputeData +256*nw +idx, t2, mask2);
      }
   }

   XorBlock(t2 + 1, t3, t2 + 1, BLOCK_SIZE - 1);
   XorBlock(t5 + 1, t2, t5 + 1, BLOCK_SIZE - 1);
   XorBlock(t4 + 1, t5, t4 + 1, BLOCK_SIZE - 1);

   nw = t3[BLOCK_SIZE - 1];
   {
      //a = (Ipp32u)AesGcmConst_table[nw];
      Ipp32u a = (Ipp32u)getAesGcmConst_table_ct(nw);
      a <<= 8;
      nw = t2[BLOCK_SIZE - 1];
      //a ^= (Ipp32u)AesGcmConst_table[nw];
      a ^= (Ipp32u)getAesGcmConst_table_ct(nw);
      a <<= 8;
      nw = t5[BLOCK_SIZE - 1];
      //a ^= (Ipp32u)AesGcmConst_table[nw];
      a ^= (Ipp32u)getAesGcmConst_table_ct(nw);

      XorBlock(t4, &a, t4, sizeof(Ipp32u));
      CopyBlock16(t4, pGhash);
   }
}

#endif

#if ((_IPP>=_IPP_V8) || (_IPP32E>=_IPP32E_N8))

__INLINE Ipp16u getAesGcmConst_table_ct(int idx)
{
   /* init current indexes */
   __ALIGN16 Ipp16u idx_start[] = { 0,1,2,3,4,5,6,7 };
   __m128i idx_curr = _mm_load_si128((__m128i*)idx_start);
   /* indexes step */
   __m128i idx_step = _mm_set1_epi16(sizeof(__m128i) / sizeof(AesGcmConst_table[0]));
   /* broadcast idx */
   __m128i idx_bcst = _mm_set1_epi16((Ipp16s)idx);

   /* init accumulator */
   __m128i acc = _mm_setzero_si128();

   int i;
   for (i = 0; i < sizeof(AesGcmConst_table); i += sizeof(__m128i)) {
      /* read 16 entries of AesGcmConst_table[] */
      __m128i tbl = _mm_load_si128((__m128i*)((Ipp8u*)AesGcmConst_table + i));
      /* set mask if idx==idx_curr[] */
      __m128i mask = _mm_cmpeq_epi16(idx_bcst, idx_curr);
      mask = _mm_and_si128(mask, tbl);
      /* accumulates masked */
      acc = _mm_or_si128(acc, mask);
      /* ad advance idx_curr[] indexes */
      idx_curr = _mm_add_epi16(idx_curr, idx_step);
   }

   /* shift accumulator to get AesGcmConst_table[idx] in low word */
   acc = _mm_or_si128(acc, _mm_srli_si128(acc, sizeof(__m128i) / 2));   /* pack result into dword */
   acc = _mm_or_si128(acc, _mm_srli_si128(acc, sizeof(__m128i) / 4));
   acc = _mm_or_si128(acc, _mm_srli_si128(acc, sizeof(__m128i) / 8));
   i = _mm_cvtsi128_si32(acc);

   return (Ipp16u)i;
}

IPP_OWN_DEFN (void, AesGcmMulGcm_table2K_ct, (Ipp8u* pHash, const Ipp8u* pPrecomputedData, const void* pParam))
{
   __m128i t5 = _mm_setzero_si128();
   __m128i t4 = _mm_setzero_si128();
   __m128i t3 = _mm_setzero_si128();
   __m128i t2 = _mm_setzero_si128();

   IPP_UNREFERENCED_PARAMETER(pParam);

   {
      int nw;
      for (nw = 0; nw < 4; nw++) {
         Ipp32u hashdw = ((Ipp32u*)pHash)[nw];

         Ipp32u a = hashdw & 0xf0f0f0f0;
         Ipp32u a0 = EBYTE(a, 0);
         Ipp32u a1 = EBYTE(a, 1);
         Ipp32u a2 = EBYTE(a, 2);
         Ipp32u a3 = EBYTE(a, 3);
         int idx;
         for (idx = 0; idx < 256; idx += 16) {
            __m128i mask0 = _mm_set1_epi32((Ipp32s)cpIsEqu_ct(a0, (BNU_CHUNK_T)idx));
            __m128i mask1 = _mm_set1_epi32((Ipp32s)cpIsEqu_ct(a1, (BNU_CHUNK_T)idx));
            __m128i mask2 = _mm_set1_epi32((Ipp32s)cpIsEqu_ct(a2, (BNU_CHUNK_T)idx));
            __m128i mask3 = _mm_set1_epi32((Ipp32s)cpIsEqu_ct(a3, (BNU_CHUNK_T)idx));
            t5 = _mm_xor_si128(t5, _mm_and_si128(mask1, _mm_load_si128((__m128i*)(pPrecomputedData + 1024 + 256 * nw + idx))));
            t4 = _mm_xor_si128(t4, _mm_and_si128(mask0, _mm_load_si128((__m128i*)(pPrecomputedData + 1024 + 256 * nw + idx))));
            t3 = _mm_xor_si128(t3, _mm_and_si128(mask3, _mm_load_si128((__m128i*)(pPrecomputedData + 1024 + 256 * nw + idx))));
            t2 = _mm_xor_si128(t2, _mm_and_si128(mask2, _mm_load_si128((__m128i*)(pPrecomputedData + 1024 + 256 * nw + idx))));
         }

         a = (hashdw << 4) & 0xf0f0f0f0;
         a0 = EBYTE(a, 0);
         a1 = EBYTE(a, 1);
         a2 = EBYTE(a, 2);
         a3 = EBYTE(a, 3);
         for (idx = 0; idx < 256; idx += 16) {
            __m128i mask0 = _mm_set1_epi32((Ipp32s)cpIsEqu_ct(a0, (BNU_CHUNK_T)idx));
            __m128i mask1 = _mm_set1_epi32((Ipp32s)cpIsEqu_ct(a1, (BNU_CHUNK_T)idx));
            __m128i mask2 = _mm_set1_epi32((Ipp32s)cpIsEqu_ct(a2, (BNU_CHUNK_T)idx));
            __m128i mask3 = _mm_set1_epi32((Ipp32s)cpIsEqu_ct(a3, (BNU_CHUNK_T)idx));
            t5 = _mm_xor_si128(t5, _mm_and_si128(mask1, _mm_load_si128((__m128i*)(pPrecomputedData + 256 * nw + idx))));
            t4 = _mm_xor_si128(t4, _mm_and_si128(mask0, _mm_load_si128((__m128i*)(pPrecomputedData + 256 * nw + idx))));
            t3 = _mm_xor_si128(t3, _mm_and_si128(mask3, _mm_load_si128((__m128i*)(pPrecomputedData + 256 * nw + idx))));
            t2 = _mm_xor_si128(t2, _mm_and_si128(mask2, _mm_load_si128((__m128i*)(pPrecomputedData + 256 * nw + idx))));
         }
      }

      {
         Ipp32u a;
         t2 = _mm_xor_si128(t2, _mm_slli_si128(t3, 1));
         t5 = _mm_xor_si128(t5, _mm_slli_si128(t2, 1));
         t4 = _mm_xor_si128(t4, _mm_slli_si128(t5, 1));

         nw = _mm_cvtsi128_si32(_mm_srli_si128(t3, 15));
         a = (Ipp32u)getAesGcmConst_table_ct(nw);
         a <<= 8;

         nw = _mm_cvtsi128_si32(_mm_srli_si128(t2, 15));
         a ^= (Ipp32u)getAesGcmConst_table_ct(nw);
         a <<= 8;

         nw = _mm_cvtsi128_si32(_mm_srli_si128(t5, 15));
         a ^= (Ipp32u)getAesGcmConst_table_ct(nw);

         t2 = _mm_cvtsi32_si128((Ipp32s)a);
         t4 = _mm_xor_si128(t4, t2);
         _mm_storeu_si128((__m128i*)pHash, t4);
      }
   }
}
#endif

#endif
