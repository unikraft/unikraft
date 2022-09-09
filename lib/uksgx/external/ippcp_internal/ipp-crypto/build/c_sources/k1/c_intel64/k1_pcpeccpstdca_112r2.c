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
//     secp112r2
// 
*/

#include "owndefs.h"
#include "owncp.h"

#include "pcpgfpstuff.h"

#if defined( _IPP_DATA )

/*
// Recommended Parameters secp112r2
*/
const BNU_CHUNK_T secp112r2_p[] = { // (2^128 -3)/76439
   LL(0xBEAD208B, 0x5E668076), LL(0x2ABF62E3, 0xDB7C)};
const BNU_CHUNK_T secp112r2_a[] = {
   LL(0x5C0EF02C, 0x8A0AAAF6), LL(0xC24C05F3, 0x6127)};
const BNU_CHUNK_T secp112r2_b[] = {
   LL(0x4C85D709, 0xED74FCC3), LL(0xF1815DB5, 0x51DE)};
const BNU_CHUNK_T secp112r2_gx[] = {
   LL(0xD0928643, 0xB4E1649D), LL(0x0AB5E892, 0x4BA3)};
const BNU_CHUNK_T secp112r2_gy[] = {
   LL(0x6E956E97, 0x3747DEF3), LL(0x46F5882E, 0xADCD)};
const BNU_CHUNK_T secp112r2_r[] = {
   LL(0x0520D04B, 0xD7597CA1), LL(0x0AAFD8B8, 0x36DF)};
BNU_CHUNK_T secp112r2_h = 4;

#endif /* _IPP_DATA */