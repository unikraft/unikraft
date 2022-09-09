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
//     Intel(R) Integrated Performance Primitives. Cryptography Primitives.
//     Internal operations over GF(p) extension.
// 
//     Context:
//        cpGFpGetOptimalWinSize()
//
*/

#include "owncp.h"
#include "pcpbnumisc.h"
#include "pcpgfpxstuff.h"
#include "gsscramble.h"

static int div_upper(int a, int d)
{
   return (a+d-1)/d;
}

static int getNumOperations(int bitsize, int w)
{
   int n_overhead = (1<<w) -1;
   int n_ops = div_upper(bitsize, w) + n_overhead;
   return n_ops;
}

IPP_OWN_DEFN (int, cpGFpGetOptimalWinSize, (int bitsize))
{
   int w_opt = 1;
   int n_opt = getNumOperations(bitsize, w_opt);
   int w_trial;
   for(w_trial=w_opt+1; w_trial<=IPP_MAX_EXPONENT_NUM; w_trial++) {
      int n_trial = getNumOperations(bitsize, w_trial);
      if(n_trial>=n_opt) break;
      w_opt = w_trial;
      n_opt = n_trial;
   }
   return w_opt;
}
