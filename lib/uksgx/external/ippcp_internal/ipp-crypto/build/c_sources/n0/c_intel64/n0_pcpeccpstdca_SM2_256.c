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
//     tpmSM2_p256_p (SM2)     (* Montgomery Friendly Modulus (+1) *)
// 
*/

#include "owndefs.h"
#include "owncp.h"

#include "pcpgfpstuff.h"

#if defined( _IPP_DATA )

/*
// Recommended Parameters tpm_SM2_p256
*/
#ifdef _SM2_SIGN_DEBUG_
const BNU_CHUNK_T tpmSM2_p256_p[] = {
   LL(0x08F1DFC3, 0x722EDB8B), LL(0x5C45517D, 0x45728391), LL(0xBF6FF7DE, 0xE8B92435), LL(0x4C044F18, 0x8542D69E), LL(0x0, 0x0)};
const BNU_CHUNK_T tpmSM2_p256_a[] = {
   LL(0x3937E498, 0xEC65228B), LL(0x6831D7E0, 0x2F3C848B), LL(0x73BBFEFF, 0x2417842E), LL(0xFA32C3FD, 0x787968B4)};
const BNU_CHUNK_T tpmSM2_p256_b[] = {
   LL(0x27C5249A, 0x6E12D1DA), LL(0xB16BA06E, 0xF61D59A5), LL(0x484BFE48, 0x9CF84241), LL(0xB23B0C84, 0x63E4C6D3)};
const BNU_CHUNK_T tpmSM2_p256_gx[] = {
   LL(0x7FEDD43D, 0x4C4E6C14), LL(0xADD50BDC, 0x32220B3B), LL(0xC3CC315E, 0x746434EB), LL(0x1B62EAB6, 0x421DEBD6)};
const BNU_CHUNK_T tpmSM2_p256_gy[] = {
   LL(0xE46E09A2, 0xA85841B9), LL(0xBFA36EA1, 0xE5D7FDFC), LL(0x153B70C4, 0xD47349D2), LL(0xCBB42C07, 0x0680512B)};
const BNU_CHUNK_T tpmSM2_p256_r[] = {
   LL(0xC32E79B7, 0x5AE74EE7), LL(0x0485628D, 0x29772063), LL(0xBF6FF7DD, 0xE8B92435), LL(0x4C044F18, 0x8542D69E)};
#else
const BNU_CHUNK_T h_tpmSM2_p256_p[] = { // half of tpmSM2_p256_p
   LL(0xFFFFFFFF, 0x7FFFFFFF), LL(0x80000000, 0xFFFFFFFF), LL(0xFFFFFFFF, 0xFFFFFFFF), LL(0x7FFFFFFF, 0x7FFFFFFF)};

const BNU_CHUNK_T tpmSM2_p256_p[] = { // 2^256 -2^224 -2^96 +2^64 -1
   LL(0xFFFFFFFF, 0xFFFFFFFF), LL(0x00000000, 0xFFFFFFFF), LL(0xFFFFFFFF, 0xFFFFFFFF), LL(0xFFFFFFFF, 0xFFFFFFFE), LL(0x0, 0x0)};
const BNU_CHUNK_T tpmSM2_p256_a[] = {
   LL(0xFFFFFFFC, 0xFFFFFFFF), LL(0x00000000, 0xFFFFFFFF), LL(0xFFFFFFFF, 0xFFFFFFFF), LL(0xFFFFFFFF, 0xFFFFFFFE)};
const BNU_CHUNK_T tpmSM2_p256_b[] = {
   LL(0x4D940E93, 0xDDBCBD41), LL(0x15AB8F92, 0xF39789F5), LL(0xCF6509A7, 0x4D5A9E4B), LL(0x9D9F5E34, 0x28E9FA9E)};
const BNU_CHUNK_T tpmSM2_p256_gx[] = {
   LL(0x334C74C7, 0x715A4589), LL(0xF2660BE1, 0x8FE30BBF), LL(0x6A39C994, 0x5F990446), LL(0x1F198119, 0x32C4AE2C)};
const BNU_CHUNK_T tpmSM2_p256_gy[] = {
   LL(0x2139F0A0, 0x02DF32E5), LL(0xC62A4740, 0xD0A9877C), LL(0x6B692153, 0x59BDCEE3), LL(0xF4F6779C, 0xBC3736A2)};
const BNU_CHUNK_T tpmSM2_p256_r[] = {
   LL(0x39D54123, 0x53BBF409), LL(0x21C6052B, 0x7203DF6B), LL(0xFFFFFFFF, 0xFFFFFFFF), LL(0xFFFFFFFF, 0xFFFFFFFE)};
#endif
BNU_CHUNK_T tpmSM2_p256_h = 1;

#endif /* _IPP_DATA */
