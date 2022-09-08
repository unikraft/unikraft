/*******************************************************************************
* Copyright 2021-2021 Intel Corporation
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
#include <internal/common/ifma_math.h>
#include <internal/common/ifma_cvt52.h>
#include <internal/rsa/ifma_rsa_arith.h>

#include <crypto_mb/ed25519.h>
#include <internal/ed25519/ifma_arith_ed25519.h>
#include <internal/ed25519/ifma_arith_p25519.h>
#include <internal/ed25519/ifma_arith_n25519.h>


/* length of SHA512 hash in bits and bytes */
#define SHA512_HASH_BITLENGTH  (512)
#define HASH_LENGTH            NUMBER_OF_DIGITS(SHA512_HASH_BITLENGTH, 8)

#include <openssl/sha.h>


static void ed25519_expand_key(int8u* pa_secret_expand[8], const ed25519_private_key* const pa_secret_key[8])
{
   /* do hash of secret keys in sequence */
   for (int n = 0; n < 8; n++) {
      const ed25519_private_key* psecret = pa_secret_key[n];
      if (psecret) {
         SHA512(*psecret, sizeof(ed25519_private_key), pa_secret_expand[n]);
         /* prune the buffer according to RFC8032 */
         pa_secret_expand[n][0]  &= 0xf8;
         pa_secret_expand[n][31] &= 0x7f;
         pa_secret_expand[n][31] |= 0x40;
      }
   }
}

/*
// Computes public key
// pa_public_key[]   array of pointers to the public keys X-coordinates
// pa_secret_key[]   array of pointers to the public keys Y-coordinates
*/
DLL_PUBLIC
mbx_status MB_FUNC_NAME(mbx_ed25519_public_key_)(ed25519_public_key* pa_public_key[8],
                                           const ed25519_private_key* const pa_private_key[8])
{
   mbx_status status = MBX_STATUS_OK;

   /* test input pointers */
   if (NULL == pa_private_key || NULL == pa_public_key) {
      status = MBX_SET_STS_ALL(MBX_STATUS_NULL_PARAM_ERR);
      return status;
   }

   /* check pointers and values */
   int buf_no;
   for (buf_no = 0; buf_no < 8; buf_no++) {
      const ed25519_private_key* private_key = pa_private_key[buf_no];
      ed25519_public_key* public_key = pa_public_key[buf_no];

      /* if any of pointer NULL set error status */
      if (NULL == private_key || NULL == public_key) {
         status = MBX_SET_STS(status, buf_no, MBX_STATUS_NULL_PARAM_ERR);
      }
   }

   /* continue processing if there are correct parameters */
   if (MBX_IS_ANY_OK_STS(status)) {

      int8u sha512_digest[8][HASH_LENGTH] = { 0 };
      int64u* pa_sha512_digest[] = {
         (int64u*)sha512_digest[0], (int64u*)sha512_digest[1], (int64u*)sha512_digest[2], (int64u*)sha512_digest[3],
         (int64u*)sha512_digest[4], (int64u*)sha512_digest[5], (int64u*)sha512_digest[6], (int64u*)sha512_digest[7]
      };
      /* expand secret keys */
      ed25519_expand_key((int8u**)pa_sha512_digest, pa_private_key);

      /* convert into mb fromat */
      U64 scalar[FE_LEN64];
      ifma_BNU_transpose_copy((int64u(*)[8])scalar, (const int64u**)pa_sha512_digest, SHA512_HASH_BITLENGTH/2);

      /* r = [scalar]*G */
      ge52_ext_mb r;
      ifma_ed25519_mul_pointbase(&r, scalar);

      /* compress point to public */
      fe52_mb packed_point;
      ge52_ext_compress(packed_point, &r);

      /* return result */
      ifma_mb8_to_BNU((int64u * const*)pa_public_key, (const int64u(*)[8])packed_point, GE25519_COMP_BITSIZE);

      /* clear memory containing potentially secret data */
      MB_FUNC_NAME(zero_)((int64u(*)[8])scalar, sizeof(scalar)/sizeof(U64));
      MB_FUNC_NAME(zero_)((int64u(*)[8])sha512_digest, sizeof(sha512_digest)/sizeof(U64));
   }

   return status;
}

/*
// Computes ED2519 signature
// pa_sign_r[]       array of pointers to the computed r-components of the signatures
// pa_sign_s[]       array of pointers to the computed s-components of the signatures
// pa_msg[]          array of pointers to the messages are being signed
// msgLen[]          array of messages lengths (in bytes) 
// pa_private_key[]  array of pointers to the signer's private keys
// pa_public_key[]   array of pointers to the signer's public keys
*/
static void ed25519_nonce(int8u* pa_nonce[8], int8u* pa_az[8], const int8u* const pa_msg[8], const int32u msgLen[8])
{
   SHA512_CTX ctx;

   for(int n=0; n<8; n++) {
      int8u* az = pa_az[n];
      const int8u* msg = pa_msg[n];
      int32u mlen = msgLen[n];
      SHA512_Init(&ctx);
      SHA512_Update(&ctx, az, 32);
      if (msg)
         SHA512_Update(&ctx, msg, mlen);
      SHA512_Final(pa_nonce[n], &ctx);
   }

   OPENSSL_cleanse(&ctx, sizeof(ctx));
}

static void ed25519_hram(int8u* pa_hram[8], const ed25519_sign_component* pa_sign_r[], const ed25519_public_key* const pa_public_key[8], const int8u* const pa_msg[8], const int32u msgLen[8])
{
   SHA512_CTX ctx;

   for (int n = 0; n < 8; n++) {
      const ed25519_sign_component* sign_r = pa_sign_r[n];
      const ed25519_public_key* public = pa_public_key[n];
      const int8u* msg = pa_msg[n];
      int32u mlen = msgLen[n];
      SHA512_Init(&ctx);
      if(sign_r)
         SHA512_Update(&ctx, sign_r, NUMBER_OF_DIGITS(GE25519_COMP_BITSIZE, 8));
      if(public)
         SHA512_Update(&ctx, public, sizeof(ed25519_public_key));
      if(msg)
         SHA512_Update(&ctx, msg, mlen);
      SHA512_Final(pa_hram[n], &ctx);
   }

   OPENSSL_cleanse(&ctx, sizeof(ctx));
}

DLL_PUBLIC
mbx_status MB_FUNC_NAME(mbx_ed25519_sign_)(ed25519_sign_component* pa_sign_r[8],
                                           ed25519_sign_component* pa_sign_s[8],
                                           const int8u* const pa_msg[8], const int32u msgLen[8],
                                           const ed25519_private_key* const pa_private_key[8],
                                           const ed25519_public_key* const pa_public_key[8])
{
   mbx_status status = MBX_STATUS_OK;

   /* test input pointers */
   if(NULL == pa_sign_r || NULL == pa_sign_s ||
      NULL == pa_msg || NULL == msgLen ||
      NULL == pa_private_key || NULL== pa_public_key) {
      status = MBX_SET_STS_ALL(MBX_STATUS_NULL_PARAM_ERR);
      return status;
   }

   /* check pointers and values */
   int buf_no;
   for (buf_no = 0; buf_no < 8; buf_no++) {
      ed25519_sign_component* sign_r = pa_sign_r[buf_no];
      ed25519_sign_component* sign_s = pa_sign_s[buf_no];
      const int8u* msg = pa_msg[buf_no];
      const ed25519_private_key* secret = pa_private_key[buf_no];
      const ed25519_public_key* public = pa_public_key[buf_no];

      /* if any of pointer NULL set error status */
      if(NULL == sign_r || NULL == sign_s || NULL== msg ||
         NULL == secret || NULL == public) {
         status = MBX_SET_STS(status, buf_no, MBX_STATUS_NULL_PARAM_ERR);
      }
   }

   /* continue processing if there are correct parameters */
   if (MBX_IS_ANY_OK_STS(status)) {
      //#define HASH_LENGTH    NUMBER_OF_DIGITS(SHA512_DIGEST_BITLENGTH, 8)

      /* expanded secret keys */
      int8u az[8][HASH_LENGTH] = { 0 };
      int64u* pa_az[8] = {
         (int64u*)az[0], (int64u*)az[1], (int64u*)az[2], (int64u*)az[3],
         (int64u*)az[4], (int64u*)az[5], (int64u*)az[6], (int64u*)az[7]
      };
      int64u* pa_az_hi[8] = {
         (int64u*)(az[0]+HASH_LENGTH/2), (int64u*)(az[1]+HASH_LENGTH/2), (int64u*)(az[2]+HASH_LENGTH/2), (int64u*)(az[3]+HASH_LENGTH/2),
         (int64u*)(az[4]+HASH_LENGTH/2), (int64u*)(az[5]+HASH_LENGTH/2), (int64u*)(az[6]+HASH_LENGTH/2), (int64u*)(az[7]+HASH_LENGTH/2)
      };

      /* nonce */
      int8u nonce[8][HASH_LENGTH] = { 0 };
      int64u* pa_nonce[8] = {
         (int64u*)nonce[0], (int64u*)nonce[1], (int64u*)nonce[2], (int64u*)nonce[3],
         (int64u*)nonce[4], (int64u*)nonce[5], (int64u*)nonce[6], (int64u*)nonce[7]
      };

      /* hram */
      int8u hram[8][HASH_LENGTH] = { 0 };
      int64u* pa_hram[8] = {
         (int64u*)hram[0], (int64u*)hram[1], (int64u*)hram[2], (int64u*)hram[3],
         (int64u*)hram[4], (int64u*)hram[5], (int64u*)hram[6], (int64u*)hram[7]
      };

      /* expands secret key,  az = H(private_key) */
      ed25519_expand_key((int8u**)pa_az, pa_private_key);
      U64 mb_az[FE_LEN52];
      ifma_BNU_to_mb8((int64u(*)[8])mb_az, (const int64u**)pa_az, SHA512_HASH_BITLENGTH/2);

      /* computes nonce, nonce = H(az+32, msg)  */
      ed25519_nonce((int8u**)pa_nonce, (int8u**)pa_az_hi, pa_msg, msgLen);

      /* reduce nonce wrt n */
      U64 mb_nonce[NUMBER_OF_DIGITS(SHA512_HASH_BITLENGTH, DIGIT_SIZE)];
      ifma_BNU_to_mb8((int64u(*)[8])mb_nonce, (const int64u**)pa_nonce, SHA512_HASH_BITLENGTH);
      ifma52_ed25519n_reduce(mb_nonce, mb_nonce);

      /* computes r-component of the signature */
      ifma_mb8_to_BNU(pa_nonce, (const int64u(*)[8])mb_nonce, N25519_BITSIZE);
      fe52_mb mb_scalar;
      ifma_BNU_transpose_copy((int64u(*)[8])mb_scalar, (const int64u**)pa_nonce, N25519_BITSIZE);

      ge52_ext_mb R;
      ifma_ed25519_mul_pointbase(&R, mb_scalar);   /* R = [scalar]*G */
      fe52_mb mb_sign_r;
      ge52_ext_compress(mb_sign_r, &R);            /* compress point to r-component */

      /* store r-component of the  signature */
      ifma_mb8_to_BNU((int64u * const*)pa_sign_r, (const int64u(*)[8])mb_sign_r, GE25519_COMP_BITSIZE);

      /* computes hram, hram = H(r_sign, public_key,  msg)  */
      ed25519_hram((int8u**)pa_hram, (const ed25519_sign_component**)pa_sign_r, pa_public_key, pa_msg, msgLen);

      U64 mb_hram[NUMBER_OF_DIGITS(SHA512_HASH_BITLENGTH, DIGIT_SIZE)];
      ifma_BNU_to_mb8((int64u(*)[8])mb_hram, (const int64u**)pa_hram, SHA512_HASH_BITLENGTH);
      ifma52_ed25519n_reduce(mb_hram, mb_hram);

      /* s = hram*az + nonce */
      fe52_mb mb_sign_s;
      ifma52_ed25519n_madd(mb_sign_s, mb_hram, mb_az, mb_nonce);

      /* store s-component of the signature */
      ifma_mb8_to_BNU((int64u * const*)pa_sign_s, (const int64u(*)[8])mb_sign_s, N25519_BITSIZE);

      /* clear memory containing potentially secret data */
      MB_FUNC_NAME(zero_)((int64u(*)[8])az, sizeof(az)/sizeof(U64));
      MB_FUNC_NAME(zero_)((int64u(*)[8])nonce, sizeof(nonce)/sizeof(U64));
      MB_FUNC_NAME(zero_)((int64u(*)[8])mb_scalar, sizeof(mb_scalar)/sizeof(U64));
   }

   return status;
}
