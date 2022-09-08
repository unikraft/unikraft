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
//  Purpose:
//     Intel(R) Integrated Performance Primitives. Cryptography Primitives.
//     Internal Miscellaneous BNU Definitions & Function Prototypes
//
//
*/

#if !defined(_PCP_BNUMISC_H)
#define _PCP_BNUMISC_H

#include "pcpbnuimpl.h"
#include "pcpmask_ct.h"


/* bit operations */
#define BITSIZE_BNU(p,ns)  ((ns)*BNU_CHUNK_BITS-cpNLZ_BNU((p)[(ns)-1]))
#define BIT_BNU(bnu, ns,nbit) ((((nbit)>>BNU_CHUNK_LOG2) < (ns))? ((((bnu))[(nbit)>>BNU_CHUNK_LOG2] >>((nbit)&(BNU_CHUNK_BITS))) &1) : 0)

#define TST_BIT(bnu, nbit)    (((Ipp8u*)(bnu))[(nbit)/8] &  ((1<<((nbit)%8)) &0xFF))
#define SET_BIT(bnu, nbit)    (((Ipp8u*)(bnu))[(nbit)/8] |= ((1<<((nbit)%8)) &0xFF))
#define CLR_BIT(bnu, nbit)    (((Ipp8u*)(bnu))[(nbit)/8] &=~((1<<((nbit)%8)) &0xFF))

/* convert bitsize nbits into  the number of BNU_CHUNK_T */
#define BITS_BNU_CHUNK(nbits) (((nbits)+BNU_CHUNK_BITS-1)/BNU_CHUNK_BITS)

/* mask for top BNU_CHUNK_T */
#define MASK_BNU_CHUNK(nbits) ((BNU_CHUNK_T)(-1) >>((BNU_CHUNK_BITS- ((nbits)&(BNU_CHUNK_BITS-1))) &(BNU_CHUNK_BITS-1)))

/* copy BNU content */
#define COPY_BNU(dst, src, len) \
{ \
   cpSize __idx; \
   for(__idx=0; __idx<(len); __idx++) (dst)[__idx] = (src)[__idx]; \
}

/* expand by zeros */
#define ZEXPAND_BNU(srcdst,srcLen, dstLen) \
{ \
   cpSize __idx; \
   for(__idx=(srcLen); __idx<(dstLen); __idx++) (srcdst)[__idx] = 0; \
}

/* copy and expand by zeros */
#define ZEXPAND_COPY_BNU(dst,dstLen, src,srcLen) \
{ \
   cpSize __idx; \
   for(__idx=0; __idx<(srcLen); __idx++) (dst)[__idx] = (src)[__idx]; \
   for(; __idx<(dstLen); __idx++)    (dst)[__idx] = 0; \
}


/* copy and set */
__INLINE void cpCpy_BNU(BNU_CHUNK_T* pDst, const BNU_CHUNK_T* pSrc, cpSize ns)
{  COPY_BNU(pDst, pSrc, ns); }

__INLINE void cpSet_BNU(BNU_CHUNK_T* pDst, cpSize ns, BNU_CHUNK_T val)
{
   ZEXPAND_BNU(pDst, 0, ns);
   pDst[0] = val;
}

/* fix up */

/*   Name: cpFix_BNU
//
// Purpose: fix up BigNums.
//
// Returns:
//    fixed nsA
//
// Parameters:
//    pA       BigNum ctx
//    nsA      Size of pA
//
*/
__INLINE int cpFix_BNU(const BNU_CHUNK_T* pA, int nsA)
{
   BNU_CHUNK_T zscan = (BNU_CHUNK_T)(-1);
   int outLen = nsA;
   for(; nsA>0; nsA--) {
      zscan &= cpIsZero_ct(pA[nsA-1]);
      outLen -= 1 & zscan;
   }
   return (int)((1 & zscan) | ((BNU_CHUNK_T)outLen & ~zscan)); // change to scanz
}

#define FIX_BNU(src,srcLen) ((srcLen) = cpFix_BNU((src), (srcLen)))

/*   Name: cpCmp_BNU
//
// Purpose: Compare two BigNums.
//
// Returns:
//    negative, if A < B
//           0, if A = B
//    positive, if A > B
//
// Parameters:
//    pA       BigNum ctx
//    nsA      Size of pA
//    pB       BigNum ctx
//    nsB      Size of pB
//
*/
#if 0
__INLINE int cpCmp_BNU(const BNU_CHUNK_T* pA, cpSize nsA, const BNU_CHUNK_T* pB, cpSize nsB)
{
   if(nsA!=nsB)
      return nsA>nsB? 1 : -1;
   else {
      BNU_CHUNK_T idx = 0;
      for(; nsA>0; nsA--)
        idx |= ~cpIsEqu_ct(pA[nsA-1], pB[nsA-1]) & cpIsZero_ct(idx) & (BNU_CHUNK_T)(nsA-1);
      return pA[idx] < pB[idx] ? -1 : (pA[idx] > pB[idx] ? 1 : 0);
   }
}
#endif

__INLINE int cpCmp_BNU0(const BNU_CHUNK_T* a, const BNU_CHUNK_T* b, int len)
{
   const Ipp32u* a32 = (const Ipp32u*)a;
   const Ipp32u* b32 = (const Ipp32u*)b;
   len *= (sizeof(BNU_CHUNK_T))/sizeof(Ipp32u);

   // borrow, difference |=  (a[]-b[])
   BNU_CHUNK_T borrow = 0;
   BNU_CHUNK_T difference = 0;
   for(int n=0; n<len; n++) {
      Ipp64u d = (Ipp64u)(a32[n]) - (Ipp64u)(b32[n]) - (Ipp64u)borrow;
      difference |= (BNU_CHUNK_T)(d & 0xFFFFFFFF);
      borrow = (BNU_CHUNK_T)(d>>63);
   }

   int resb = (int)( cpIsEqu_ct(borrow, 1) );
   int resd = (int)(~cpIsZero_ct(difference) ) &1;
   return (int)(resb|resd);
}

__INLINE int cpCmp_BNU(const BNU_CHUNK_T* a, int aLen, const BNU_CHUNK_T* b, int bLen)
{
   BNU_CHUNK_T aLen_eq_bLen = cpIsZero_ct((BNU_CHUNK_T)(aLen-bLen));    // FFFF/0000 if (aLen=bLen) / (aLen!=bLen)
   BNU_CHUNK_T aLen_gt_bLen = cpIsMsb_ct((BNU_CHUNK_T)(bLen-aLen)) & 1; // 1/0       if (aLen>bLen) / (aLen<bLen)
   BNU_CHUNK_T aLen_lt_bLen = cpIsMsb_ct((BNU_CHUNK_T)(aLen-bLen));     // FFFF/0000 if (aLen<bLen) / (aLen>bLen)


   int len = (int)(((Ipp32u)aLen & aLen_lt_bLen) | ((Ipp32u)bLen & ~aLen_lt_bLen));
   int cmp_res = cpCmp_BNU0(a, b, len);

   return (int)( aLen_gt_bLen | (aLen_eq_bLen & (Ipp32u)cmp_res) | aLen_lt_bLen );
}

/*   Name: cpEqu_BNU_CHUNK
//
// Purpose: Compare two BNU_CHUNKs.
//
// Returns:
//    positive, if A  = b
//    0       , if A != b
//
// Parameters:
//    pA       BigNum ctx
//    nsA      Size of pA
//    b        BNU_CHUNK_T to compare
//
*/
__INLINE int cpEqu_BNU_CHUNK(const BNU_CHUNK_T* pA, cpSize nsA, BNU_CHUNK_T b)
{
   BNU_CHUNK_T res = pA[0] ^ b;
   int n;
   for(n=1; n<nsA; n++)
      res |= pA[n];
   return cpIsZero_ct(res) & 1;
}

/*
// test
//
// returns
//     0, if A = 0
//    >0, if A > 0
//    <0, looks like impossible (or error) case
*/
__INLINE int cpTst_BNU(const BNU_CHUNK_T* pA, int nsA)
{
   for(; (nsA>0) && (0==pA[nsA-1]); nsA--) ;
   return nsA;
}

/* number of leading/trailing zeros */
#if !((_IPP >= _IPP_H9) || (_IPP32E >= _IPP32E_L9))
#define cpNLZ_BNU OWNAPI(cpNLZ_BNU)
   IPP_OWN_DECL (cpSize, cpNLZ_BNU, (BNU_CHUNK_T x))
#else
   __INLINE cpSize cpNLZ_BNU(BNU_CHUNK_T x)
   {
      #if (BNU_CHUNK_BITS == BNU_CHUNK_64BIT)
         return (cpSize)_lzcnt_u64(x);
      #else
         return (cpSize)_lzcnt_u32(x);
      #endif
   }
#endif

#define cpNTZ_BNU OWNAPI(cpNTZ_BNU)
   IPP_OWN_DECL (cpSize, cpNTZ_BNU, (BNU_CHUNK_T x))

/* logical shift left/right */
#define cpLSR_BNU OWNAPI(cpLSR_BNU)
   IPP_OWN_DECL (int, cpLSR_BNU, (BNU_CHUNK_T* pR, const BNU_CHUNK_T* pA, cpSize nsA, cpSize nBits))

/* most significant BNU bit */
#define cpMSBit_BNU OWNAPI(cpMSBit_BNU)
   IPP_OWN_DECL (int, cpMSBit_BNU, (const BNU_CHUNK_T* pA, cpSize nsA))

/* BNU <-> hex-string conversion */
#define cpToOctStr_BNU OWNAPI(cpToOctStr_BNU)
   IPP_OWN_DECL (int, cpToOctStr_BNU, (Ipp8u* pStr, cpSize strLen, const BNU_CHUNK_T* pA, cpSize nsA))
#define cpFromOctStr_BNU OWNAPI(cpFromOctStr_BNU)
   IPP_OWN_DECL (int, cpFromOctStr_BNU, (BNU_CHUNK_T* pA, const Ipp8u* pStr, cpSize strLen))

#endif /* _PCP_BNUMISC_H */
