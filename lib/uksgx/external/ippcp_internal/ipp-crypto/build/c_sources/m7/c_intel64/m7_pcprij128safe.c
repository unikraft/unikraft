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
//     Internal Safe Rijndael Encrypt, Decrypt
//     (It's the special free from Sbox/tables implementation)
// 
*/

#include "owndefs.h"
#include "owncp.h"

#if ((_IPP <_IPP_V8) && (_IPP32E <_IPP32E_U8)) /* no pshufb instruction */

#if (_ALG_AES_SAFE_==_ALG_AES_SAFE_COMPOSITE_GF_)
#include "pcprij128safe.h"

#if defined _PCP_RIJ_SAFE_OLD
/*
// old version
*/

IPP_OWN_DEFN (Ipp8u, TransformByte, (Ipp8u x, const Ipp8u Transformation[]))
{
   Ipp32u y = 0;

   Ipp32u testBit = 0x01;
   int bit;
   for(bit=0; bit<8; bit++) {
      Ipp32u mask = (x & testBit)? 0xFF : 0;
      y ^= Transformation[bit] & mask;
      testBit <<= 1;
   }

   return (Ipp8u)y;
}

static Ipp8u Native2CompositeTransformation[] = {0x01,0x2E,0x49,0x43,0x35,0xD0,0x3D,0xE9};

IPP_OWN_DEFN (void, TransformNative2Composite, (Ipp8u out[], const Ipp8u inp[]))
{
   int n;
   for(n=0; n<16; n++)
      out[n] = TransformByte(inp[n], Native2CompositeTransformation);
}

static Ipp8u Composite2NativeTransformation[] = {0x01,0x5C,0xE0,0x50,0x1F,0xEE,0x55,0x6A};

IPP_OWN_DEFN (void, TransformComposite2Native, (Ipp8u out[], const Ipp8u inp[]))
{
   int n;
   for(n=0; n<16; n++)
      out[n] = TransformByte(inp[n], Composite2NativeTransformation);
}

IPP_OWN_DEFN (void, AddRoundKey, (Ipp8u out[], const Ipp8u inp[], const Ipp8u pKey[]))
{
   int n;
   for(n=0; n<16; n++)
      out[n] = (Ipp8u)( inp[n] ^ pKey[n] );
}

#define MASK_BIT(x,n)   ((Ipp32s)((x)<<(31-n)) >>31)
#define GF16mulX(x)     ( ((x)<<1) ^ ( MASK_BIT(x,3) & 0x13) )

static Ipp8u GF16mul(Ipp8u a, Ipp8u b)
{
   Ipp32u a0 = a;
   Ipp32u a1 = GF16mulX(a0);
   Ipp32u a2 = GF16mulX(a1);
   Ipp32u a4 = GF16mulX(a2);

   Ipp32u r = (a0 & MASK_BIT(b,0))
             ^(a1 & MASK_BIT(b,1))
             ^(a2 & MASK_BIT(b,2))
             ^(a4 & MASK_BIT(b,3));

   return (Ipp8u)r;
}

static Ipp8u GF16_sqr[] = {0x00,0x01,0x04,0x05,0x03,0x02,0x07,0x06,
                           0x0C,0x0D,0x08,0x09,0x0F,0x0E,0x0B,0x0A};
static Ipp8u GF16_sqr1[]= {0x00,0x09,0x02,0x0B,0x08,0x01,0x0A,0x03,
                           0x06,0x0F,0x04,0x0D,0x0E,0x07,0x0C,0x05};
static Ipp8u GF16_inv[] = {0x00,0x01,0x09,0x0E,0x0D,0x0B,0x07,0x06,
                           0x0F,0x02,0x0C,0x05,0x0A,0x04,0x03,0x08};
IPP_OWN_DEFN (Ipp8u, InverseComposite, (Ipp8u x))
{
   /* split x = {bc} => b*t + c */
   int b = (x>>4) & 0x0F;
   int c = x & 0x0F;

   int D = GF16mul((Ipp8u)b, (Ipp8u)c)
          ^GF16_sqr[c]
          ^GF16_sqr1[b];

   D = GF16_inv[D];

   c = GF16mul((Ipp8u)(b^c), (Ipp8u)D);
   b = GF16mul((Ipp8u)b, (Ipp8u)D);

   /* merge p*t + q => {pq} = x */
   x = (Ipp8u)((b<<4) + c);
   return x;
}
#endif /* _PCP_RIJ_SAFE_OLD */

#if !defined _PCP_RIJ_SAFE_OLD
/*
// new version
*/

/* GF(2^128) -> GF((2^4)^2) isomorphous transformation matrix Ipp8u Native2CompositeTransformation[] = {0x01,0x2E,0x49,0x43,0x35,0xD0,0x3D,0xE9};
   is defined in reference code, see doc for details */
static Ipp8u Native2CompositeTransformationLO[] = { /* defived from Native2CompositeTransformation[i], i=0,1,2,3 */
   /* 0 */      0x00,
   /* 1 */      0x01,
   /* 2 */      0x2E,
   /* 3 */      0x2E^0x01,
   /* 4 */      0x49,
   /* 5 */      0x49^0x01,
   /* 6 */      0x49^0x2E,
   /* 7 */      0x49^0x2E^0x01,
   /* 8 */      0x43,
   /* 9 */      0x43^0x01,
   /* a */      0x43^0x2E,
   /* b */      0x43^0x2E^0x01,
   /* c */      0x43^0x49,
   /* d */      0x43^0x49^0x01,
   /* e */      0x43^0x49^0x2E,
   /* f */      0x43^0x49^0x2E^0x01
};
static Ipp8u Native2CompositeTransformationHI[] = { /* defived from Native2CompositeTransformation[i], i=4,5,6,7 */
   /* 0 */      0x00,
   /* 1 */      0x35,
   /* 2 */      0xD0,
   /* 3 */      0xD0^0x35,
   /* 4 */      0x3D,
   /* 5 */      0x3D^0x35,
   /* 6 */      0x3D^0xD0,
   /* 7 */      0x3D^0xD0^0x35,
   /* 8 */      0xE9,
   /* 9 */      0xE9^0x35,
   /* a */      0xE9^0xD0,
   /* b */      0xE9^0xD0^0x35,
   /* c */      0xE9^0x3D,
   /* d */      0xE9^0x3D^0x35,
   /* e */      0xE9^0x3D^0xD0,
   /* f */      0xE9^0x3D^0xD0^0x35
};
IPP_OWN_DEFN (void, TransformNative2Composite, (Ipp8u out[16], const Ipp8u inp[16]))
{
   Ipp8u blk_lo[16], blk_hi[16];
   ((Ipp64u*)blk_lo)[0] = ((Ipp64u*)inp)[0] & 0x0F0F0F0F0F0F0F0F;
   ((Ipp64u*)blk_lo)[1] = ((Ipp64u*)inp)[1] & 0x0F0F0F0F0F0F0F0F;
   ((Ipp64u*)blk_hi)[0] = (((Ipp64u*)inp)[0]>>4) & 0x0F0F0F0F0F0F0F0F;
   ((Ipp64u*)blk_hi)[1] = (((Ipp64u*)inp)[1]>>4) & 0x0F0F0F0F0F0F0F0F;
   {
      int n;
      for(n=0; n<16; n++) {
         Ipp8u lo = Native2CompositeTransformationLO[blk_lo[n]];
         Ipp8u hi = Native2CompositeTransformationHI[blk_hi[n]];
         out[n] = lo^hi;
      }
   }
}

/* GF((2^4)^2) -> GF(2^128) isomorphous transformation matrix Ipp8u Composite2NativeTransformation[] = {0x01,0x5C,0xE0,0x50,0x1F,0xEE,0x55,0x6A};
   is defined in reference code, see doc for details */
static Ipp8u Composite2NativeTransformationLO[] = { /* defived from Composite2NativeTransformation[i], i=0,1,2,3 */
   /* 0 */      0x00,
   /* 1 */      0x01,
   /* 2 */      0x5C,
   /* 3 */      0x5C^0x01,
   /* 4 */      0xE0,
   /* 5 */      0xE0^0x01,
   /* 6 */      0xE0^0x5C,
   /* 7 */      0xE0^0x5C^0x01,
   /* 8 */      0x50,
   /* 9 */      0x50^0x01,
   /* a */      0x50^0x5C,
   /* b */      0x50^0x5C^0x01,
   /* c */      0x50^0xE0,
   /* d */      0x50^0xE0^0x01,
   /* e */      0x50^0xE0^0x5C,
   /* f */      0x50^0xE0^0x5C^0x01
};
static Ipp8u Composite2NativeTransformationHI[] = {/* defived from Composite2NativeTransformation[i], i=4,5,6,7 */
   /* 0 */      0x00,
   /* 1 */      0x1F,
   /* 2 */      0xEE,
   /* 3 */      0xEE^0x1F,
   /* 4 */      0x55,
   /* 5 */      0x55^0x1F,
   /* 6 */      0x55^0xEE,
   /* 7 */      0x55^0xEE^0x1F,
   /* 8 */      0x6A,
   /* 9 */      0x6A^0x1F,
   /* a */      0x6A^0xEE,
   /* b */      0x6A^0xEE^0x1F,
   /* c */      0x6A^0x55,
   /* d */      0x6A^0x55^0x1F,
   /* e */      0x6A^0x55^0xEE,
   /* f */      0x6A^0x55^0xEE^0x1F
};
IPP_OWN_DEFN (void, TransformComposite2Native, (Ipp8u out[16], const Ipp8u inp[16]))
{
   Ipp8u blk_lo[16], blk_hi[16];
   ((Ipp64u*)blk_lo)[0] = ((Ipp64u*)inp)[0] & 0x0F0F0F0F0F0F0F0F;
   ((Ipp64u*)blk_lo)[1] = ((Ipp64u*)inp)[1] & 0x0F0F0F0F0F0F0F0F;
   ((Ipp64u*)blk_hi)[0] = (((Ipp64u*)inp)[0]>>4) & 0x0F0F0F0F0F0F0F0F;
   ((Ipp64u*)blk_hi)[1] = (((Ipp64u*)inp)[1]>>4) & 0x0F0F0F0F0F0F0F0F;
   {
      int n;
      for(n=0; n<16; n++) {
         Ipp8u lo = Composite2NativeTransformationLO[blk_lo[n]];
         Ipp8u hi = Composite2NativeTransformationHI[blk_hi[n]];
         out[n] = lo^hi;
      }
   }
}
#endif /* !_PCP_RIJ_SAFE_OLD */

#endif /* _ALG_AES_SAFE_COMPOSITE_GF_ */

#endif /* (_IPP <_IPP_V8) && (_IPP32E <_IPP32E_U8) */
