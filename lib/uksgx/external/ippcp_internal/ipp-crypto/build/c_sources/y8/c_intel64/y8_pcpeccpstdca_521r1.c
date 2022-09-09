/*******************************************************************************
* Copyright 2020-2021 Intel Corporation
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
//     ECC over Prime Finite Field (recommended ECC parameters)
// 
//  Contents:
//     secp521r1               (* Montgomery Friendly Modulus (+1) *)
// 
*/

#include "owndefs.h"
#include "owncp.h"

#include "pcpgfpstuff.h"

#if defined( _IPP_DATA )

/*
// Recommended Parameters secp521r1
*/
const BNU_CHUNK_T h_secp521r1_p[] = { // half of secp521r1_p
   LL(0xFFFFFFFF, 0xFFFFFFFF), LL(0xFFFFFFFF, 0xFFFFFFFF), LL(0xFFFFFFFF, 0xFFFFFFFF),
   LL(0xFFFFFFFF, 0xFFFFFFFF), LL(0xFFFFFFFF, 0xFFFFFFFF), LL(0xFFFFFFFF, 0xFFFFFFFF),
   LL(0xFFFFFFFF, 0xFFFFFFFF), LL(0xFFFFFFFF, 0xFFFFFFFF), L_(0x000000FF)};

const BNU_CHUNK_T secp521r1_p[] = { // 2^521 -1
   LL(0xFFFFFFFF, 0xFFFFFFFF), LL(0xFFFFFFFF, 0xFFFFFFFF), LL(0xFFFFFFFF, 0xFFFFFFFF),
   LL(0xFFFFFFFF, 0xFFFFFFFF), LL(0xFFFFFFFF, 0xFFFFFFFF), LL(0xFFFFFFFF, 0xFFFFFFFF),
   LL(0xFFFFFFFF, 0xFFFFFFFF), LL(0xFFFFFFFF, 0xFFFFFFFF), L_(0x000001FF)};
const BNU_CHUNK_T secp521r1_a[] = {
   LL(0xFFFFFFFC, 0xFFFFFFFF), LL(0xFFFFFFFF, 0xFFFFFFFF), LL(0xFFFFFFFF, 0xFFFFFFFF),
   LL(0xFFFFFFFF, 0xFFFFFFFF), LL(0xFFFFFFFF, 0xFFFFFFFF), LL(0xFFFFFFFF, 0xFFFFFFFF),
   LL(0xFFFFFFFF, 0xFFFFFFFF), LL(0xFFFFFFFF, 0xFFFFFFFF), L_(0x000001FF)};
const BNU_CHUNK_T secp521r1_b[] = {
   LL(0x6B503F00, 0xEF451FD4), LL(0x3D2C34F1, 0x3573DF88), LL(0x3BB1BF07, 0x1652C0BD),
   LL(0xEC7E937B, 0x56193951), LL(0x8EF109E1, 0xB8B48991), LL(0x99B315F3, 0xA2DA725B),
   LL(0xB68540EE, 0x929A21A0), LL(0x8E1C9A1F, 0x953EB961), L_(0x00000051)};
const BNU_CHUNK_T secp521r1_gx[] = {
   LL(0xC2E5BD66, 0xF97E7E31), LL(0x856A429B, 0x3348B3C1), LL(0xA2FFA8DE, 0xFE1DC127),
   LL(0xEFE75928, 0xA14B5E77), LL(0x6B4D3DBA, 0xF828AF60), LL(0x053FB521, 0x9C648139),
   LL(0x2395B442, 0x9E3ECB66), LL(0x0404E9CD, 0x858E06B7), L_(0x000000C6)};
const BNU_CHUNK_T secp521r1_gy[] = {
   LL(0x9FD16650, 0x88BE9476), LL(0xA272C240, 0x353C7086), LL(0x3FAD0761, 0xC550B901),
   LL(0x5EF42640, 0x97EE7299), LL(0x273E662C, 0x17AFBD17), LL(0x579B4468, 0x98F54449),
   LL(0x2C7D1BD9, 0x5C8A5FB4), LL(0x9A3BC004, 0x39296A78), L_(0x00000118)};
const BNU_CHUNK_T secp521r1_r[] = {
   LL(0x91386409, 0xBB6FB71E), LL(0x899C47AE, 0x3BB5C9B8), LL(0xF709A5D0, 0x7FCC0148),
   LL(0xBF2F966B, 0x51868783), LL(0xFFFFFFFA, 0xFFFFFFFF), LL(0xFFFFFFFF, 0xFFFFFFFF),
   LL(0xFFFFFFFF, 0xFFFFFFFF), LL(0xFFFFFFFF, 0xFFFFFFFF), L_(0x000001FF)};
BNU_CHUNK_T secp521r1_h = 1;

#endif /* _IPP_DATA */
