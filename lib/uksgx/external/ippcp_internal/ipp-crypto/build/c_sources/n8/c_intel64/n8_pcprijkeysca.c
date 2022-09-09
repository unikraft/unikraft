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
//     Initialization of Rijndael
// 
//  Contents:
//     EncRijndaelKeys()
//     DecRijndaelKeys()
// 
// 
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcprij.h"
#include "pcprijtables.h"
#include "pcptool.h"
#include "pcprij128safe.h"
#include "pcprij128safe2.h"
/*
// Pseudo Code for Key Expansion
// was shown in Sec 5.2 of FIPS-197
//
// KeyExpansion(byte key[4*Nk], word w[Nb*(Nr+1)], Nk)
// begin
//    word temp
//
//    i = 0
//
//    while (i < Nk)
//       w[i] = word(key[4*i], key[4*i+1], key[4*i+2], key[4*i+3])
//       i = i+1
//    end while
//
//    i = Nk
//
//    while (i < Nb * (Nr+1)]
//       temp = w[i-1]
//       if (i mod Nk = 0)
//          temp = SubWord(RotWord(temp)) xor Rcon[i/Nk]
//       else if (Nk > 6 and i mod Nk = 4)
//          temp = SubWord(temp)
//       end if
//       w[i] = w[i-Nk] xor temp
//       i = i + 1
//    end while
// end
//
// Note:
//    I see nothing any reason for optimizing reference code above
//    because it run once for each encryption/decryption procedure.
//
//
// We are going to use so called Equivalent Inverse Cipher.
// Look the reason are in pcaesdecryptpxca.c.
//
// For the Equivalent Inverse Cipher, the following pseudo code is added at
// the end of the Key Expansion routine (Sec. 5.2):
//
// for i = 0 step 1 to (Nr+1)*Nb-1
//    dw[i] = w[i]
// end for
//
// for round = 1 step 1 to Nr-1
//    InvMixColumns(dw[round*Nb, (round+1)*Nb-1]) // note change of type
// end for
//
// Note that, since InvMixColumns operates on a two-dimensional array of bytes
// while the Round Keys are held in an array of words, the call to
// InvMixColumns in this code sequence involves a change of type (i.e. the
// input to InvMixColumns() is normally the State array, which is considered
// to be a two-dimensional array of bytes, whereas the input here is a Round
// Key computed as a one-dimensional array of words).
//
//
//    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//    Brief Consideration of InvMixColumn()
//    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Let words U and V are input and output of InvMixColumn() operation.
// Let U(0), U(1), U(2) and U(3) are bytes of word U.
// And V(0), V(1), V(2) and V(3) are bytes of word V.
//
// According to sec 5.3.3. of FIPS-197
//    V(0) = {E}U(0) xor {B}U(1) xor {D}U(2) xor {9}U(3)
//    V(1) = {9}U(0) xor {E}U(1) xor {B}U(2) xor {D}U(3)
//    V(2) = {D}U(0) xor {9}U(1) xor {E}U(2) xor {B}U(3)
//    V(3) = {B}U(0) xor {D}U(1) xor {9}U(2) xor {E}U(3)
// where {hex}U(n) means GF(256) multiplication
//
// Or
//    V = word( {E}U(0), {9}U(0), {D}U(0), {B}U(0) ) xor
//        word( {B}U(1), {E}U(1), {9}U(1), {D}U(1) ) xor
//        word( {D}U(2), {B}U(2), {E}U(2), {9}U(2) ) xor
//        word( {9}U(3), {D}U(3), {B}U(3), {E}U(3) )
//
// Word values
//    word( {E}x, {9}x, {D}x, {B}x )
//    word( {B}y, {E}y, {9}y, {D}y )
//    word( {D}z, {B}z, {E}z, {9}z )
//    word( {9}t, {D}t, {B}t, {E}t )
// are precomputed tables (for x,y,z,t = 0x00, ... 0xff)
//
// Tables InvMixCol_Tbl[4] are contents exactly as we want
// and macro InvMixColumn() provide necassary operation.
*/

/*
// RconTbl[] contains [x**(i),{00},{00},{00}], i=0,..,10 GF(256)
//
// Note:
//    Reference sec 4.2 of FIPS-197 for calculation
*/
static const Ipp32u RconTbl[] = {
   BYTE0_TO_WORD(0x01), BYTE0_TO_WORD(0x02), BYTE0_TO_WORD(0x04), BYTE0_TO_WORD(0x08),
   BYTE0_TO_WORD(0x10), BYTE0_TO_WORD(0x20), BYTE0_TO_WORD(0x40), BYTE0_TO_WORD(0x80),
   BYTE0_TO_WORD(0x1B), BYTE0_TO_WORD(0x36), BYTE0_TO_WORD(0x6C), BYTE0_TO_WORD(0xD8),
   BYTE0_TO_WORD(0xAB), BYTE0_TO_WORD(0x4D), BYTE0_TO_WORD(0x9A), BYTE0_TO_WORD(0x2F),
   BYTE0_TO_WORD(0x5E), BYTE0_TO_WORD(0xBC), BYTE0_TO_WORD(0x63), BYTE0_TO_WORD(0xC6),
   BYTE0_TO_WORD(0x97), BYTE0_TO_WORD(0x35), BYTE0_TO_WORD(0x6A), BYTE0_TO_WORD(0xD4),
   BYTE0_TO_WORD(0xB3), BYTE0_TO_WORD(0x7D), BYTE0_TO_WORD(0xFA), BYTE0_TO_WORD(0xEF),
   BYTE0_TO_WORD(0xC5)
};

/// commented due to mitigation
//
///* precomputed table for InvMixColumn() operation */ 
//static const Ipp32u InvMixCol_Tbl[4][256] = {
//   { LINE(inv_t0) },
//   { LINE(inv_t1) },
//   { LINE(inv_t2) },
//   { LINE(inv_t3) }
//};
//
//#define InvMixColumn(x, tbl) \
//   ( (tbl)[0][ EBYTE((x),0) ] \
//    ^(tbl)[1][ EBYTE((x),1) ] \
//    ^(tbl)[2][ EBYTE((x),2) ] \
//    ^(tbl)[3][ EBYTE((x),3) ] )

__INLINE Ipp32u InvMixColumn(Ipp32u x)
{
  Ipp32u x_mul_2 = xtime4(x);
  Ipp32u x_mul_4 = xtime4(x_mul_2);
  Ipp32u x_mul_8 = xtime4(x_mul_4);

  Ipp32u x_mul_9 = x_mul_8 ^ x;
  Ipp32u x_mul_B = x_mul_8 ^ x_mul_2 ^ x;
  Ipp32u x_mul_D = x_mul_8 ^ x_mul_4 ^ x;
  Ipp32u x_mul_E = x_mul_8 ^ x_mul_4 ^ x_mul_2;

  x = x_mul_E ^ ROR32(x_mul_B, 8) ^ ROR32(x_mul_D, 16) ^ ROR32(x_mul_9, 24);
  return x;
}

/*
// Expansion of key for Rijndael's Encryption
*/
IPP_OWN_DEFN (void, ExpandRijndaelKey, (const Ipp8u* pKey, int NK, int NB, int NR, int nKeys, Ipp8u* pEncKeys, Ipp8u* pDecKeys))
{
   Ipp32u* enc_keys = (Ipp32u*)pEncKeys;
   Ipp32u* dec_keys = (Ipp32u*)pDecKeys;
   /* convert security key to WORD and save into the enc_key array */
   int n;
   for(n=0; n<NK; n++)
      enc_keys[n] = BYTES_TO_WORD(pKey[4*n+0], pKey[4*n+1], pKey[4*n+2], pKey[4*n+3]);

   /* 128-bits Key */
   if(NK128 == NK) {
      const Ipp32u* rtbl = RconTbl;
      Ipp32u k0 = enc_keys[0];
      Ipp32u k1 = enc_keys[1];
      Ipp32u k2 = enc_keys[2];
      Ipp32u k3 = enc_keys[3];

      for(n=NK128; n<nKeys; n+=NK128) {
         /* key expansion: extract bytes, substitute via Sbox and rorate */
         k0 ^= BYTES_TO_WORD(getSboxValue(EBYTE(k3, 1)),
                             getSboxValue(EBYTE(k3, 2)),
                             getSboxValue(EBYTE(k3, 3)),
                             getSboxValue(EBYTE(k3, 0))) ^ *rtbl++;
         k1 ^= k0;
         k2 ^= k1;
         k3 ^= k2;

         /* add key expansion */
         enc_keys[n  ] = k0;
         enc_keys[n+1] = k1;
         enc_keys[n+2] = k2;
         enc_keys[n+3] = k3;
      }
   }

   /* 192-bits Key */
   else if(NK192 == NK) {
      const Ipp32u* rtbl = RconTbl;
      Ipp32u k0 = enc_keys[0];
      Ipp32u k1 = enc_keys[1];
      Ipp32u k2 = enc_keys[2];
      Ipp32u k3 = enc_keys[3];
      Ipp32u k4 = enc_keys[4];
      Ipp32u k5 = enc_keys[5];

      for(n=NK192; n<nKeys; n+=NK192) {
         /* key expansion: extract bytes, substitute via Sbox and rorate */
         k0 ^= BYTES_TO_WORD(getSboxValue(EBYTE(k5, 1)),
                             getSboxValue(EBYTE(k5, 2)),
                             getSboxValue(EBYTE(k5, 3)),
                             getSboxValue(EBYTE(k5, 0))) ^ *rtbl++;
         k1 ^= k0;
         k2 ^= k1;
         k3 ^= k2;
         k4 ^= k3;
         k5 ^= k4;

         /* add key expansion */
         enc_keys[n  ] = k0;
         enc_keys[n+1] = k1;
         enc_keys[n+2] = k2;
         enc_keys[n+3] = k3;
         enc_keys[n+4] = k4;
         enc_keys[n+5] = k5;
      }
   }

   /* 256-bits Key */
   else {
      const Ipp32u* rtbl = RconTbl;
      Ipp32u k0 = enc_keys[0];
      Ipp32u k1 = enc_keys[1];
      Ipp32u k2 = enc_keys[2];
      Ipp32u k3 = enc_keys[3];
      Ipp32u k4 = enc_keys[4];
      Ipp32u k5 = enc_keys[5];
      Ipp32u k6 = enc_keys[6];
      Ipp32u k7 = enc_keys[7];

      for(n=NK256; n<nKeys; n+=NK256) {
         /* key expansion: extract bytes, substitute via Sbox and rorate */
         k0 ^= BYTES_TO_WORD(getSboxValue(EBYTE(k7, 1)),
                             getSboxValue(EBYTE(k7, 2)),
                             getSboxValue(EBYTE(k7, 3)),
                             getSboxValue(EBYTE(k7, 0))) ^ *rtbl++;
         k1 ^= k0;
         k2 ^= k1;
         k3 ^= k2;
         k4 ^= BYTES_TO_WORD(getSboxValue(EBYTE(k3, 0)),
                             getSboxValue(EBYTE(k3, 1)),
                             getSboxValue(EBYTE(k3, 2)),
                             getSboxValue(EBYTE(k3, 3)));
         k5 ^= k4;
         k6 ^= k5;
         k7 ^= k6;

         /* add key expansion */
         enc_keys[n  ] = k0;
         enc_keys[n+1] = k1;
         enc_keys[n+2] = k2;
         enc_keys[n+3] = k3;
         enc_keys[n+4] = k4;
         enc_keys[n+5] = k5;
         enc_keys[n+6] = k6;
         enc_keys[n+7] = k7;
      }
   }


   /*
   // Key Expansion for Decryption
   */
   /* copy keys */
   CopyBlock(enc_keys, dec_keys, (Ipp32s)sizeof(Ipp32u)*nKeys);

   /* update decryption keys */
   for(n=NB; n<NR*NB; n++) {
      #if ((_IPP>=_IPP_W7) || (_IPP32E==_IPP32E_M7))
      _mm_lfence(); /* lfence added because of potential exploit of speculative execution (KW); lfence accessible on SSE2 and above */
      #endif
      dec_keys[n] = InvMixColumn(dec_keys[n]);
   }
}
