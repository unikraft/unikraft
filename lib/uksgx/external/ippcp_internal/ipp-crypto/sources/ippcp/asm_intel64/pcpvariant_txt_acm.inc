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
;        Update standard ippCP variant
;
;  do not changes in definitions below!
;

%ifdef _TXT_ACM_

;;
;; HASH algs outside settings
;;
%assign _SHA_NI_ENABLING_ _FEATURE_TICKTOCK_

;;
;; select Hash algorithm
;;
; %assign _ENABLE_ALG_MD5_ _FEATURE_OFF_

%endif
