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

#ifndef STATUS_H
#define STATUS_H

#include <crypto_mb/defs.h>

typedef int32u mbx_status;
typedef int64u mbx_status16;

// error statuses and manipulators
#define MBX_STATUS_OK                 (0)
#define MBX_STATUS_MISMATCH_PARAM_ERR (1)
#define MBX_STATUS_NULL_PARAM_ERR     (2)
#define MBX_STATUS_LOW_ORDER_ERR      (4)
#define MBX_STATUS_SIGNATURE_ERR      (8)

__INLINE mbx_status MBX_SET_STS(mbx_status status, int numb, mbx_status sttVal)
{
   numb &= 7; /* 0 <= numb < 8 */
   status &= (mbx_status)(~(0xF << (numb*4)));
   return status |= (sttVal & 0xF) << (numb*4);
}

__INLINE mbx_status MBX_GET_STS(mbx_status status, int numb)
{
   return (status >>(numb*4)) & 0xF;
}
__INLINE mbx_status MBX_SET_STS_ALL(mbx_status stsVal)
{
   return (stsVal<<4*7) | (stsVal<<4*6) | (stsVal<<4*5) | (stsVal<<4*4)  | (stsVal<<4*3) | (stsVal<<4*2) | (stsVal<<4*1) | stsVal;
}

__INLINE mbx_status MBX_SET_STS_BY_MASK(mbx_status status, int8u mask, mbx_status sttVal)
{
   int numb;

   for(numb=0; numb<8; numb++) {
      mbx_status buf_stt = (0 - ((mask>>numb) &1)) & sttVal;
      status = MBX_SET_STS(status, numb, buf_stt);
   }
   return status;
}

__INLINE int MBX_IS_ANY_OK_STS(mbx_status status)
{
   int ret = MBX_STATUS_OK==MBX_GET_STS(status, 0)
          || MBX_STATUS_OK==MBX_GET_STS(status, 1)
          || MBX_STATUS_OK==MBX_GET_STS(status, 2)
          || MBX_STATUS_OK==MBX_GET_STS(status, 3)
          || MBX_STATUS_OK==MBX_GET_STS(status, 4)
          || MBX_STATUS_OK==MBX_GET_STS(status, 5)
          || MBX_STATUS_OK==MBX_GET_STS(status, 6)
          || MBX_STATUS_OK==MBX_GET_STS(status, 7);
   return ret;
}

/*
// Helpers for 64-bit status mbx_status16
*/

/* Accessors for the low and high part of 64-bit status */
__INLINE mbx_status MBX_GET_HIGH_PART_STS16(mbx_status16 status16)
{
   return ((mbx_status)(((mbx_status16)(status16) >> 32) & 0xFFFFFFFF));
}

__INLINE mbx_status MBX_GET_LOW_PART_STS16(mbx_status16 status16)
{
   return ((mbx_status)(status16));
}

__INLINE mbx_status16 MBX_SET_STS16_ALL(mbx_status16 stsVal)
{
   return (stsVal<<4*15) | (stsVal<<4*14) | (stsVal<<4*13) | (stsVal<<4*12) | (stsVal<<4*11)  | (stsVal<<4*10) | (stsVal<<4*9) | (stsVal<<4*8) | \
          (stsVal<<4*7)  | (stsVal<<4*6)  | (stsVal<<4*5)  | (stsVal<<4*4)  | (stsVal<<4*3)   | (stsVal<<4*2)  | (stsVal<<4*1) | stsVal;
}

__INLINE mbx_status16 MBX_SET_STS16(mbx_status16 status, int numb, mbx_status16 sttVal)
{
   numb &= 15; /* 0 <= numb < 16 */
   status &= (mbx_status16)(~(0xF << (numb*4)));
   return status |= (sttVal & 0xF) << (numb*4);

}

__INLINE int MBX_IS_ANY_OK_STS16(mbx_status16 status)
{
   return MBX_IS_ANY_OK_STS(MBX_GET_HIGH_PART_STS16(status)) && \
          MBX_IS_ANY_OK_STS(MBX_GET_LOW_PART_STS16(status));
}

#endif /* STATUS_H */
