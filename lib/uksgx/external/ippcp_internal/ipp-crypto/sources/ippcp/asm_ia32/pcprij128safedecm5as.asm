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
;
;     Purpose:  Cryptography Primitive.
;               Rijndael-128 (AES) inverse cipher functions.
;               (It's the special free from Sbox/tables implementation)
;
;     Content:
;        SafeDecrypt_RIJ128()
;
;     History:
;
;   Notes.
;   The implementation is based on compact S-box usage.
;
;




%include "asmdefs.inc"
%include "ia_emm.inc"

%if (_IPP == _IPP_M5)

;;
;; transpose 16x16 martix to [dst]
;; eax,ebx,ecx,edx constins matrix rows
;;
%macro TRANSPOSE 1.nolist
  %xdefine %%dst %1

   mov   byte [%%dst+0 ], al
   shr   eax, 8
   mov   byte [%%dst+1 ], bl
   shr   ebx, 8
   mov   byte [%%dst+2 ], cl
   shr   ecx, 8
   mov   byte [%%dst+3 ], dl
   shr   edx, 8

   mov   byte [%%dst+4 ], al
   shr   eax, 8
   mov   byte [%%dst+5 ], bl
   shr   ebx, 8
   mov   byte [%%dst+6 ], cl
   shr   ecx, 8
   mov   byte [%%dst+7 ], dl
   shr   edx, 8

   mov   byte [%%dst+8 ], al
   shr   eax, 8
   mov   byte [%%dst+9 ], bl
   shr   ebx, 8
   mov   byte [%%dst+10], cl
   shr   ecx, 8
   mov   byte [%%dst+11], dl
   shr   edx, 8

   mov   byte [%%dst+12], al
   shr   eax, 8
   mov   byte [%%dst+13], bl
   shr   ebx, 8
   mov   byte [%%dst+14], cl
   shr   ecx, 8
   mov   byte [%%dst+15], dl
   shr   edx, 8
%endmacro


;;
;; SubBute
;;
%macro SBOX_ZERO 1.nolist
  %xdefine %%inpb %1

   mov   al, byte [esi+%%inpb]
%endmacro


%macro SBOX_QUAD 1.nolist
  %xdefine %%inpb %1

   mov      eax, 03Fh
   and      eax, %%inpb                        ; x%64
   shr      %%inpb, 6                          ; x/64
   add      esi, eax                         ; &Sbox[x%64]

   mov      al, byte [esi+64*0]
   mov      bl, byte [esi+64*1]
   mov      cl, byte [esi+64*2]
   mov      dl, byte [esi+64*3]
   mov      byte [esp+oBuffer+1*0], al
   mov      byte [esp+oBuffer+1*1], bl
   mov      byte [esp+oBuffer+1*2], cl
   mov      byte [esp+oBuffer+1*3], dl

   mov      al, byte [esp+oBuffer+%%inpb]
%endmacro


%macro SBOX_FULL 1.nolist
  %xdefine %%inpb %1

   mov      eax, 0Fh
   and      eax, %%inpb                        ; x%16
   shr      %%inpb, 4                          ; x/16
   add      esi, eax                         ; &Sbox[x%16]

   movzx    eax, byte [esi+16*15]
   movzx    ebx, byte [esi+16*14]
   movzx    ecx, byte [esi+16*13]
   movzx    edx, byte [esi+16*12]
   push     eax
   push     ebx
   push     ecx
   push     edx

   movzx    eax, byte [esi+16*11]
   movzx    ebx, byte [esi+16*10]
   movzx    ecx, byte [esi+16*9]
   movzx    edx, byte [esi+16*8]
   push     eax
   push     ebx
   push     ecx
   push     edx

   movzx    eax, byte [esi+16*7]
   movzx    ebx, byte [esi+16*6]
   movzx    ecx, byte [esi+16*5]
   movzx    edx, byte [esi+16*4]
   push     eax
   push     ebx
   push     ecx
   push     edx

   movzx    eax, byte [esi+16*3]
   movzx    ebx, byte [esi+16*2]
   movzx    ecx, byte [esi+16*1]
   movzx    edx, byte [esi+16*0]
   push     eax
   push     ebx
   push     ecx
   push     edx

   mov      eax, dword [esp+%%inpb*sizeof(dword)]
   add      esp, 16*sizeof(dword)
%endmacro


;;
;; AddRoundKey
;;
%macro ADD_ROUND_KEY 5.nolist
  %xdefine %%x0 %1
  %xdefine %%x1 %2
  %xdefine %%x2 %3
  %xdefine %%x3 %4
  %xdefine %%key %5

   xor   %%x0, dword [%%key+sizeof(dword)*0]
   xor   %%x1, dword [%%key+sizeof(dword)*1]
   xor   %%x2, dword [%%key+sizeof(dword)*2]
   xor   %%x3, dword [%%key+sizeof(dword)*3]
%endmacro


;;
;; GFMULx   x, t0,t1
;;
;; mask  = x & 0x80808080
;; mask = (mask<<1) - (mask>>7)
;;
;; x = (x<<=1) & 0xFEFEFEFE
;; x ^= msk & 0x1B1B1B1B
;;
%macro GFMULx 3.nolist
  %xdefine %%x %1
  %xdefine %%msk %2
  %xdefine %%t %3

   mov   %%t, %%x
   add   %%x, %%x              ;; mul: x = (x<<=1) & 0xFEFEFEFE
   and   %%t, 080808080h     ;; mask: t = x & 0x80808080
   and   %%x, 0FEFEFEFEh     ;;
   lea   %%msk, [%%t+%%t]        ;; mask: msk = (t<<1) - (t>>7)
   shr   %%t, 7              ;; mask:
   sub   %%msk, %%t            ;; mask:
   and   %%msk, 01B1B1B1Bh   ;; mul: x ^= msk & 0x1B1B1B1B
   xor   %%x, %%msk
%endmacro


;;
;; MixColumn
;;
%macro MIX_COLUMNS 6.nolist
  %xdefine %%x0 %1
  %xdefine %%x1 %2
  %xdefine %%x2 %3
  %xdefine %%x3 %4
  %xdefine %%t0 %5
  %xdefine %%t1 %6

   ;; Ipp32u y0 = state[1] ^ state[2] ^ state[3];
   ;; Ipp32u y1 = state[0] ^ state[2] ^ state[3];
   ;; Ipp32u y2 = state[0] ^ state[1] ^ state[3];
   ;; Ipp32u y3 = state[0] ^ state[1] ^ state[2];
   mov   %%t0, %%x1
   xor   %%t0, %%x2
   xor   %%t0, %%x3
   mov   %%t1, %%x0
   xor   %%t1, %%x2
   xor   %%t1, %%x3
   mov   dword [esp+oBuffer+sizeof(dword)*0], %%t0
   mov   dword [esp+oBuffer+sizeof(dword)*1], %%t1
   mov   %%t0, %%x0
   xor   %%t0, %%x1
   xor   %%t0, %%x3
   mov   %%t1, %%x0
   xor   %%t1, %%x1
   xor   %%t1, %%x2
   mov   dword [esp+oBuffer+sizeof(dword)*2], %%t0
   mov   dword [esp+oBuffer+sizeof(dword)*3], %%t1

   ;; state[0] = xtime4(state[0]);
   ;; state[1] = xtime4(state[1]);
   ;; state[2] = xtime4(state[2]);
   ;; state[3] = xtime4(state[3]);
   GFMULx   %%x0, %%t0,%%t1
   GFMULx   %%x1, %%t0,%%t1
   GFMULx   %%x2, %%t0,%%t1
   GFMULx   %%x3, %%t0,%%t1

   ;; y0 ^= state[0] ^ state[1];
   ;; y1 ^= state[1] ^ state[2];
   ;; y2 ^= state[2] ^ state[3];
   ;; y3 ^= state[3] ^ state[0];
   mov   %%t0, dword [esp+oBuffer+sizeof(dword)*0]
   mov   %%t1, dword [esp+oBuffer+sizeof(dword)*1]
   xor   %%t0, %%x0
   xor   %%t1, %%x1
   xor   %%t0, %%x1
   xor   %%t1, %%x2
   mov   dword [esp+oBuffer+sizeof(dword)*0], %%t0
   mov   dword [esp+oBuffer+sizeof(dword)*1], %%t1

   mov   %%t0, dword [esp+oBuffer+sizeof(dword)*2]
   mov   %%t1, dword [esp+oBuffer+sizeof(dword)*3]
   xor   %%t0, %%x2
   xor   %%t1, %%x3
   xor   %%t0, %%x3
   xor   %%t1, %%x0
   mov   dword [esp+oBuffer+sizeof(dword)*2], %%t0
   mov   dword [esp+oBuffer+sizeof(dword)*3], %%t1

   ;; t02 = state[0] ^ state[2];
   ;; t13 = state[1] ^ state[3];
   ;; t02 = xtime4(t02);
   ;; t13 = xtime4(t13);
   mov      %%t0, %%x0
   mov      %%t1, %%x1
   xor      %%t0, %%x2
   xor      %%t1, %%x3
   GFMULx   %%t0, %%x0,%%x1   ;; t02
   GFMULx   %%t1, %%x0,%%x1   ;; t13

   ;; t0123 = t02^t13;
   ;; t0123 = xtime4(t0123);
   mov      %%x0, %%t0
   xor      %%x0, %%t1
   GFMULx   %%x0, %%x1,%%x2

   ;; state[0] = y0 ^t02 ^t0123;
   ;; state[1] = y1 ^t13 ^t0123;
   ;; state[2] = y2 ^t02 ^t0123;
   ;; state[3] = y3 ^t13 ^t0123;
   xor   %%t0, %%x0   ;; t02^t0123
   xor   %%t1, %%x0   ;; t13^t0123
   mov   %%x0, dword [esp+oBuffer+sizeof(dword)*0]
   mov   %%x1, dword [esp+oBuffer+sizeof(dword)*1]
   mov   %%x2, dword [esp+oBuffer+sizeof(dword)*2]
   mov   %%x3, dword [esp+oBuffer+sizeof(dword)*3]
   xor   %%x0, %%t0
   xor   %%x1, %%t1
   xor   %%x2, %%t0
   xor   %%x3, %%t1
%endmacro


segment .text align=IPP_ALIGN_FACTOR

IPPASM Safe2Decrypt_RIJ128,PUBLIC
  USES_GPR esi,edi,ebx,ebp

  mov   ebp, esp ; save original esp to use it to reach parameters

%xdefine pInp    [ebp + ARG_1 + 0*sizeof(dword)] ; input buffer
%xdefine pOut    [ebp + ARG_1 + 1*sizeof(dword)] ; outpu buffer
%xdefine nrounds [ebp + ARG_1 + 2*sizeof(dword)] ; number of rounds
%xdefine pRK     [ebp + ARG_1 + 3*sizeof(dword)] ; round keys
%xdefine pSbox   [ebp + ARG_1 + 4*sizeof(dword)] ; S-box

;; stack
%assign oState    0                       ; 4*dword state
%assign oBuffer   oState+sizeof(dword)*4  ; 4*dword buffer
%assign oSbox     oBuffer+sizeof(dword)*4 ; S-box address
%assign oSaveEBP  oSbox+sizeof(dword)     ; save EBP slot

%assign stackSize oSaveEBP+sizeof(dword)  ; stack size

   sub      esp, stackSize    ; state on the stack

   mov      edi, nrounds

   ; read input block and transpose one
   mov      esi, pInp
   mov      eax, dword [esi+sizeof(dword)*0]
   mov      ebx, dword [esi+sizeof(dword)*1]
   mov      ecx, dword [esi+sizeof(dword)*2]
   mov      edx, dword [esi+sizeof(dword)*3]
   TRANSPOSE {esp+oState}

   shl      edi, 4      ; nrounds*16
   mov      esi, pRK
   add      esi, edi

   ; read input block
   mov      eax, dword [esp+oState+sizeof(dword)*0]
   mov      ebx, dword [esp+oState+sizeof(dword)*1]
   mov      ecx, dword [esp+oState+sizeof(dword)*2]
   mov      edx, dword [esp+oState+sizeof(dword)*3]

   ; add round key
   ADD_ROUND_KEY  eax,ebx,ecx,edx, esi ; add round key
   sub      esi, sizeof(byte)*16
   mov      pRK, esi
   mov      dword [esp+oState+sizeof(dword)*0], eax
   mov      dword [esp+oState+sizeof(dword)*1], ebx
   mov      dword [esp+oState+sizeof(dword)*2], ecx
   mov      dword [esp+oState+sizeof(dword)*3], edx

   dec      nrounds
   mov      esi, pSbox
   mov      dword [esp+oSbox], esi
   mov      dword [esp+oSaveEBP], ebp

   ;; regular rounds
.next_aes_round:

   ;; shift rows
   ror      ebx, 24
   ror      ecx, 16
   ror      edx, 8
   mov      dword [esp+oState+sizeof(dword)*0], eax
   mov      dword [esp+oState+sizeof(dword)*1], ebx
   mov      dword [esp+oState+sizeof(dword)*2], ecx
   mov      dword [esp+oState+sizeof(dword)*3], edx

   ;; sub bytes
   xor      ebp, ebp
.sub_byte_loop:
   mov      esi, dword [esp+oSbox]        ; Sbox address
   movzx    edi, byte [esp+oState+ebp]    ; x input byte
   add      ebp, 1
   SBOX_FULL edi
   ;;SBOX_QUAD edi
   ;;SBOX_ZERO edi
   mov      byte [esp+oState+ebp-1], al
   cmp      ebp, 16
   jl       .sub_byte_loop

   ;; add round key
   mov      ebp, dword [esp+oSaveEBP]
   mov      esi, pRK

   mov      eax, dword [esp+oState+sizeof(dword)*0]
   mov      ebx, dword [esp+oState+sizeof(dword)*1]
   mov      ecx, dword [esp+oState+sizeof(dword)*2]
   mov      edx, dword [esp+oState+sizeof(dword)*3]
   ADD_ROUND_KEY  eax,ebx,ecx,edx, esi    ; add round key
   sub      esi, sizeof(byte)*16
   mov      pRK, esi

   ;; mix columns
   MIX_COLUMNS eax,ebx,ecx,edx, esi, edi  ; mix columns

   sub      nrounds, 1
   jne      .next_aes_round

   ;; irregular round: shift rows
   ror      ebx, 24
   ror      ecx, 16
   ror      edx, 8
   mov      dword [esp+oState+sizeof(dword)*0], eax
   mov      dword [esp+oState+sizeof(dword)*1], ebx
   mov      dword [esp+oState+sizeof(dword)*2], ecx
   mov      dword [esp+oState+sizeof(dword)*3], edx

   ;; irregular round: sub bytes
   xor      ebp, ebp
.sub_byte_irr_loop:
   mov      esi, dword [esp+oSbox]
   movzx    edi, byte [esp+oState+ebp]    ; x input byte
   add      ebp, 1
   SBOX_FULL edi
   ;;SBOX_QUAD edi
   ;;SBOX_ZERO edi
   mov      byte [esp+oState+ebp-1], al
   cmp      ebp, 16
   jl       .sub_byte_irr_loop

   ;; irregular round: add round key
   mov      ebp, dword [esp+oSaveEBP]
   mov      esi, pRK

   mov      eax, dword [esp+oState+sizeof(dword)*0]
   mov      ebx, dword [esp+oState+sizeof(dword)*1]
   mov      ecx, dword [esp+oState+sizeof(dword)*2]
   mov      edx, dword [esp+oState+sizeof(dword)*3]
   ADD_ROUND_KEY  eax,ebx,ecx,edx, esi    ; add round key

   mov      edi, pOut
   TRANSPOSE edi

   add      esp, stackSize ; remove state
   REST_GPR
   ret
ENDFUNC Safe2Decrypt_RIJ128


%endif    ; _IPP == _IPP_M5
