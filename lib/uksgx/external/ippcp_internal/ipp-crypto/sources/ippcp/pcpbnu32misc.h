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
//     Internal Miscellaneous BNU 32 bit Definitions & Function Prototypes
//
//
*/

#if !defined(_CP_BNU32_MISC_H)
#define _CP_BNU32_MISC_H


/* bit operations */
#define BITSIZE_BNU32(p,ns)  ((ns)*BNU_CHUNK_32BIT-cpNLZ_BNU32((p)[(ns)-1]))

/* number of leading/trailing zeros */
#if (_IPP < _IPP_H9)
#define cpNLZ_BNU32 OWNAPI(cpNLZ_BNU32)
   IPP_OWN_DECL (cpSize, cpNLZ_BNU32, (Ipp32u x))
#else
   __INLINE cpSize cpNLZ_BNU32(Ipp32u x)
   {
      return (cpSize)_lzcnt_u32(x);
   }
#endif

/*   Name: cpFix_BNU32
//
// Purpose: fix up BNU.
//
// Returns:
//    fixed nsA
//
// Parameters:
//    pA       BNU ptr
//    nsA      size of BNU
//
*/
__INLINE int cpFix_BNU32(const Ipp32u* pA, int nsA)
{
   Ipp32u zscan = (Ipp32u)(-1);
   int outLen = nsA;
   for(; nsA>0; nsA--) {
      zscan &= (Ipp32u)cpIsZero_ct((BNU_CHUNK_T)pA[nsA-1]);
      outLen -= 1 & zscan;
   }
   return (int)((1 & zscan) | ((BNU_CHUNK_T)outLen & ~zscan)); // change to scanz
}

#define FIX_BNU32(src,srcLen) ((srcLen) = cpFix_BNU32((src), (srcLen)))

/* most significant BNU bit */
#if 0
__INLINE int cpMSBit_BNU32(const Ipp32u* pA, cpSize nsA)
{
   FIX_BNU(pA, nsA);
   return nsA*BITSIZE(Ipp32u) - cpNLZ_BNU32(pA[nsA-1]) -1;
}
#endif

#if 0
__INLINE int cpCmp_BNU32(const Ipp32u* pA, cpSize nsA, const Ipp32u* pB, cpSize nsB)
{
   if(nsA!=nsB)
      return nsA>nsB? 1 : -1;
   else {
      BNU_CHUNK_T idx = 0;
      for(; nsA>0; nsA--)
        idx |= ~cpIsEqu_ct(pA[nsA-1], pB[nsA-1]) & cpIsZero_ct(idx) & (nsA-1);
      return pA[idx] < pB[idx] ? -1 : (pA[idx] > pB[idx] ? 1 : 0);
   }
}
#endif

/* to/from oct string conversion */
#define cpToOctStr_BNU32 OWNAPI(cpToOctStr_BNU32)
   IPP_OWN_DECL (cpSize, cpToOctStr_BNU32, (Ipp8u* pStr, cpSize strLen, const Ipp32u* pBNU, cpSize bnuSize))
#define cpFromOctStr_BNU32 OWNAPI(cpFromOctStr_BNU32)
   IPP_OWN_DECL (cpSize, cpFromOctStr_BNU32, (Ipp32u* pBNU, const Ipp8u* pOctStr, cpSize strLen))

#endif /* _CP_BNU32_MISC_H */
