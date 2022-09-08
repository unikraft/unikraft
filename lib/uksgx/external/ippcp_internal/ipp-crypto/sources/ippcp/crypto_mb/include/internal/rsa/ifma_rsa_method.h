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

#ifndef IFMA_RSA_METHOD_H
#define IFMA_RSA_METHOD_H

#include <crypto_mb/defs.h>


/* exponentiations */
typedef void(*EXP52x_65537_mb8)(int64u out[][8],
   const int64u base[][8],
   const int64u modulus[][8],
   const int64u toMont[][8],
   const int64u  k0[8],
   int64u work_buffer[][8]);
typedef void(*EXP52x_mb8)(int64u out[][8],
   const int64u base[][8],
   const int64u exponent[][8],
   const int64u modulus[][8],
   const int64u toMont[][8],
   const int64u k0_mb8[8],
   int64u work_buffer[][8]);


/*
// auxiliary mb8 arithmethic
*/
/* Mont reduction */
typedef void (*amred52x_mb8)(int64u res[][8], const int64u inpA[][8], const int64u inpM[][8], const int64u k0[8]);
/* Mont multiplication */
typedef void   (*ammul52x_mb8)(int64u* out_mb8, const int64u* inpA_mb8, const int64u* inpB_mb8, const int64u* inpM_mb8, const int64u* k0_mb8);
/* modular subtraction */
typedef void (*modsub52x_mb8)(int64u res[][8], const int64u inpA[][8], const int64u inpB[][8], const int64u inpM[][8]);
/* multiply and add */
typedef void (*addmul52x_mb8)(int64u res[][8], const int64u inpA[][8], const int64u inpB[][8]);

/* RSA operation */
typedef enum {
   RSA_PUB_KEY  = (0x01<<16),
   RSA_PRV2_KEY = (0x20<<16),
   RSA_PRV5_KEY = (0x50<<16),
} RSA_OP_ID;

/* RSA size */
typedef enum {
   RSA1024 = 1024,
   RSA2048 = 2048,
   RSA3072 = 3072,
   RSA4096 = 4096,
} RSA_BITSIZE_ID;

/* RSA ID */
#define RSA_ID(OP,BITSIZE) ((OP) | (BITSIZE))
#define OP_RSA_ID(ID)      ((ID) & (0xFF<<16))
#define BISIZE_RSA_ID(ID)  ((ID) & 0xFFFF)

struct _ifma_rsa_method {
   int          id;             /* exponentiation's id (=1/2/5 -- public(fixed)/private/private_crt */
   int          rsaBitsize;     /* size of rsa modulus (bits) */
   int          buffSize;       /* size of scratch buffer */
   //cvt52BN_to_mb8   cvt52;      /* convert non-contiguos BN to radix 2^52 and strore in mb8 forman */
   //tcopyBN_to_mb8   tcopy;      /* copy non-contiguos BN into mb8 format */
   EXP52x_65537_mb8 expfunc65537;   /* "exp52x_fix_mb8" fixed exponentiation */
   EXP52x_mb8       expfun;         /* "exp52x_arb_mb8" exponentiation */
   amred52x_mb8  amred52x;          /* reduction */
   ammul52x_mb8  ammul52x;          /* multiplication */
   modsub52x_mb8 modsub52x;         /* subtration */
   addmul52x_mb8 mla52x;            /* multiply & add */
};

#endif /* IFMA_RSA_METHOD_H */
