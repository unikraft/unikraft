;===============================================================================
; Copyright 2015-2021 Intel Corporation
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

;
;               Intel(R) Integrated Performance Primitives
;                   Cryptographic Primitives (ippcp)
;
;      Purpose:
;         Define ippCP variant
;
;  do not changes in definitions below!
;

;;
;; modes of the feature
;;
%assign _FEATURE_OFF_      0   ;; feature is OFF
%assign _FEATURE_ON_       1   ;; feature is ON
%assign _FEATURE_TICKTOCK_ 2   ;; dectect is feature OFF/ON

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; %define _XMM7560_ 1
%ifdef _XMM7560_
%include "pcpvariant_xmm7560.inc"
%endif

; %define _TXT_ACM_ 1
%ifdef _TXT_ACM_
%include "pcpvariant_txt_acm.inc"
%endif
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;
;; it possible to force use of C-version of some implementtaions
;; instead of ASM one
;;
%ifndef _USE_C_cpAdd_BNU_
  %assign _USE_C_cpAdd_BNU_ _FEATURE_OFF_
%endif

%ifndef _USE_C_cpSub_BNU_
  %assign _USE_C_cpSub_BNU_ _FEATURE_OFF_
%endif

%ifndef _USE_C_cpInc_BNU_
  %assign _USE_C_cpInc_BNU_ _FEATURE_OFF_
%endif

%ifndef _USE_C_cpAddMulDgt_BNU_
  %assign _USE_C_cpAddMulDgt_BNU_ _FEATURE_OFF_
%endif

%ifndef _USE_C_cpMulAdc_BNU_school_
  %assign _USE_C_cpMulAdc_BNU_school_ _FEATURE_OFF_
%endif

%ifndef _USE_C_cpSqrAdc_BNU_school_
  %assign _USE_C_cpMulSqr_BNU_school_ _FEATURE_OFF_
%endif

%ifndef _USE_C_cpMontRedAdc_BNU_
  %assign _USE_C_cpMontRedAdc_BNU_ _FEATURE_OFF_
%endif

;;
;; set _AES_NI_ENABLING_
;;
%ifdef _IPP_AES_NI_
  %if (_IPP_AES_NI_ == 0)
    %assign _AES_NI_ENABLING_ _FEATURE_OFF_
  %elif (_IPP_AES_NI_ == 1)
    %assign _AES_NI_ENABLING_ _FEATURE_ON_
  %else
    %error <Define _IPP_AES_NI_=0 or 1 or omit _IPP_AES_NI_ at all>
  %endif
%else
  %if (_IPP32E >= _IPP32E_Y8)
    %assign _AES_NI_ENABLING_ _FEATURE_TICKTOCK_
  %else
    %assign _AES_NI_ENABLING_ _FEATURE_OFF_
  %endif
%endif

;;
;; if there is no outside assignment
;; set _SHA_NI_ENABLING_  based on CPU specification
;;
%ifndef _SHA_NI_ENABLING_
  %if (_IPP32E >= _IPP32E_Y8 )
    %assign _SHA_NI_ENABLING_ _FEATURE_TICKTOCK_
  %else
    %assign _SHA_NI_ENABLING_ _FEATURE_OFF_
  %endif
%endif

;;
;; set _ADCOX_NI_ENABLING_
;;
%ifdef _IPP_ADCX_NI_
  %if (_IPP_ADCX_NI_ == 0)
    %assign _ADCOX_NI_ENABLING_ _FEATURE_OFF_
  %elif (_IPP_ADCX_NI_ == 1)
    %assign _ADCOX_NI_ENABLING_ _FEATURE_ON_
  %else
    %error  <Define _IPP_ADCX_NI_=0 or 1 or omit _IPP_ADCX_NI_ at all>
  %endif
%else
  %if (_IPP32E >= _IPP32E_L9)
    %assign _ADCOX_NI_ENABLING_ _FEATURE_TICKTOCK_
  %else
    %assign _ADCOX_NI_ENABLING_ _FEATURE_OFF_
  %endif
%endif


;;
;; select Hash algorithm
;;
%ifndef _DISABLE_ALG_SHA1_
  %assign _ENABLE_ALG_SHA1_ _FEATURE_ON_  ;; SHA1 on
%else
  %assign _ENABLE_ALG_SHA1_ _FEATURE_OFF_ ;; SHA1 on
%endif

%ifndef _DISABLE_ALG_SHA256_
  %assign _ENABLE_ALG_SHA256_ _FEATURE_ON_  ;; SHA256 on
%else
  %assign _ENABLE_ALG_SHA256_ _FEATURE_OFF_ ;; SHA256 off
%endif

%ifndef _DISABLE_ALG_SHA521_
  %assign _ENABLE_ALG_SHA512_ _FEATURE_ON_  ;; SHA512 on
%else
  %assign _ENABLE_ALG_SHA512_ _FEATURE_OFF_ ;; SHA512 off
%endif

%ifndef _DISABLE_ALG_MD5_
  %assign _ENABLE_ALG_MD5_ _FEATURE_ON_  ;; MD5 on
%else
  %assign _ENABLE_ALG_MD5_ _FEATURE_OFF_ ;; MD5 off
%endif

%ifndef _DISABLE_ALG_SM3_
  %assign _ENABLE_ALG_SM3_ _FEATURE_ON_  ;; SM3 on
%else
  %assign _ENABLE_ALG_SM3_ _FEATURE_OFF_ ;; SM3 off
%endif

;;
;; BN arithmetic
;;
%assign _ENABLE_KARATSUBA_ _FEATURE_OFF_  ;; not use Karatsuba method for multiplication

;;
;; EC specific
;;
%assign _ECP_IMPL_NONE_       0
%assign _ECP_IMPL_ARBIRTRARY_ 1
%assign _ECP_IMPL_SPECIFIC_   2
%assign _ECP_IMPL_MFM_        3

%ifndef _ECP_128_
  %assign _ECP_128_ _ECP_IMPL_SPECIFIC_
%endif

%ifndef _ECP_192_
  %assign _ECP_192_ _ECP_IMPL_MFM_
%endif

%ifndef _ECP_224_
  %assign _ECP_224_ _ECP_IMPL_MFM_
%endif

%ifndef _ECP_256_
  %assign _ECP_256_ _ECP_IMPL_MFM_
%endif

%ifndef _ECP_384_
  %assign _ECP_384_ _ECP_IMPL_MFM_
%endif

%ifndef _ECP_521_
  %assign _ECP_521_ _ECP_IMPL_MFM_
%endif

%ifndef _ECP_SM2_
  %assign _ECP_SM2_ _ECP_IMPL_MFM_
%endif
