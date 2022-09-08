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

#ifndef BN_OPENSSL_DISABLE

#if !defined(_IFMA_INTERNAL_SSL_LAYER_H_)
#define _IFMA_INTERNAL_SSL_LAYER_H_

#include <crypto_mb/defs.h>
#include <openssl/bn.h>

EXTERN_C void ifma_ssl_rsa1K_pub_layer_mb8(const int8u* const from_pa[8],
                                                 int8u* const to_pa[8],
                                           const BIGNUM* const n_pa[8]);
EXTERN_C void ifma_ssl_rsa2K_pub_layer_mb8(const int8u* const from_pa[8],
                                                 int8u* const to_pa[8],
                                           const BIGNUM* const n_pa[8]);
EXTERN_C void ifma_ssl_rsa3K_pub_layer_mb8(const int8u* const from_pa[8],
                                                 int8u* const to_pa[8],
                                           const BIGNUM* const n_pa[8]);
EXTERN_C void ifma_ssl_rsa4K_pub_layer_mb8(const int8u* const from_pa[8],
                                                 int8u* const to_pa[8],
                                           const BIGNUM* const n_pa[8]);

EXTERN_C void ifma_ssl_rsa1K_prv2_layer_mb8(const int8u* const from_pa[8],
                                                  int8u* const to_pa[8],
                                            const BIGNUM* const d_pa[8],
                                            const BIGNUM* const n_pa[8]);
EXTERN_C void ifma_ssl_rsa2K_prv2_layer_mb8(const int8u* const from_pa[8],
                                                  int8u* const to_pa[8],
                                            const BIGNUM* const d_pa[8],
                                            const BIGNUM* const n_pa[8]);
EXTERN_C void ifma_ssl_rsa3K_prv2_layer_mb8(const int8u* const from_pa[8],
                                                  int8u* const to_pa[8],
                                            const BIGNUM* const d_pa[8],
                                            const BIGNUM* const n_pa[8]);
EXTERN_C void ifma_ssl_rsa4K_prv2_layer_mb8(const int8u* const from_pa[8],
                                                  int8u* const to_pa[8],
                                            const BIGNUM* const d_pa[8],
                                            const BIGNUM* const n_pa[8]);

EXTERN_C void ifma_ssl_rsa1K_prv5_layer_mb8(const int8u* const from_pa[8],
                                                  int8u* const to_pa[8],
                                             const BIGNUM* const  p_pa[8],
                                             const BIGNUM* const  q_pa[8],
                                             const BIGNUM* const dp_pa[8],
                                             const BIGNUM* const dq_pa[8],
                                             const BIGNUM* const iq_pa[8]);
EXTERN_C void ifma_ssl_rsa2K_prv5_layer_mb8(const int8u* const from_pa[8],
                                                  int8u* const to_pa[8],
                                            const BIGNUM* const  p_pa[8],
                                            const BIGNUM* const  q_pa[8],
                                            const BIGNUM* const dp_pa[8],
                                            const BIGNUM* const dq_pa[8],
                                            const BIGNUM* const iq_pa[8]);
EXTERN_C void ifma_ssl_rsa3K_prv5_layer_mb8(const int8u* const from_pa[8],
                                                  int8u* const to_pa[8],
                                            const BIGNUM* const  p_pa[8],
                                            const BIGNUM* const  q_pa[8],
                                            const BIGNUM* const dp_pa[8],
                                            const BIGNUM* const dq_pa[8],
                                            const BIGNUM* const iq_pa[8]);
EXTERN_C void ifma_ssl_rsa4K_prv5_layer_mb8(const int8u* const from_pa[8],
                                                  int8u* const to_pa[8],
                                            const BIGNUM* const  p_pa[8],
                                            const BIGNUM* const  q_pa[8],
                                            const BIGNUM* const dp_pa[8],
                                            const BIGNUM* const dq_pa[8],
                                            const BIGNUM* const iq_pa[8]);
#endif /* _IFMA_INTERNAL_SSL_LAYER_H_ */

#endif /* BN_OPENSSL_DISABLE */
