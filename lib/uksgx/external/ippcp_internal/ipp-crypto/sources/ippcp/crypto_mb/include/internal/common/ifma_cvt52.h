/*******************************************************************************
* Copyright 2019-2021 Intel Corporation
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

#ifndef IFMA_CVT52_H
#define IFMA_CVT52_H

#ifndef BN_OPENSSL_DISABLE
  #include <openssl/bn.h>
#endif

#include <crypto_mb/defs.h>

// from 8 buffers regular (radix2^64) to mb8 redundant (radix 2^52) representation
EXTERN_C int8u ifma_BNU_to_mb8(int64u out_mb8[][8], const int64u* const bn[8], int bitLen);
EXTERN_C int8u ifma_HexStr8_to_mb8(int64u out_mb8[][8], const int8u* const pStr[8], int bitLen);
#ifndef BN_OPENSSL_DISABLE
EXTERN_C int8u ifma_BN_to_mb8(int64u res[][8], const BIGNUM* const bn[8], int bitLen);
#endif

// from 8 buffers mb8 redundant (radix 2^52) to regular (radix2^64) representation
EXTERN_C int8u ifma_mb8_to_BNU(int64u* const out_bn[8], const int64u inp_mb8[][8], const int bitLen);
EXTERN_C int8u ifma_mb8_to_HexStr8(int8u* const pStr[8], const int64u inp_mb8[][8], int bitLen);

EXTERN_C int8u ifma_BNU_transpose_copy(int64u out_mb8[][8], const int64u* const inp[8], int bitLen);
#ifndef BN_OPENSSL_DISABLE
EXTERN_C int8u ifma_BN_transpose_copy(int64u out_mb8[][8], const BIGNUM* const inp[8], int bitLen);
#endif /* BN_OPENSSL_DISABLE */

#endif /* IFMA_CVT52_H */
