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
//     DES Cipher function (MemJam mitigation incuded)
// 
//  Contents:
//     initial permutation: ip()
//     final permutation: fp()
//     round function: rndm()
//     DES block encypt/decrypt: Chipher_DES()
// 
// 
*/


#include "owndefs.h"
#include "owncp.h"
#include "pcpdes.h"
#include "pcpmask_ct.h"

/*
// Following implementation of ip(), fp() and rndm()
// assumes input and output of DES as sequence of bytes.
*/
#define ip(x,y) {                \
    Ipp32u t; \
    (x) = ROR32((x), 4);           \
    (t) = ((x)^(y)) & 0x0f0f0f0fL; \
    (y)^= (t);                     \
    (x) = ROR32((x)^(t), 12);      \
    (t) = ((y)^(x)) & 0xffff0000L; \
    (y)^= (t);                     \
    (x) = ROR32((x)^(t), 18);      \
    (t) = ((x)^(y)) & 0x33333333L; \
    (y)^= (t);                     \
    (x) = ROR32((x)^(t), 22);      \
    (t) = ((y)^(x)) & 0xff00ff00L; \
    (y)^= (t);                     \
    (x) = ROR32((x)^(t),  9);      \
    (t) = ((x)^(y)) & 0x55555555L; \
    (x) = ROL32((x)^(t), 2);       \
    (y) = ROL32((y)^(t), 1); \
}

#define fp(x,y) {                \
    Ipp32u t; \
    (y) = ROR32((y), 1);           \
    (x) = ROR32((x), 2);           \
    (t) = ((x)^(y)) & 0x55555555L; \
    (y)^= (t);                     \
    (x) = ROL32((x)^(t),  9);      \
    (t) = ((y)^(x)) & 0xff00ff00L; \
    (y)^= (t);                     \
    (x) = ROL32((x)^(t), 22);      \
    (t) = ((x)^(y)) & 0x33333333L; \
    (y)^= (t);                     \
    (x) = ROL32((x)^(t), 18);      \
    (t) = ((y)^(x)) & 0xffff0000L; \
    (y)^= (t);                     \
    (x) = ROL32((x)^(t), 12);      \
    (t) = ((x)^(y)) & 0x0f0f0f0fL; \
    (y)^= (t);                     \
    (x) = ROL32((x)^(t), 4); \
}


static BNU_CHUNK_T getSbox(const BNU_CHUNK_T sbox[], int idx)
{
   unsigned int i;
   BNU_CHUNK_T res = 0;
   for(i=0; i<(64/sizeof(BNU_CHUNK_T)); i++) {
      BNU_CHUNK_T mask = cpIsEqu_ct(i, (BNU_CHUNK_T)idx);
      res |= sbox[i] & mask;
   }
   return res;
}
static Ipp8u getSbox8u(const Ipp8u sbox[], int idx)
{
   int shift = idx % (Ipp32s)((sizeof(BNU_CHUNK_T))/sizeof(Ipp8u));
   idx /= ((sizeof(BNU_CHUNK_T))/sizeof(Ipp8u));
   return (Ipp8u)( getSbox((BNU_CHUNK_T*)sbox, idx) >>(shift*8) );
}
static Ipp32u getSbox32u(const Ipp8u sbox[], int idx)
{
   int shift = idx % (Ipp32s)((sizeof(BNU_CHUNK_T))/sizeof(Ipp32u));
   idx /= ((sizeof(BNU_CHUNK_T))/sizeof(Ipp32u));
   return (Ipp32u)( getSbox((BNU_CHUNK_T*)sbox, idx)>>shift*32 );
}

static Ipp32u rndm(Ipp32u x0, Ipp32u x1, Ipp32u* key, const Ipp8u* sbox)
{
   Ipp32u
   tt = key[0] ^ (x1 &0x3F3F3F3F);
   x0 ^= getSbox32u(sbox+512+64*0, getSbox8u(sbox+64*0, (Ipp8u)tt));
   tt >>= 8;
   x0 ^= getSbox32u(sbox+512+64*2, getSbox8u(sbox+64*2, (Ipp8u)tt));
   tt >>= 8;
   x0 ^= getSbox32u(sbox+512+64*4, getSbox8u(sbox+64*4, (Ipp8u)tt));
   tt >>= 8;
   x0 ^= getSbox32u(sbox+512+64*6, getSbox8u(sbox+64*6, (Ipp8u)tt));

   tt = (key)[1] ^ (ROR32((x1 &0xF3F3F3F3),4));
   x0 ^= getSbox32u(sbox+512+64*1, getSbox8u(sbox+64*1, (Ipp8u)tt));
   tt >>= 8;
   x0 ^= getSbox32u(sbox+512+64*3, getSbox8u(sbox+64*3, (Ipp8u)tt));
   tt >>= 8;
   x0 ^= getSbox32u(sbox+512+64*5, getSbox8u(sbox+64*5, (Ipp8u)tt));
   tt >>= 8;
   x0 ^= getSbox32u(sbox+512+64*7, getSbox8u(sbox+64*7, (Ipp8u)tt));

   return x0;
}

IPP_OWN_DEFN (Ipp64u, Cipher_DES, (Ipp64u inpBlk, const RoundKeyDES* pRKey, const Ipp32u sbox[]))
{
   const Ipp8u* sbox8 = (const Ipp8u*)sbox;

   #if (IPP_ENDIAN == IPP_BIG_ENDIAN)
   Ipp32u q0 = IPP_LODWORD(inpBlk);
   Ipp32u q1 = IPP_HIDWORD(inpBlk);
   q0 = ENDIANNESS(q0);
   q1 = ENDIANNESS(q1);
   #else
   Ipp32u q0 = IPP_HIDWORD(inpBlk);
   Ipp32u q1 = IPP_LODWORD(inpBlk);
   #endif

   /* apply inverse permutation IP */
   ip(q0,q1);

   /* 16 magic encrypt iterations */
   q0 = rndm(q0,q1, ((HalfRoundKeyDES*)(pRKey)),    sbox8);
   q1 = rndm(q1,q0, ((HalfRoundKeyDES*)(pRKey+1 )), sbox8);
   q0 = rndm(q0,q1, ((HalfRoundKeyDES*)(pRKey+2 )), sbox8);
   q1 = rndm(q1,q0, ((HalfRoundKeyDES*)(pRKey+3 )), sbox8);
   q0 = rndm(q0,q1, ((HalfRoundKeyDES*)(pRKey+4 )), sbox8);
   q1 = rndm(q1,q0, ((HalfRoundKeyDES*)(pRKey+5 )), sbox8);
   q0 = rndm(q0,q1, ((HalfRoundKeyDES*)(pRKey+6 )), sbox8);
   q1 = rndm(q1,q0, ((HalfRoundKeyDES*)(pRKey+7 )), sbox8);
   q0 = rndm(q0,q1, ((HalfRoundKeyDES*)(pRKey+8 )), sbox8);
   q1 = rndm(q1,q0, ((HalfRoundKeyDES*)(pRKey+9 )), sbox8);
   q0 = rndm(q0,q1, ((HalfRoundKeyDES*)(pRKey+10)), sbox8);
   q1 = rndm(q1,q0, ((HalfRoundKeyDES*)(pRKey+11)), sbox8);
   q0 = rndm(q0,q1, ((HalfRoundKeyDES*)(pRKey+12)), sbox8);
   q1 = rndm(q1,q0, ((HalfRoundKeyDES*)(pRKey+13)), sbox8);
   q0 = rndm(q0,q1, ((HalfRoundKeyDES*)(pRKey+14)), sbox8);
   q1 = rndm(q1,q0, ((HalfRoundKeyDES*)(pRKey+15)), sbox8);

   /* apply forward permutation FP */
   fp(q1,q0);

   #if (IPP_ENDIAN == IPP_BIG_ENDIAN)
   q0 = ENDIANNESS(q0);
   q1 = ENDIANNESS(q1);
   return IPP_MAKEDWORD(q1,q0);
   #else
   return IPP_MAKEDWORD(q0,q1);
   #endif
}
