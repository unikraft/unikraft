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
//     secp256r1               (* Montgomery Friendly Modulus (+1) *)
// 
*/

#include "owndefs.h"
#include "owncp.h"

#include "pcpgfpstuff.h"

#if defined( _IPP_DATA )

/*
// Recommended Parameters secp256r1
*/
const BNU_CHUNK_T h_secp256r1_p[] = { // half of secp256r1_p
   LL(0xFFFFFFFF, 0xFFFFFFFF), LL(0x7FFFFFFF, 0x00000000), LL(0x00000000, 0x80000000),
   LL(0x80000000, 0x7FFFFFFF)};

const BNU_CHUNK_T secp256r1_p[] = { // 2^256 -2^224 +2^192 +2^96 -1
   LL(0xFFFFFFFF, 0xFFFFFFFF), LL(0xFFFFFFFF, 0x00000000), LL(0x00000000, 0x00000000),
   LL(0x00000001, 0xFFFFFFFF), LL(0x0, 0x0)};
const BNU_CHUNK_T secp256r1_a[] = {
   LL(0xFFFFFFFC, 0xFFFFFFFF), LL(0xFFFFFFFF, 0x00000000), LL(0x00000000, 0x00000000),
   LL(0x00000001, 0xFFFFFFFF)};
const BNU_CHUNK_T secp256r1_b[] = {
   LL(0x27D2604B, 0x3BCE3C3E), LL(0xCC53B0F6, 0x651D06B0), LL(0x769886BC, 0xB3EBBD55),
   LL(0xAA3A93E7, 0x5AC635D8)};
const BNU_CHUNK_T secp256r1_gx[] = {
   LL(0xD898C296, 0xF4A13945), LL(0x2DEB33A0, 0x77037D81), LL(0x63A440F2, 0xF8BCE6E5),
   LL(0xE12C4247, 0x6B17D1F2)};
const BNU_CHUNK_T secp256r1_gy[] = {
   LL(0x37BF51F5, 0xCBB64068), LL(0x6B315ECE, 0x2BCE3357), LL(0x7C0F9E16, 0x8EE7EB4A),
   LL(0xFE1A7F9B, 0x4FE342E2)};
const BNU_CHUNK_T secp256r1_r[] = {
   LL(0xFC632551, 0xF3B9CAC2), LL(0xA7179E84, 0xBCE6FAAD), LL(0xFFFFFFFF, 0xFFFFFFFF),
   LL(0x00000000, 0xFFFFFFFF)};
BNU_CHUNK_T secp256r1_h = 1;

#endif /* _IPP_DATA */
