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

#ifndef EC_NISTP521_H
#define EC_NISTP521_H

#include <crypto_mb/defs.h>
#include <crypto_mb/status.h>

#ifndef BN_OPENSSL_DISABLE
    #include <openssl/bn.h>
    #include <openssl/ec.h>
#endif // BN_OPENSSL_DISABLE


/*
// ECDHE
*/

/*
// Computes public key
// pa_pubx[]   array of pointers to the public keys X-coordinates
// pa_puby[]   array of pointers to the public keys Y-coordinates
// pa_pubz[]   array of pointers to the public keys Z-coordinates
// pa_skey[]   array of pointers to the private keys
// pBuffer     pointer to the scratch buffer
//
// Note:
// output public key is represented by (X:Y:Z) projective Jacobian coordinates
*/
#ifndef BN_OPENSSL_DISABLE
EXTERN_C mbx_status mbx_nistp521_ecpublic_key_ssl_mb8(BIGNUM* pa_pubx[8],
                                                      BIGNUM* pa_puby[8],
                                                      BIGNUM* pa_pubz[8],
                                                const BIGNUM* const pa_skey[8],
                                                      int8u* pBuffer);
#endif // BN_OPENSSL_DISABLE


EXTERN_C mbx_status mbx_nistp521_ecpublic_key_mb8(int64u* pa_pubx[8],
                                                  int64u* pa_puby[8],
                                                  int64u* pa_pubz[8],
                                            const int64u* const pa_skey[8],
                                                   int8u* pBuffer);
/*
// Computes shared key
// pa_shared_key[]   array of pointers to the shared keys
// pa_skey[]   array of pointers to the own (ephemeral) private keys
// pa_pubx[]   array of pointers to the party's public keys X-coordinates
// pa_puby[]   array of pointers to the party's public keys Y-coordinates
// pa_pubz[]   array of pointers to the party's public keys Z-coordinates
// pBuffer     pointer to the scratch buffer
//
// Note:
// input party's public key is represented by (X:Y:Z) projective Jacobian coordinates
*/
#ifndef BN_OPENSSL_DISABLE
EXTERN_C mbx_status mbx_nistp521_ecdh_ssl_mb8(int8u* pa_shared_key[8],
                                       const BIGNUM* const pa_skey[8], 
                                       const BIGNUM* const pa_pubx[8],
                                       const BIGNUM* const pa_puby[8],
                                       const BIGNUM* const pa_pubz[8],
                                              int8u* pBuffer);
#endif // BN_OPENSSL_DISABLE


EXTERN_C mbx_status mbx_nistp521_ecdh_mb8(int8u* pa_shared_key[8],
                                    const int64u* const pa_skey[8],
                                    const int64u* const pa_pubx[8],
                                    const int64u* const pa_puby[8],
                                    const int64u* const pa_pubz[8],
                                           int8u* pBuffer);


/*
// ECDSA signature generation
*/

/*
// Pre-computes ECDSA signature
// pa_inv_eph_skey[] array of pointers to the inversion of signer's ephemeral private keys
// pa_sign_rp[]      array of pointers to the pre-computed r-components of the signatures 
// pa_eph_skey[]     array of pointers to the signer's ephemeral private keys
// pBuffer           pointer to the scratch buffer
*/
EXTERN_C mbx_status mbx_nistp521_ecdsa_sign_setup_mb8(int64u* pa_inv_eph_skey[8],
                                                      int64u* pa_sign_rp[8],
                                                const int64u* const pa_eph_skey[8],
                                                       int8u* pBuffer);
/*
// computes ECDSA signature
//
// pa_sign_pr[]      array of pointers to the r-components of the signatures 
// pa_sign_ps[]      array of pointers to the s-components of the signatures 
// pa_msg[]          array of pointers to the messages are being signed
// pa_sign_rp[]      array of pointers to the pre-computed r-components of the signatures 
// pa_inv_eph_skey[] array of pointers to the inversion of signer's ephemeral private keys
// pa_reg_skey[]     array of pointers to the regular signer's ephemeral private keys
// pBuffer           pointer to the scratch buffer
*/
EXTERN_C mbx_status mbx_nistp521_ecdsa_sign_complete_mb8(int8u* pa_sign_r[8],
                                                         int8u* pa_sign_s[8],
                                                   const int8u* const pa_msg[8],
                                                  const int64u* const pa_sgn_rp[8],
                                                  const int64u* const pa_inv_eph_skey[8],
                                                  const int64u* const pa_reg_skey[8],
                                                         int8u* pBuffer);
/*
// Computes ECDSA signature
// pa_sign_r[]       array of pointers to the computed r-components of the signatures 
// pa_sign_s[]       array of pointers to the computed s-components of the signatures
// pa_msg[]          array of pointers to the messages are being signed
// pa_eph_skey[]     array of pointers to the signer's ephemeral private keys
// pa_reg_skey[]     array of pointers to the signer's regular private keys
// pBuffer           pointer to the scratch buffer
*/
EXTERN_C mbx_status mbx_nistp521_ecdsa_sign_mb8(int8u* pa_sign_r[8],
                                                int8u* pa_sign_s[8],
                                          const int8u* const pa_msg[8],
                                         const int64u* const pa_eph_skey[8],
                                         const int64u* const pa_reg_skey[8],
                                                int8u* pBuffer);

/*
// Verifies ECDSA signature
// pa_sign_r[]       array of pointers to the computed r-components of the signatures 
// pa_sign_s[]       array of pointers to the computed s-components of the signatures
// pa_msg[]          array of pointers to the messages are being signed
// pa_pubx[]         array of pointers to the public keys X-coordinates
// pa_puby[]         array of pointers to the public keys Y-coordinates
// pa_pubz[]         array of pointers to the public keys Z-coordinates
// pBuffer           pointer to the scratch buffer
*/
EXTERN_C mbx_status mbx_nistp521_ecdsa_verify_mb8(const int8u* const pa_sign_r[8],
                                                  const int8u* const pa_sign_s[8],
                                                  const int8u* const pa_msg[8],
                                                 const int64u* const pa_pubx[8],
                                                 const int64u* const pa_puby[8],
                                                 const int64u* const pa_pubz[8],                                                   
                                                        int8u* pBuffer);
/*
// OpenSSL's specific similar APIs
*/
#ifndef BN_OPENSSL_DISABLE
EXTERN_C mbx_status mbx_nistp521_ecdsa_sign_setup_ssl_mb8(BIGNUM* pa_inv_eph_skey[8],
                                                          BIGNUM* pa_sign_pr[8],
                                                    const BIGNUM* const pa_eph_skey[8],
                                                           int8u* pBuffer);

EXTERN_C mbx_status mbx_nistp521_ecdsa_sign_complete_ssl_mb8(int8u* pa_sign_r[8],
                                                             int8u* pa_sign_s[8],
                                                       const int8u* const pa_msg[8],
                                                      const BIGNUM* const pa_sgn_rp[8],
                                                      const BIGNUM* const pa_inv_eph_skey[8],
                                                      const BIGNUM* const pa_reg_skey[8],
                                                             int8u* pBuffer);

EXTERN_C mbx_status mbx_nistp521_ecdsa_sign_ssl_mb8(int8u* pa_sign_r[8],
                                                    int8u* pa_sign_s[8],
                                              const int8u* const pa_msg[8],
                                             const BIGNUM* const pa_eph_skey[8],
                                             const BIGNUM* const pa_reg_skey[8],
                                                    int8u* pBuffer);

EXTERN_C mbx_status mbx_nistp521_ecdsa_verify_ssl_mb8(const ECDSA_SIG* const pa_sig[8],
                                                      const int8u* const pa_msg[8],
                                                     const BIGNUM* const pa_pubx[8],
                                                     const BIGNUM* const pa_puby[8],
                                                     const BIGNUM* const pa_pubz[8],                                                   
                                                            int8u* pBuffer);

#endif // BN_OPENSSL_DISABLE
#endif /* EC_NISTP521_H */
