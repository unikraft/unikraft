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
//     Fixed window exponentiation scramble/unscramble
// 
//  Contents:
//     cpScramblePut()
//     cpScrambleGet()
// 
// 
*/

#if !defined(_PC_SCRAMBLE_H)
#define _PC_SCRAMBLE_H

/*
// cpsScramblePut/cpsScrambleGet
// stores to/retrieves from pScrambleEntry position
// pre-computed data if fixed window method is used
*/
__INLINE void cpScramblePut(Ipp8u* pArray, cpSize colummSize,
                      const Ipp32u* pData, cpSize dataSize)
{
   int i;
   switch(colummSize) {
      case 1: // column - byte
         dataSize *= sizeof(Ipp32u);
         for(i=0; i<dataSize; i++)
            pArray[i*CACHE_LINE_SIZE] = ((Ipp8u*)pData)[i];
         break;
      case 2: // column - word (2 bytes)
         dataSize *= sizeof(Ipp16u);
         for(i=0; i<dataSize; i++)
            ((Ipp16u*)pArray)[i*CACHE_LINE_SIZE/sizeof(Ipp16u)] = ((Ipp16u*)pData)[i];
         break;
      case 4: // column - dword (4 bytes)
         for(i=0; i<dataSize; i++)
            ((Ipp32u*)pArray)[i*CACHE_LINE_SIZE/sizeof(Ipp32u)] = pData[i];
         break;
      case 8: // column - qword (8 bytes => 2 dword)
         for(; dataSize>=2; dataSize-=2, pArray+=CACHE_LINE_SIZE, pData+=2) {
            ((Ipp32u*)pArray)[0] = pData[0];
            ((Ipp32u*)pArray)[1] = pData[1];
         }
         if(dataSize)
            ((Ipp32u*)pArray)[0] = pData[0];
         break;
      case 16: // column - oword (16 bytes => 4 dword)
         for(; dataSize>=4; dataSize-=4, pArray+=CACHE_LINE_SIZE, pData+=4) {
            ((Ipp32u*)pArray)[0] = pData[0];
            ((Ipp32u*)pArray)[1] = pData[1];
            ((Ipp32u*)pArray)[2] = pData[2];
            ((Ipp32u*)pArray)[3] = pData[3];
         }
         for(; dataSize>0; dataSize--, pArray+=sizeof(Ipp32u), pData++)
            ((Ipp32u*)pArray)[0] = pData[0];
         break;
      case 32: // column - 2 oword (32 bytes => 8 dword)
         for(; dataSize>=8; dataSize-=8, pArray+=CACHE_LINE_SIZE, pData+=8) {
            ((Ipp32u*)pArray)[0] = pData[0];
            ((Ipp32u*)pArray)[1] = pData[1];
            ((Ipp32u*)pArray)[2] = pData[2];
            ((Ipp32u*)pArray)[3] = pData[3];
            ((Ipp32u*)pArray)[4] = pData[4];
            ((Ipp32u*)pArray)[5] = pData[5];
            ((Ipp32u*)pArray)[6] = pData[6];
            ((Ipp32u*)pArray)[7] = pData[7];
         }
         for(; dataSize>0; dataSize--, pArray+=sizeof(Ipp32u), pData++)
            ((Ipp32u*)pArray)[0] = pData[0];
         break;
      default:
         break;
   }
}


/*
// Retrieve data from pArray
*/
#define u8_to_u32(b0,b1,b2,b3, x) \
  ((x) = (b0), \
   (x)|=((b1)<<8), \
   (x)|=((b2)<<16), \
   (x)|=((b3)<<24))
#define u16_to_u32(w0,w1, x) \
  ((x) = (w0), \
   (x)|=((w1)<<16))
#define u32_to_u64(dw0,dw1, x) \
  ((x) = (Ipp64u)(dw0), \
   (x)|= (((Ipp64u)(dw1))<<32))

__INLINE void cpScrambleGet(Ipp32u* pData, cpSize dataSize,
                      const Ipp8u* pArray, cpSize colummSize)
{
   int i;
   switch(colummSize) {
      case 1: // column - byte
         for(i=0; i<dataSize; i++, pArray+=sizeof(Ipp32u)*CACHE_LINE_SIZE)
            u8_to_u32(pArray[0*CACHE_LINE_SIZE], pArray[1*CACHE_LINE_SIZE], pArray[2*CACHE_LINE_SIZE], pArray[3*CACHE_LINE_SIZE], pData[i]);
         break;
      case 2: // column - word (2 bytes)
         for(i=0; i<dataSize; i++, pArray+=sizeof(Ipp16u)*CACHE_LINE_SIZE) {
            Ipp16u w0 = *((Ipp16u*)(pArray));
            Ipp16u w1 = *((Ipp16u*)(pArray+CACHE_LINE_SIZE));
            u16_to_u32( w0, w1, pData[i]);
         }
         break;
      case 4: // column - dword (4 bytes)
         for(i=0; i<dataSize; i++, pArray+=CACHE_LINE_SIZE)
            pData[i] = ((Ipp32u*)pArray)[0];
         break;
      case 8: // column - qword (8 bytes => 2 dword)
         for(; dataSize>=2; dataSize-=2, pArray+=CACHE_LINE_SIZE, pData+=2) {
            pData[0] = ((Ipp32u*)pArray)[0];
            pData[1] = ((Ipp32u*)pArray)[1];
         }
         if(dataSize)
            pData[0] = ((Ipp32u*)pArray)[0];
         break;
      case 16: // column - oword (16 bytes => 4 dword)
         for(; dataSize>=4; dataSize-=4, pArray+=CACHE_LINE_SIZE, pData+=4) {
            pData[0] = ((Ipp32u*)pArray)[0];
            pData[1] = ((Ipp32u*)pArray)[1];
            pData[2] = ((Ipp32u*)pArray)[2];
            pData[3] = ((Ipp32u*)pArray)[3];

         }
         for(; dataSize>0; dataSize--, pArray+=sizeof(Ipp32u), pData++)
            pData[0] = ((Ipp32u*)pArray)[0];
         break;
      case 32: // column - 2 oword (32 bytes => 8 dword)
         for(; dataSize>=8; dataSize-=8, pArray+=CACHE_LINE_SIZE, pData+=8) {
            pData[0] = ((Ipp32u*)pArray)[0];
            pData[1] = ((Ipp32u*)pArray)[1];
            pData[2] = ((Ipp32u*)pArray)[2];
            pData[3] = ((Ipp32u*)pArray)[3];
            pData[4] = ((Ipp32u*)pArray)[4];
            pData[5] = ((Ipp32u*)pArray)[5];
            pData[6] = ((Ipp32u*)pArray)[6];
            pData[7] = ((Ipp32u*)pArray)[7];
         }
         for(; dataSize>0; dataSize--, pArray+=sizeof(Ipp32u), pData++)
            pData[0] = ((Ipp32u*)pArray)[0];
         break;
      default:
         break;
   }
}

#endif /* _PC_SCRAMBLE_H */
