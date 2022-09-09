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
//     secp192r1               (* Montgomery Friendly Modulus (+1) *)
// 
*/

#include "owndefs.h"
#include "owncp.h"

#include "pcpgfpstuff.h"

#if defined( _IPP_DATA )

/*
// Recommended Parameters secp192r1
*/
const BNU_CHUNK_T h_secp192r1_p[] = { // half of secp192r1_p
   LL(0xFFFFFFFF, 0x7FFFFFFF), LL(0xFFFFFFFF, 0xFFFFFFFF), LL(0xFFFFFFFF, 0x7FFFFFFF)};

const BNU_CHUNK_T secp192r1_p[] = { // 2^192 -2^64 -1
   LL(0xFFFFFFFF, 0xFFFFFFFF), LL(0xFFFFFFFE, 0xFFFFFFFF), LL(0xFFFFFFFF, 0xFFFFFFFF), LL(0x0, 0x0)};
const BNU_CHUNK_T secp192r1_a[] = {
   LL(0xFFFFFFFC, 0xFFFFFFFF), LL(0xFFFFFFFE, 0xFFFFFFFF), LL(0xFFFFFFFF, 0xFFFFFFFF)};
const BNU_CHUNK_T secp192r1_b[] = {
   LL(0xC146B9B1, 0xFEB8DEEC), LL(0x72243049, 0x0FA7E9AB), LL(0xE59C80E7, 0x64210519)};
const BNU_CHUNK_T secp192r1_gx[] = {
   LL(0x82FF1012, 0xF4FF0AFD), LL(0x43A18800, 0x7CBF20EB), LL(0xB03090F6, 0x188DA80E)};
const BNU_CHUNK_T secp192r1_gy[] = {
   LL(0x1E794811, 0x73F977A1), LL(0x6B24CDD5, 0x631011ED), LL(0xFFC8DA78, 0x07192B95)};
const BNU_CHUNK_T secp192r1_r[] = {
   LL(0xB4D22831, 0x146BC9B1), LL(0x99DEF836, 0xFFFFFFFF), LL(0xFFFFFFFF, 0xFFFFFFFF)};
BNU_CHUNK_T secp192r1_h = 1;

#endif /* _IPP_DATA */
