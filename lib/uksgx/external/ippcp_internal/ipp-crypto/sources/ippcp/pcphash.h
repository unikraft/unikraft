/*******************************************************************************
* Copyright 2014-2021 Intel Corporation
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
//     Security Hash Standard
//     Internal Definitions and Internal Functions Prototypes
// 
// 
*/

#if !defined(_PCP_HASH_H)
#define _PCP_HASH_H


/* messge block size */
#define MBS_SHA1     (64)           /* SHA1 message block size (bytes) */
#define MBS_SHA256   (64)           /* SHA256 and SHA224               */
#define MBS_SHA224   (64)           /* SHA224                          */
#define MBS_SHA512   (128)          /* SHA512 and SHA384               */
#define MBS_SHA384   (128)          /* SHA384                          */
#define MBS_MD5      (64)           /* MD5                             */
#define MBS_SM3      (64)           /* SM3                             */
#define MBS_HASH_MAX (MBS_SHA512)   /* max message block size (bytes)  */

#define MAX_HASH_SIZE (IPP_SHA512_DIGEST_BITSIZE/8)   /* hash of the max len (bytes) */

/* size of processed message length representation (bytes) */
#define MLR_SHA1     (sizeof(Ipp64u))
#define MLR_SHA256   (sizeof(Ipp64u))
#define MLR_SHA224   (sizeof(Ipp64u))
#define MLR_SHA512   (sizeof(Ipp64u)*2)
#define MLR_SHA384   (sizeof(Ipp64u)*2)
#define MLR_MD5      (sizeof(Ipp64u))
#define MLR_SM3      (sizeof(Ipp64u))

/* hold some old definition for a purpose */
typedef Ipp32u DigestSHA1[5];   /* SHA1 digest   */
typedef Ipp32u DigestSHA224[7]; /* SHA224 digest */
typedef Ipp32u DigestSHA256[8]; /* SHA256 digest */
typedef Ipp64u DigestSHA384[6]; /* SHA384 digest */
typedef Ipp64u DigestSHA512[8]; /* SHA512 digest */
typedef Ipp32u DigestMD5[4];    /* MD5 digest */
typedef Ipp32u DigestSM3[8];    /* SM3 digest */

#define HASH_ALIGNMENT     ((int)(sizeof(void*)))
#define   SHA1_ALIGNMENT   HASH_ALIGNMENT
#define SHA224_ALIGNMENT   HASH_ALIGNMENT
#define SHA256_ALIGNMENT   HASH_ALIGNMENT
#define SHA384_ALIGNMENT   HASH_ALIGNMENT
#define SHA512_ALIGNMENT   HASH_ALIGNMENT
#define    MD5_ALIGNMENT   HASH_ALIGNMENT
#define    SM3_ALIGNMENT   HASH_ALIGNMENT


struct _cpSHA1 {
   Ipp32u      idCtx;      /* SHA1 identifier         */
   int         msgBuffIdx; /* buffer entry            */
   Ipp64u      msgLenLo;   /* message length (bytes)  */
   Ipp8u       msgBuffer[MBS_SHA1]; /* buffer         */
   DigestSHA1  msgHash;    /* intermediate hash       */
};

struct _cpSHA256 {
   Ipp32u       idCtx;        /* SHA224 identifier    */
   int          msgBuffIdx;   /* buffer entry         */
   Ipp64u       msgLenLo;     /* message length       */
   Ipp8u        msgBuffer[MBS_SHA256]; /* buffer      */
   DigestSHA256 msgHash;      /* intermediate hash    */
};

struct _cpSHA512 {
   Ipp32u       idCtx;        /* SHA384 identifier    */
   int          msgBuffIdx;   /* buffer entry         */
   Ipp64u       msgLenLo;     /* message length       */
   Ipp64u       msgLenHi;     /* message length       */
   Ipp8u        msgBuffer[MBS_SHA512]; /* buffer      */
   DigestSHA512 msgHash;      /* intermediate hash    */
};

struct _cpMD5 {
   Ipp32u       idCtx;        /* MD5 identifier       */
   int          msgBuffIdx;   /* buffer entry         */
   Ipp64u       msgLenLo;     /* message length       */
   Ipp8u        msgBuffer[MBS_MD5]; /* buffer         */
   DigestMD5    msgHash;      /* intermediate hash    */
};

struct _cpSM3 {
   Ipp32u       idCtx;        /* SM3    identifier    */
   int          msgBuffIdx;   /* buffer entry         */
   Ipp64u       msgLenLo;     /* message length       */
   Ipp8u        msgBuffer[MBS_SM3]; /* buffer         */
   DigestSM3    msgHash;      /* intermediate hash    */
};


/* hash alg attributes */
typedef struct _cpHashAttr {
   int         ivSize;        /* attr: length (bytes) of initial value cpHashIV */
   int         hashSize;      /* attr: length (bytes) of hash */
   int         msgBlkSize;    /* attr: length (bytes) of message block */
   int         msgLenRepSize; /* attr: length (bytes) in representation of processed message length */
   Ipp64u      msgLenMax[2];  /* attr: max message length (bytes) (low high) */
} cpHashAttr;

/* hash value */
typedef Ipp64u cpHash[IPP_SHA512_DIGEST_BITSIZE/BITSIZE(Ipp64u)]; /* hash value */

/* hash update function */
IPP_OWN_FUNPTR (void, cpHashProc, (void* pHash, const Ipp8u* pMsg, int msgLen, const void* pParam))

/* generalized hash context */
struct _cpHashCtx {
   Ipp32u      idCtx;                     /* hash identifier   */
   IppHashAlgId   algID;                  /* hash algorithm ID */
   Ipp64u      msgLenLo;                  /* processed message:*/
   Ipp64u      msgLenHi;                  /*           length  */
   cpHashProc  hashProc;                  /* hash update func  */
   const void* pParam;                    /* hashProc's params */
   cpHash      msgHash;                   /* intermadiate hash */
   int         msgBuffIdx;                /* buffer entry      */
   Ipp8u       msgBuffer[MBS_HASH_MAX];   /* buffer            */
};

/* accessors */
#define HASH_SET_ID(stt,ctxid)      ((stt)->idCtx = (Ipp32u)ctxid ^ (Ipp32u)IPP_UINT_PTR(stt))
#define HASH_RESET_ID(stt,ctxid)    ((stt)->idCtx = (Ipp32u)ctxid)
#define HASH_ALG_ID(stt)            ((stt)->algID)
#define HASH_LENLO(stt)             ((stt)->msgLenLo)
#define HASH_LENHI(stt)             ((stt)->msgLenHi)
#define HASH_FUNC(stt)              ((stt)->hashProc)
#define HASH_FUNC_PAR(stt)          ((stt)->pParam)
#define HASH_VALUE(stt)             ((stt)->msgHash)
#define HAHS_BUFFIDX(stt)           ((stt)->msgBuffIdx)
#define HASH_BUFF(stt)              ((stt)->msgBuffer)
#define HASH_VALID_ID(stt,ctxId)    ((((stt)->idCtx) ^ (Ipp32u)IPP_UINT_PTR((stt))) == (Ipp32u)ctxId)


/* initial hash values */
extern const Ipp32u SHA1_IV[];
extern const Ipp32u SHA256_IV[];
extern const Ipp32u SHA224_IV[];
extern const Ipp64u SHA512_IV[];
extern const Ipp64u SHA384_IV[];
extern const Ipp32u MD5_IV[];
extern const Ipp32u SM3_IV[];
extern const Ipp64u SHA512_224_IV[];
extern const Ipp64u SHA512_256_IV[];

/* hash alg additive constants */
extern __ALIGN16 const Ipp32u SHA1_cnt[];
extern __ALIGN16 const Ipp32u SHA256_cnt[];
extern __ALIGN16 const Ipp64u SHA512_cnt[];
extern __ALIGN16 const Ipp32u MD5_cnt[];
extern __ALIGN16 const Ipp32u SM3_cnt[];

/*  hash alg opt argument */
extern const void* cpHashProcFuncOpt[];

/* enabled hash alg */
extern const IppHashAlgId cpEnabledHashAlgID[];

/* hash alg IV (init value) */
extern const Ipp8u* cpHashIV[];

/* hash alg attribute DB */
extern const cpHashAttr cpHashAlgAttr[];

/* IV size helper */
__INLINE int cpHashIvSize(IppHashAlgId algID)
{ return cpHashAlgAttr[algID].ivSize; }

/* hash size helper */
__INLINE int cpHashSize(IppHashAlgId algID)
{ return cpHashAlgAttr[algID].hashSize; }

/* message block size helper */
__INLINE int cpHashMBS(IppHashAlgId algID)
{ return cpHashAlgAttr[algID].msgBlkSize; }

/* maps algID into enabled IppHashAlgId value */
__INLINE IppHashAlgId cpValidHashAlg(IppHashAlgId algID)
{
   /* maps algID into the valid range */
   algID = (((int)ippHashAlg_Unknown < (int)algID) && ((int)algID < (int)ippHashAlg_MaxNo))? algID : ippHashAlg_Unknown;
   return cpEnabledHashAlgID[algID];
}

/* common functions */
#define cpComputeDigest OWNAPI(cpComputeDigest)
   IPP_OWN_DECL (void, cpComputeDigest, (Ipp8u* pHashTag, int hashTagLen, const IppsHashState* pCtx))

/* processing functions */
#define UpdateSHA1   OWNAPI(UpdateSHA1)
   IPP_OWN_DECL (void, UpdateSHA1, (void* pHash, const Ipp8u* mblk, int mlen, const void* pParam))
#define UpdateSHA256 OWNAPI(UpdateSHA256)
   IPP_OWN_DECL (void, UpdateSHA256, (void* pHash, const Ipp8u* mblk, int mlen, const void* pParam))
#define UpdateSHA512 OWNAPI(UpdateSHA512)
   IPP_OWN_DECL (void, UpdateSHA512, (void* pHash, const Ipp8u* mblk, int mlen, const void* pParam))
#define UpdateMD5    OWNAPI(UpdateMD5)
   IPP_OWN_DECL (void, UpdateMD5, (void* pHash, const Ipp8u* mblk, int mlen, const void* pParam))
#define UpdateSM3    OWNAPI(UpdateSM3)
   IPP_OWN_DECL (void, UpdateSM3, (void* pHash, const Ipp8u* mblk, int mlen, const void* pParam))

#if (_SHA_NI_ENABLING_ == _FEATURE_TICKTOCK_) || (_SHA_NI_ENABLING_ == _FEATURE_ON_)
#define UpdateSHA1ni   OWNAPI(UpdateSHA1ni)
   IPP_OWN_DECL (void, UpdateSHA1ni, (void* pHash, const Ipp8u* mblk, int mlen, const void* pParam))
#define UpdateSHA256ni OWNAPI(UpdateSHA256ni)
   IPP_OWN_DECL (void, UpdateSHA256ni, (void* pHash, const Ipp8u* mblk, int mlen, const void* pParam))
#endif

/* general methods */
#define cpInitHash OWNAPI(cpInitHash)
   IPP_OWN_DECL (int, cpInitHash, (IppsHashState* pCtx, IppHashAlgId algID))
#define cpReInitHash OWNAPI(cpReInitHash)
   IPP_OWN_DECL (int, cpReInitHash, (IppsHashState* pCtx, IppHashAlgId algID))

#endif /* _PCP_HASH_H */
