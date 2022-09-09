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
// 
//  Purpose:
//     Cryptography Primitive.
//     Decrypt 128-bit data block according to Rijndael
//     (It's the special free from Sbox/tables implementation)
// 
//  Contents:
//     SafeDecrypt_RIJ128()
// 
// 
*/

#include "owncp.h"

#if ((_IPP <_IPP_V8) && (_IPP32E <_IPP32E_U8)) /* no pshufb instruction */

#if (_ALG_AES_SAFE_==_ALG_AES_SAFE_COMPOSITE_GF_)

#include "pcprij.h"
#include "pcprij128safe.h"

#if defined _PCP_RIJ_SAFE_OLD
/*
// old version
*/

static Ipp8u AffineMatrix[] = {0x50,0x36,0x15,0x82,0x01,0x34,0x40,0x3E};
static void InvSubByte(Ipp8u inp_out[])
{
   Ipp8u AffineCnt = 0x48;
   int n;
   for(n=0; n<16; n++) {
      Ipp8u x = inp_out[n];
      x = TransformByte(x, AffineMatrix);
      x^= AffineCnt;
      x = InverseComposite(x);
      inp_out[n] = x;
   }
}

static int ShiftRowsInx[] = {0,13,10,7,4,1,14,11,8,5,2,15,12,9,6,3};
static void InvShiftRows(Ipp8u inp_out[])
{
   Ipp8u tmp[16];

   int n;
   for(n=0; n<16; n++)
      tmp[n] = inp_out[n];

   for(n=0; n<16; n++) {
      int idx = ShiftRowsInx[n];
      inp_out[n] = tmp[idx];
   }
}

static Ipp8u GF16mul_4_2x[] = {0x00,0x24,0x48,0x6C,0x83,0xA7,0xCB,0xEF,
                               0x36,0x12,0x7E,0x5A,0xB5,0x91,0xFD,0xD9};
static Ipp8u GF16mul_1_6x[] = {0x00,0x61,0xC2,0xA3,0xB4,0xD5,0x76,0x17,
                               0x58,0x39,0x9A,0xFB,0xEC,0x8D,0x2E,0x4F};

static Ipp8u GF16mul_C_6x[] = {0x00,0x6C,0xCB,0xA7,0xB5,0xD9,0x7E,0x12,
                               0x5A,0x36,0x91,0xFD,0xEF,0x83,0x24,0x48};
static Ipp8u GF16mul_3_Ax[] = {0x00,0xA3,0x76,0xD5,0xEC,0x4F,0x9A,0x39,
                               0xFB,0x58,0x8D,0x2E,0x17,0xB4,0x61,0xC2};

static Ipp8u GF16mul_B_0x[] = {0x00,0x0B,0x05,0x0E,0x0A,0x01,0x0F,0x04,
                               0x07,0x0C,0x02,0x09,0x0D,0x06,0x08,0x03};
static Ipp8u GF16mul_0_Bx[] = {0x00,0xB0,0x50,0xE0,0xA0,0x10,0xF0,0x40,
                               0x70,0xC0,0x20,0x90,0xD0,0x60,0x80,0x30};

static Ipp8u GF16mul_2_4x[] = {0x00,0x42,0x84,0xC6,0x38,0x7A,0xBC,0xFE,
                               0x63,0x21,0xE7,0xA5,0x5B,0x19,0xDF,0x9D};
static Ipp8u GF16mul_2_6x[] = {0x00,0x62,0xC4,0xA6,0xB8,0xDA,0x7C,0x1E,
                               0x53,0x31,0x97,0xF5,0xEB,0x89,0x2F,0x4D};
static void InvMixColumn(Ipp8u inp_out[])
{

   Ipp8u out[16];
   Ipp32u* pInp32 = (Ipp32u*)inp_out;

   int n;

   for(n=0; n<16; n++) {
      int xL = inp_out[n] & 0xF;
      int xH = (inp_out[n]>>4) & 0xF;
      out[n] = (Ipp8u)( GF16mul_4_2x[xL] ^ GF16mul_1_6x[xH] );
   }

   pInp32[0] = ROR32(pInp32[0], 8);
   pInp32[1] = ROR32(pInp32[1], 8);
   pInp32[2] = ROR32(pInp32[2], 8);
   pInp32[3] = ROR32(pInp32[3], 8);
   for(n=0; n<16; n++) {
      int xL = inp_out[n] & 0xF;
      int xH = (inp_out[n]>>4) & 0xF;
      out[n]^= (Ipp8u)( GF16mul_C_6x[xL] ^ GF16mul_3_Ax[xH] );
   }

   pInp32[0] = ROR32(pInp32[0], 8);
   pInp32[1] = ROR32(pInp32[1], 8);
   pInp32[2] = ROR32(pInp32[2], 8);
   pInp32[3] = ROR32(pInp32[3], 8);
   for(n=0; n<16; n++) {
      int xL = inp_out[n] & 0xF;
      int xH = (inp_out[n]>>4) & 0xF;
      out[n]^= (Ipp8u)( GF16mul_B_0x[xL] ^ GF16mul_0_Bx[xH] );
   }

   pInp32[0] = ROR32(pInp32[0], 8);
   pInp32[1] = ROR32(pInp32[1], 8);
   pInp32[2] = ROR32(pInp32[2], 8);
   pInp32[3] = ROR32(pInp32[3], 8);
   for(n=0; n<16; n++) {
      int xL = inp_out[n] & 0xF;
      int xH = (inp_out[n]>>4) & 0xF;
      out[n]^= (Ipp8u)( GF16mul_2_4x[xL] ^ GF16mul_2_6x[xH] );
   }

   for(n=0; n<16; n++)
      inp_out[n] = out[n];
}

/* define number of column in the state */
#define SC           NB(128)
#define STATE_SIZE   (sizeof(Ipp32u)*SC)

IPP_OWN_DEFN (void, SafeDecrypt_RIJ128, (const Ipp8u* pInpBlk, Ipp8u* pOutBlk, int nr, const Ipp8u* pKeys, const void* pTables))
{
   int r;

   Ipp8u state[STATE_SIZE]; /* state */

   IPP_UNREFERENCED_PARAMETER(pTables);

   /* native => composite */
   TransformNative2Composite(state, pInpBlk);

   pKeys += nr*STATE_SIZE;

   /* input whitening */
   AddRoundKey(state, state, pKeys);
   pKeys -= STATE_SIZE;

   /* regular (nr-1) rounds */
   for(r=1; r<nr; r++) {
      InvSubByte(state);
      InvShiftRows(state);
      InvMixColumn(state);
      AddRoundKey(state, state, pKeys);
      pKeys -= STATE_SIZE;
   }

   /* irregular round */
   InvSubByte(state);
   InvShiftRows(state);
   AddRoundKey(state, state, pKeys);

   /* composite => native */
   TransformComposite2Native(pOutBlk, state);
}
#endif /* _PCP_RIJ_SAFE_OLD */

#if !defined _PCP_RIJ_SAFE_OLD
/*
// new version
*/

/*
// SubByte operation in composite GF((2^4)^2)
// sdetails are in the doc.
//
// multiplication in basic GF(2^4) performs by log-and-exp sequence
*/
static Ipp8u GF16_sqr1[]   = {0x00,0x09,0x02,0x0B,0x08,0x01,0x0A,0x03,     /* (x^2)*{9} */
                              0x06,0x0F,0x04,0x0D,0x0E,0x07,0x0C,0x05};

static Ipp8u GF16_log[]    = {0xC0,0x00,0x01,0x04,0x02,0x08,0x05,0x0A,     /* log element x */
                              0x03,0x0E,0x09,0x07,0x06,0x0D,0x0B,0x0C};
static Ipp8u GF16_invlog[] = {0xC0,0x00,0x0E,0x0B,0x0D,0x07,0x0A,0x05,     /* log of multiple inversion element x^-1 */
                              0x0C,0x01,0x06,0x08,0x09,0x02,0x04,0x03};
static Ipp8u GF16_exp[]    = {0x01,0x02,0x04,0x08,0x03,0x06,0x0C,0x0B,
                              0x05,0x0A,0x07,0x0E,0x0F,0x0D,0x09,0x00};   /* exp[15]= 0!!! */

/* affine transformation matrix Ipp8u AffineMatrix[] = {0x50,0x36,0x15,0x82,0x01,0x34,0x40,0x3E};
   is defined in reference code, see doc for details */
static Ipp8u InvAffineMatrixLO[] = { /* defived from AffineMatrix[i], i=0,1,2,3 */
   /* 0 */      0x00,
   /* 1 */      0x50,
   /* 2 */      0x36,
   /* 3 */      0x36^0x50,
   /* 4 */      0x15,
   /* 5 */      0x15^0x50,
   /* 6 */      0x15^0x36,
   /* 7 */      0x15^0x36^0x50,
   /* 8 */      0x82,
   /* 9 */      0x82^0x50,
   /* a */      0x82^0x36,
   /* b */      0x82^0x36^0x50,
   /* c */      0x82^0x15,
   /* d */      0x82^0x15^0x50,
   /* e */      0x82^0x15^0x36,
   /* f */      0x82^0x15^0x36^0x50
};
static Ipp8u InvAffineMatrixHI[] = { /* defived from AffineMatrix[i], i=4,5,6,7 */
   /* 0 */      0x00,
   /* 1 */      0x01,
   /* 2 */      0x34,
   /* 3 */      0x34^0x01,
   /* 4 */      0x40,
   /* 5 */      0x40^0x01,
   /* 6 */      0x40^0x34,
   /* 7 */      0x40^0x34^0x01,
   /* 8 */      0x3E,
   /* 9 */      0x3E^0x01,
   /* a */      0x3E^0x34,
   /* b */      0x3E^0x34^0x01,
   /* c */      0x3E^0x40,
   /* d */      0x3E^0x40^0x01,
   /* e */      0x3E^0x40^0x34,
   /* f */      0x3E^0x40^0x34^0x01
};

static void InvSubByte(Ipp8u blk[16])
{
   Ipp8u blk_c[16], blk_b[16];
   ((Ipp64u*)blk_c)[0] = ((Ipp64u*)blk)[0] & 0x0F0F0F0F0F0F0F0F;
   ((Ipp64u*)blk_c)[1] = ((Ipp64u*)blk)[1] & 0x0F0F0F0F0F0F0F0F;
   ((Ipp64u*)blk_b)[0] = (((Ipp64u*)blk)[0]>>4) & 0x0F0F0F0F0F0F0F0F;
   ((Ipp64u*)blk_b)[1] = (((Ipp64u*)blk)[1]>>4) & 0x0F0F0F0F0F0F0F0F;

   {
      const Ipp8u affineCnt = 0x48;  /* /* value of H(0x05) */
      int n;
      for(n=0; n<16; n++) {
         Ipp8u c = blk_c[n];
         Ipp8u b = blk_b[n];

         /* affine transformation y = (AT)*(x) + affineCnt */
         Ipp8u t = InvAffineMatrixLO[c] ^ InvAffineMatrixHI[b] ^ affineCnt;
         c = t & 0xF;
         b = t >> 4;

         /* {c+b*t} element inversion => {c+b*t} */
         {
            Ipp8u log_c  = GF16_log[c];
            Ipp8u log_b  = GF16_log[b];
            Ipp8u log_cb = GF16_log[c^b];

            Ipp8u d = GF16_sqr1[b];
            Ipp8u t = AddLogGF16(log_c, log_cb);
            d ^= GF16_exp[t];

            d = GF16_invlog[d];
            c = AddLogGF16(log_cb, d);
            b = AddLogGF16(log_b, d);
            c = GF16_exp[c];
            b = GF16_exp[b];
         }

         blk[n] = b<<4 | c;
      }
   }
}

/* inplace ShiftRows operation */
/* int ShiftRowsInx[] = {0,13,10,7, 4,1,14,11, 8,5,2,15, 12,9,6,3}; */
static void InvShiftRows(Ipp8u blk[16])
{
   Ipp8u x = blk[13];
   blk[13]= blk[9];
   blk[9] = blk[5];
   blk[5] = blk[1];
   blk[1] = x;

   x = blk[10];
   blk[10]= blk[2];
   blk[2] = x;
   x = blk[14];
   blk[14]= blk[6];
   blk[6] = x;

   x = blk[3];
   blk[3] = blk[7];
   blk[7] = blk[11];
   blk[11]= blk[15];
   blk[15]= x;
}

static Ipp8u GF16mul_4_2x[] = {0x00,0x24,0x48,0x6C,0x83,0xA7,0xCB,0xEF,
                               0x36,0x12,0x7E,0x5A,0xB5,0x91,0xFD,0xD9};
static Ipp8u GF16mul_1_6x[] = {0x00,0x61,0xC2,0xA3,0xB4,0xD5,0x76,0x17,
                               0x58,0x39,0x9A,0xFB,0xEC,0x8D,0x2E,0x4F};

static Ipp8u GF16mul_C_6x[] = {0x00,0x6C,0xCB,0xA7,0xB5,0xD9,0x7E,0x12,
                               0x5A,0x36,0x91,0xFD,0xEF,0x83,0x24,0x48};
static Ipp8u GF16mul_3_Ax[] = {0x00,0xA3,0x76,0xD5,0xEC,0x4F,0x9A,0x39,
                               0xFB,0x58,0x8D,0x2E,0x17,0xB4,0x61,0xC2};

static Ipp8u GF16mul_B_0x[] = {0x00,0x0B,0x05,0x0E,0x0A,0x01,0x0F,0x04,
                               0x07,0x0C,0x02,0x09,0x0D,0x06,0x08,0x03};
static Ipp8u GF16mul_0_Bx[] = {0x00,0xB0,0x50,0xE0,0xA0,0x10,0xF0,0x40,
                               0x70,0xC0,0x20,0x90,0xD0,0x60,0x80,0x30};

static Ipp8u GF16mul_2_4x[] = {0x00,0x42,0x84,0xC6,0x38,0x7A,0xBC,0xFE,
                               0x63,0x21,0xE7,0xA5,0x5B,0x19,0xDF,0x9D};
static Ipp8u GF16mul_2_6x[] = {0x00,0x62,0xC4,0xA6,0xB8,0xDA,0x7C,0x1E,
                               0x53,0x31,0x97,0xF5,0xEB,0x89,0x2F,0x4D};
static void InvMixColumn(Ipp8u blk[16])
{
   Ipp8u out[16];
   Ipp32u* pInp32 = (Ipp32u*)blk;

   int n;
   for(n=0; n<16; n++) {
      int xL = blk[n] & 0xF;
      int xH = (blk[n]>>4) & 0xF;
      out[n] = GF16mul_4_2x[xL] ^ GF16mul_1_6x[xH];
   }
   pInp32[0] = ROR32(pInp32[0], 8);
   pInp32[1] = ROR32(pInp32[1], 8);
   pInp32[2] = ROR32(pInp32[2], 8);
   pInp32[3] = ROR32(pInp32[3], 8);

   for(n=0; n<16; n++) {
      int xL = blk[n] & 0xF;
      int xH = (blk[n]>>4) & 0xF;
      out[n]^= GF16mul_C_6x[xL] ^ GF16mul_3_Ax[xH];
   }
   pInp32[0] = ROR32(pInp32[0], 8);
   pInp32[1] = ROR32(pInp32[1], 8);
   pInp32[2] = ROR32(pInp32[2], 8);
   pInp32[3] = ROR32(pInp32[3], 8);

   for(n=0; n<16; n++) {
      int xL = blk[n] & 0xF;
      int xH = (blk[n]>>4) & 0xF;
      out[n]^= GF16mul_B_0x[xL] ^ GF16mul_0_Bx[xH];
   }
   pInp32[0] = ROR32(pInp32[0], 8);
   pInp32[1] = ROR32(pInp32[1], 8);
   pInp32[2] = ROR32(pInp32[2], 8);
   pInp32[3] = ROR32(pInp32[3], 8);

   for(n=0; n<16; n++) {
      int xL = blk[n] & 0xF;
      int xH = (blk[n]>>4) & 0xF;
      blk[n] = out[n] ^ GF16mul_2_4x[xL] ^ GF16mul_2_6x[xH];
   }
}

/* define number of column in the state */
#define SC           NB(128)
#define STATE_SIZE   (sizeof(Ipp32u)*SC)

IPP_OWN_DEFN (void, SafeDecrypt_RIJ128, (const Ipp8u* pInpBlk, Ipp8u* pOutBlk, int nr, const Ipp8u* pKeys, const void* pTables))
{
   int r;

   Ipp8u state[STATE_SIZE]; /* state */

   IPP_UNREFERENCED_PARAMETER(pTables);

   /* native => composite */
   TransformNative2Composite(state, pInpBlk);

   pKeys += nr*STATE_SIZE;

   /* input whitening */
   AddRoundKey(state, state, pKeys);
   pKeys -= STATE_SIZE;

   /* regular (nr-1) rounds */
   for(r=1; r<nr; r++) {
      InvSubByte(state);
      InvShiftRows(state);
      InvMixColumn(state);
      AddRoundKey(state, state, pKeys);
      pKeys -= STATE_SIZE;
   }

   /* irregular round */
   InvSubByte(state);
   InvShiftRows(state);
   AddRoundKey(state, state, pKeys);

   /* composite => native */
   TransformComposite2Native(pOutBlk, state);
}
#endif /* !_PCP_RIJ_SAFE_OLD */

#endif /* _ALG_AES_SAFE_COMPOSITE_GF_ */

#endif /* (_IPP <_IPP_V8) && (_IPP32E <_IPP32E_U8) */
