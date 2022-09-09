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
//     Encrypt 128-bit data block according to Rijndael
//     (It's the special free from Sbox/tables implementation)
// 
//  Contents:
//     SafeEncrypt_RIJ128()
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

static Ipp8u AffineMatrix[] = {0x10,0x22,0x55,0x82,0x41,0x34,0x40,0x2A};
static void FwdSubByte(Ipp8u inp_out[])
{
   Ipp8u AffineCnt = 0xC2;
   int n;
   for(n=0; n<16; n++) {
      Ipp8u x = inp_out[n];
      x = InverseComposite(x);
      x = TransformByte(x, AffineMatrix);
      x^= AffineCnt;
      inp_out[n] = x;
   }
}

static int ShiftRowsInx[] = {0,5,10,15,4,9,14,3,8,13,2,7,12,1,6,11};
static void FwdShiftRows(Ipp8u inp_out[])
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

static Ipp8u GF16mul_E_2x[] = {0x00,0x2E,0x4F,0x61,0x8D,0xA3,0xC2,0xEC,
                               0x39,0x17,0x76,0x58,0xB4,0x9A,0xFB,0xD5};
static Ipp8u GF16mul_1_Cx[] = {0x00,0xC1,0xB2,0x73,0x54,0x95,0xE6,0x27,
                               0xA8,0x69,0x1A,0xDB,0xFC,0x3D,0x4E,0x8F};
static void FwdMixColumn(Ipp8u inp_out[])
{

   Ipp8u tmp[16];
   Ipp32u* pTmp32 = (Ipp32u*)tmp;
   Ipp32u* pInp32 = (Ipp32u*)inp_out;

   int n;
   for(n=0; n<16; n++) {
      Ipp8u xL = (Ipp8u)( inp_out[n] & 0xF );
      Ipp8u xH = (Ipp8u)( (inp_out[n]>>4) & 0xF );
      tmp[n] = (Ipp8u)( GF16mul_E_2x[xL] ^ GF16mul_1_Cx[xH] );
   }

   pTmp32[0] ^= ROR32(pTmp32[0], 8);
   pTmp32[1] ^= ROR32(pTmp32[1], 8);
   pTmp32[2] ^= ROR32(pTmp32[2], 8);
   pTmp32[3] ^= ROR32(pTmp32[3], 8);

   pInp32[0] = ROR32(pInp32[0], 8);
   pInp32[1] = ROR32(pInp32[1], 8);
   pInp32[2] = ROR32(pInp32[2], 8);
   pInp32[3] = ROR32(pInp32[3], 8);
   pTmp32[0] ^= pInp32[0];
   pTmp32[1] ^= pInp32[1];
   pTmp32[2] ^= pInp32[2];
   pTmp32[3] ^= pInp32[3];

   pInp32[0] = ROR32(pInp32[0], 8);
   pInp32[1] = ROR32(pInp32[1], 8);
   pInp32[2] = ROR32(pInp32[2], 8);
   pInp32[3] = ROR32(pInp32[3], 8);
   pTmp32[0] ^= pInp32[0];
   pTmp32[1] ^= pInp32[1];
   pTmp32[2] ^= pInp32[2];
   pTmp32[3] ^= pInp32[3];

   pInp32[0] = ROR32(pInp32[0], 8);
   pInp32[1] = ROR32(pInp32[1], 8);
   pInp32[2] = ROR32(pInp32[2], 8);
   pInp32[3] = ROR32(pInp32[3], 8);
   pInp32[0]^= pTmp32[0];
   pInp32[1]^= pTmp32[1];
   pInp32[2]^= pTmp32[2];
   pInp32[3]^= pTmp32[3];
}

/* define number of column in the state */
#define SC           NB(128)
#define STATE_SIZE   (sizeof(Ipp32u)*SC)

IPP_OWN_DEFN (void, SafeEncrypt_RIJ128, (const Ipp8u* pInpBlk, Ipp8u* pOutBlk, int nr, const Ipp8u* pKeys, const void* pTables))
{
   int r;

   Ipp8u state[STATE_SIZE]; /* state */

   IPP_UNREFERENCED_PARAMETER(pTables);

   /* native => composite */
   TransformNative2Composite(state, pInpBlk);

   /* input whitening */
   AddRoundKey(state, state, pKeys);
   pKeys += STATE_SIZE;

   /* regular (nr-1) rounds */
   for(r=1; r<nr; r++) {
      FwdSubByte(state);
      FwdShiftRows(state);
      FwdMixColumn(state);
      AddRoundKey(state, state, pKeys);
      pKeys += STATE_SIZE;
   }

   /* irregular round */
   FwdSubByte(state);
   FwdShiftRows(state);
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

/* affine transformation matrix Ipp8u AffineMatrix[] = {0x10,0x22,0x55,0x82,0x41,0x34,0x40,0x2A};
   is defined in reference code, see doc for details */
static Ipp8u FwdAffineMatrixLO[] = { /* defived from AffineMatrix[i], i=0,1,2,3 */
   /* 0 */      0x00,
   /* 1 */      0x10,
   /* 2 */      0x22,
   /* 3 */      0x22^0x10,
   /* 4 */      0x55,
   /* 5 */      0x55^0x10,
   /* 6 */      0x55^0x22,
   /* 7 */      0x55^0x22^0x10,
   /* 8 */      0x82,
   /* 9 */      0x82^0x10,
   /* a */      0x82^0x22,
   /* b */      0x82^0x22^0x10,
   /* c */      0x82^0x55,
   /* d */      0x82^0x55^0x10,
   /* e */      0x82^0x55^0x22,
   /* f */      0x82^0x55^0x22^0x10
};
static Ipp8u FwdAffineMatrixHI[] = { /* defived from AffineMatrix[i], i=4,5,6,7 */
   /* 0 */      0x00,
   /* 1 */      0x41,
   /* 2 */      0x34,
   /* 3 */      0x34^0x41,
   /* 4 */      0x40,
   /* 5 */      0x40^0x41,
   /* 6 */      0x40^0x34,
   /* 7 */      0x40^0x34^0x41,
   /* 8 */      0x2A,
   /* 9 */      0x2A^0x41,
   /* a */      0x2A^0x34,
   /* b */      0x2A^0x34^0x41,
   /* c */      0x2A^0x40,
   /* d */      0x2A^0x40^0x41,
   /* e */      0x2A^0x40^0x34,
   /* f */      0x2A^0x40^0x34^0x41
};

/* inplace SubByte */
static void FwdSubByte(Ipp8u blk[16])
{
   Ipp8u blk_c[16], blk_b[16];
   ((Ipp64u*)blk_c)[0] = ((Ipp64u*)blk)[0] & 0x0F0F0F0F0F0F0F0F;
   ((Ipp64u*)blk_c)[1] = ((Ipp64u*)blk)[1] & 0x0F0F0F0F0F0F0F0F;
   ((Ipp64u*)blk_b)[0] = (((Ipp64u*)blk)[0]>>4) & 0x0F0F0F0F0F0F0F0F;
   ((Ipp64u*)blk_b)[1] = (((Ipp64u*)blk)[1]>>4) & 0x0F0F0F0F0F0F0F0F;

   {
      const Ipp8u affineCnt = 0xc2; /* value of H(0x63) */
      int n;
      for(n=0; n<16; n++) {
         Ipp8u c = blk_c[n];
         Ipp8u b = blk_b[n];

         /* {c+b*t} element inversion => {c+b*t} */
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

         /* affine transformation y = (AT)*(x^-1) + affineCnt */
         c = FwdAffineMatrixLO[c];
         b = FwdAffineMatrixHI[b];
         blk[n] = (c^b) ^affineCnt;
      }
   }
}

/* inplace ShifttRows operation */
/* int ShiftRowsInx[] = {0,5,10,15, 4,9,14,3, 8,13,2,7, 12,1,6,11}; */
__INLINE void FwdShiftRows(Ipp8u blk[16])
{
   Ipp8u x = blk[1];
   blk[1] = blk[5];
   blk[5] = blk[9];
   blk[9] = blk[13];
   blk[13]= x;

   x = blk[2];
   blk[2] = blk[10];
   blk[10]= x;
   x = blk[6];
   blk[6] = blk[14];
   blk[14]= x;

   x = blk[15];
   blk[15] = blk[11];
   blk[11] = blk[7];
   blk[7]  = blk[3];
   blk[3]  = x;
}

/* inplace  Mixcolumns operation */
static Ipp8u GF16mul_E_2x[] = {0x00,0x2E,0x4F,0x61,0x8D,0xA3,0xC2,0xEC,
                               0x39,0x17,0x76,0x58,0xB4,0x9A,0xFB,0xD5};
static Ipp8u GF16mul_1_Cx[] = {0x00,0xC1,0xB2,0x73,0x54,0x95,0xE6,0x27,
                               0xA8,0x69,0x1A,0xDB,0xFC,0x3D,0x4E,0x8F};
static void FwdMixColumn(Ipp8u blk[16])
{
   Ipp8u tmp[16];
   Ipp32u* pTmp32 = (Ipp32u*)tmp;
   Ipp32u* pBlk32 = (Ipp32u*)blk;

   int n;
   for(n=0; n<16; n++) {
      Ipp8u xL = blk[n] & 0xF;
      Ipp8u xH =(blk[n]>>4) & 0xF;
      tmp[n] = GF16mul_E_2x[xL] ^ GF16mul_1_Cx[xH];
   }

   pTmp32[0] ^= ROR32(pTmp32[0], 8);
   pTmp32[1] ^= ROR32(pTmp32[1], 8);
   pTmp32[2] ^= ROR32(pTmp32[2], 8);
   pTmp32[3] ^= ROR32(pTmp32[3], 8);

   pBlk32[0] = ROR32(pBlk32[0], 8);
   pBlk32[1] = ROR32(pBlk32[1], 8);
   pBlk32[2] = ROR32(pBlk32[2], 8);
   pBlk32[3] = ROR32(pBlk32[3], 8);
   pTmp32[0] ^= pBlk32[0];
   pTmp32[1] ^= pBlk32[1];
   pTmp32[2] ^= pBlk32[2];
   pTmp32[3] ^= pBlk32[3];

   pBlk32[0] = ROR32(pBlk32[0], 8);
   pBlk32[1] = ROR32(pBlk32[1], 8);
   pBlk32[2] = ROR32(pBlk32[2], 8);
   pBlk32[3] = ROR32(pBlk32[3], 8);
   pTmp32[0] ^= pBlk32[0];
   pTmp32[1] ^= pBlk32[1];
   pTmp32[2] ^= pBlk32[2];
   pTmp32[3] ^= pBlk32[3];

   pBlk32[0] = ROR32(pBlk32[0], 8);
   pBlk32[1] = ROR32(pBlk32[1], 8);
   pBlk32[2] = ROR32(pBlk32[2], 8);
   pBlk32[3] = ROR32(pBlk32[3], 8);
   pBlk32[0]^= pTmp32[0];
   pBlk32[1]^= pTmp32[1];
   pBlk32[2]^= pTmp32[2];
   pBlk32[3]^= pTmp32[3];
}


/* define number of column in the state */
#define SC           NB(128)
#define STATE_SIZE   (sizeof(Ipp32u)*SC)


IPP_OWN_DEFN (void, SafeEncrypt_RIJ128, (const Ipp8u* pInpBlk, Ipp8u* pOutBlk, int nr, const Ipp8u* pKeys, const void* pTables))
{
   int r;

   Ipp8u state[STATE_SIZE]; /* local state */

   IPP_UNREFERENCED_PARAMETER(pTables);

   /* native => composite */
   TransformNative2Composite(state, pInpBlk);

   /* input whitening */
   AddRoundKey(state, state, pKeys);
   pKeys += STATE_SIZE;

   /* regular (nr-1) rounds */
   for(r=1; r<nr; r++) {
      FwdSubByte(state);
      FwdShiftRows(state);
      FwdMixColumn(state);
      AddRoundKey(state, state, pKeys);
      pKeys += STATE_SIZE;
   }

   /* last irregular round */
   FwdSubByte(state);
   FwdShiftRows(state);
   AddRoundKey(state, state, pKeys);

   /* composite => native */
   TransformComposite2Native(pOutBlk, state);
}
#endif /* !_PCP_RIJ_SAFE_OLD */

#endif /* _ALG_AES_SAFE_COMPOSITE_GF_ */

#endif /* (_IPP <_IPP_V8) && (_IPP32E <_IPP32E_U8) */
