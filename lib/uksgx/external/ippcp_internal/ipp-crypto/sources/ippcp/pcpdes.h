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
//     Internal Definitions and
//     Internal DES based Encrypt/Decrypt Function Prototypes
// 
// 
*/


#if !defined(_PCP_DES_H)
#define _PCP_DES_H


/*
// really DES round key saved in terms of pear of 24-bit half mentioned in FIPS 46-3
*/
typedef Ipp64u RoundKeyDES;
typedef Ipp32u HalfRoundKeyDES;

/*
// DES context
*/
struct _cpDES {
   Ipp32u       idCtx;        /* DES spec identifier           */
   RoundKeyDES  enc_keys[16]; /* array of keys for encryprion  */
   RoundKeyDES  dec_keys[16]; /* array of keys for decryprion  */
};

/* alignment */
#define DES_ALIGNMENT ((int)(sizeof(Ipp64u)))

#define MBS_DES   (8)      /* data block (bytes) */

/*
// Useful macros
*/
#define DES_SET_ID(ctx)       ((ctx)->idCtx = (Ipp32u)idCtxDES ^ (Ipp32u)IPP_UINT_PTR(ctx))
#define DES_RESET_ID(ctx)     ((ctx)->idCtx = (Ipp32u)idCtxDES)
#define DES_EKEYS(ctx)        ((ctx)->enc_keys)
#define DES_DKEYS(ctx)        ((ctx)->dec_keys)

#define VALID_DES_ID(ctx)  ((((ctx)->idCtx) ^ (Ipp32u)IPP_UINT_PTR((ctx))) == (Ipp32u)idCtxDES)

/*
// Internal Tables
*/
#define MITIGATED    (4)

#define IMPLEMENTATION MITIGATED


#if (IMPLEMENTATION == MITIGATED)
   extern const __ALIGN64 Ipp32u DESspbox[16*16];
#endif


/*
// internal functions
*/
#define SetKey_DES OWNAPI(SetKey_DES)
   IPP_OWN_DECL (void, SetKey_DES, (const Ipp8u* pKey, IppsDESSpec* pCtx))

#define Cipher_DES OWNAPI(Cipher_DES)
   IPP_OWN_DECL (Ipp64u, Cipher_DES, (Ipp64u inpBlk, const RoundKeyDES* pRKey, const Ipp32u spbox[]))

#define ENCRYPT_DES(blk, pCtx)   Cipher_DES((blk), DES_EKEYS((pCtx)), DESspbox)
#define DECRYPT_DES(blk, pCtx)   Cipher_DES((blk), DES_DKEYS((pCtx)), DESspbox)

/* TDES prototypes */
#define ECB_TDES OWNAPI(ECB_TDES)
   IPP_OWN_DECL (void, ECB_TDES, (const Ipp64u* pSrc, Ipp64u* pDst, int nBlocks, const RoundKeyDES* pRKey[3], const Ipp32u spbox[]))
#define EncryptCBC_TDES OWNAPI(EncryptCBC_TDES)
   IPP_OWN_DECL (void, EncryptCBC_TDES, (const Ipp64u* pSrc, Ipp64u* pDst, int nBlocks, const RoundKeyDES* pRKey[3], Ipp64u iv, const Ipp32u spbox[]))
#define DecryptCBC_TDES OWNAPI(DecryptCBC_TDES)
   IPP_OWN_DECL (void, DecryptCBC_TDES, (const Ipp64u* pSrc, Ipp64u* pDst, int nBlocks, const RoundKeyDES* pRKey[3], Ipp64u iv, const Ipp32u spbox[]))

#endif /* _PCP_DES_H */
