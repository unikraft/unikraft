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


#include <crypto_mb/status.h>
#include <crypto_mb/rsa.h>

#include <internal/common/ifma_defs.h>
#include <internal/rsa/ifma_rsa_arith.h>
#include <internal/rsa/ifma_rsa_method.h>
#include <internal/rsa/ifma_rsa_layer_cp.h>

#if !defined(NO_USE_MALLOC)
#include <stdlib.h>
#endif

// y = x^65537 mod n
DLL_PUBLIC
mbx_status mbx_rsa_public_mb8(const int8u* const from_pa[8],
                                    int8u* const to_pa[8],
                             const int64u* const n_pa[8],
                                       int expected_rsa_bitsize,
                     const mbx_RSA_Method* m,
                                    int8u* pBuffer)
{
   const mbx_RSA_Method* meth = m;

   mbx_status status = 0;
   int buf_no;

   /* test input pointers */
   if(NULL==from_pa || NULL==to_pa || NULL==n_pa) {
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
      const int64u* n = n_pa[buf_no];

      /* if any of pointer NULL set error status */
      if(NULL==inp || NULL==out || NULL==n) {
         status = MBX_SET_STS(status, buf_no, MBX_STATUS_NULL_PARAM_ERR);
         continue;
      }
   }

   /* test method */
   if(NULL==meth) {
      meth = mbx_RSA_pub65537_Method(expected_rsa_bitsize);
      if(NULL==meth) {
         status = MBX_SET_STS_ALL(MBX_STATUS_NULL_PARAM_ERR);
         return status;
      }
   }
   /* check if requested operation matched to method's */
   if(RSA_PUB_KEY != OP_RSA_ID(meth->id)) {
      status = MBX_SET_STS_ALL(MBX_STATUS_MISMATCH_PARAM_ERR);
      return status;
   }
   /* check if requested RSA matched to method's */
   if(expected_rsa_bitsize != BISIZE_RSA_ID(meth->id)) {
      status = MBX_SET_STS_ALL(MBX_STATUS_MISMATCH_PARAM_ERR);
      return status;
   }

   /*
   // processing
   */
   if( MBX_IS_ANY_OK_STS(status) ) {
      int8u* buffer = pBuffer;

      #if !defined(NO_USE_MALLOC)
      int allocated_buf = 0;

      /* check if allocated buffer) */
      if(NULL==buffer) {
         buffer = (int8u*)( malloc(meth->buffSize) );
         if(NULL==buffer) {
            status = MBX_SET_STS_ALL(MBX_STATUS_NULL_PARAM_ERR);
            return status;
         }
         allocated_buf = 1;
      }
      #endif
   
      ifma_cp_rsa_pub_layer_mb8(from_pa, to_pa, n_pa,
                                expected_rsa_bitsize, meth,
                                buffer);
      
      #if !defined(NO_USE_MALLOC)
      /* release buffer */
      if(allocated_buf)
         free(buffer);
      #endif
   }

   return status;
}

DLL_PUBLIC
mbx_status mbx_rsa_private_mb8(const int8u* const from_pa[8],
                                     int8u* const to_pa[8],
                              const int64u* const d_pa[8],
                              const int64u* const n_pa[8],
                                       int expected_rsa_bitsize,
                     const mbx_RSA_Method* m,
                                    int8u* pBuffer)
{
   const mbx_RSA_Method* meth = m;

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
      const int64u* d = d_pa[buf_no];
      const int64u* n = n_pa[buf_no];

      /* if any of pointer NULL set error status */
      if(NULL==inp || NULL==out || NULL==d || NULL==n) {
         status = MBX_SET_STS(status, buf_no, MBX_STATUS_NULL_PARAM_ERR);
         continue;
      }
   }

   /* test method */
   if(NULL==meth) {
      meth = mbx_RSA_private_Method(expected_rsa_bitsize);
      if(NULL==meth) {
         status = MBX_SET_STS_ALL(MBX_STATUS_NULL_PARAM_ERR);
         return status;
      }
   }
   /* check if requested operation matched to method's */
   if(RSA_PRV2_KEY != OP_RSA_ID(meth->id)) {
      status = MBX_SET_STS_ALL(MBX_STATUS_MISMATCH_PARAM_ERR);
      return status;
   }
   /* check if requested RSA matched to method's */
   if(expected_rsa_bitsize != BISIZE_RSA_ID(meth->id)) {
      status = MBX_SET_STS_ALL(MBX_STATUS_MISMATCH_PARAM_ERR);
      return status;
   }

   /*
   // processing
   */
   if( MBX_IS_ANY_OK_STS(status) ) {
      int8u* buffer = pBuffer;

      #if !defined(NO_USE_MALLOC)
      int allocated_buf = 0;

      /* check if allocated buffer) */
      if(NULL==buffer) {
         buffer = (int8u*)( malloc(meth->buffSize) );
         if(NULL==buffer) {
            status = MBX_SET_STS_ALL(MBX_STATUS_NULL_PARAM_ERR);
            return status;
         }
         allocated_buf = 1;
      }
      #endif
   
      ifma_cp_rsa_prv2_layer_mb8(from_pa, to_pa, d_pa, n_pa,
                                expected_rsa_bitsize, meth,
                                buffer);

      #if !defined(NO_USE_MALLOC)
      /* release buffer */
      if(allocated_buf)
         free(buffer);
      #endif
   }

   return status;
}

DLL_PUBLIC
mbx_status mbx_rsa_private_crt_mb8(const int8u* const from_pa[8],
                                         int8u* const to_pa[8],
                                  const int64u* const p_pa[8],
                                  const int64u* const q_pa[8],
                                  const int64u* const dp_pa[8],
                                  const int64u* const dq_pa[8],
                                  const int64u* const iq_pa[8],
                                            int expected_rsa_bitsize,
                          const mbx_RSA_Method* m,
                                         int8u* pBuffer)
{
   const mbx_RSA_Method* meth = m;

   mbx_status status = 0;
   int buf_no;

   /* test input pointers */
   if(NULL==from_pa || NULL==to_pa ||
      NULL==p_pa || NULL==q_pa || NULL==dp_pa || NULL==dq_pa || NULL==iq_pa) {
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
      const int64u* p = p_pa[buf_no];
      const int64u* q = q_pa[buf_no];
      const int64u* dp = dp_pa[buf_no];
      const int64u* dq = dq_pa[buf_no];
      const int64u* iq = iq_pa[buf_no];

      /* if any of pointer NULL set error status */
      if(NULL==inp || NULL==out || NULL==q || NULL==p || NULL==dq || NULL==dp || NULL==iq) {
         status = MBX_SET_STS(status, buf_no, MBX_STATUS_NULL_PARAM_ERR);
         continue;
      }
   }

   /* test method */
   if(NULL==meth) {
      meth = mbx_RSA_private_crt_Method(expected_rsa_bitsize);
      if(NULL==meth) {
         status = MBX_SET_STS_ALL(MBX_STATUS_NULL_PARAM_ERR);
         return status;
      }
   }
   /* check if requested operation matched to method's */
   if(RSA_PRV5_KEY != OP_RSA_ID(meth->id)) {
      status = MBX_SET_STS_ALL(MBX_STATUS_MISMATCH_PARAM_ERR);
      return status;
   }
   /* check if requested RSA matched to method's */
   if(expected_rsa_bitsize != BISIZE_RSA_ID(meth->id)) {
      status = MBX_SET_STS_ALL(MBX_STATUS_MISMATCH_PARAM_ERR);
      return status;
   }

   /*
   // processing
   */
   if( MBX_IS_ANY_OK_STS(status) ) {
      int8u* buffer = pBuffer;

      #if !defined(NO_USE_MALLOC)
      int allocated_buf = 0;

      /* check if allocated buffer) */
      if(NULL==buffer) {
         buffer = (int8u*)( malloc(meth->buffSize) );
         if(NULL==buffer) {
            status = MBX_SET_STS_ALL(MBX_STATUS_NULL_PARAM_ERR);
            return status;
         }
         allocated_buf = 1;
      }
      #endif
   
      ifma_cp_rsa_prv5_layer_mb8(from_pa, to_pa, p_pa, q_pa, dp_pa, dq_pa, iq_pa,
                                expected_rsa_bitsize, meth,
                                buffer);

      #if !defined(NO_USE_MALLOC)
      /* release buffer */
      if(allocated_buf)
         free(buffer);
      #endif
   }

   return status;
}
