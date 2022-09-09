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
//     tpm_BN_p256 (BN, TPM)
// 
*/

#include "owndefs.h"
#include "owncp.h"

#include "pcpgfpstuff.h"

#if defined( _IPP_DATA )

/*
// Recommended Parameters tpm_BN_p256 (Barreto-Naehrig)
*/
const BNU_CHUNK_T tpmBN_p256p_p[] = {
   LL(0xAED33013, 0xD3292DDB), LL(0x12980A82, 0x0CDC65FB), LL(0xEE71A49F, 0x46E5F25E),
   LL(0xFFFCF0CD, 0xFFFFFFFF)};
const BNU_CHUNK_T tpmBN_p256p_a[] = {
   LL(0, 0)};
const BNU_CHUNK_T tpmBN_p256p_b[] = {
   LL(3, 0)};
const BNU_CHUNK_T tpmBN_p256p_gx[] = {
   LL(1, 0)};
const BNU_CHUNK_T tpmBN_p256p_gy[] = {
   LL(2, 0)};
const BNU_CHUNK_T tpmBN_p256p_r[] = {
   LL(0xD10B500D, 0xF62D536C), LL(0x1299921A, 0x0CDC65FB), LL(0xEE71A49E, 0x46E5F25E),
   LL(0xFFFCF0CD, 0xFFFFFFFF)};
BNU_CHUNK_T tpmBN_p256p_h = 1;

#endif /* _IPP_DATA */
