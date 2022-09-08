/*******************************************************************************
* Copyright 2007-2021 Intel Corporation
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
//     Internal Safe Rijndael Encrypt, Decrypt
// 
// 
*/

#if !defined(_PCP_RIJ_SAFE_H)
#define _PCP_RIJ_SAFE_H

#include "owncp.h"
#include "pcprijtables.h"
#include "pcpbnuimpl.h"

#if defined _PCP_RIJ_SAFE_OLD
/* old version */
#define TransformByte OWNAPI(TransformByte)
    IPP_OWN_DECL (Ipp8u, TransformByte, (Ipp8u x, const Ipp8u Transformation[]))
#define TransformNative2Composite OWNAPI(TransformNative2Composite)
    IPP_OWN_DECL (void, TransformNative2Composite, (Ipp8u out[16], const Ipp8u inp[16]))
#define TransformComposite2Native OWNAPI(TransformComposite2Native)
    IPP_OWN_DECL (void, TransformComposite2Native, (Ipp8u out[16], const Ipp8u inp[16]))
#define InverseComposite OWNAPI(InverseComposite)
    IPP_OWN_DECL (Ipp8u, InverseComposite, (Ipp8u x))
#define AddRoundKey OWNAPI(AddRoundKey)
    IPP_OWN_DECL (void, AddRoundKey, (Ipp8u out[], const Ipp8u inp[], const Ipp8u pKey[]))
#endif


#if !defined _PCP_RIJ_SAFE_OLD
/* new version */
#define TransformNative2Composite OWNAPI(TransformNative2Composite)
    IPP_OWN_DECL (void, TransformNative2Composite, (Ipp8u out[16], const Ipp8u inp[16]))
#define TransformComposite2Native OWNAPI(TransformComposite2Native)
    IPP_OWN_DECL (void, TransformComposite2Native, (Ipp8u out[16], const Ipp8u inp[16]))

/* add round key operation */
__INLINE void AddRoundKey(Ipp8u out[16], const Ipp8u inp[16], const Ipp8u rkey[16])
{
   ((Ipp64u*)out)[0] = ((Ipp64u*)inp)[0] ^ ((Ipp64u*)rkey)[0];
   ((Ipp64u*)out)[1] = ((Ipp64u*)inp)[1] ^ ((Ipp64u*)rkey)[1];
}

/* add logs of GF(2^4) elements
// the exp table has been build matched for that implementation
*/
__INLINE Ipp8u AddLogGF16(Ipp8u loga, Ipp8u logb)
{
   //Ipp8u s = loga+logb;
   //return (s>2*14)? 15 : (s>14)? s-15 : s;
   Ipp8u s = loga+logb;
   Ipp8u delta = ((0xF-1)-s)>>7;
   s -= delta;
   s |= 0-(s>>7);
   return s & (0xF);
}
#endif

#define SELECTION_BITS  ((sizeof(BNU_CHUNK_T)/sizeof(Ipp8u)) -1)

#if defined(__INTEL_COMPILER)
__INLINE Ipp8u getSboxValue(Ipp8u x)
{
  BNU_CHUNK_T selection = 0;
  const BNU_CHUNK_T* SboxEntry = (BNU_CHUNK_T*)RijEncSbox;

  BNU_CHUNK_T i_sel = x / sizeof(BNU_CHUNK_T);  /* selection index */
  BNU_CHUNK_T i;
  for (i = 0; i<sizeof(RijEncSbox) / sizeof(BNU_CHUNK_T); i++) {
    BNU_CHUNK_T mask = (i == i_sel) ? (BNU_CHUNK_T)(-1) : 0;  /* ipp and IPP build specific avoid jump instruction here */
    selection |= SboxEntry[i] & mask;
  }
  selection >>= (x & SELECTION_BITS) * 8;
  return (Ipp8u)(selection & 0xFF);
}

#else
#include "pcpmask_ct.h"
__INLINE Ipp8u getSboxValue(Ipp8u x)
{
  BNU_CHUNK_T selection = 0;
  const BNU_CHUNK_T* SboxEntry = (BNU_CHUNK_T*)RijEncSbox;

  Ipp32u _x = x / sizeof(BNU_CHUNK_T);
  Ipp32u i;
  for (i = 0; i<sizeof(RijEncSbox) / (sizeof(BNU_CHUNK_T)); i++) {
    BNS_CHUNK_T mask = (BNS_CHUNK_T)cpIsEqu_ct(_x, i);
    selection |= SboxEntry[i] & (BNU_CHUNK_T)mask;
  }
  selection >>= (x & SELECTION_BITS) * 8;
  return (Ipp8u)(selection & 0xFF);
}
#endif

#endif /* _PCP_RIJ_SAFE_H */
