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
//     Internal Safe Rijndael Encrypt, Decrypt
// 
// 
*/

#if !defined(_PCP_RIJ_SAFE2_H)
#define _PCP_RIJ_SAFE2_H

// transpose 4x4 Ipp8u matrix
#define TRANSPOSE(out, inp) \
   (out)[ 0] = (inp)[ 0]; \
   (out)[ 4] = (inp)[ 1]; \
   (out)[ 8] = (inp)[ 2]; \
   (out)[12] = (inp)[ 3]; \
   \
   (out)[ 1] = (inp)[ 4]; \
   (out)[ 5] = (inp)[ 5]; \
   (out)[ 9] = (inp)[ 6]; \
   (out)[13] = (inp)[ 7]; \
   \
   (out)[ 2] = (inp)[ 8]; \
   (out)[ 6] = (inp)[ 9]; \
   (out)[10] = (inp)[10]; \
   (out)[14] = (inp)[11]; \
   \
   (out)[ 3] = (inp)[12]; \
   (out)[ 7] = (inp)[13]; \
   (out)[11] = (inp)[14]; \
   (out)[15] = (inp)[15]

__INLINE void XorRoundKey(Ipp32u* state, const Ipp32u* RoundKey)
{
   state[0] ^= RoundKey[0];
   state[1] ^= RoundKey[1];
   state[2] ^= RoundKey[2];
   state[3] ^= RoundKey[3];
}

// xtime is a macro that finds the product of {02} and the argument to xtime modulo {1b}
__INLINE Ipp32u mask4(Ipp32u x)
{
   x &= 0x80808080;
   return (Ipp32u)((x<<1) - (x>>7));
}

__INLINE Ipp32u xtime4(Ipp32u x)
{
   Ipp32u t = (x+x) &0xFEFEFEFE;
   t ^= mask4(x) & 0x1B1B1B1B;
   return t;
}

#endif /* _PCP_RIJ_SAFE2_H */
