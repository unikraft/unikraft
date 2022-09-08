;===============================================================================
; Copyright 2020-2021 Intel Corporation
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

%ifndef GCM_KEYS_VAES_AVX512_INCLUDED
%define GCM_KEYS_VAES_AVX512_INCLUDED

;; Define the fields of gcm_key_data struct:
;; uint8_t expanded_keys[GCM_ENC_KEY_LEN * GCM_KEY_SETS];
;; uint8_t shifted_hkey_9_128[GCM_ENC_KEY_LEN * (128 - 8)];
;; uint8_t shifted_hkey_8[GCM_ENC_KEY_LEN]; // HashKey^8 <<1 mod poly
;; uint8_t shifted_hkey_7[GCM_ENC_KEY_LEN]; // HashKey^7 <<1 mod poly
;; uint8_t shifted_hkey_6[GCM_ENC_KEY_LEN]; // HashKey^6 <<1 mod poly
;; uint8_t shifted_hkey_5[GCM_ENC_KEY_LEN]; // HashKey^5 <<1 mod poly
;; uint8_t shifted_hkey_4[GCM_ENC_KEY_LEN]; // HashKey^4 <<1 mod poly
;; uint8_t shifted_hkey_3[GCM_ENC_KEY_LEN]; // HashKey^3 <<1 mod poly
;; uint8_t shifted_hkey_2[GCM_ENC_KEY_LEN]; // HashKey^2 <<1 mod poly
;; uint8_t shifted_hkey_1[GCM_ENC_KEY_LEN]; // HashKey   <<1 mod poly

%ifdef GCM_BIG_DATA
;;
;; Key structure holds up to 128 ghash keys
;;
%define HashKey_128     (16*15)   ; HashKey^128 <<1 mod poly
%define HashKey_127     (16*16)   ; HashKey^127 <<1 mod poly
%define HashKey_126     (16*17)   ; HashKey^126 <<1 mod poly
%define HashKey_125     (16*18)   ; HashKey^125 <<1 mod poly
%define HashKey_124     (16*19)   ; HashKey^124 <<1 mod poly
%define HashKey_123     (16*20)   ; HashKey^123 <<1 mod poly
%define HashKey_122     (16*21)   ; HashKey^122 <<1 mod poly
%define HashKey_121     (16*22)   ; HashKey^121 <<1 mod poly
%define HashKey_120     (16*23)   ; HashKey^120 <<1 mod poly
%define HashKey_119     (16*24)   ; HashKey^119 <<1 mod poly
%define HashKey_118     (16*25)   ; HashKey^118 <<1 mod poly
%define HashKey_117     (16*26)   ; HashKey^117 <<1 mod poly
%define HashKey_116     (16*27)   ; HashKey^116 <<1 mod poly
%define HashKey_115     (16*28)   ; HashKey^115 <<1 mod poly
%define HashKey_114     (16*29)   ; HashKey^114 <<1 mod poly
%define HashKey_113     (16*30)   ; HashKey^113 <<1 mod poly
%define HashKey_112     (16*31)   ; HashKey^112 <<1 mod poly
%define HashKey_111     (16*32)   ; HashKey^111 <<1 mod poly
%define HashKey_110     (16*33)   ; HashKey^110 <<1 mod poly
%define HashKey_109     (16*34)   ; HashKey^109 <<1 mod poly
%define HashKey_108     (16*35)   ; HashKey^108 <<1 mod poly
%define HashKey_107     (16*36)   ; HashKey^107 <<1 mod poly
%define HashKey_106     (16*37)   ; HashKey^106 <<1 mod poly
%define HashKey_105     (16*38)   ; HashKey^105 <<1 mod poly
%define HashKey_104     (16*39)   ; HashKey^104 <<1 mod poly
%define HashKey_103     (16*40)   ; HashKey^103 <<1 mod poly
%define HashKey_102     (16*41)   ; HashKey^102 <<1 mod poly
%define HashKey_101     (16*42)   ; HashKey^101 <<1 mod poly
%define HashKey_100     (16*43)   ; HashKey^100 <<1 mod poly
%define HashKey_99      (16*44)   ; HashKey^99 <<1 mod poly
%define HashKey_98      (16*45)   ; HashKey^98 <<1 mod poly
%define HashKey_97      (16*46)   ; HashKey^97 <<1 mod poly
%define HashKey_96      (16*47)   ; HashKey^96 <<1 mod poly
%define HashKey_95      (16*48)   ; HashKey^95 <<1 mod poly
%define HashKey_94      (16*49)   ; HashKey^94 <<1 mod poly
%define HashKey_93      (16*50)   ; HashKey^93 <<1 mod poly
%define HashKey_92      (16*51)   ; HashKey^92 <<1 mod poly
%define HashKey_91      (16*52)   ; HashKey^91 <<1 mod poly
%define HashKey_90      (16*53)   ; HashKey^90 <<1 mod poly
%define HashKey_89      (16*54)   ; HashKey^89 <<1 mod poly
%define HashKey_88      (16*55)   ; HashKey^88 <<1 mod poly
%define HashKey_87      (16*56)   ; HashKey^87 <<1 mod poly
%define HashKey_86      (16*57)   ; HashKey^86 <<1 mod poly
%define HashKey_85      (16*58)   ; HashKey^85 <<1 mod poly
%define HashKey_84      (16*59)   ; HashKey^84 <<1 mod poly
%define HashKey_83      (16*60)   ; HashKey^83 <<1 mod poly
%define HashKey_82      (16*61)   ; HashKey^82 <<1 mod poly
%define HashKey_81      (16*62)   ; HashKey^81 <<1 mod poly
%define HashKey_80      (16*63)   ; HashKey^80 <<1 mod poly
%define HashKey_79      (16*64)   ; HashKey^79 <<1 mod poly
%define HashKey_78      (16*65)   ; HashKey^78 <<1 mod poly
%define HashKey_77      (16*66)   ; HashKey^77 <<1 mod poly
%define HashKey_76      (16*67)   ; HashKey^76 <<1 mod poly
%define HashKey_75      (16*68)   ; HashKey^75 <<1 mod poly
%define HashKey_74      (16*69)   ; HashKey^74 <<1 mod poly
%define HashKey_73      (16*70)   ; HashKey^73 <<1 mod poly
%define HashKey_72      (16*71)   ; HashKey^72 <<1 mod poly
%define HashKey_71      (16*72)   ; HashKey^71 <<1 mod poly
%define HashKey_70      (16*73)   ; HashKey^70 <<1 mod poly
%define HashKey_69      (16*74)   ; HashKey^69 <<1 mod poly
%define HashKey_68      (16*75)   ; HashKey^68 <<1 mod poly
%define HashKey_67      (16*76)   ; HashKey^67 <<1 mod poly
%define HashKey_66      (16*77)   ; HashKey^66 <<1 mod poly
%define HashKey_65      (16*78)   ; HashKey^65 <<1 mod poly
%define HashKey_64      (16*79)   ; HashKey^64 <<1 mod poly
%define HashKey_63      (16*80)   ; HashKey^63 <<1 mod poly
%define HashKey_62      (16*81)   ; HashKey^62 <<1 mod poly
%define HashKey_61      (16*82)   ; HashKey^61 <<1 mod poly
%define HashKey_60      (16*83)   ; HashKey^60 <<1 mod poly
%define HashKey_59      (16*84)   ; HashKey^59 <<1 mod poly
%define HashKey_58      (16*85)   ; HashKey^58 <<1 mod poly
%define HashKey_57      (16*86)   ; HashKey^57 <<1 mod poly
%define HashKey_56      (16*87)   ; HashKey^56 <<1 mod poly
%define HashKey_55      (16*88)   ; HashKey^55 <<1 mod poly
%define HashKey_54      (16*89)   ; HashKey^54 <<1 mod poly
%define HashKey_53      (16*90)   ; HashKey^53 <<1 mod poly
%define HashKey_52      (16*91)   ; HashKey^52 <<1 mod poly
%define HashKey_51      (16*92)   ; HashKey^51 <<1 mod poly
%define HashKey_50      (16*93)   ; HashKey^50 <<1 mod poly
%define HashKey_49      (16*94)   ; HashKey^49 <<1 mod poly
%define HashKey_48      (16*95)   ; HashKey^48 <<1 mod poly
%define HashKey_47      (16*96)   ; HashKey^47 <<1 mod poly
%define HashKey_46      (16*97)   ; HashKey^46 <<1 mod poly
%define HashKey_45      (16*98)   ; HashKey^45 <<1 mod poly
%define HashKey_44      (16*99)   ; HashKey^44 <<1 mod poly
%define HashKey_43      (16*100)  ; HashKey^43 <<1 mod poly
%define HashKey_42      (16*101)  ; HashKey^42 <<1 mod poly
%define HashKey_41      (16*102)  ; HashKey^41 <<1 mod poly
%define HashKey_40      (16*103)  ; HashKey^40 <<1 mod poly
%define HashKey_39      (16*104)  ; HashKey^39 <<1 mod poly
%define HashKey_38      (16*105)  ; HashKey^38 <<1 mod poly
%define HashKey_37      (16*106)  ; HashKey^37 <<1 mod poly
%define HashKey_36      (16*107)  ; HashKey^36 <<1 mod poly
%define HashKey_35      (16*108)  ; HashKey^35 <<1 mod poly
%define HashKey_34      (16*109)  ; HashKey^34 <<1 mod poly
%define HashKey_33      (16*110)  ; HashKey^33 <<1 mod poly
%define HashKey_32      (16*111)  ; HashKey^32 <<1 mod poly
%define HashKey_31      (16*112)  ; HashKey^31 <<1 mod poly
%define HashKey_30      (16*113)  ; HashKey^30 <<1 mod poly
%define HashKey_29      (16*114)  ; HashKey^29 <<1 mod poly
%define HashKey_28      (16*115)  ; HashKey^28 <<1 mod poly
%define HashKey_27      (16*116)  ; HashKey^27 <<1 mod poly
%define HashKey_26      (16*117)  ; HashKey^26 <<1 mod poly
%define HashKey_25      (16*118)  ; HashKey^25 <<1 mod poly
%define HashKey_24      (16*119)  ; HashKey^24 <<1 mod poly
%define HashKey_23      (16*120)  ; HashKey^23 <<1 mod poly
%define HashKey_22      (16*121)  ; HashKey^22 <<1 mod poly
%define HashKey_21      (16*122)  ; HashKey^21 <<1 mod poly
%define HashKey_20      (16*123)  ; HashKey^20 <<1 mod poly
%define HashKey_19      (16*124)  ; HashKey^19 <<1 mod poly
%define HashKey_18      (16*125)  ; HashKey^18 <<1 mod poly
%define HashKey_17      (16*126)  ; HashKey^17 <<1 mod poly
%define HashKey_16      (16*127)  ; HashKey^16 <<1 mod poly
%define HashKey_15      (16*128)  ; HashKey^15 <<1 mod poly
%define HashKey_14      (16*129)  ; HashKey^14 <<1 mod poly
%define HashKey_13      (16*130)  ; HashKey^13 <<1 mod poly
%define HashKey_12      (16*131)  ; HashKey^12 <<1 mod poly
%define HashKey_11      (16*132)  ; HashKey^11 <<1 mod poly
%define HashKey_10      (16*133)  ; HashKey^10 <<1 mod poly
%define HashKey_9       (16*134)  ; HashKey^9 <<1 mod poly
%define HashKey_8       (16*135)  ; HashKey^8 <<1 mod poly
%define HashKey_7       (16*136)  ; HashKey^7 <<1 mod poly
%define HashKey_6       (16*137)  ; HashKey^6 <<1 mod poly
%define HashKey_5       (16*138)  ; HashKey^5 <<1 mod poly
%define HashKey_4       (16*139)  ; HashKey^4 <<1 mod poly
%define HashKey_3       (16*140)  ; HashKey^3 <<1 mod poly
%define HashKey_2       (16*141)  ; HashKey^2 <<1 mod poly
%define HashKey_1       (16*142)  ; HashKey <<1 mod poly
%define HashKey         (16*142)  ; HashKey <<1 mod poly
%else
;;
;; Key structure holds up to 48 ghash keys
;;
%define HashKey_48      (16*15)   ; HashKey^48 <<1 mod poly
%define HashKey_47      (16*16)   ; HashKey^47 <<1 mod poly
%define HashKey_46      (16*17)   ; HashKey^46 <<1 mod poly
%define HashKey_45      (16*18)   ; HashKey^45 <<1 mod poly
%define HashKey_44      (16*19)   ; HashKey^44 <<1 mod poly
%define HashKey_43      (16*20)   ; HashKey^43 <<1 mod poly
%define HashKey_42      (16*21)   ; HashKey^42 <<1 mod poly
%define HashKey_41      (16*22)   ; HashKey^41 <<1 mod poly
%define HashKey_40      (16*23)   ; HashKey^40 <<1 mod poly
%define HashKey_39      (16*24)   ; HashKey^39 <<1 mod poly
%define HashKey_38      (16*25)   ; HashKey^38 <<1 mod poly
%define HashKey_37      (16*26)   ; HashKey^37 <<1 mod poly
%define HashKey_36      (16*27)   ; HashKey^36 <<1 mod poly
%define HashKey_35      (16*28)   ; HashKey^35 <<1 mod poly
%define HashKey_34      (16*29)   ; HashKey^34 <<1 mod poly
%define HashKey_33      (16*30)   ; HashKey^33 <<1 mod poly
%define HashKey_32      (16*31)   ; HashKey^32 <<1 mod poly
%define HashKey_31      (16*32)   ; HashKey^31 <<1 mod poly
%define HashKey_30      (16*33)   ; HashKey^30 <<1 mod poly
%define HashKey_29      (16*34)   ; HashKey^29 <<1 mod poly
%define HashKey_28      (16*35)   ; HashKey^28 <<1 mod poly
%define HashKey_27      (16*36)   ; HashKey^27 <<1 mod poly
%define HashKey_26      (16*37)   ; HashKey^26 <<1 mod poly
%define HashKey_25      (16*38)   ; HashKey^25 <<1 mod poly
%define HashKey_24      (16*39)   ; HashKey^24 <<1 mod poly
%define HashKey_23      (16*40)   ; HashKey^23 <<1 mod poly
%define HashKey_22      (16*41)   ; HashKey^22 <<1 mod poly
%define HashKey_21      (16*42)   ; HashKey^21 <<1 mod poly
%define HashKey_20      (16*43)   ; HashKey^20 <<1 mod poly
%define HashKey_19      (16*44)   ; HashKey^19 <<1 mod poly
%define HashKey_18      (16*45)   ; HashKey^18 <<1 mod poly
%define HashKey_17      (16*46)   ; HashKey^17 <<1 mod poly
%define HashKey_16      (16*47)   ; HashKey^16 <<1 mod poly
%define HashKey_15      (16*48)   ; HashKey^15 <<1 mod poly
%define HashKey_14      (16*49)   ; HashKey^14 <<1 mod poly
%define HashKey_13      (16*50)   ; HashKey^13 <<1 mod poly
%define HashKey_12      (16*51)   ; HashKey^12 <<1 mod poly
%define HashKey_11      (16*52)   ; HashKey^11 <<1 mod poly
%define HashKey_10      (16*53)   ; HashKey^10 <<1 mod poly
%define HashKey_9       (16*54)   ; HashKey^9 <<1 mod poly
%define HashKey_8       (16*55)   ; HashKey^8 <<1 mod poly
%define HashKey_7       (16*56)   ; HashKey^7 <<1 mod poly
%define HashKey_6       (16*57)   ; HashKey^6 <<1 mod poly
%define HashKey_5       (16*58)   ; HashKey^5 <<1 mod poly
%define HashKey_4       (16*59)   ; HashKey^4 <<1 mod poly
%define HashKey_3       (16*60)   ; HashKey^3 <<1 mod poly
%define HashKey_2       (16*61)   ; HashKey^2 <<1 mod poly
%define HashKey_1       (16*62)   ; HashKey <<1 mod poly
%define HashKey         (16*62)   ; HashKey <<1 mod poly
%endif  ; !GCM_BIG_DATA

%endif ; GCM_KEYS_VAES_AVX512_INCLUDED
