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

#ifndef RSA_H
#define RSA_H

#include <crypto_mb/defs.h>
#include <crypto_mb/status.h>

#ifndef BN_OPENSSL_DISABLE
#include <openssl/bn.h>

EXTERN_C mbx_status mbx_rsa_public_ssl_mb8(const int8u*  const from_pa[8],
                                                 int8u*  const to_pa[8],
                                          const BIGNUM* const e_pa[8],
                                          const BIGNUM* const n_pa[8],
                                          int expected_rsa_bitsize);

EXTERN_C mbx_status mbx_rsa_private_ssl_mb8(const int8u*  const from_pa[8],
                                                  int8u*  const to_pa[8],
                                            const BIGNUM* const d_pa[8],
                                            const BIGNUM* const n_pa[8],
                                            int expected_rsa_bitsize);

EXTERN_C mbx_status mbx_rsa_private_crt_ssl_mb8(const int8u*  const from_pa[8],
                                                      int8u*  const to_pa[8],
                                               const BIGNUM* const  p_pa[8],
                                               const BIGNUM* const  q_pa[8],
                                               const BIGNUM* const dp_pa[8],
                                               const BIGNUM* const dq_pa[8],
                                               const BIGNUM* const iq_pa[8],
                                               int expected_rsa_bitsize);
#endif /* BN_OPENSSL_DISABLE */


/*
// rsa cp methods
*/
typedef struct _ifma_rsa_method mbx_RSA_Method;

/* rsa public key opertaion */
EXTERN_C const mbx_RSA_Method* mbx_RSA1K_pub65537_Method(void);
EXTERN_C const mbx_RSA_Method* mbx_RSA2K_pub65537_Method(void);
EXTERN_C const mbx_RSA_Method* mbx_RSA3K_pub65537_Method(void);
EXTERN_C const mbx_RSA_Method* mbx_RSA4K_pub65537_Method(void);
EXTERN_C const mbx_RSA_Method* mbx_RSA_pub65537_Method(int rsaBitsize);

/* rsa private key opertaion */
EXTERN_C const mbx_RSA_Method* mbx_RSA1K_private_Method(void);
EXTERN_C const mbx_RSA_Method* mbx_RSA2K_private_Method(void);
EXTERN_C const mbx_RSA_Method* mbx_RSA3K_private_Method(void);
EXTERN_C const mbx_RSA_Method* mbx_RSA4K_private_Method(void);
EXTERN_C const mbx_RSA_Method* mbx_RSA_private_Method(int rsaBitsize);

/* rsa private key opertaion (ctr) */
EXTERN_C const mbx_RSA_Method* mbx_RSA1K_private_crt_Method(void);
EXTERN_C const mbx_RSA_Method* mbx_RSA2K_private_crt_Method(void);
EXTERN_C const mbx_RSA_Method* mbx_RSA3K_private_crt_Method(void);
EXTERN_C const mbx_RSA_Method* mbx_RSA4K_private_crt_Method(void);
EXTERN_C const mbx_RSA_Method* mbx_RSA_private_crt_Method(int rsaBitsize);

EXTERN_C int mbx_RSA_Method_BufSize(const mbx_RSA_Method* m);


EXTERN_C mbx_status mbx_rsa_public_mb8(const int8u* const from_pa[8],
                                             int8u* const to_pa[8],
                                      const int64u* const n_pa[8],
                                                int rsaBitlen,
                              const mbx_RSA_Method* m,
                                             int8u* pBuffer);

EXTERN_C mbx_status mbx_rsa_private_mb8(const int8u* const from_pa[8],
                                              int8u* const to_pa[8],
                                       const int64u* const d_pa[8],
                                       const int64u* const n_pa[8],
                                                 int rsaBitlen,
                               const mbx_RSA_Method* m,
                                              int8u* pBuffer);

EXTERN_C mbx_status mbx_rsa_private_crt_mb8(const int8u* const from_pa[8],
                                                  int8u* const to_pa[8],
                                           const int64u* const p_pa[8],
                                           const int64u* const q_pa[8],
                                           const int64u* const dp_pa[8],
                                           const int64u* const dq_pa[8],
                                           const int64u* const iq_pa[8],
                                                     int rsaBitlen,
                                   const mbx_RSA_Method* m,
                                                  int8u* pBuffer);
#endif /* RSA_H */
