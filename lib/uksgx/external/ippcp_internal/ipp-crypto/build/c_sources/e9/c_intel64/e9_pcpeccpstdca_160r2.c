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
//     secp160r2
// 
*/

#include "owndefs.h"
#include "owncp.h"

#include "pcpgfpstuff.h"

#if defined( _IPP_DATA )

/*
// Recommended Parameters secp160r2
*/
const BNU_CHUNK_T secp160r2_p[] = { // 2^160 -2^32 -2^14 -2^12 -2^9 -2^8 -2^7 -2^2 -1
   LL(0xFFFFAC73, 0xFFFFFFFE), LL(0xFFFFFFFF, 0xFFFFFFFF), L_(0xFFFFFFFF)};
const BNU_CHUNK_T secp160r2_a[] = {
   LL(0xFFFFAC70, 0xFFFFFFFE), LL(0xFFFFFFFF, 0xFFFFFFFF), L_(0xFFFFFFFF)};
const BNU_CHUNK_T secp160r2_b[] = {
   LL(0xF50388BA, 0x04664D5A), LL(0xAB572749, 0xFB59EB8B), L_(0xB4E134D3)};
const BNU_CHUNK_T secp160r2_gx[] = {
   LL(0x3144CE6D, 0x30F7199D), LL(0x1F4FF11B, 0x293A117E), L_(0x52DCB034)};
const BNU_CHUNK_T secp160r2_gy[] = {
   LL(0xA7D43F2E, 0xF9982CFE), LL(0xE071FA0D, 0xE331F296), L_(0xFEAFFEF2)};
const BNU_CHUNK_T secp160r2_r[] = {
   LL(0xF3A1A16B, 0xE786A818), LL(0x0000351E, 0x00000000), LL(0x00000000, 0x1)};
BNU_CHUNK_T secp160r2_h = 1;

#endif /* _IPP_DATA */