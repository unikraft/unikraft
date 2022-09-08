/*******************************************************************************
* Copyright 2003-2021 Intel Corporation
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
//     Internal BN Resource Definitions & Function Prototypes
//
//
*/

#if !defined(_PCP_BNRESOURCE_H)
#define _PCP_BNRESOURCE_H


typedef struct {
   void*            pNext;
   IppsBigNumState* pBN;
} BigNumNode;


/* size (byte) of BN resource */
#define cpBigNumListGetSize OWNAPI(cpBigNumListGetSize)
   IPP_OWN_DECL (int, cpBigNumListGetSize, (int feBitSize, int nodes))

/* init BN resource */
#define cpBigNumListInit OWNAPI(cpBigNumListInit)
   IPP_OWN_DECL (void, cpBigNumListInit, (int feBitSize, int nodes, BigNumNode* pList))

/* get BN from resource */
#define cpBigNumListGet OWNAPI(cpBigNumListGet)
   IPP_OWN_DECL (IppsBigNumState*, cpBigNumListGet, (BigNumNode** pList))

#endif /* _PCP_BNRESOURCE_H */
