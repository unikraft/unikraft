/*******************************************************************************
* Copyright 2020-2021 Intel Corporation
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

#include <internal/sm3/sm3_avx512.h>

__ALIGN64 static const int32u tj_calculated[] = { 0x79CC4519,0xF3988A32,0xE7311465,0xCE6228CB,0x9CC45197,0x3988A32F,0x7311465E,0xE6228CBC,
                                                  0xCC451979,0x988A32F3,0x311465E7,0x6228CBCE,0xC451979C,0x88A32F39,0x11465E73,0x228CBCE6,
                                                  0x9D8A7A87,0x3B14F50F,0x7629EA1E,0xEC53D43C,0xD8A7A879,0xB14F50F3,0x629EA1E7,0xC53D43CE,
                                                  0x8A7A879D,0x14F50F3B,0x29EA1E76,0x53D43CEC,0xA7A879D8,0x4F50F3B1,0x9EA1E762,0x3D43CEC5,
                                                  0x7A879D8A,0xF50F3B14,0xEA1E7629,0xD43CEC53,0xA879D8A7,0x50F3B14F,0xA1E7629E,0x43CEC53D,
                                                  0x879D8A7A,0x0F3B14F5,0x1E7629EA,0x3CEC53D4,0x79D8A7A8,0xF3B14F50,0xE7629EA1,0xCEC53D43 };


__INLINE void TRANSPOSE_16X16_I32(int32u out[][16], const int32u* inp[16])
{
    __m512i r0 = _mm512_loadu_si512(inp[0]);
    __m512i r1 = _mm512_loadu_si512(inp[1]);
    __m512i r2 = _mm512_loadu_si512(inp[2]);
    __m512i r3 = _mm512_loadu_si512(inp[3]);
    __m512i r4 = _mm512_loadu_si512(inp[4]);
    __m512i r5 = _mm512_loadu_si512(inp[5]);
    __m512i r6 = _mm512_loadu_si512(inp[6]);
    __m512i r7 = _mm512_loadu_si512(inp[7]);
    __m512i r8 = _mm512_loadu_si512(inp[8]);
    __m512i r9 = _mm512_loadu_si512(inp[9]);
    __m512i r10 = _mm512_loadu_si512(inp[10]);
    __m512i r11 = _mm512_loadu_si512(inp[11]);
    __m512i r12 = _mm512_loadu_si512(inp[12]);
    __m512i r13 = _mm512_loadu_si512(inp[13]);
    __m512i r14 = _mm512_loadu_si512(inp[14]);
    __m512i r15 = _mm512_loadu_si512(inp[15]);

    // tansposition
    __m512i t0 = _mm512_unpacklo_epi32(r0, r1);  //   0  16   1  17   4  20   5  21   8  24   9  25  12  28  13  29 
    __m512i t1 = _mm512_unpackhi_epi32(r0, r1);  //   2  18   3  19   6  22   7  23  10  26  11  27  14  30  15  31
    __m512i t2 = _mm512_unpacklo_epi32(r2, r3);  //  32  48  33  49 ...
    __m512i t3 = _mm512_unpackhi_epi32(r2, r3);  //  34  50  35  51 ...
    __m512i t4 = _mm512_unpacklo_epi32(r4, r5);  //  64  80  65  81 ...  
    __m512i t5 = _mm512_unpackhi_epi32(r4, r5);  //  66  82  67  83 ...
    __m512i t6 = _mm512_unpacklo_epi32(r6, r7);  //  96 112  97 113 ...
    __m512i t7 = _mm512_unpackhi_epi32(r6, r7);  //  98 114  99 115 ...
    __m512i t8 = _mm512_unpacklo_epi32(r8, r9);  // 128 ...
    __m512i t9 = _mm512_unpackhi_epi32(r8, r9);  // 130 ...
    __m512i t10 = _mm512_unpacklo_epi32(r10, r11); // 160 ...
    __m512i t11 = _mm512_unpackhi_epi32(r10, r11); // 162 ...
    __m512i t12 = _mm512_unpacklo_epi32(r12, r13); // 196 ...
    __m512i t13 = _mm512_unpackhi_epi32(r12, r13); // 198 ...
    __m512i t14 = _mm512_unpacklo_epi32(r14, r15); // 228 ...
    __m512i t15 = _mm512_unpackhi_epi32(r14, r15); // 230 ...

    r0 = _mm512_unpacklo_epi64(t0, t2); //   0  16  32  48 ...
    r1 = _mm512_unpackhi_epi64(t0, t2); //   1  17  33  49 ...
    r2 = _mm512_unpacklo_epi64(t1, t3); //   2  18  34  49 ...
    r3 = _mm512_unpackhi_epi64(t1, t3); //   3  19  35  51 ...
    r4 = _mm512_unpacklo_epi64(t4, t6); //  64  80  96 112 ...  
    r5 = _mm512_unpackhi_epi64(t4, t6); //  65  81  97 114 ...
    r6 = _mm512_unpacklo_epi64(t5, t7); //  66  82  98 113 ...
    r7 = _mm512_unpackhi_epi64(t5, t7); //  67  83  99 115 ...
    r8 = _mm512_unpacklo_epi64(t8, t10); // 128 144 160 176 ...  
    r9 = _mm512_unpackhi_epi64(t8, t10); // 129 145 161 178 ...
    r10 = _mm512_unpacklo_epi64(t9, t11); // 130 146 162 177 ... 
    r11 = _mm512_unpackhi_epi64(t9, t11); // 131 147 163 179 ...
    r12 = _mm512_unpacklo_epi64(t12, t14); // 192 208 228 240 ... 
    r13 = _mm512_unpackhi_epi64(t12, t14); // 193 209 229 241 ...
    r14 = _mm512_unpacklo_epi64(t13, t15); // 194 210 230 242 ...
    r15 = _mm512_unpackhi_epi64(t13, t15); // 195 211 231 243 ...

    t0 = _mm512_shuffle_i32x4(r0, r4, 0x88);  //   0  16  32  48   8  24  40  56  64  80  96  112 ...
    t1 = _mm512_shuffle_i32x4(r1, r5, 0x88);  //   1  17  33  49 ...
    t2 = _mm512_shuffle_i32x4(r2, r6, 0x88);  //   2  18  34  50 ...
    t3 = _mm512_shuffle_i32x4(r3, r7, 0x88);  //   3  19  35  51 ...
    t4 = _mm512_shuffle_i32x4(r0, r4, 0xdd);  //   4  20  36  52 ...
    t5 = _mm512_shuffle_i32x4(r1, r5, 0xdd);  //   5  21  37  53 ...
    t6 = _mm512_shuffle_i32x4(r2, r6, 0xdd);  //   6  22  38  54 ...
    t7 = _mm512_shuffle_i32x4(r3, r7, 0xdd);  //   7  23  39  55 ...
    t8 = _mm512_shuffle_i32x4(r8, r12, 0x88); // 128 144 160 176 ...
    t9 = _mm512_shuffle_i32x4(r9, r13, 0x88); // 129 145 161 177 ...
    t10 = _mm512_shuffle_i32x4(r10, r14, 0x88); // 130 146 162 178 ...
    t11 = _mm512_shuffle_i32x4(r11, r15, 0x88); // 131 147 163 179 ...
    t12 = _mm512_shuffle_i32x4(r8, r12, 0xdd); // 132 148 164 180 ...
    t13 = _mm512_shuffle_i32x4(r9, r13, 0xdd); // 133 149 165 181 ...
    t14 = _mm512_shuffle_i32x4(r10, r14, 0xdd); // 134 150 166 182 ...
    t15 = _mm512_shuffle_i32x4(r11, r15, 0xdd); // 135 151 167 183 ...

    r0 = _mm512_shuffle_i32x4(t0, t8, 0x88); //   0  16  32  48  64  80  96 112 ... 240
    r1 = _mm512_shuffle_i32x4(t1, t9, 0x88); //   1  17  33  49  66  81  97 113 ... 241
    r2 = _mm512_shuffle_i32x4(t2, t10, 0x88); //   2  18  34  50  67  82  98 114 ... 242
    r3 = _mm512_shuffle_i32x4(t3, t11, 0x88); //   3  19  35  51  68  83  99 115 ... 243
    r4 = _mm512_shuffle_i32x4(t4, t12, 0x88); //   4 ...
    r5 = _mm512_shuffle_i32x4(t5, t13, 0x88); //   5 ...
    r6 = _mm512_shuffle_i32x4(t6, t14, 0x88); //   6 ...
    r7 = _mm512_shuffle_i32x4(t7, t15, 0x88); //   7 ...
    r8 = _mm512_shuffle_i32x4(t0, t8, 0xdd); //   8 ...
    r9 = _mm512_shuffle_i32x4(t1, t9, 0xdd); //   9 ...
    r10 = _mm512_shuffle_i32x4(t2, t10, 0xdd); //  10 ...
    r11 = _mm512_shuffle_i32x4(t3, t11, 0xdd); //  11 ...
    r12 = _mm512_shuffle_i32x4(t4, t12, 0xdd); //  12 ...
    r13 = _mm512_shuffle_i32x4(t5, t13, 0xdd); //  13 ...
    r14 = _mm512_shuffle_i32x4(t6, t14, 0xdd); //  14 ...
    r15 = _mm512_shuffle_i32x4(t7, t15, 0xdd); //  15  31  47  63  79  96 111 127 ... 255

    _mm512_storeu_si512(out[0], r0);
    _mm512_storeu_si512(out[1], r1);
    _mm512_storeu_si512(out[2], r2);
    _mm512_storeu_si512(out[3], r3);
    _mm512_storeu_si512(out[4], r4);
    _mm512_storeu_si512(out[5], r5);
    _mm512_storeu_si512(out[6], r6);
    _mm512_storeu_si512(out[7], r7);
    _mm512_storeu_si512(out[8], r8);
    _mm512_storeu_si512(out[9], r9);
    _mm512_storeu_si512(out[10], r10);
    _mm512_storeu_si512(out[11], r11);
    _mm512_storeu_si512(out[12], r12);
    _mm512_storeu_si512(out[13], r13);
    _mm512_storeu_si512(out[14], r14);
    _mm512_storeu_si512(out[15], r15);
}

/* Boolean functions (0<=nr<16) */
#define FF1(X,Y,Z) (_mm512_xor_epi32(_mm512_xor_epi32(X,Y), Z))
#define GG1(X,Y,Z) (_mm512_xor_epi32(_mm512_xor_epi32(X,Y), Z))
/* Boolean functions (16<=nr<64)  */
#define FF2(X,Y,Z) (_mm512_or_epi32(_mm512_or_epi32(_mm512_and_epi32(X,Y),_mm512_and_epi32(X,Z)),_mm512_and_epi32(Y,Z)))
#define GG2(X,Y,Z) (_mm512_or_epi32(_mm512_and_epi32(X,Y),_mm512_andnot_epi32(X,Z)))

/* P0 permutation: */
#define P0(X)  (_mm512_xor_epi32(_mm512_xor_epi32(X, _mm512_rol_epi32 (X, 9)), _mm512_rol_epi32 (X, 17)))
/* P1 permutation: */
#define P1(X)  (_mm512_xor_epi32(_mm512_xor_epi32(X, _mm512_rol_epi32 (X, 15)), _mm512_rol_epi32 (X, 23)))

/* Update W */
#define WUPDATE(nr, W) (_mm512_xor_epi32(_mm512_xor_epi32(P1(_mm512_xor_epi32(_mm512_xor_epi32(W[(nr-16)&15], W[(nr-9)&15]), _mm512_rol_epi32 (W[(nr-3)&15], 15))), _mm512_rol_epi32(W[(nr-13)&15],7)), W[(nr-6)&15]))

// SM3 steps
/* (0<=nr<16) */
#define STEP1_SM3(nr, A,B,C,D,E,F,G,H, Tj, W) {\
    __m512i SS1 = _mm512_rol_epi32(_mm512_add_epi32(_mm512_add_epi32(_mm512_rol_epi32(A, 12), E), _mm512_set1_epi32((int)Tj)),7);\
    __m512i SS2 = _mm512_xor_epi32(SS1, _mm512_rol_epi32(A, 12));\
    __m512i TT1 = _mm512_add_epi32(_mm512_add_epi32(_mm512_add_epi32(FF1(A, B, C), D), SS2), _mm512_xor_epi32(W[nr&15], W[(nr+4)&15]));\
    __m512i TT2 = _mm512_add_epi32(_mm512_add_epi32(_mm512_add_epi32(GG1(E, F, G), H), SS1), W[nr&15]);\
    D = _mm512_load_epi32((void*)&C);  \
    C = _mm512_rol_epi32(B, 9);        \
    B = _mm512_load_epi32((void*)&A);  \
    A = _mm512_load_epi32((void*)&TT1);\
    H = _mm512_load_epi32((void*)&G);  \
    G = _mm512_rol_epi32(F, 19);       \
    F = _mm512_load_epi32((void*)&E);  \
    E = P0(TT2);                       \
    W[(nr)&15]=WUPDATE(nr, W);         \
}

/* (16<=nr<64)  */
#define STEP2_SM3(nr, A,B,C,D,E,F,G,H, Tj, W) {\
    __m512i SS1 = _mm512_rol_epi32(_mm512_add_epi32(_mm512_add_epi32(_mm512_rol_epi32(A, 12), E), _mm512_set1_epi32((int)Tj)),7);\
    __m512i SS2 = _mm512_xor_epi32(SS1, _mm512_rol_epi32(A, 12));\
    __m512i TT1 = _mm512_add_epi32(_mm512_add_epi32(_mm512_add_epi32(FF2(A, B, C), D), SS2), _mm512_xor_epi32(W[nr&15], W[(nr+4)&15]));\
    __m512i TT2 = _mm512_add_epi32(_mm512_add_epi32(_mm512_add_epi32(GG2(E, F, G), H), SS1), W[nr&15]);\
    D = _mm512_load_epi32((void*)&C);  \
    C = _mm512_rol_epi32(B, 9);        \
    B = _mm512_load_epi32((void*)&A);  \
    A = _mm512_load_epi32((void*)&TT1);\
    H = _mm512_load_epi32((void*)&G);  \
    G = _mm512_rol_epi32(F, 19);       \
    F = _mm512_load_epi32((void*)&E);  \
    E = P0(TT2);                       \
    W[(nr)&15]=WUPDATE(nr, W);         \
}



void sm3_avx512_mb16(int32u* hash_pa[16], const int8u* msg_pa[16], int len[16])
{
    int i;

    int32u* loc_data[SM3_NUM_BUFFERS];
    int loc_len[SM3_NUM_BUFFERS];

    __m512i W[16];
    __m512i Vi[8];
    __m512i A, B, C, D, E, F, G, H;

    /* Allocate memory to handle numBuffers < 16, set data in not valid buffers to zero */
    __m512i zero_buffer = _mm512_setzero_si512();
    /* Allocate memory for transposed data */
    __m512i transposed_data[SM3_NUM_BUFFERS];

    /* Load processing mask */
    __mmask16 mb_mask = _mm512_cmp_epi32_mask(_mm512_loadu_si512(len), zero_buffer, 4);

    /* Load data and set the data to zero in not valid buffers */
    M512(loc_len) = _mm512_loadu_si512(len);

    _mm512_storeu_si512(loc_data, _mm512_mask_loadu_epi64(_mm512_set1_epi64((long long)&zero_buffer), (__mmask8)mb_mask, msg_pa));
    _mm512_storeu_si512(loc_data+8, _mm512_mask_loadu_epi64(_mm512_set1_epi64((long long)&zero_buffer), *((__mmask8*)&mb_mask + 1), msg_pa + 8));

    /* Load hash value */
    A = _mm512_loadu_si512(hash_pa);
    B = _mm512_loadu_si512(hash_pa + 1 * SM3_SIZE_IN_WORDS);
    C = _mm512_loadu_si512(hash_pa + 2 * SM3_SIZE_IN_WORDS);
    D = _mm512_loadu_si512(hash_pa + 3 * SM3_SIZE_IN_WORDS);
    E = _mm512_loadu_si512(hash_pa + 4 * SM3_SIZE_IN_WORDS);
    F = _mm512_loadu_si512(hash_pa + 5 * SM3_SIZE_IN_WORDS);
    G = _mm512_loadu_si512(hash_pa + 6 * SM3_SIZE_IN_WORDS);
    H = _mm512_loadu_si512(hash_pa + 7 * SM3_SIZE_IN_WORDS);

    /* Loop over the message */
    while (mb_mask){
        /* Transpose the message data */
        TRANSPOSE_16X16_I32((int32u(*)[16])transposed_data, (const int32u**)loc_data);  

        /* Init W (remember about endian) */
        for (i = 0; i < 16; i++) {
            W[i] = transposed_data[i];
            W[i] = SIMD_ENDIANNESS32(W[i]);
        }

        /* Store previous hash for xor operation V(i+1) = ABCDEFGH XOR V(i) */
        Vi[0] = _mm512_load_epi32((void*)&A);
        Vi[1] = _mm512_load_epi32((void*)&B);
        Vi[2] = _mm512_load_epi32((void*)&C);
        Vi[3] = _mm512_load_epi32((void*)&D);
        Vi[4] = _mm512_load_epi32((void*)&E);
        Vi[5] = _mm512_load_epi32((void*)&F);
        Vi[6] = _mm512_load_epi32((void*)&G);
        Vi[7] = _mm512_load_epi32((void*)&H);

        /* Compression function */
        {
            STEP1_SM3(0, A, B, C, D, E, F, G, H, tj_calculated[0], W);
            STEP1_SM3(1, A, B, C, D, E, F, G, H, tj_calculated[1], W);
            STEP1_SM3(2, A, B, C, D, E, F, G, H, tj_calculated[2], W);
            STEP1_SM3(3, A, B, C, D, E, F, G, H, tj_calculated[3], W);
            STEP1_SM3(4, A, B, C, D, E, F, G, H, tj_calculated[4], W);
            STEP1_SM3(5, A, B, C, D, E, F, G, H, tj_calculated[5], W);
            STEP1_SM3(6, A, B, C, D, E, F, G, H, tj_calculated[6], W);
            STEP1_SM3(7, A, B, C, D, E, F, G, H, tj_calculated[7], W);

            STEP1_SM3(8, A, B, C, D, E, F, G, H, tj_calculated[8], W);
            STEP1_SM3(9, A, B, C, D, E, F, G, H, tj_calculated[9], W);
            STEP1_SM3(10, A, B, C, D, E, F, G, H, tj_calculated[10], W);
            STEP1_SM3(11, A, B, C, D, E, F, G, H, tj_calculated[11], W);
            STEP1_SM3(12, A, B, C, D, E, F, G, H, tj_calculated[12], W);
            STEP1_SM3(13, A, B, C, D, E, F, G, H, tj_calculated[13], W);
            STEP1_SM3(14, A, B, C, D, E, F, G, H, tj_calculated[14], W);
            STEP1_SM3(15, A, B, C, D, E, F, G, H, tj_calculated[15], W);

            STEP2_SM3(16, A, B, C, D, E, F, G, H, tj_calculated[16], W);
            STEP2_SM3(17, A, B, C, D, E, F, G, H, tj_calculated[17], W);
            STEP2_SM3(18, A, B, C, D, E, F, G, H, tj_calculated[18], W);
            STEP2_SM3(19, A, B, C, D, E, F, G, H, tj_calculated[19], W);
            STEP2_SM3(20, A, B, C, D, E, F, G, H, tj_calculated[20], W);
            STEP2_SM3(21, A, B, C, D, E, F, G, H, tj_calculated[21], W);
            STEP2_SM3(22, A, B, C, D, E, F, G, H, tj_calculated[22], W);
            STEP2_SM3(23, A, B, C, D, E, F, G, H, tj_calculated[23], W);

            STEP2_SM3(24, A, B, C, D, E, F, G, H, tj_calculated[24], W);
            STEP2_SM3(25, A, B, C, D, E, F, G, H, tj_calculated[25], W);
            STEP2_SM3(26, A, B, C, D, E, F, G, H, tj_calculated[26], W);
            STEP2_SM3(27, A, B, C, D, E, F, G, H, tj_calculated[27], W);
            STEP2_SM3(28, A, B, C, D, E, F, G, H, tj_calculated[28], W);
            STEP2_SM3(29, A, B, C, D, E, F, G, H, tj_calculated[29], W);
            STEP2_SM3(30, A, B, C, D, E, F, G, H, tj_calculated[30], W);
            STEP2_SM3(31, A, B, C, D, E, F, G, H, tj_calculated[31], W);

            STEP2_SM3(32, A, B, C, D, E, F, G, H, tj_calculated[32], W);
            STEP2_SM3(33, A, B, C, D, E, F, G, H, tj_calculated[33], W);
            STEP2_SM3(34, A, B, C, D, E, F, G, H, tj_calculated[34], W);
            STEP2_SM3(35, A, B, C, D, E, F, G, H, tj_calculated[35], W);
            STEP2_SM3(36, A, B, C, D, E, F, G, H, tj_calculated[36], W);
            STEP2_SM3(37, A, B, C, D, E, F, G, H, tj_calculated[37], W);
            STEP2_SM3(38, A, B, C, D, E, F, G, H, tj_calculated[38], W);
            STEP2_SM3(39, A, B, C, D, E, F, G, H, tj_calculated[39], W);

            STEP2_SM3(40, A, B, C, D, E, F, G, H, tj_calculated[40], W);
            STEP2_SM3(41, A, B, C, D, E, F, G, H, tj_calculated[41], W);
            STEP2_SM3(42, A, B, C, D, E, F, G, H, tj_calculated[42], W);
            STEP2_SM3(43, A, B, C, D, E, F, G, H, tj_calculated[43], W);
            STEP2_SM3(44, A, B, C, D, E, F, G, H, tj_calculated[44], W);
            STEP2_SM3(45, A, B, C, D, E, F, G, H, tj_calculated[45], W);
            STEP2_SM3(46, A, B, C, D, E, F, G, H, tj_calculated[46], W);
            STEP2_SM3(47, A, B, C, D, E, F, G, H, tj_calculated[47], W);

            STEP2_SM3(48, A, B, C, D, E, F, G, H, tj_calculated[16], W);
            STEP2_SM3(49, A, B, C, D, E, F, G, H, tj_calculated[17], W);
            STEP2_SM3(50, A, B, C, D, E, F, G, H, tj_calculated[18], W);
            STEP2_SM3(51, A, B, C, D, E, F, G, H, tj_calculated[19], W);
            STEP2_SM3(52, A, B, C, D, E, F, G, H, tj_calculated[20], W);
            STEP2_SM3(53, A, B, C, D, E, F, G, H, tj_calculated[21], W);
            STEP2_SM3(54, A, B, C, D, E, F, G, H, tj_calculated[22], W);
            STEP2_SM3(55, A, B, C, D, E, F, G, H, tj_calculated[23], W);

            STEP2_SM3(56, A, B, C, D, E, F, G, H, tj_calculated[24], W);
            STEP2_SM3(57, A, B, C, D, E, F, G, H, tj_calculated[25], W);
            STEP2_SM3(58, A, B, C, D, E, F, G, H, tj_calculated[26], W);
            STEP2_SM3(59, A, B, C, D, E, F, G, H, tj_calculated[27], W);
            STEP2_SM3(60, A, B, C, D, E, F, G, H, tj_calculated[28], W);
            STEP2_SM3(61, A, B, C, D, E, F, G, H, tj_calculated[29], W);
            STEP2_SM3(62, A, B, C, D, E, F, G, H, tj_calculated[30], W);
            STEP2_SM3(63, A, B, C, D, E, F, G, H, tj_calculated[31], W);
        }

        A = _mm512_mask_xor_epi32(Vi[0], mb_mask, A, Vi[0]);
        B = _mm512_mask_xor_epi32(Vi[1], mb_mask, B, Vi[1]);
        C = _mm512_mask_xor_epi32(Vi[2], mb_mask, C, Vi[2]);
        D = _mm512_mask_xor_epi32(Vi[3], mb_mask, D, Vi[3]);
        E = _mm512_mask_xor_epi32(Vi[4], mb_mask, E, Vi[4]);
        F = _mm512_mask_xor_epi32(Vi[5], mb_mask, F, Vi[5]);
        G = _mm512_mask_xor_epi32(Vi[6], mb_mask, G, Vi[6]);
        H = _mm512_mask_xor_epi32(Vi[7], mb_mask, H, Vi[7]);

        _mm512_storeu_si512(hash_pa, A);
        _mm512_storeu_si512(hash_pa + 1 * SM3_SIZE_IN_WORDS, B);
        _mm512_storeu_si512(hash_pa + 2 * SM3_SIZE_IN_WORDS, C);
        _mm512_storeu_si512(hash_pa + 3 * SM3_SIZE_IN_WORDS, D);
        _mm512_storeu_si512(hash_pa + 4 * SM3_SIZE_IN_WORDS, E);
        _mm512_storeu_si512(hash_pa + 5 * SM3_SIZE_IN_WORDS, F);
        _mm512_storeu_si512(hash_pa + 6 * SM3_SIZE_IN_WORDS, G);
        _mm512_storeu_si512(hash_pa + 7 * SM3_SIZE_IN_WORDS, H);
 
        /* Update pointers to data, local  lengths and mask */
        M512(loc_data) = _mm512_mask_add_epi64(_mm512_set1_epi64((long long)&zero_buffer), (__mmask8)mb_mask, _mm512_loadu_si512(loc_data), _mm512_set1_epi64(SM3_MSG_BLOCK_SIZE));
        M512(loc_data + 8) = _mm512_mask_add_epi64(_mm512_set1_epi64((long long)&zero_buffer), *((__mmask8*)&mb_mask + 1), _mm512_loadu_si512(loc_data+8), _mm512_set1_epi64(SM3_MSG_BLOCK_SIZE));

        M512(loc_len) = _mm512_mask_sub_epi32(zero_buffer, mb_mask, _mm512_loadu_si512(loc_len), _mm512_set1_epi32(SM3_MSG_BLOCK_SIZE));
        mb_mask = _mm512_cmp_epi32_mask(_mm512_loadu_si512(loc_len), zero_buffer, 4);
    }
}
