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
//     secp128r2    (* Montgomery Friendly Modulus (+1) *)
// 
*/

#include "owndefs.h"
#include "owncp.h"

#include "pcpgfpstuff.h"

#if defined( _IPP_DATA )

/*
// Recommended Parameters secp128r2
*/
const BNU_CHUNK_T secp128r2_p[] = { // 2^128 -2^97 -1
   LL(0xFFFFFFFF, 0xFFFFFFFF), LL(0xFFFFFFFF, 0xFFFFFFFD), LL(0, 0)};
const BNU_CHUNK_T secp128r2_a[] = {
   LL(0xBFF9AEE1, 0xBF59CC9B), LL(0xD1B3BBFE, 0xD6031998)};
const BNU_CHUNK_T secp128r2_b[] = {
   LL(0xBB6D8A5D, 0xDC2C6558), LL(0x80D02919, 0x5EEEFCA3)};
const BNU_CHUNK_T secp128r2_gx[] = {
   LL(0xCDEBC140, 0xE6FB32A7), LL(0x5E572983, 0x7B6AA5D8)};
const BNU_CHUNK_T secp128r2_gy[] = {
   LL(0x5FC34B44, 0x7106FE80), LL(0x894D3AEE, 0x27B6916A)};
const BNU_CHUNK_T secp128r2_r[] = {
   LL(0x0613B5A3, 0xBE002472), LL(0x7FFFFFFF, 0x3FFFFFFF)};
BNU_CHUNK_T secp128r2_h = 4;

#endif /* _IPP_DATA */