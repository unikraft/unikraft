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

#include <crypto_mb/status.h>

#include <internal/common/ifma_defs.h>
#include <internal/common/ifma_cvt52.h>
#include <internal/ecnist/ifma_ecpoint_p384.h>
#include <internal/rsa/ifma_rsa_arith.h>

#ifndef BN_OPENSSL_DISABLE
#include <openssl/bn.h>
#endif

#ifndef BN_OPENSSL_DISABLE
/*
// Computes shared key
// pa_shared_key[]   array of pointers to the shared keys
// pa_skey[]   array of pointers to the own (ephemeral) private keys
// pa_pubx[]   array of pointers to the party's public keys X-coordinates
// pa_puby[]   array of pointers to the party's public keys Y-coordinates
// pa_pubz[]   array of pointers to the party's public keys Z-coordinates  (or NULL, if affine coordinate requested)
// pBuffer     pointer to the scratch buffer
//
// Note:
// input party's public key depends on is pa_pubz[] parameter and represented either
//    - in (X:Y:Z) projective Jacobian coordinates
//    or
//    - in (x:y) affine coordinate
*/
DLL_PUBLIC
mbx_status mbx_nistp384_ecdh_ssl_mb8(int8u* pa_shared_key[8],
                              const BIGNUM* const pa_skey[8],
                              const BIGNUM* const pa_pubx[8],
                              const BIGNUM* const pa_puby[8],
                              const BIGNUM* const pa_pubz[8],
                                     int8u* pBuffer)
{
   mbx_status status = 0;
   int buf_no;

   /* pa_pubz!=0 means the output is in Jacobian projective coordinates */
   int use_jproj_coords = NULL!=pa_pubz;

   /* test input pointers */
   if(NULL==pa_shared_key || NULL==pa_skey || NULL==pa_pubx || NULL==pa_puby) {
      status = MBX_SET_STS_ALL(MBX_STATUS_NULL_PARAM_ERR);
      return status;
   }

   /* check pointers and values */
   for(buf_no=0; buf_no<8; buf_no++) {
      int8u* shared = pa_shared_key[buf_no];
      const BIGNUM* skey = pa_skey[buf_no];
      const BIGNUM* pubx = pa_pubx[buf_no];
      const BIGNUM* puby = pa_puby[buf_no];
      const BIGNUM* pubz = use_jproj_coords? pa_pubz[buf_no] : NULL;

      /* if any of pointer NULL set error status */
      if(NULL==shared || NULL==skey || NULL==pubx || NULL==puby || (use_jproj_coords && NULL==pubz)) {
         status = MBX_SET_STS(status, buf_no, MBX_STATUS_NULL_PARAM_ERR);
      }
   }

   if(!MBX_IS_ANY_OK_STS(status))
      return status;

   /*
   // processing
   */

   /* zero padded private keys */
   U64 secretz[P384_LEN64+1];
   ifma_BN_transpose_copy((int64u (*)[8])secretz, (const BIGNUM**)pa_skey, P384_BITSIZE);
   secretz[P384_LEN64] = get_zero64();

   status |= MBX_SET_STS_BY_MASK(status, is_zero(secretz, P384_LEN64+1), MBX_STATUS_MISMATCH_PARAM_ERR);

   if(!MBX_IS_ANY_OK_STS(status)) {
      /* clear copy of the secret keys */
      MB_FUNC_NAME(zero_)((int64u (*)[8])secretz, sizeof(secretz)/sizeof(U64));
      return status;
   }

   P384_POINT P;
   /* set party's public */
   ifma_BN_to_mb8((int64u (*)[8])P.X, (const BIGNUM* (*))pa_pubx, P384_BITSIZE); /* P-> radix 2^52 */
   ifma_BN_to_mb8((int64u (*)[8])P.Y, (const BIGNUM* (*))pa_puby, P384_BITSIZE);
   if(use_jproj_coords)
      ifma_BN_to_mb8((int64u (*)[8])P.Z, (const BIGNUM* (*))pa_pubz, P384_BITSIZE);
   else
      MB_FUNC_NAME(mov_FE384_)(P.Z, (U64*)ones);
   /* convert to Montgomery */
   MB_FUNC_NAME(ifma_tomont52_p384_)(P.X, P.X);
   MB_FUNC_NAME(ifma_tomont52_p384_)(P.Y, P.Y);
   MB_FUNC_NAME(ifma_tomont52_p384_)(P.Z, P.Z);

   /* check if P does not belong to EC */
   __mb_mask not_on_curve_mask = ~MB_FUNC_NAME(ifma_is_on_curve_p384_)(&P, use_jproj_coords);
   /* set points out of EC to infinity */
   MB_FUNC_NAME(mask_set_point_to_infinity_)(&P, not_on_curve_mask);
   /* update status */
   status |= MBX_SET_STS_BY_MASK(status, not_on_curve_mask, MBX_STATUS_MISMATCH_PARAM_ERR);

   if(!MBX_IS_ANY_OK_STS(status)) {
      /* clear copy of the secret keys */
      MB_FUNC_NAME(zero_)((int64u (*)[8])secretz, sizeof(secretz)/sizeof(U64));
      return status;
   }

   P384_POINT R;
   /* compute R = [secretz]*P */
   MB_FUNC_NAME(ifma_ec_nistp384_mul_point_)(&R, &P, secretz);

   /* clear ephemeral secret copy */
   MB_FUNC_NAME(zero_)((int64u (*)[8])secretz, sizeof(secretz)/sizeof(U64));

   /* return affine R.x */
   __ALIGN64 U64 Z2[P384_LEN52];
   ifma_aminv52_p384_mb8(Z2, R.Z);     /* 1/Z   */
   ifma_ams52_p384_mb8(Z2, Z2);        /* 1/Z^2 */
   ifma_amm52_p384_mb8(R.X, R.X, Z2);  /* x = (X) * (1/Z^2) */
   /* to regular domain */
   MB_FUNC_NAME(ifma_frommont52_p384_)(R.X, R.X);

   /* store result */
   ifma_mb8_to_HexStr8(pa_shared_key, (const int64u (*)[8])R.X, P384_BITSIZE);

   /* clear shared secret */
   MB_FUNC_NAME(zero_)((int64u (*)[8])&R, sizeof(R)/sizeof(U64));

   return status;
}
#endif // BN_OPENSSL_DISABLE

DLL_PUBLIC
mbx_status mbx_nistp384_ecdh_mb8(int8u* pa_shared_key[8],
                          const int64u* const pa_skey[8],
                          const int64u* const pa_pubx[8],
                          const int64u* const pa_puby[8],
                          const int64u* const pa_pubz[8],
                                 int8u* pBuffer)
{
   mbx_status status = 0;
   int buf_no;

   /* pa_pubz!=0 means the output is in Jacobian projective coordinates */
   int use_jproj_coords = NULL!=pa_pubz;

   /* test input pointers */
   if(NULL==pa_shared_key || NULL==pa_skey || NULL==pa_pubx || NULL==pa_puby) {
      status = MBX_SET_STS_ALL(MBX_STATUS_NULL_PARAM_ERR);
      return status;
   }

   /* check pointers and values */
   for(buf_no=0; buf_no<8; buf_no++) {
      int8u* shared = pa_shared_key[buf_no];
      const int64u* skey = pa_skey[buf_no];
      const int64u* pubx = pa_pubx[buf_no];
      const int64u* puby = pa_puby[buf_no];
      const int64u* pubz = use_jproj_coords? pa_pubz[buf_no] : NULL;

      /* if any of pointer NULL set error status */
      if(NULL==shared || NULL==skey || NULL==pubx || NULL== puby || (use_jproj_coords && NULL==pubz)) {
         status = MBX_SET_STS(status, buf_no, MBX_STATUS_NULL_PARAM_ERR);
      }
   }

   if(!MBX_IS_ANY_OK_STS(status))
      return status;

   /*
   // processing
   */

   /* zero padded private keys */
   U64 secretz[P384_LEN64+1];
   ifma_BNU_transpose_copy((int64u (*)[8])secretz, (const int64u**)pa_skey, P384_BITSIZE);
   secretz[P384_LEN64] = get_zero64();

   status |= MBX_SET_STS_BY_MASK(status, is_zero(secretz, P384_LEN64+1), MBX_STATUS_MISMATCH_PARAM_ERR);

   if(!MBX_IS_ANY_OK_STS(status)) {
      /* clear copy of the secret keys */
      MB_FUNC_NAME(zero_)((int64u (*)[8])secretz, sizeof(secretz)/sizeof(U64));
      return status;
   }

   P384_POINT P;
   /* set party's public */
   ifma_BNU_to_mb8((int64u (*)[8])P.X, (const int64u* (*))pa_pubx, P384_BITSIZE); // P-> crypto_mb radix 2^52
   ifma_BNU_to_mb8((int64u (*)[8])P.Y, (const int64u* (*))pa_puby, P384_BITSIZE);
   if(use_jproj_coords)
      ifma_BNU_to_mb8((int64u (*)[8])P.Z, (const int64u* (*))pa_pubz, P384_BITSIZE);
   else
      MB_FUNC_NAME(mov_FE384_)(P.Z, (U64*)ones);
   /* convert to Montgomery */
   MB_FUNC_NAME(ifma_tomont52_p384_)(P.X, P.X);
   MB_FUNC_NAME(ifma_tomont52_p384_)(P.Y, P.Y);
   MB_FUNC_NAME(ifma_tomont52_p384_)(P.Z, P.Z);

   /* check if P does not belong to EC */
   __mb_mask not_on_curve_mask = ~MB_FUNC_NAME(ifma_is_on_curve_p384_)(&P, use_jproj_coords);
   /* set points out of EC to infinity */
   MB_FUNC_NAME(mask_set_point_to_infinity_)(&P, not_on_curve_mask);
   /* update status */
   status |= MBX_SET_STS_BY_MASK(status, not_on_curve_mask, MBX_STATUS_MISMATCH_PARAM_ERR);

   if(!MBX_IS_ANY_OK_STS(status)) {
      /* clear copy of the secret keys */
      MB_FUNC_NAME(zero_)((int64u (*)[8])secretz, sizeof(secretz)/sizeof(U64));
      return status;
   }

   P384_POINT R;
   /* compute R = [secretz]*P */
   MB_FUNC_NAME(ifma_ec_nistp384_mul_point_)(&R, &P, secretz);

   /* clear ephemeral secret copy */
   MB_FUNC_NAME(zero_)((int64u (*)[8])secretz, sizeof(secretz)/sizeof(U64));

   /* return affine R.x */
   __ALIGN64 U64 Z2[P384_LEN52];
   ifma_aminv52_p384_mb8(Z2, R.Z);     /* 1/Z   */
   ifma_ams52_p384_mb8(Z2, Z2);        /* 1/Z^2 */
   ifma_amm52_p384_mb8(R.X, R.X, Z2);  /* x = (X) * (1/Z^2) */
   /* to regular domain */
   MB_FUNC_NAME(ifma_frommont52_p384_)(R.X, R.X);

   /* store result */
   ifma_mb8_to_HexStr8(pa_shared_key, (const int64u (*)[8])R.X, P384_BITSIZE);

   /* clear shared secret */
   MB_FUNC_NAME(zero_)((int64u (*)[8])&R, sizeof(R)/sizeof(U64));

   return status;
}
