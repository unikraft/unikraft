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
//     Internal Rijndael based Encrypt/Decrypt Function Prototypes
//
//
*/

#if !defined(_PCP_RIJ_H)
#define _PCP_RIJ_H


/*
// The GF(256) modular polynomial and elements
*/
#define WPOLY  0x011B
#define BPOLY    0x1B

/*
// Make WORD using 4 arbitrary bytes
*/
#define BYTES_TO_WORD(b0,b1,b2,b3) ( ( ((Ipp32u)((Ipp8u)(b3))) <<24 ) \
                                    |( ((Ipp32u)((Ipp8u)(b2))) <<16 ) \
                                    |( ((Ipp32u)((Ipp8u)(b1))) << 8 ) \
                                    |( ((Ipp32u)((Ipp8u)(b0))) ) )
/*
// Make WORD setting byte in specified position
*/
#define BYTE0_TO_WORD(b)   BYTES_TO_WORD((b), 0,  0,  0)
#define BYTE1_TO_WORD(b)   BYTES_TO_WORD( 0, (b), 0,  0)
#define BYTE2_TO_WORD(b)   BYTES_TO_WORD( 0,  0, (b), 0)
#define BYTE3_TO_WORD(b)   BYTES_TO_WORD( 0,  0,  0, (b))

/*
// Extract byte from specified position n.
// Sure, n=0,1,2 or 3 only
*/
#define EBYTE(w,n) ((Ipp8u)((w) >> (8 * (n))))

/* alignment */
#define RIJ_ALIGNMENT (16)
/* alignment in words */
#define RIJ_ALIGNMENT_WORD (16/4)

#define MBS_RIJ128   (128/8)  /* message block size (bytes) */
#define MBS_RIJ192   (192/8)
#define MBS_RIJ256   (256/8)

#define SR          (4)            /* number of rows in STATE data */

#define NB(msgBlks) ((msgBlks)/32) /* message block size (words)     */
                                   /* 4-word for 128-bits data block */
                                   /* 6-word for 192-bits data block */
                                   /* 8-word for 256-bits data block */

#define NK(keybits) ((keybits)/32)   /* key length (words): */
#define NK128 NK(ippRijndaelKey128)  /* 4-word for 128-bits security key */
#define NK192 NK(ippRijndaelKey192)  /* 6-word for 192-bits security key */
#define NK256 NK(ippRijndaelKey256)  /* 8-word for 256-bits security key */

#define NR128_128 (10)  /* number of rounds data: 128 bits key: 128 bits are used */
#define NR128_192 (12)  /* number of rounds data: 128 bits key: 192 bits are used */
#define NR128_256 (14)  /* number of rounds data: 128 bits key: 256 bits are used */
#define NR192_128 (12)  /* number of rounds data: 192 bits key: 128 bits are used */
#define NR192_192 (12)  /* number of rounds data: 192 bits key: 192 bits are used */
#define NR192_256 (14)  /* number of rounds data: 192 bits key: 256 bits are used */
#define NR256_128 (14)  /* number of rounds data: 256 bits key: 128 bits are used */
#define NR256_192 (14)  /* number of rounds data: 256 bits key: 192 bits are used */
#define NR256_256 (14)  /* number of rounds data: 256 bits key: 256 bits are used */

#define NSK128_256  ((NR128_256+1)*NK128) /* max number of scheduled keys for 128-bit data (256-bit key) (in words) */
#define NSK192_256  ((NR192_256+1)*NK192) /* max number of scheduled keys for 192-bit data (256-bit key) (in words) */
#define NSK256_256  ((NR256_256+1)*NK256) /* max number of scheduled keys for 256-bit data (256-bit key) (in words) */

/*
// Rijndael's spec
//
// Rijndael128, Rijndael192 and Rijndael256
// reserve space for maximum number of expanded keys
*/
IPP_OWN_FUNPTR (void, RijnCipher, (const Ipp8u* pInpBlk, Ipp8u* pOutBlk, int nr, const Ipp8u* pKeys, const void* pTbl))

struct _cpRijndael128 {
   Ipp32u      idCtx;                      /* Rijndael spec identifier      */
   int         nk;                         /* security key length (words)   */
   int         nb;                         /* data block size (words)       */
   int         nr;                         /* number of rounds              */
   RijnCipher  encoder;                    /* encoder/decoder               */
   RijnCipher  decoder;                    /* entry point                   */
   Ipp32u*     pEncTbl;                    /* expanded S-boxes for          */
   Ipp32u*     pDecTbl;                    /* encryption and decryption     */
   Ipp8u*      pEncKeys;                   /* pointer to array of keys for encryption  */
   Ipp8u*      pDecKeys;                   /* pointer to array of keys for decryption  */
   Ipp32u      aesNI;                      /* AES instruction available     */
   Ipp32u      safeInit;                   /* SafeInit performed            */
   Ipp32u      keys[2*NSK128_256 + RIJ_ALIGNMENT_WORD]; /* array of keys for encryption/decryption  */
};

struct _cpRijndael192 {
   Ipp32u      idCtx;                      /* Rijndael spec identifier      */
   int         nk;                         /* security key length (words)   */
   int         nb;                         /* data block size (words)       */
   int         nr;                         /* number of rounds              */
   RijnCipher  encoder;                    /* encoder/decoder               */
   RijnCipher  decoder;                    /* entry point                   */
   Ipp32u*     pEncTbl;                    /* expanded S-boxes for          */
   Ipp32u*     pDecTbl;                    /* encryption and decryption     */
   Ipp8u*      pEncKeys;                   /* pointer to array of keys for encryption  */
   Ipp8u*      pDecKeys;                   /* pointer to array of keys for decryption  */
   Ipp32u      aesNI;                      /* AES instruction available     */
   Ipp32u      safeInit;                   /* SafeInit performed            */
   Ipp32u      keys[2*NSK192_256 + RIJ_ALIGNMENT_WORD]; /* array of keys for encryption/decryption  */
};

 struct _cpRijndael256 {
   Ipp32u      idCtx;                        /* Rijndael spec identifier      */
   int         nk;                           /* security key length (words)   */
   int         nb;                           /* data block size (words)       */
   int         nr;                           /* number of rounds              */
   RijnCipher  encoder;                      /* encoder/decoder               */
   RijnCipher  decoder;                      /* entry point                   */
   Ipp32u*     pEncTbl;                      /* expanded S-boxes for          */
   Ipp32u*     pDecTbl;                      /* encryption and decryption     */
   Ipp8u*      pEncKeys;                     /* pointer array of keys for encryption  */
   Ipp8u*      pDecKeys;                     /* pointer array of keys for decryprion  */
   Ipp32u      aesNI;                        /* AES instruction available     */
   Ipp32u      safeInit;                     /* SafeInit performed            */
   Ipp32u      keys[2*NSK256_256 + RIJ_ALIGNMENT_WORD];   /* array of keys for encryption/decryption  */
};

#define AES_MB_MAX_KERNEL_SIZE (16) /* max number of buffers in multi buffer */
#define CFB16_BLOCK_SIZE (16)       /* CFB mode block size for multi buffer  */

/*
// Useful macros
*/
#define RIJ_SET_ID(ctx)    ((ctx)->idCtx = (Ipp32u)idCtxRijndael ^ (Ipp32u)IPP_UINT_PTR(ctx))
#define RIJ_RESET_ID(ctx)  ((ctx)->idCtx = (Ipp32u)idCtxRijndael)
#define RIJ_NB(ctx)        ((ctx)->nb)
#define RIJ_NK(ctx)        ((ctx)->nk)
#define RIJ_NR(ctx)        ((ctx)->nr)
#define RIJ_ENCODER(ctx)   ((ctx)->encoder)
#define RIJ_DECODER(ctx)   ((ctx)->decoder)
#define RIJ_ENC_SBOX(ctx)  ((ctx)->pEncTbl)
#define RIJ_DEC_SBOX(ctx)  ((ctx)->pDecTbl)
#define RIJ_EKEYS(ctx)     ((ctx)->pEncKeys)
#define RIJ_DKEYS(ctx)     ((ctx)->pDecKeys)
#define RIJ_AESNI(ctx)     ((ctx)->aesNI)
#define RIJ_SAFE_INIT(ctx) ((ctx)->safeInit)
#define RIJ_KEYS_BUFFER(ctx) ((ctx)->keys)

#define VALID_RIJ_ID(ctx)  ((((ctx)->idCtx) ^ (Ipp32u)IPP_UINT_PTR((ctx))) == (Ipp32u)idCtxRijndael)

/*
// Internal functions
*/
#if (_ALG_AES_SAFE_==_ALG_AES_SAFE_COMPOSITE_GF_)
#define SafeEncrypt_RIJ128 OWNAPI(SafeEncrypt_RIJ128)
   IPP_OWN_DECL (void, SafeEncrypt_RIJ128, (const Ipp8u* pInpBlk, Ipp8u* pOutBlk, int nr, const Ipp8u* pKeys, const void* pTbl))
#define SafeDecrypt_RIJ128 OWNAPI(SafeDecrypt_RIJ128)
   IPP_OWN_DECL (void, SafeDecrypt_RIJ128, (const Ipp8u* pInpBlk, Ipp8u* pOutBlk, int nr, const Ipp8u* pKeys, const void* pTbl))
#endif

#if  (_ALG_AES_SAFE_==_ALG_AES_SAFE_COMPACT_SBOX_)
#define Safe2Encrypt_RIJ128 OWNAPI(Safe2Encrypt_RIJ128)
   IPP_OWN_DECL (void, Safe2Encrypt_RIJ128, (const Ipp8u* pInpBlk, Ipp8u* pOutBlk, int nr, const Ipp8u* pKeys, const void* pTbl))
#define Safe2Decrypt_RIJ128 OWNAPI(Safe2Decrypt_RIJ128)
   IPP_OWN_DECL (void, Safe2Decrypt_RIJ128, (const Ipp8u* pInpBlk, Ipp8u* pOutBlk, int nr, const Ipp8u* pKeys, const void* pTbl))
#endif

#if (_IPP>=_IPP_P8) || (_IPP32E>=_IPP32E_Y8)
#define Encrypt_RIJ128_AES_NI OWNAPI(Encrypt_RIJ128_AES_NI)
   IPP_OWN_DECL (void, Encrypt_RIJ128_AES_NI, (const Ipp8u* pInpBlk, Ipp8u* pOutBlk, int nr, const Ipp8u* pKeys, const void* pTbl))
#define EncryptECB_RIJ128pipe_AES_NI OWNAPI(EncryptECB_RIJ128pipe_AES_NI)
   IPP_OWN_DECL (void, EncryptECB_RIJ128pipe_AES_NI, (const Ipp8u* pSrc, Ipp8u* pDst, int nr, const Ipp8u* pKeys, int len))
#define EncryptCBC_RIJ128_AES_NI OWNAPI(EncryptCBC_RIJ128_AES_NI)
   IPP_OWN_DECL (void, EncryptCBC_RIJ128_AES_NI, (const Ipp8u* pSrc, Ipp8u* pDst, int nr, const Ipp8u* pKeys, int len, const Ipp8u* pIV))
#define EncryptCTR_RIJ128pipe_AES_NI OWNAPI(EncryptCTR_RIJ128pipe_AES_NI)
   IPP_OWN_DECL (void, EncryptCTR_RIJ128pipe_AES_NI, (const Ipp8u* pSrc, Ipp8u* pDst, int nr, const Ipp8u* pKeys, int len, Ipp8u* pCtrValue, const Ipp8u* pCtrBitMask))
#define EncryptStreamCTR32_AES_NI OWNAPI(EncryptStreamCTR32_AES_NI)
   IPP_OWN_DECL (void, EncryptStreamCTR32_AES_NI, (const Ipp8u* pSrc, Ipp8u* pDst, int nr, const Ipp8u* pKeys, int len, Ipp8u* pCtrValue))

#define EncryptCFB_RIJ128_AES_NI OWNAPI(EncryptCFB_RIJ128_AES_NI)
   IPP_OWN_DECL (void, EncryptCFB_RIJ128_AES_NI, (const Ipp8u* pSrc, Ipp8u* pDst, int nr, const Ipp8u* pKeys, int len, int cfbBlkSize, const Ipp8u* pIV))
#define EncryptCFB32_RIJ128_AES_NI OWNAPI(EncryptCFB32_RIJ128_AES_NI)
   IPP_OWN_DECL (void, EncryptCFB32_RIJ128_AES_NI, (const Ipp8u* pSrc, Ipp8u* pDst, int nr, const Ipp8u* pKeys, int len, int cfbBlkSize, const Ipp8u* pIV))
#define EncryptCFB128_RIJ128_AES_NI OWNAPI(EncryptCFB128_RIJ128_AES_NI)
   IPP_OWN_DECL (void, EncryptCFB128_RIJ128_AES_NI, (const Ipp8u* pSrc, Ipp8u* pDst, int nr, const Ipp8u* pKeys, int len, const Ipp8u* pIV))
#define EncryptOFB_RIJ128_AES_NI OWNAPI(EncryptOFB_RIJ128_AES_NI)
   IPP_OWN_DECL (void, EncryptOFB_RIJ128_AES_NI, (const Ipp8u* pSrc, Ipp8u* pDst, int nr, const Ipp8u* pKeys, int len, int ofbBlkSize, Ipp8u* pIV))
#define EncryptOFB128_RIJ128_AES_NI OWNAPI(EncryptOFB128_RIJ128_AES_NI)
   IPP_OWN_DECL (void, EncryptOFB128_RIJ128_AES_NI, (const Ipp8u* pSrc, Ipp8u* pDst, int nr, const Ipp8u* pKeys, int len, Ipp8u* pIV))

#define Decrypt_RIJ128_AES_NI OWNAPI(Decrypt_RIJ128_AES_NI)
   IPP_OWN_DECL (void, Decrypt_RIJ128_AES_NI, (const Ipp8u* pInpBlk, Ipp8u* pOutBlk, int nr, const Ipp8u* pKeys, const void* pTbl))
#define DecryptECB_RIJ128pipe_AES_NI OWNAPI(DecryptECB_RIJ128pipe_AES_NI)
   IPP_OWN_DECL (void, DecryptECB_RIJ128pipe_AES_NI, (const Ipp8u* pSrc, Ipp8u* pDst, int nr, const Ipp8u* pKeys, int len))
#define DecryptCBC_RIJ128pipe_AES_NI OWNAPI(DecryptCBC_RIJ128pipe_AES_NI)
   IPP_OWN_DECL (void, DecryptCBC_RIJ128pipe_AES_NI, (const Ipp8u* pSrc, Ipp8u* pDst, int nr, const Ipp8u* pKeys, int len, const Ipp8u* pIV))
#define DecryptCFB_RIJ128pipe_AES_NI OWNAPI(DecryptCFB_RIJ128pipe_AES_NI)
   IPP_OWN_DECL (void, DecryptCFB_RIJ128pipe_AES_NI, (const Ipp8u* pSrc, Ipp8u* pDst, int nr, const Ipp8u* pKeys, int cfbBlocks, int cfbBlkSize, const Ipp8u* pIV))
#define DecryptCFB32_RIJ128pipe_AES_NI OWNAPI(DecryptCFB32_RIJ128pipe_AES_NI)
   IPP_OWN_DECL (void, DecryptCFB32_RIJ128pipe_AES_NI, (const Ipp8u* pSrc, Ipp8u* pDst, int nr, const Ipp8u* pKeys, int cfbBlocks, int cfbBlkSize, const Ipp8u* pIV))
#define DecryptCFB128_RIJ128pipe_AES_NI OWNAPI(DecryptCFB128_RIJ128pipe_AES_NI)
   IPP_OWN_DECL (void, DecryptCFB128_RIJ128pipe_AES_NI, (const Ipp8u* pSrc, Ipp8u* pDst, int nr, const Ipp8u* pKeys, int len, const Ipp8u* pIV))

#define cpExpandAesKey_NI OWNAPI(cpExpandAesKey_NI)
   IPP_OWN_DECL (void, cpExpandAesKey_NI, (const Ipp8u* pSecret, IppsAESSpec* pCtx))

#define cpAESEncryptXTS_AES_NI OWNAPI(cpAESEncryptXTS_AES_NI)
   IPP_OWN_DECL (void, cpAESEncryptXTS_AES_NI, (Ipp8u* outBlk, const Ipp8u* inpBlk, int nBlks, const Ipp8u* pRKey, int nr, Ipp8u* pTweak))
#define cpAESDecryptXTS_AES_NI OWNAPI(cpAESDecryptXTS_AES_NI)
   IPP_OWN_DECL (void, cpAESDecryptXTS_AES_NI, (Ipp8u* outBlk, const Ipp8u* inpBlk, int nBlks, const Ipp8u* pRKey, int nr, Ipp8u* pTweak))

#if (_IPP32E>=_IPP32E_K1)
#define cpAESEncryptXTS_VAES OWNAPI(cpAESEncryptXTS_VAES)
   IPP_OWN_DECL (void, cpAESEncryptXTS_VAES, (Ipp8u* outBlk, const Ipp8u* inpBlk, int nBlks, const Ipp8u* pRKey, int nr, Ipp8u* pTweak))
#define cpAESDecryptXTS_VAES OWNAPI(cpAESDecryptXTS_VAES)
   IPP_OWN_DECL (void, cpAESDecryptXTS_VAES, (Ipp8u* outBlk, const Ipp8u* inpBlk, int nBlks, const Ipp8u* pRKey, int nr, Ipp8u* pTweak))

#define EncryptECB_RIJ128pipe_VAES_NI OWNAPI(EncryptECB_RIJ128pipe_VAES_NI)
   IPP_OWN_DECL (void, EncryptECB_RIJ128pipe_VAES_NI, (const Ipp8u* pSrc, Ipp8u* pDst, int len, const IppsAESSpec* pCtx))
#define EncryptCTR_RIJ128pipe_VAES_NI OWNAPI(EncryptCTR_RIJ128pipe_VAES_NI)
   IPP_OWN_DECL (void, EncryptCTR_RIJ128pipe_VAES_NI, (const Ipp8u* pSrc, Ipp8u* pDst, int nr, const Ipp8u* pKeys, int len, Ipp8u* pCtrValue, const Ipp8u* pCtrBitMask))
#define EncryptStreamCTR32_VAES_NI OWNAPI(EncryptStreamCTR32_VAES_NI)
   IPP_OWN_DECL (void, EncryptStreamCTR32_VAES_NI, (const Ipp8u* pSrc, Ipp8u* pDst, int nr, const Ipp8u* pKeys, int len, Ipp8u* pCtrValue))

#define DecryptECB_RIJ128pipe_VAES_NI OWNAPI(DecryptECB_RIJ128pipe_VAES_NI)
   IPP_OWN_DECL (void, DecryptECB_RIJ128pipe_VAES_NI, (const Ipp8u* pSrc, Ipp8u* pDst, int len, const IppsAESSpec* pCtx))
#define DecryptCBC_RIJ128pipe_VAES_NI OWNAPI(DecryptCBC_RIJ128pipe_VAES_NI)
   IPP_OWN_DECL (void, DecryptCBC_RIJ128pipe_VAES_NI, (const Ipp8u* pSrc, Ipp8u* pDst, int len, const IppsAESSpec* pCtx, const Ipp8u* pIV))

#define DecryptCFB_RIJ128pipe_VAES_NI OWNAPI(DecryptCFB_RIJ128pipe_VAES_NI)
   IPP_OWN_DECL (void, DecryptCFB_RIJ128pipe_VAES_NI, (const Ipp8u* pSrc, Ipp8u* pDst, int len, int cfbBlkSize, const IppsAESSpec* pCtx, const Ipp8u* pIV))
#define DecryptCFB64_RIJ128pipe_VAES_NI OWNAPI(DecryptCFB64_RIJ128pipe_VAES_NI)
   IPP_OWN_DECL (void, DecryptCFB64_RIJ128pipe_VAES_NI, (const Ipp8u* pSrc, Ipp8u* pDst, int len, const IppsAESSpec* pCtx, const Ipp8u* pIV))
#define DecryptCFB128_RIJ128pipe_VAES_NI OWNAPI(DecryptCFB128_RIJ128pipe_VAES_NI)
   IPP_OWN_DECL (void, DecryptCFB128_RIJ128pipe_VAES_NI, (const Ipp8u* pSrc, Ipp8u* pDst, int len, const IppsAESSpec* pCtx, const Ipp8u* pIV))
#endif /* _IPP32E>=_IPP32E_K1 */

#endif /* _IPP>=_IPP_P8 || _IPP32E>=_IPP32E_Y8 */

#define ExpandRijndaelKey OWNAPI(ExpandRijndaelKey)
   IPP_OWN_DECL (void, ExpandRijndaelKey, (const Ipp8u* pKey, int NK, int NB, int NR, int nKeys, Ipp8u* pEncKeys, Ipp8u* pDecKeys))

#if(_IPP>_IPP_PX || _IPP32E>_IPP32E_PX)
#define Touch_SubsDword_8uT OWNAPI(Touch_SubsDword_8uT)
   IPP_OWN_DECL (Ipp32u, Touch_SubsDword_8uT, (Ipp32u inp, const Ipp8u* pTbl, int tblLen))
#endif

#endif /* _PCP_RIJ_H */
