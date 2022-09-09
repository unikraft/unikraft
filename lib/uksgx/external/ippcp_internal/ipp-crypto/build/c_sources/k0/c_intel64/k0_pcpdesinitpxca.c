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
//     Set Round Keys for DES/TDES
// 
//  Contents:
//     SetKey_DES()
//     ippcSetKey_DES()
// 
// 
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpdes.h"


/*
// bit 0 is left-most in byte (reference FIPS 46-3)
*/
static int bytebit[] = {
    0x80,0x40,0x20,0x10,0x8,0x4,0x2,0x1
};

/*
// Key schedule-related tables (reference FIPS 46-3)
*/

/* PC-1 permuted table (for the user key) */
static Ipp8u pc1[] = {
    57, 49, 41, 33, 25, 17,  9,
     1, 58, 50, 42, 34, 26, 18,
    10,  2, 59, 51, 43, 35, 27,
    19, 11,  3, 60, 52, 44, 36,

    63, 55, 47, 39, 31, 23, 15,
     7, 62, 54, 46, 38, 30, 22,
    14,  6, 61, 53, 45, 37, 29,
    21, 13,  5, 28, 20, 12,  4
};

/* number left rotations of PC-1 key */
static int rotations[] = {
    1,2,4,6,8,10,12,14,15,17,19,21,23,25,27,28
};

/* PC-2 table (for round key constuction) */
static Ipp8u pc2[] = {
    14, 17, 11, 24,  1,  5,
     3, 28, 15,  6, 21, 10,
    23, 19, 12,  4, 26,  8,
    16,  7, 27, 20, 13,  2,
    41, 52, 31, 37, 47, 55,
    30, 40, 51, 45, 33, 48,
    44, 49, 39, 56, 34, 53,
    46, 42, 50, 36, 29, 32
};

#define SWAP8U(a,b) { Ipp8u x=(a); (a)=(b); (b)=x; }
/*
// Generate key schedule for encryption and decryption
*/
IPP_OWN_DEFN (void, SetKey_DES, (const Ipp8u* pKey, IppsDESSpec* pCtx))
{
   RoundKeyDES* pEncRoundKey = DES_EKEYS(pCtx);
   RoundKeyDES* pDecRoundKey = DES_DKEYS(pCtx);

   __ALIGN64 Ipp8u pc1key[56];        /* PC-1 permuted user key bits */
   __ALIGN64 Ipp8u rndkey[56];        /* rounded key bits */

   int nkey;
   int i;

   /*
   // apply permutation PC-1
   */
   for(i=0; i<56; i++) {
      /* location into the user key (bit number) */
      int nBit = pc1[i]-1;
      /* location into the user key (byte and offset in the byte) */
      int nByte  = nBit>>3;
      int offset = nBit & 0x07;

      /* test bit in the user key and set into the permuted */
      pc1key[i] = (Ipp8u)( (pKey[nByte] & bytebit[offset] ) >>(7-offset));
   }

   /*
   // key schedule for encryption
   */
   for(nkey=0; nkey<16; nkey++) {
      Ipp64u tmp = 0;
      Ipp8u* rkeyNibble = (Ipp8u*)(&tmp);

      int pc1keyBit, pc1keyBit_m28;
      Ipp32u mask;

      /* rotate right pc1key */
      for (i=0; i<28; i++) {
         pc1keyBit = i+rotations[nkey];
         pc1keyBit_m28 = pc1keyBit-28;
         mask = (Ipp32u)(pc1keyBit_m28>>(BITSIZE(Ipp32u)-1));
         pc1keyBit = (Ipp32s)((Ipp32u)pc1keyBit & mask) | (Ipp32s)((Ipp32u)pc1keyBit_m28 &~mask);
         rndkey[i] = pc1key[pc1keyBit];
      }
      for (; i<56; i++) {
         pc1keyBit = i+rotations[nkey];
         pc1keyBit_m28 = pc1keyBit-28;
         mask = (Ipp32u)((pc1keyBit-56)>>(BITSIZE(Ipp32u)-1));
         pc1keyBit = (Ipp32s)((Ipp32u)pc1keyBit & mask) | (Ipp32s)((Ipp32u)pc1keyBit_m28 &~mask);
         rndkey[i] = pc1key[pc1keyBit];
      }

      /*
      // construct eight 6-bit nibbles of round key
      // applying PC-2 permutation to rndkey[]
      */
      for(i=0; i<48; i++) {
         int offset = i%6;
         rkeyNibble[i/6] |= (rndkey[pc2[i]-1]<<offset);
      }

      /* re-order rkey for Cipher_DES() implementation matching */
      SWAP8U(rkeyNibble[1], rkeyNibble[2]);
      SWAP8U(rkeyNibble[5], rkeyNibble[6]);
      SWAP8U(rkeyNibble[5], rkeyNibble[3]);
      SWAP8U(rkeyNibble[4], rkeyNibble[2]);

      /* remember tmp and rkeyNibble are aliases */
      #if (IPP_ENDIAN == IPP_BIG_ENDIAN)
      tmp = IPP_MAKEDWORD(ENDIANNESS(IPP_LODWORD(tmp)), ENDIANNESS(IPP_HIDWORD(tmp)));
      #endif
      pEncRoundKey[nkey] = tmp;
   }

   /*
   // key schedule for decription (just copy in reverse order)
   */
   for(nkey=0; nkey<16; nkey++)
      pDecRoundKey[nkey] = pEncRoundKey[16-nkey-1];
}
