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
//     secp112r1
// 
*/

#include "owndefs.h"
#include "owncp.h"

#include "pcpgfpstuff.h"

#if defined( _IPP_DATA )

/*
// Recommended Parameters secp112r1
*/
const BNU_CHUNK_T secp112r1_p[] = { // (2^128 -3)/76439
   LL(0xBEAD208B, 0x5E668076), LL(0x2ABF62E3, 0xDB7C)};
const BNU_CHUNK_T secp112r1_a[] = {
   LL(0xBEAD2088, 0x5E668076), LL(0x2ABF62E3, 0xDB7C)};
const BNU_CHUNK_T secp112r1_b[] = {
   LL(0x11702B22, 0x16EEDE89), LL(0xF8BA0439, 0x659E)};
const BNU_CHUNK_T secp112r1_gx[] = {
   LL(0xF9C2F098, 0x5EE76B55), LL(0x7239995A, 0x0948)};
const BNU_CHUNK_T secp112r1_gy[] = {
   LL(0x0FF77500, 0xC0A23E0E), LL(0xE5AF8724, 0xA89C)};
const BNU_CHUNK_T secp112r1_r[] = {
   LL(0xAC6561C5, 0x5E7628DF), LL(0x2ABF62E3, 0xDB7C)};
BNU_CHUNK_T secp112r1_h = 1;

#endif /* _IPP_DATA */