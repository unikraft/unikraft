typedef int to_avoid_translation_unit_is_empty_warning;

#ifndef BN_OPENSSL_DISABLE

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

#include <openssl/bn.h>

#include <crypto_mb/status.h>

#include <internal/common/ifma_defs.h>
#include <internal/rsa/ifma_rsa_arith.h>
#include <internal/rsa/ifma_rsa_layer_ssl.h>


// y = x^d mod n
DLL_PUBLIC
mbx_status mbx_rsa_private_ssl_mb8(const int8u* const from_pa[8],
                                         int8u* const to_pa[8],
                                  const BIGNUM* const d_pa[8],
                                  const BIGNUM* const n_pa[8],
                                  int expected_rsa_bitsize)
{
   mbx_status status = 0;
   int buf_no;

   /* test input pointers */
   if(NULL==from_pa || NULL==to_pa || NULL==d_pa || NULL==n_pa) {
      status = MBX_SET_STS_ALL(MBX_STATUS_NULL_PARAM_ERR);
      return status;
   }
   /* test rsa modulus size */
   if(RSA_1K != expected_rsa_bitsize && RSA_2K != expected_rsa_bitsize &&
      RSA_3K != expected_rsa_bitsize && RSA_4K != expected_rsa_bitsize) {
      status = MBX_SET_STS_ALL(MBX_STATUS_MISMATCH_PARAM_ERR);
      return status;
   }

   /* check pointers and values */
   for(buf_no=0; buf_no<8; buf_no++) {
      const int8u* inp = from_pa[buf_no];
            int8u* out = to_pa[buf_no];
      const BIGNUM* d = d_pa[buf_no];
      const BIGNUM* n = n_pa[buf_no];

      /* if any of pointer NULL set error status */
      if(NULL==inp || NULL==out || NULL==d || NULL==n) {
         status = MBX_SET_STS(status, buf_no, MBX_STATUS_NULL_PARAM_ERR);
         continue;
      }

      /* check rsa size */
      if(expected_rsa_bitsize != BN_num_bits(n)) {
         status = MBX_SET_STS(status, buf_no, MBX_STATUS_MISMATCH_PARAM_ERR);
         continue;
      }
   }

   /* continue processing if there are correct parameters */
   if( MBX_IS_ANY_OK_STS(status) ) {
      /* select exponentiation */
      switch (expected_rsa_bitsize) {
      case RSA_1K: ifma_ssl_rsa1K_prv2_layer_mb8(from_pa, to_pa, d_pa, n_pa); break;
      case RSA_2K: ifma_ssl_rsa2K_prv2_layer_mb8(from_pa, to_pa, d_pa, n_pa); break;
      case RSA_3K: ifma_ssl_rsa3K_prv2_layer_mb8(from_pa, to_pa, d_pa, n_pa); break;
      case RSA_4K: ifma_ssl_rsa4K_prv2_layer_mb8(from_pa, to_pa, d_pa, n_pa); break;
      }
   }

   return status;
}

#endif /* BN_OPENSSL_DISABLE */
