/*******************************************************************************
* Copyright 2010-2021 Intel Corporation
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
//     Intel(R) Integrated Performance Primitives. Cryptography Primitives.
//     Internal EC over GF(p^m) basic Definitions & Function Prototypes
// 
//     Context:
//        setupTable()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpgfpecstuff.h"
#include "gsscramble.h"


/* sscm version */
IPP_OWN_DEFN (void, setupTable, (BNU_CHUNK_T* pTbl, const BNU_CHUNK_T* pPdata, IppsGFpECState* pEC))
{
   int pointLen = ECP_POINTLEN(pEC);
   //int pointLen32 = pointLen*sizeof(BNU_CHUNK_T)/sizeof(ipp32u);

   const int npoints = 3;
   BNU_CHUNK_T* A = cpEcGFpGetPool(npoints, pEC);
   BNU_CHUNK_T* B = A+pointLen;
   BNU_CHUNK_T* C = B+pointLen;

   // Table[0]
   // Table[0] is implicitly (0,0,0) {point at infinity}, therefore no need to store it
   // All other values are actually stored with an offset of -1

   // Table[1] ( =[1]p )
   //cpScatter32((Ipp32u*)pTbl, 16, 0, (Ipp32u*)pPdata, pointLen32);
   gsScramblePut(pTbl, (1-1), pPdata, pointLen, (5-1));

   // Table[2] ( =[2]p )
   gfec_point_double(A, pPdata, pEC);
   //cpScatter32((Ipp32u*)pTbl, 16, 1, (Ipp32u*)A, pointLen32);
   gsScramblePut(pTbl, (2-1), A, pointLen, (5-1));

   // Table[3] ( =[3]p )
   gfec_point_add(B, A, pPdata, pEC);
   //cpScatter32((Ipp32u*)pTbl, 16, 2, (Ipp32u*)B, pointLen32);
   gsScramblePut(pTbl, (3-1), B, pointLen, (5-1));

   // Table[4] ( =[4]p )
   gfec_point_double(A, A, pEC);
   //cpScatter32((Ipp32u*)pTbl, 16, 3, (Ipp32u*)A, pointLen32);
   gsScramblePut(pTbl, (4-1), A, pointLen, (5-1));

   // Table[5] ( =[5]p )
   gfec_point_add(C, A, pPdata, pEC);
   //cpScatter32((Ipp32u*)pTbl, 16, 4, (Ipp32u*)C, pointLen32);
   gsScramblePut(pTbl, (5-1), C, pointLen, (5-1));

   // Table[10] ( =[10]p )
   gfec_point_double(C, C, pEC);
   //cpScatter32((Ipp32u*)pTbl, 16, 9, (Ipp32u*)C, pointLen32);
   gsScramblePut(pTbl, (10-1), C, pointLen, (5-1));

   // Table[11] ( =[11]p )
   gfec_point_add(C, C, pPdata, pEC);
   //cpScatter32((Ipp32u*)pTbl, 16, 10, (Ipp32u*)C, pointLen32);
   gsScramblePut(pTbl, (11-1), C, pointLen, (5-1));

   // Table[6] ( =[6]p )
   gfec_point_double(B, B, pEC);
   //cpScatter32((Ipp32u*)pTbl, 16, 5, (Ipp32u*)B, pointLen32);
   gsScramblePut(pTbl, (6-1), B, pointLen, (5-1));

   // Table[7] ( =[7]p )
   gfec_point_add(C, B, pPdata, pEC);
   //cpScatter32((Ipp32u*)pTbl, 16, 6, (Ipp32u*)C, pointLen32);
   gsScramblePut(pTbl, (7-1), C, pointLen, (5-1));

   // Table[14] ( =[14]p )
   gfec_point_double(C, C, pEC);
   //cpScatter32((Ipp32u*)pTbl, 16, 13, (Ipp32u*)C, pointLen32);
   gsScramblePut(pTbl, (14-1), C, pointLen, (5-1));

   // Table[15] ( =[15]p )
   gfec_point_add(C, C, pPdata, pEC);
   //cpScatter32((Ipp32u*)pTbl, 16, 14, (Ipp32u*)C, pointLen32);
   gsScramblePut(pTbl, (15-1), C, pointLen, (5-1));

   // Table[12] ( =[12]p )
   gfec_point_double(B, B, pEC);
   //cpScatter32((Ipp32u*)pTbl, 16, 11, (Ipp32u*)B, pointLen32);
   gsScramblePut(pTbl, (12-1), B, pointLen, (5-1));

   // Table[13] ( =[13]p )
   gfec_point_add(B, B, pPdata, pEC);
   //cpScatter32((Ipp32u*)pTbl, 16, 12, (Ipp32u*)B, pointLen32);
   gsScramblePut(pTbl, (13-1), B, pointLen, (5-1));

   // Table[8] ( =[8]p )
   gfec_point_double(A, A, pEC);
   //cpScatter32((Ipp32u*)pTbl, 16, 7, (Ipp32u*)A, pointLen32);
   gsScramblePut(pTbl, (8-1), A, pointLen, (5-1));

   // Table[9] ( =[9]p )
   gfec_point_add(B, A, pPdata, pEC);
   //cpScatter32((Ipp32u*)pTbl, 16, 8, (Ipp32u*)B, pointLen32);
   gsScramblePut(pTbl, (9-1), B, pointLen, (5-1));

   // Table[16] ( =[16]p )
   gfec_point_double(A, A, pEC);
   //cpScatter32((Ipp32u*)pTbl, 16, 15, (Ipp32u*)A, pointLen32);
   gsScramblePut(pTbl, (16-1), A, pointLen, (5-1));

   cpEcGFpReleasePool(npoints, pEC);
}
