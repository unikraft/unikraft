/*******************************************************************************
* Copyright 2017-2021 Intel Corporation
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

#include "gsmodmethodstuff.h"

/*
   methods
*/

/* ******************************************** */

static gsModMethod* gsModArithMont_C(void)
{
   static gsModMethod m = {
      gs_mont_encode,
      NULL,
      gs_mont_mul,
      gs_mont_sqr,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
   };
   return &m;
}
#if (_IPP32E>=_IPP32E_L9)
static gsModMethod* gsModArithMont_X(void)
{
   static gsModMethod m = {
      gs_mont_encodeX,
      NULL,
      gs_mont_mulX,
      gs_mont_sqrX,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
   };
   return &m;
}
#endif

IPP_OWN_DEFN (gsModMethod*, gsModArithMont, (void))
{
   #if (_IPP32E>=_IPP32E_L9)
   if(IsFeatureEnabled(ippCPUID_ADCOX))
      return gsModArithMont_X();
   else
   #endif
      return gsModArithMont_C();
}
/* ******************************************** */
