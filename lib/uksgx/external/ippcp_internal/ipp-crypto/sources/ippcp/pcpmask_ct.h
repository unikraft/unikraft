/*******************************************************************************
* Copyright 2018-2021 Intel Corporation
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
//     Constant time Mask operations
//
//
*/

#if !defined(_PCP_MASK_CT_H)
#define _PCP_MASK_CT_H

#include "owncp.h"
#include "pcpbnuimpl.h"

/*
// The following functions test particular conditions
// and returns either 0 or 0xffffffff.
//
// The result is suitable for boolean and masked operations.
//
// Inspite of operation below are using BNU_CHUNK_T operand(s) it can be applied to Ipp32u, Ipp32s, Ipp16u, Ipp16s, Ipp8u and Ipp8s too.
// For example, if
//    Ipp32u uns_int;
//    Ipp32s sgn_int;
//    Ipp8u  uns_char;
//    Ipp8s  sgn_char;
// then
//    cpIs_msb_ct((Ipp32s)uns_int)     tests 31 bit of uns_int
//    cpIs_msb_ct(        sgn_int)     tests 31 bit of sgn_int
//    cpIs_msb_ct((Ipp8u)uns_char)     tests  7 bit of uns_char
//    cpIs_msb_ct(       sgn_char)     tests  7 bit of sgn_char
*/

/* tests if MSB(a)==1 */
__INLINE BNU_CHUNK_T cpIsMsb_ct(BNU_CHUNK_T a)
{
   return (BNU_CHUNK_T)0 - (a >> (sizeof(a) * 8 - 1));
}

/* tests if LSB(a)==1 */
__INLINE BNU_CHUNK_T cpIsLsb_ct(BNU_CHUNK_T a)
{
   return (BNU_CHUNK_T)0 - (a & 1);
}

/* tests if a is odd */
__INLINE BNU_CHUNK_T cpIsOdd_ct(BNU_CHUNK_T a)
{
   return cpIsLsb_ct(a);
}

/* tests if a is even */
__INLINE BNU_CHUNK_T cpIsEven_ct(BNU_CHUNK_T a)
{
   return ~cpIsLsb_ct(a);
}

/* tests if a==0 */
__INLINE BNU_CHUNK_T cpIsZero_ct(BNU_CHUNK_T a)
{
   return cpIsMsb_ct(~a & (a - 1));
}

/* tests if a==b */
__INLINE BNU_CHUNK_T cpIsEqu_ct(BNU_CHUNK_T a, BNU_CHUNK_T b)
{
   return cpIsZero_ct(a ^ b);
}

/* replace under mask: dst[] = replaceFlag? src[] : dst[] */
__INLINE void cpMaskedReplace_ct(BNU_CHUNK_T* dst, const BNU_CHUNK_T* src, int len, BNU_CHUNK_T replaceMask)
{
   BNU_CHUNK_T dstMask = ~replaceMask;
   int n;
   for(n=0; n<len; n++)
      dst[n] = (src[n] & replaceMask) ^  (dst[n] & dstMask);
}

/* copy under mask: dst[] = src1[] & mask) ^ src2[] & ~mask  */
__INLINE void cpMaskedCopyBNU_ct(BNU_CHUNK_T* dst, BNU_CHUNK_T mask, const BNU_CHUNK_T* src1, const BNU_CHUNK_T* src2, int len)
{
   int i;
   for(i=0; i<(len); i++)
      dst[i] = (src1[i] & mask) ^ (src2[i] & ~mask);
}

/* test if GF element is equal to x chunk */
__INLINE BNU_CHUNK_T cpIsGFpElemEquChunk_ct(const BNU_CHUNK_T* pE, int nsE, BNU_CHUNK_T x)
{
   int i;
   BNU_CHUNK_T accum = pE[0] ^ x;
   for (i = 1; i < nsE; i++) {
    accum |= pE[i];
   }
   return cpIsZero_ct(accum);
}

#define GFPE_IS_ZERO_CT(a,size)  cpIsGFpElemEquChunk_ct((a),(size), 0)

#endif /* _PCP_MASK_CT_H */
