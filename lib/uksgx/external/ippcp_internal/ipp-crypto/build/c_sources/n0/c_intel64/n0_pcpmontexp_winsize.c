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
//  Contents:
//        cpMontExp_WinSize()
//
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpbn.h"
#include "pcpmontgomery.h"
#include "gsscramble.h"


#if !defined(_USE_WINDOW_EXP_)
IPP_OWN_DEFN (cpSize, cpMontExp_WinSize, (int bitsize))
{
   IPP_UNREFERENCED_PARAMETER(bitsize);
   return 1;
}

#else
/*
// returns (optimal) window width
// Because of safety Window width depend on CACHE LINE size:
//    P4,EM64T, ITP - 64 bytes
//    XScale        - 32 bytes
//    Blend         - no cache
*/
#if !((_IPP==_IPP_W7) || (_IPP==_IPP_T7) || \
      (_IPP==_IPP_V8) || (_IPP==_IPP_P8) || \
      (_IPP==_IPP_S8) || (_IPP>=_IPP_G9) || \
      (_IPP32E==_IPP32E_M7) || \
      (_IPP32E==_IPP32E_U8) || (_IPP32E==_IPP32E_Y8) || \
      (_IPP32E==_IPP32E_N8) || (_IPP32E>=_IPP32E_E9))
IPP_OWN_DEFN (cpSize, cpMontExp_WinSize, (int bitsize))
{
   IPP_UNREFERENCED_PARAMETER(bitsize);
   return 1;
}
#endif

#if ((_IPP==_IPP_W7) || (_IPP==_IPP_T7) || \
     (_IPP==_IPP_V8) || (_IPP==_IPP_P8) || \
     (_IPP==_IPP_S8) || (_IPP>=_IPP_G9) || \
     (_IPP32E==_IPP32E_M7) || \
     (_IPP32E==_IPP32E_U8) || (_IPP32E==_IPP32E_Y8) || \
     (_IPP32E==_IPP32E_N8) || (_IPP32E>=_IPP32E_E9))
IPP_OWN_DEFN (cpSize, cpMontExp_WinSize, (int bitsize))
{
   return
          //bitsize>3715? 8 : /*limited by 6 or 4 (LOG_CACHE_LINE_SIZE); we use it for windowing-exp imtigation */
          //bitsize>1434? 7 :
      #if (_IPP !=_IPP_M5)
          bitsize> 539? 6 :
          bitsize> 197? 5 :
      #endif
          bitsize>  70? 4 :
          bitsize>  25? 3 :
          bitsize>   9? 2 : 1;
}
#endif

#endif /* _USE_WINDOW_EXP_ */
