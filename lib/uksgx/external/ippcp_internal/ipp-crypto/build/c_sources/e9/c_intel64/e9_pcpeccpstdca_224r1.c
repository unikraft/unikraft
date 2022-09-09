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
//     secp224r1               (* Montgomery Friendly Modulus (-1) *)
// 
*/

#include "owndefs.h"
#include "owncp.h"

#include "pcpgfpstuff.h"

#if defined( _IPP_DATA )

/*
// Recommended Parameters secp224r1
*/
const BNU_CHUNK_T h_secp224r1_p[] = { // half of secp224r1_p
   LL(0x00000000, 0x00000000), LL(0x80000000, 0xFFFFFFFF), LL(0xFFFFFFFF, 0xFFFFFFFF),
   LL(0x7FFFFFFF, 0x0)};

const BNU_CHUNK_T secp224r1_p[] = { // 2^224 -2^96 +1
   LL(0x00000001, 0x00000000), LL(0x00000000, 0xFFFFFFFF), LL(0xFFFFFFFF, 0xFFFFFFFF),
   LL(0xFFFFFFFF, 0x0)};
const BNU_CHUNK_T secp224r1_a[] = {
   LL(0xFFFFFFFE, 0xFFFFFFFF), LL(0xFFFFFFFF, 0xFFFFFFFE), LL(0xFFFFFFFF, 0xFFFFFFFF),
   L_(0xFFFFFFFF)};
const BNU_CHUNK_T secp224r1_b[] = {
   LL(0x2355FFB4, 0x270B3943), LL(0xD7BFD8BA, 0x5044B0B7), LL(0xF5413256, 0x0C04B3AB),
   L_(0xB4050A85)};
const BNU_CHUNK_T secp224r1_gx[] = {
   LL(0x115C1D21, 0x343280D6), LL(0x56C21122, 0x4A03C1D3), LL(0x321390B9, 0x6BB4BF7F),
   L_(0xB70E0CBD)};
const BNU_CHUNK_T secp224r1_gy[] = {
   LL(0x85007E34, 0x44D58199), LL(0x5A074764, 0xCD4375A0), LL(0x4C22DFE6, 0xB5F723FB),
   L_(0xBD376388)};
const BNU_CHUNK_T secp224r1_r[] = {
   LL(0x5C5C2A3D, 0x13DD2945), LL(0xE0B8F03E, 0xFFFF16A2), LL(0xFFFFFFFF, 0xFFFFFFFF),
   L_(0xFFFFFFFF)};
BNU_CHUNK_T secp224r1_h = 1;

#endif /* _IPP_DATA */
