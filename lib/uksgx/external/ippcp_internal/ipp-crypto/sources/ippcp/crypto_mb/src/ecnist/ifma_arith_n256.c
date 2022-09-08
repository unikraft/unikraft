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

#include <internal/ecnist/ifma_arith_p256.h>

/* Constants */
#define LEN52    NUMBER_OF_DIGITS(256,DIGIT_SIZE)

/*
// EC NIST-P256 prime base point order
// in 2^52 radix
*/
__ALIGN64 static const int64u n256_mb[LEN52][8] = {
   {0x9cac2fc632551, 0x9cac2fc632551, 0x9cac2fc632551, 0x9cac2fc632551, 0x9cac2fc632551, 0x9cac2fc632551, 0x9cac2fc632551, 0x9cac2fc632551},
   {0xada7179e84f3b, 0xada7179e84f3b, 0xada7179e84f3b, 0xada7179e84f3b, 0xada7179e84f3b, 0xada7179e84f3b, 0xada7179e84f3b, 0xada7179e84f3b},
   {0xfffffffbce6fa, 0xfffffffbce6fa, 0xfffffffbce6fa, 0xfffffffbce6fa, 0xfffffffbce6fa, 0xfffffffbce6fa, 0xfffffffbce6fa, 0xfffffffbce6fa},
   {0x0000fffffffff, 0x0000fffffffff, 0x0000fffffffff, 0x0000fffffffff, 0x0000fffffffff, 0x0000fffffffff, 0x0000fffffffff, 0x0000fffffffff},
   {0x0ffffffff0000, 0x0ffffffff0000, 0x0ffffffff0000, 0x0ffffffff0000, 0x0ffffffff0000, 0x0ffffffff0000, 0x0ffffffff0000, 0x0ffffffff0000}
};

__ALIGN64 static const int64u n256x2_mb[LEN52][8] = {
   {0x39585f8c64aa2, 0x39585f8c64aa2, 0x39585f8c64aa2, 0x39585f8c64aa2, 0x39585f8c64aa2, 0x39585f8c64aa2, 0x39585f8c64aa2, 0x39585f8c64aa2},
   {0x5b4e2f3d09e77, 0x5b4e2f3d09e77, 0x5b4e2f3d09e77, 0x5b4e2f3d09e77, 0x5b4e2f3d09e77, 0x5b4e2f3d09e77, 0x5b4e2f3d09e77, 0x5b4e2f3d09e77},
   {0xfffffff79cdf5, 0xfffffff79cdf5, 0xfffffff79cdf5, 0xfffffff79cdf5, 0xfffffff79cdf5, 0xfffffff79cdf5, 0xfffffff79cdf5, 0xfffffff79cdf5},
   {0x0001fffffffff, 0x0001fffffffff, 0x0001fffffffff, 0x0001fffffffff, 0x0001fffffffff, 0x0001fffffffff, 0x0001fffffffff, 0x0001fffffffff},
   {0x1fffffffe0000, 0x1fffffffe0000, 0x1fffffffe0000, 0x1fffffffe0000, 0x1fffffffe0000, 0x1fffffffe0000, 0x1fffffffe0000, 0x1fffffffe0000}
};

/* k0 = -( (1/n256 mod 2^DIGIT_SIZE) ) mod 2^DIGIT_SIZE */
__ALIGN64 static const int64u n256_k0_mb[8] = {
   0x1c8aaee00bc4f, 0x1c8aaee00bc4f, 0x1c8aaee00bc4f, 0x1c8aaee00bc4f, 0x1c8aaee00bc4f, 0x1c8aaee00bc4f, 0x1c8aaee00bc4f, 0x1c8aaee00bc4f
};

/* to Montgomery conversion constant
// rr = 2^((LEN52*DIGIT_SIZE)*2) mod n256
*/
__ALIGN64 static const int64u n256_rr_mb[LEN52][8] = {
   {0x0005cc0dea6dc3ba,0x0005cc0dea6dc3ba,0x0005cc0dea6dc3ba,0x0005cc0dea6dc3ba,0x0005cc0dea6dc3ba,0x0005cc0dea6dc3ba,0x0005cc0dea6dc3ba,0x0005cc0dea6dc3ba},
   {0x000192a067d8a084,0x000192a067d8a084,0x000192a067d8a084,0x000192a067d8a084,0x000192a067d8a084,0x000192a067d8a084,0x000192a067d8a084,0x000192a067d8a084},
   {0x000bec59615571bb,0x000bec59615571bb,0x000bec59615571bb,0x000bec59615571bb,0x000bec59615571bb,0x000bec59615571bb,0x000bec59615571bb,0x000bec59615571bb},
   {0x0001fc245b2392b6,0x0001fc245b2392b6,0x0001fc245b2392b6,0x0001fc245b2392b6,0x0001fc245b2392b6,0x0001fc245b2392b6,0x0001fc245b2392b6,0x0001fc245b2392b6},
   {0x0000e12d9559d956,0x0000e12d9559d956,0x0000e12d9559d956,0x0000e12d9559d956,0x0000e12d9559d956,0x0000e12d9559d956,0x0000e12d9559d956,0x0000e12d9559d956}
};


/*=====================================================================

 Specialized single operations in n256 - sqr & mul

=====================================================================*/
EXTERN_C U64* MB_FUNC_NAME(ifma_n256_)(void)
{
   return (U64*)n256_mb;
}

void MB_FUNC_NAME(ifma_ams52_n256_)(U64 r[], const U64 a[])
{
   MB_FUNC_NAME(ifma_ams52x5_)(r, a, (U64*)n256_mb, n256_k0_mb);
}

void MB_FUNC_NAME(ifma_amm52_n256_)(U64 r[], const U64 a[], const U64 b[])
{
   MB_FUNC_NAME(ifma_amm52x5_)(r, a, b, (U64*)n256_mb, n256_k0_mb);
}

void MB_FUNC_NAME(ifma_tomont52_n256_)(U64 r[], const U64 a[])
{
   MB_FUNC_NAME(ifma_amm52x5_)(r, a, (U64*)n256_rr_mb, (U64*)n256_mb, n256_k0_mb);
}

void MB_FUNC_NAME(ifma_frommont52_n256_)(U64 r[], const U64 a[])
{
   MB_FUNC_NAME(ifma_amm52_n256_)(r, a, (U64*)ones);
}

/*
// computes r = 1/z = z^(n256-2) mod n256
//
// note: z in in Montgomery domain (as soon mul() and sqr() below are amm-functions
//       r in Montgomery domain too
*/
#define sqr_n256    MB_FUNC_NAME(ifma_ams52_n256_)
#define mul_n256    MB_FUNC_NAME(ifma_amm52_n256_)

void MB_FUNC_NAME(ifma_aminv52_n256_)(U64 r[], const U64 z[])
{
   int i;

   // pwr_z_Tbl[i][] = z^i, i=0,..,15
   __ALIGN64 U64 pwr_z_Tbl[16][LEN52];

   MB_FUNC_NAME(ifma_tomont52_n256_)(pwr_z_Tbl[0], (U64*)ones);
   MB_FUNC_NAME(mov_FE256_)(pwr_z_Tbl[1], z);

   for(i=2; i<16; i+=2) {
      sqr_n256(pwr_z_Tbl[i], pwr_z_Tbl[i/2]);
      mul_n256(pwr_z_Tbl[i+1], pwr_z_Tbl[i], z);
   }

   // pwr = (n256-2) in big endian
   int8u pwr[] = "\xFF\xFF\xFF\xFF\x00\x00\x00\x00"
                 "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF"
                 "\xBC\xE6\xFA\xAD\xA7\x17\x9E\x84"
                 "\xF3\xB9\xCA\xC2\xFC\x63\x25\x4F";
   // init r = 1
   MB_FUNC_NAME(mov_FE256_)(r, pwr_z_Tbl[0]);

   for(i=0; i<32; i++) {
      int v = pwr[i];
      int hi = (v>>4) &0xF;
      int lo = v & 0xF;

      sqr_n256(r, r);
      sqr_n256(r, r);
      sqr_n256(r, r);
      sqr_n256(r, r);
      if(hi)
         mul_n256(r, r, pwr_z_Tbl[hi]);
      sqr_n256(r, r);
      sqr_n256(r, r);
      sqr_n256(r, r);
      sqr_n256(r, r);
      if(lo)
         mul_n256(r, r, pwr_z_Tbl[lo]);
   }
}

/*=====================================================================

 Specialized single operations in n256 - add, sub & neg

=====================================================================*/

void MB_FUNC_NAME(ifma_add52_n256_)(U64 r[], const U64 a[], const U64 b[])
{
   MB_FUNC_NAME(ifma_add52x5_)(r, a, b, (U64*)n256x2_mb);
}

void MB_FUNC_NAME(ifma_sub52_n256_)(U64 r[], const U64 a[], const U64 b[])
{
   MB_FUNC_NAME(ifma_sub52x5_)(r, a, b, (U64*)n256x2_mb);
}

void MB_FUNC_NAME(ifma_neg52_n256_)(U64 r[], const U64 a[])
{
   MB_FUNC_NAME(ifma_neg52x5_)(r, a, (U64*)n256x2_mb);
}

__mb_mask MB_FUNC_NAME(lt_mbx_digit_)(const U64 a, const U64 b, const __mb_mask lt_mask)
{
   U64 d = mask_sub64(sub64(a, b), lt_mask, sub64(a, b), set1(1));
   return cmp64_mask(d, get_zero64(), _MM_CMPINT_LT);
}

/* r = (a>=n256)? a-n256 : a */
void MB_FUNC_NAME(ifma_fastred52_pn256_)(U64 R[], const U64 A[])
{
   /* r = a - b */
   U64 r0 = sub64(A[0], ((U64*)(n256_mb))[0]);
   U64 r1 = sub64(A[1], ((U64*)(n256_mb))[1]);
   U64 r2 = sub64(A[2], ((U64*)(n256_mb))[2]);
   U64 r3 = sub64(A[3], ((U64*)(n256_mb))[3]);
   U64 r4 = sub64(A[4], ((U64*)(n256_mb))[4]);

   /* lt = {r0 - r4} < 0 */
   __mb_mask
   lt = MB_FUNC_NAME(lt_mbx_digit_)(r0, get_zero64(), 0);
   lt = MB_FUNC_NAME(lt_mbx_digit_)(r1, get_zero64(), lt);
   lt = MB_FUNC_NAME(lt_mbx_digit_)(r2, get_zero64(), lt);
   lt = MB_FUNC_NAME(lt_mbx_digit_)(r3, get_zero64(), lt);
   lt = MB_FUNC_NAME(lt_mbx_digit_)(r4, get_zero64(), lt);

   r0 = mask_mov64(A[0], ~lt, r0);
   r1 = mask_mov64(A[1], ~lt, r1);
   r2 = mask_mov64(A[2], ~lt, r2);
   r3 = mask_mov64(A[3], ~lt, r3);
   r4 = mask_mov64(A[4], ~lt, r4);

   /* normalize r0 - r4 */
   NORM_ASHIFTR(r, 0,1)
   NORM_ASHIFTR(r, 1,2)
   NORM_ASHIFTR(r, 2,3)
   NORM_ASHIFTR(r, 3,4)

   R[0] = r0;
   R[1] = r1;
   R[2] = r2;
   R[3] = r3;
   R[4] = r4;
}

__mb_mask MB_FUNC_NAME(ifma_cmp_lt_n256_)(const U64 a[])
{
   return MB_FUNC_NAME(cmp_lt_FE256_)(a,(const U64 (*))n256_mb);
}

__mb_mask MB_FUNC_NAME(ifma_check_range_n256_)(const U64 A[])
{
   __mb_mask
   mask = MB_FUNC_NAME(is_zero_FE256_)(A);
   mask |= ~MB_FUNC_NAME(ifma_cmp_lt_n256_)(A);

   return mask;
}

