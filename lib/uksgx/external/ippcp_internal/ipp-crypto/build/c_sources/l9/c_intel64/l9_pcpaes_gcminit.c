/*******************************************************************************
* Copyright 2013-2021 Intel Corporation
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
//     AES-GCM
//
//  Contents:
//        ippsAES_GCMInit()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpaesm.h"
#include "pcptool.h"

#if (_ALG_AES_SAFE_==_ALG_AES_SAFE_COMPACT_SBOX_)
#  include "pcprijtables.h"
#endif

#if(_IPP32E>=_IPP32E_K0)
#include "pcpaesauthgcm_avx512.h"
#include "aes_keyexp.h"
#else
#include "pcpaesauthgcm.h"
#endif /* #if(_IPP32E>=_IPP32E_K0) */

/*F*
//    Name: ippsAES_GCMInit
//
// Purpose: Init AES_GCM context for future usage.
//
// Returns:                Reason:
//    ippStsNullPtrErr        pState == NULL
//    ippStsMemAllocErr       size of buffer is not match fro operation
//    ippStsLengthErr         keyLen != 16 &&
//                                   != 24 &&
//                                   != 32
//    ippStsNoErr             no errors
//
// Parameters:
//    pKey        pointer to the secret key
//    keyLen      length of secret key
//    pState      pointer to the AES-GCM context
//    ctxSize     available size (in bytes) of buffer above
//
*F*/
IPPFUN(IppStatus, ippsAES_GCMInit,(const Ipp8u* pKey, int keyLen, IppsAES_GCMState* pState, int ctxSize))
{
   /* test pCtx pointer */
   IPP_BAD_PTR1_RET(pState);

   /* test available size of context buffer */
   IPP_BADARG_RET(ctxSize<cpSizeofCtx_AESGCM(), ippStsMemAllocErr);

   /* use aligned context */
   pState = (IppsAES_GCMState*)( IPP_ALIGNED_PTR(pState, AESGCM_ALIGNMENT) );

   /* set and clear GCM context */
   AESGCM_SET_ID(pState);
   ippsAES_GCMReset(pState);

   #if(_IPP32E>=_IPP32E_K0)
   
   /* make sure in legal keyLen */
   IPP_BADARG_RET(keyLen!=16 && keyLen!=24 && keyLen!=32, ippStsLengthErr);
   AES_GCM_KEY_LEN(pState) = (Ipp64u)keyLen;

   Ipp8u zeroKey[32] = {0};
   const Ipp8u* pActualKey = pKey? pKey : zeroKey;

   if (IsFeatureEnabled(ippCPUID_AVX512VAES) && IsFeatureEnabled(ippCPUID_AVX512VCLMUL)) {

      switch AES_GCM_KEY_LEN(pState) {
         case 16:
            aes_keyexp_128_enc(pActualKey, &AES_GCM_KEY_DATA(pState));
            aes_gcm_precomp_128_vaes_avx512(&AES_GCM_KEY_DATA(pState));
            
            AES_GCM_ENCRYPT_UPDATE(pState)    = aes_gcm_enc_128_update_vaes_avx512; 
            AES_GCM_DECRYPT_UPDATE(pState)    = aes_gcm_dec_128_update_vaes_avx512;
            AES_GCM_GET_TAG(pState)           = aes_gcm_gettag_128_vaes_avx512;
            break;
         case 24:
            aes_keyexp_192_enc(pActualKey, &AES_GCM_KEY_DATA(pState));
            aes_gcm_precomp_192_vaes_avx512(&AES_GCM_KEY_DATA(pState));

            AES_GCM_ENCRYPT_UPDATE(pState)    = aes_gcm_enc_192_update_vaes_avx512; 
            AES_GCM_DECRYPT_UPDATE(pState)    = aes_gcm_dec_192_update_vaes_avx512;
            AES_GCM_GET_TAG(pState)           = aes_gcm_gettag_192_vaes_avx512;
            break;
         case 32:
            aes_keyexp_256_enc(pActualKey, &AES_GCM_KEY_DATA(pState));
            aes_gcm_precomp_256_vaes_avx512(&AES_GCM_KEY_DATA(pState));

            AES_GCM_ENCRYPT_UPDATE(pState)    = aes_gcm_enc_256_update_vaes_avx512; 
            AES_GCM_DECRYPT_UPDATE(pState)    = aes_gcm_dec_256_update_vaes_avx512;
            AES_GCM_GET_TAG(pState)           = aes_gcm_gettag_256_vaes_avx512;
            break;
      }

      AES_GCM_IV_UPDATE(pState)    = aes_gcm_iv_hash_update_vaes512;
      AES_GCM_IV_FINALIZE(pState)  = aes_gcm_iv_hash_finalize_vaes512;
      AES_GCM_AAD_UPDATE(pState)   = aes_gcm_aad_hash_update_vaes512;     
      AES_GCM_AAD_FINALIZE(pState) = aes_gcm_aad_hash_finalize_vaes512;
   }
   else {
      
      switch AES_GCM_KEY_LEN(pState) {
         case 16:
            aes_keyexp_128_enc(pActualKey, &AES_GCM_KEY_DATA(pState));
            aes_gcm_precomp_128_avx512(&AES_GCM_KEY_DATA(pState));

            AES_GCM_ENCRYPT_UPDATE(pState)    = aes_gcm_enc_128_update_avx512; 
            AES_GCM_DECRYPT_UPDATE(pState)    = aes_gcm_dec_128_update_avx512;
            AES_GCM_GET_TAG(pState)           = aes_gcm_gettag_128_avx512;
            break;
         case 24:
            aes_keyexp_192_enc(pActualKey, &AES_GCM_KEY_DATA(pState));
            aes_gcm_precomp_192_avx512(&AES_GCM_KEY_DATA(pState));

            AES_GCM_ENCRYPT_UPDATE(pState)    = aes_gcm_enc_192_update_avx512; 
            AES_GCM_DECRYPT_UPDATE(pState)    = aes_gcm_dec_192_update_avx512;
            AES_GCM_GET_TAG(pState)           = aes_gcm_gettag_192_avx512;
            break;
         case 32:
            aes_keyexp_256_enc(pActualKey, &AES_GCM_KEY_DATA(pState));
            aes_gcm_precomp_256_avx512(&AES_GCM_KEY_DATA(pState));

            AES_GCM_ENCRYPT_UPDATE(pState)    = aes_gcm_enc_256_update_avx512; 
            AES_GCM_DECRYPT_UPDATE(pState)    = aes_gcm_dec_256_update_avx512;
            AES_GCM_GET_TAG(pState)           = aes_gcm_gettag_256_avx512;
            break;
      }

      AES_GCM_IV_UPDATE(pState)    = aes_gcm_iv_hash_update_avx512;
      AES_GCM_IV_FINALIZE(pState)  = aes_gcm_iv_hash_finalize_avx512;
      AES_GCM_AAD_UPDATE(pState)   = aes_gcm_aad_hash_update_avx512;     
      AES_GCM_AAD_FINALIZE(pState) = aes_gcm_aad_hash_finalize_avx512;
   }

   #else

   /* init cipher */
   {
      IppStatus sts = ippsAESInit(pKey, keyLen, AESGCM_CIPHER(pState), cpSizeofCtx_AES());
      if(ippStsNoErr!=sts)
         return sts;
   }

   /* set up:
   // - ghash function
   // - authentication function
   */
   AESGCM_HASH(pState) = AesGcmMulGcm_table2K_ct;//AesGcmMulGcm_table2K;
   AESGCM_AUTH(pState) = AesGcmAuth_table2K_ct; //AesGcmAuth_table2K;
   AESGCM_ENC(pState)  = wrpAesGcmEnc_table2K;
   AESGCM_DEC(pState)  = wrpAesGcmDec_table2K;

   #if (_IPP>=_IPP_P8) || (_IPP32E>=_IPP32E_Y8)
   #if(_IPP32E>=_IPP32E_K0)
   if (IsFeatureEnabled(ippCPUID_AVX512VAES)) {
      AESGCM_HASH(pState) = AesGcmMulGcm_vaes;
      AESGCM_AUTH(pState) = AesGcmAuth_vaes;
      AESGCM_ENC(pState) = AesGcmEnc_vaes;
      AESGCM_DEC(pState) = AesGcmDec_vaes;
   }
   else
   #endif /* #if(_IPP32E>=_IPP32E_K0) */
   if( IsFeatureEnabled(ippCPUID_AES|ippCPUID_CLMUL) ) {
      AESGCM_HASH(pState) = AesGcmMulGcm_avx;
      AESGCM_AUTH(pState) = AesGcmAuth_avx;
      AESGCM_ENC(pState) = wrpAesGcmEnc_avx;
      AESGCM_DEC(pState) = wrpAesGcmDec_avx;
   }
   #endif

   /* precomputations (for constant multiplier(s)) */
   {
      IppsAESSpec* pAES = AESGCM_CIPHER(pState);
      RijnCipher encoder = RIJ_ENCODER(pAES);

      /* multiplier c = Enc({0}) */
      PadBlock(0, AESGCM_HKEY(pState), BLOCK_SIZE);
      //encoder((Ipp32u*)AESGCM_HKEY(pState), (Ipp32u*)AESGCM_HKEY(pState), RIJ_NR(pAES), RIJ_EKEYS(pAES), (const Ipp32u (*)[256])RIJ_ENC_SBOX(pAES));
      #if (_ALG_AES_SAFE_==_ALG_AES_SAFE_COMPACT_SBOX_)
      encoder(AESGCM_HKEY(pState), AESGCM_HKEY(pState), RIJ_NR(pAES), RIJ_EKEYS(pAES), RijEncSbox/*NULL*/);
      #else
      encoder(AESGCM_HKEY(pState), AESGCM_HKEY(pState), RIJ_NR(pAES), RIJ_EKEYS(pAES), NULL);
      #endif
   }

   #if (_IPP>=_IPP_P8) || (_IPP32E>=_IPP32E_Y8)
   #if(_IPP32E>=_IPP32E_K0)
   if (IsFeatureEnabled(ippCPUID_AVX512VAES)) {
      /* pre-compute hKey<<1, (hKey<<1)^2, (hKey<<1)^3, ... , (hKey<<1)^15 and corresponding
         Karatsuba constant multipliers for aggregated reduction */
      AesGcmPrecompute_vaes(AESGCM_CPWR(pState), AESGCM_HKEY(pState));   
   }
   else
   #endif /* #if(_IPP32E>=_IPP32E_K0) */
   if( IsFeatureEnabled(ippCPUID_AES|ippCPUID_CLMUL) ) {
      /* pre-compute reflect(hkey) and hKey<<1, (hKey<<1)^2 and (hKey<<1)^4 powers of hKey */
      AesGcmPrecompute_avx(AESGCM_CPWR(pState), AESGCM_HKEY(pState));
   }
   else
   #endif
      AesGcmPrecompute_table2K(AES_GCM_MTBL(pState), AESGCM_HKEY(pState));

   #endif /* #if(_IPP32E>=_IPP32E_K0) */

   return ippStsNoErr;
}
