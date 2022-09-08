;===============================================================================
; Copyright 2016-2021 Intel Corporation
;
; Licensed under the Apache License, Version 2.0 (the "License");
; you may not use this file except in compliance with the License.
; You may obtain a copy of the License at
;
;     http://www.apache.org/licenses/LICENSE-2.0
;
; Unless required by applicable law or agreed to in writing, software
; distributed under the License is distributed on an "AS IS" BASIS,
; WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
; See the License for the specific language governing permissions and
; limitations under the License.
;===============================================================================

;               Intel(R) Integrated Performance Primitives
;                   Cryptographic Primitives (ippcp)
;
;     Purpose:
;        Update standard ippCP variant
;        for code size optimization
;

%ifdef _XMM7560_
%if (_IPP >= _IPP_P8)

%assign OFF 0
%assign ON  1

;%assign _USE_C_cpAdd_BNU_  ON
;%assign _USE_C_cpSub_BNU_  ON
;%assign _USE_C_cpInc_BNU_  ON
;%assign _USE_C_cpAddMulDgt_BNU_  ON
;%assign _USE_C_cpSubMulDgt_BNU_  ON
%assign _USE_C_cpMulAdc_BNU_school_  ON
%assign _USE_C_cpSqrAdc_BNU_school_  ON
%assign _USE_C_cpMontRedAdc_BNU_     ON

%assign _DISABLE_ECP_256R1_HARDCODED_BP_TBL_  OFF
%assign _DISABLE_ECP_384R1_HARDCODED_BP_TBL_  OFF

%endif
%endif
