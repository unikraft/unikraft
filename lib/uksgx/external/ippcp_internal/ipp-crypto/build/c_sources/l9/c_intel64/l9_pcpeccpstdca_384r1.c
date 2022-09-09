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
//     secp384r1               (* Montgomery Friendly Modulus (0x0000000100000001) *)
// 
*/

#include "owndefs.h"
#include "owncp.h"

#include "pcpgfpstuff.h"

#if defined( _IPP_DATA )

/*
// Recommended Parameters secp384r1
*/
const BNU_CHUNK_T h_secp384r1_p[] = { // half of secp384r1_p
   LL(0x7FFFFFFF, 0x00000000), LL(0x80000000, 0x7FFFFFFF), LL(0xFFFFFFFF, 0xFFFFFFFF),
   LL(0xFFFFFFFF, 0xFFFFFFFF), LL(0xFFFFFFFF, 0xFFFFFFFF), LL(0xFFFFFFFF, 0x7FFFFFFF)};

const BNU_CHUNK_T secp384r1_p[] = { // 2^384 -2^128 -2^96 +2^32 -1
   LL(0xFFFFFFFF, 0x00000000), LL(0x00000000, 0xFFFFFFFF), LL(0xFFFFFFFE, 0xFFFFFFFF),
   LL(0xFFFFFFFF, 0xFFFFFFFF), LL(0xFFFFFFFF, 0xFFFFFFFF), LL(0xFFFFFFFF, 0xFFFFFFFF),
   LL(0x0, 0x0)};
const BNU_CHUNK_T secp384r1_a[] = {
   LL(0xFFFFFFFC, 0x00000000), LL(0x00000000, 0xFFFFFFFF), LL(0xFFFFFFFE, 0xFFFFFFFF),
   LL(0xFFFFFFFF, 0xFFFFFFFF), LL(0xFFFFFFFF, 0xFFFFFFFF), LL(0xFFFFFFFF, 0xFFFFFFFF)};
const BNU_CHUNK_T secp384r1_b[] = {
   LL(0xD3EC2AEF, 0x2A85C8ED), LL(0x8A2ED19D, 0xC656398D), LL(0x5013875A, 0x0314088F),
   LL(0xFE814112, 0x181D9C6E), LL(0xE3F82D19, 0x988E056B), LL(0xE23EE7E4, 0xB3312FA7)};
const BNU_CHUNK_T secp384r1_gx[] = {
   LL(0x72760AB7, 0x3A545E38), LL(0xBF55296C, 0x5502F25D), LL(0x82542A38, 0x59F741E0),
   LL(0x8BA79B98, 0x6E1D3B62), LL(0xF320AD74, 0x8EB1C71E), LL(0xBE8B0537, 0xAA87CA22)};
const BNU_CHUNK_T secp384r1_gy[] = {
   LL(0x90EA0E5F, 0x7A431D7C), LL(0x1D7E819D, 0x0A60B1CE), LL(0xB5F0B8C0, 0xE9DA3113),
   LL(0x289A147C, 0xF8F41DBD), LL(0x9292DC29, 0x5D9E98BF), LL(0x96262C6F, 0x3617DE4A)};
const BNU_CHUNK_T secp384r1_r[] = {
   LL(0xCCC52973, 0xECEC196A), LL(0x48B0A77A, 0x581A0DB2), LL(0xF4372DDF, 0xC7634D81),
   LL(0xFFFFFFFF, 0xFFFFFFFF), LL(0xFFFFFFFF, 0xFFFFFFFF), LL(0xFFFFFFFF, 0xFFFFFFFF)};
BNU_CHUNK_T secp384r1_h = 1;

#endif /* _IPP_DATA */
