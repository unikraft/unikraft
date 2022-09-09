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
//     secp128r1    (* Montgomery Friendly Modulus (+1) *)
// 
*/

#include "owndefs.h"
#include "owncp.h"

#include "pcpgfpstuff.h"

#if defined( _IPP_DATA )

/*
// Recommended Parameters secp128r1
*/
const BNU_CHUNK_T h_secp128r1_p[] = { // halpf of secp128r1_p
   LL(0xFFFFFFFF, 0xFFFFFFFF), LL(0xFFFFFFFF, 0x7FFFFFFE)};

const BNU_CHUNK_T secp128r1_p[] = { // 2^128 -2^97 -1
   LL(0xFFFFFFFF, 0xFFFFFFFF), LL(0xFFFFFFFF, 0xFFFFFFFD), LL(0, 0)};
const BNU_CHUNK_T secp128r1_a[] = {
   LL(0xFFFFFFFC, 0xFFFFFFFF), LL(0xFFFFFFFF, 0xFFFFFFFD)};
const BNU_CHUNK_T secp128r1_b[] = {
   LL(0x2CEE5ED3, 0xD824993C), LL(0x1079F43D, 0xE87579C1)};
const BNU_CHUNK_T secp128r1_gx[] = {
   LL(0xA52C5B86, 0x0C28607C), LL(0x8B899B2D, 0x161FF752)};
const BNU_CHUNK_T secp128r1_gy[] = {
   LL(0xDDED7A83, 0xC02DA292), LL(0x5BAFEB13, 0xCF5AC839)};
const BNU_CHUNK_T secp128r1_r[] = {
   LL(0x9038A115, 0x75A30D1B), LL(0x00000000, 0xFFFFFFFE)};
BNU_CHUNK_T secp128r1_h = 1;

#endif /* _IPP_DATA */