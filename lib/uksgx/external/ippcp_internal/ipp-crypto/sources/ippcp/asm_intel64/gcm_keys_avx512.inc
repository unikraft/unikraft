;===============================================================================
; Copyright 2019-2021 Intel Corporation
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

%ifndef GCM_KEYS_AVX512_INCLUDED
%define GCM_KEYS_AVX512_INCLUDED

;; Define the fields of gcm_key_data struct:
;; uint8_t expanded_keys[GCM_ENC_KEY_LEN * GCM_KEY_SETS];
;; uint8_t shifted_hkey_8[GCM_ENC_KEY_LEN]; // HashKey^8 <<1 mod poly
;; uint8_t shifted_hkey_7[GCM_ENC_KEY_LEN]; // HashKey^7 <<1 mod poly
;; uint8_t shifted_hkey_6[GCM_ENC_KEY_LEN]; // HashKey^6 <<1 mod poly
;; uint8_t shifted_hkey_5[GCM_ENC_KEY_LEN]; // HashKey^5 <<1 mod poly
;; uint8_t shifted_hkey_4[GCM_ENC_KEY_LEN]; // HashKey^4 <<1 mod poly
;; uint8_t shifted_hkey_3[GCM_ENC_KEY_LEN]; // HashKey^3 <<1 mod poly
;; uint8_t shifted_hkey_2[GCM_ENC_KEY_LEN]; // HashKey^2 <<1 mod poly
;; uint8_t shifted_hkey_1[GCM_ENC_KEY_LEN]; // HashKey   <<1 mod poly

%define HashKey_8       (16*15)  ; HashKey^8 <<1 mod poly
%define HashKey_7       (16*16)  ; HashKey^7 <<1 mod poly
%define HashKey_6       (16*17)  ; HashKey^6 <<1 mod poly
%define HashKey_5       (16*18)  ; HashKey^5 <<1 mod poly
%define HashKey_4       (16*19)  ; HashKey^4 <<1 mod poly
%define HashKey_3       (16*20)  ; HashKey^3 <<1 mod poly
%define HashKey_2       (16*21)  ; HashKey^2 <<1 mod poly
%define HashKey_1       (16*22)  ; HashKey <<1 mod poly
%define HashKey         (16*22)  ; HashKey <<1 mod poly

%endif ; GCM_KEYS_AVX512_INCLUDED
