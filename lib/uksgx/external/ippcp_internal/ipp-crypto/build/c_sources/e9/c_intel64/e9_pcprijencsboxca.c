/*******************************************************************************
* Copyright 2002-2021 Intel Corporation
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
//     Encrypt tables for Rijndael
// 
//  Contents:
//     RijEncSbox[256]
// 
// 
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcprijtables.h"


/*
// Reference to pcrijencryptpxca.c
// for details
*/

/*
// Pure Encryprion S-boxes
*/
#if defined( _IPP_DATA )

const __ALIGN64 Ipp8u RijEncSbox[256] = { ENC_SBOX(none_t) };

#endif /* _IPP_DATA */
