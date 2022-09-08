/*******************************************************************************
* Copyright 2015-2021 Intel Corporation
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
//     (compact S-box based implementation)
// 
//  Contents:
//     Safe2Encrypt_RIJ128()
// 
// 
*/

#include "owncp.h"

#if ((_IPP <_IPP_V8) && (_IPP32E <_IPP32E_U8)) /* no pshufb instruction */

#if (_ALG_AES_SAFE_==_ALG_AES_SAFE_COMPACT_SBOX_)

#include "pcprij128safe.h"
#include "pcprij128safe2.h"
#include "pcprijtables.h"

__INLINE void SubBytes(Ipp8u state[])
{
   int i;
   for(i=0;i<16;i++) {
      state[i] = getSboxValue(state[i]);
   }
}


__INLINE void ShiftRows(Ipp32u* state)
{
   state[1] =  ROR32(state[1], 8);
   state[2] =  ROR32(state[2], 16);
   state[3] =  ROR32(state[3], 24);
}

// MixColumns4 function mixes the columns of the state matrix
__INLINE void MixColumns(Ipp32u* state)
{
   Ipp32u y0 = state[1] ^ state[2] ^ state[3];
   Ipp32u y1 = state[0] ^ state[2] ^ state[3];
   Ipp32u y2 = state[0] ^ state[1] ^ state[3];
   Ipp32u y3 = state[0] ^ state[1] ^ state[2];

   state[0] = xtime4(state[0]);
   state[1] = xtime4(state[1]);
   state[2] = xtime4(state[2]);
   state[3] = xtime4(state[3]);

   y0 ^= state[0] ^ state[1];
   y1 ^= state[1] ^ state[2];
   y2 ^= state[2] ^ state[3];
   y3 ^= state[3] ^ state[0];

   state[0] = y0;
   state[1] = y1;
   state[2] = y2;
   state[3] = y3;
}

IPP_OWN_DEFN (void, Safe2Encrypt_RIJ128, (const Ipp8u* in, Ipp8u* out, int Nr, const Ipp8u* RoundKey, const void* sbox))
{
   Ipp32u state[4];

   int round=0;

   IPP_UNREFERENCED_PARAMETER(sbox);

   // copy input to the state array
   TRANSPOSE((Ipp8u*)state, in);

   // add round key to the state before starting the rounds.
   XorRoundKey(state, (Ipp32u*)(RoundKey+0*16));

   // there will be Nr rounds
   for(round=1;round<Nr;round++) {
      SubBytes((Ipp8u*)state);
      ShiftRows(state);
      MixColumns(state);
      XorRoundKey(state, (Ipp32u*)(RoundKey+round*16));
   }

   // last round
   SubBytes((Ipp8u*)state);
   ShiftRows(state);
   XorRoundKey(state, (Ipp32u*)(RoundKey+Nr*16));

   // copy from the state to output
   TRANSPOSE(out, (Ipp8u*)state);
}
#endif /* _ALG_AES_SAFE_COMPACT_SBOX_ */

#endif /* (_IPP <_IPP_V8) && (_IPP32E <_IPP32E_U8) */
