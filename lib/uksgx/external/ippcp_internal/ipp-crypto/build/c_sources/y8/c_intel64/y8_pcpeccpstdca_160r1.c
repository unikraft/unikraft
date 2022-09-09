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
//     secp160r1
// 
*/

#include "owndefs.h"
#include "owncp.h"

#include "pcpgfpstuff.h"

#if defined( _IPP_DATA )

/*
// Recommended Parameters secp160r1
*/
const BNU_CHUNK_T secp160r1_p[] = { // 2^160 -2^31 -1
   LL(0x7FFFFFFF, 0xFFFFFFFF), LL(0xFFFFFFFF, 0xFFFFFFFF), L_(0xFFFFFFFF)};
const BNU_CHUNK_T secp160r1_a[] = {
   LL(0x7FFFFFFC, 0xFFFFFFFF), LL(0xFFFFFFFF, 0xFFFFFFFF), L_(0xFFFFFFFF)};
const BNU_CHUNK_T secp160r1_b[] = {
   LL(0xC565FA45, 0x81D4D4AD), LL(0x65ACF89F, 0x54BD7A8B), L_(0x1C97BEFC)};
const BNU_CHUNK_T secp160r1_gx[] = {
   LL(0x13CBFC82, 0x68C38BB9), LL(0x46646989, 0x8EF57328), L_(0x4A96B568)};
const BNU_CHUNK_T secp160r1_gy[] = {
   LL(0x7AC5FB32, 0x04235137), LL(0x59DCC912, 0x3168947D), L_(0x23A62855)};
const BNU_CHUNK_T secp160r1_r[] = {
   LL(0xCA752257, 0xF927AED3), LL(0x0001F4C8, 0x00000000), LL(0x00000000, 0x1)};
BNU_CHUNK_T secp160r1_h = 1;

#endif /* _IPP_DATA */