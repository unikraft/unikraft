/*******************************************************************************
* Copyright 2002-2021 Intel Corporation
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

#include <internal/ecnist/ifma_ecpoint_p521.h>
#include <internal/rsa/ifma_rsa_arith.h>


/* simplify naming */
#define sqr    MB_FUNC_NAME(ifma_ams52_p521_)
#define mul    MB_FUNC_NAME(ifma_amm52_p521_)
#define add    MB_FUNC_NAME(ifma_add52_p521_)
#define sub    MB_FUNC_NAME(ifma_sub52_p521_)
#define mul2   MB_FUNC_NAME(ifma_double52_p521_)
#define mul3   MB_FUNC_NAME(ifma_tripple52_p521_)
#define div2   MB_FUNC_NAME(ifma_half52_p521_)


/*
// Presentation of point at infinity:
//    - projective (X : Y : 0) 
//    - affine     (0 : 0)   
*/

/*
// R(X3:Y3:Z3) = [2]P(X1:Y1:Z1)
//
// formulas:
//    A = 4*X1*Y1^2
//    B = 3*(X1^2-Z1^4)
//    X3= B^2 -2*A
//    Y3= B*(A-X3) -8*Y1^4
//    Z3= 2*Y1*Z1
//
// cost: 4S+4M+9A
//
*/
void MB_FUNC_NAME(ifma_ec_nistp521_dbl_point_)(P521_POINT* r, const P521_POINT* p)
{
   __ALIGN64 U64 T[P521_LEN52];
   __ALIGN64 U64 U[P521_LEN52];
   __ALIGN64 U64 V[P521_LEN52];
   __ALIGN64 U64 A[P521_LEN52];
   __ALIGN64 U64 B[P521_LEN52];

   const U64* X1 = p->X;   /* input point */
   const U64* Y1 = p->Y;
   const U64* Z1 = p->Z;
   U64* X3 = r->X;         /* output point */
   U64* Y3 = r->Y;
   U64* Z3 = r->Z;

   mul2(T, Y1);      /* T = 2*Y1 */

   sqr(V, T);        /* V = 4*Y1^2 */        /* sqr_dual */
   sqr(U, Z1);       /* U = Z1^2 */

   sub(B, X1, U);    /* B = X1-Z1^2 */
   add(U, X1, U);    /* U = X1+Z1^2 */

   mul(A, V, X1);    /* A = 4*X*Y1^2 */      /* mul_dual */
   mul(B, B, U);     /* B = (X1^2-Z1^4) */

   mul2(X3, A);      /* X3 = 2*A */
   mul3(B, B);       /* B = 3*(X1^2-Z1^4) */

   sqr(U, B);        /* U = B^2 */           /* sqr_dual */
   sqr(Y3, V);       /* Y3= V^2 = 16*Y1^4 */

   sub(X3, U, X3);   /* X3=B^2 - 2*A */
   div2(Y3,Y3);      /* Y3=Y3/2 = 8*Y1^4 */

   sub(U, A, X3);    /* U = A-X3 */

   mul(Z3, T, Z1);   /* Z3= 2*Y1*Z1 */       /* mul_dual */
   mul(U, U, B);     /* U = B*(A-X3) */

   sub(Y3, U, Y3);   /* Y3 = B*(A-X3) -8*Y1^4 */
}


/*
// R(X3:Y3:Z3) = P(X1:Y1:Z1) + Q(X2:Y2:Z2)
//
// formulas:
//    A = X1*Z2^2     B = X2*Z1^2     C = Y1*Z2^3     D = Y2*Z1^3
//    E = B-A         F = D-C
//    X3= -E^3 -2*A*E^2 + F^2
//    Y3= -C*E^3 + F*(A*E^2 -X3)
//    Z3= Z1*Z2*E
//
// cost: 4S+12M+7A
//
*/
void MB_FUNC_NAME(ifma_ec_nistp521_add_point_)(P521_POINT* r, const P521_POINT* p, const P521_POINT* q)
{
   /* coordinates of p */
   const U64* X1 = p->X;
   const U64* Y1 = p->Y;
   const U64* Z1 = p->Z;
   __mb_mask p_at_infinity = MB_FUNC_NAME(is_zero_point_cordinate_)(p->Z);

   /* coordinates of q */
   const U64* X2 = q->X;
   const U64* Y2 = q->Y;
   const U64* Z2 = q->Z;
   __mb_mask q_at_infinity = MB_FUNC_NAME(is_zero_point_cordinate_)(q->Z);

   /* coordinates of temp point T(X3:Y3:Z3) */
   __ALIGN64 U64 X3[P521_LEN52];
   __ALIGN64 U64 Y3[P521_LEN52];
   __ALIGN64 U64 Z3[P521_LEN52];

   /* temporary */
   __ALIGN64 U64 U1[P521_LEN52];
   __ALIGN64 U64 U2[P521_LEN52];
   __ALIGN64 U64 S1[P521_LEN52];
   __ALIGN64 U64 S2[P521_LEN52];
   __ALIGN64 U64  H[P521_LEN52];
   __ALIGN64 U64  R[P521_LEN52];

   mul(S1, Y1, Z2);     /* S1 = Y1*Z2 */
   sqr(U1, Z2);         /* U1 = Z2^2  */

   mul(S2, Y2, Z1);     /* S2 = Y2*Z1 */
   sqr(U2, Z1);         /* U2 = Z1^2 */

   mul(S1, S1, U1);     /* S1 = Y1*Z2^3 */
   mul(S2, S2, U2);     /* S2 = Y2*Z1^3 */

   mul(U1, X1, U1);     /* U1 = X1*Z2^2 */
   mul(U2, X2, U2);     /* U2 = X2*Z1^2 */

   sub(R, S2, S1);      /* R = S2-S1 */
   sub(H, U2, U1);      /* H = U2-U1 */

   /*
   // Spefial case handlig omitted
   // due to function is using in context of point multiplication
   // and even more precisely in context of ECDH, ECDSA operations.
   //
   // inside ECDH and ECDSA are using 0 < scalar < order_of_base_point
   //
    if (is_equal(U1, U2) && !in1infty && !in2infty) {
        if (is_equal(S1, S2)) {
            point_double(r, a);
            return;
        } else {
            memset(r, 0, sizeof(*r));
            return;
        }
    }
   */

   mul(Z3, Z1, Z2);     /* Z3 = Z1*Z2 */
   sqr(U2, H);          /* U2 = H^2 */
   mul(Z3, Z3, H);      /* Z3 = (Z1*Z2)*H */
   sqr(S2, R);          /* S2 = R^2 */
   mul(H, H, U2);       /* H = H^3 */

   mul(U1, U1, U2);     /* U1 = U1*H^2 */
   sub(X3, S2, H);      /* X3 = R^2 - H^3 */
   mul2(U2, U1);        /* U2 = 2*U1*H^2 */
   mul(S1, S1, H);      /* S1 = S1*H^3 */
   sub(X3, X3, U2);     /* X3 = (R^2 - H^3) -2*U1*H^2 */

   sub(Y3, U1, X3);    /* Y3 = R*(U1*H^2 - X3) -S1*H^3 */
   mul(Y3, Y3, R);
   sub(Y3, Y3, S1);

   /* T = p_at_infinity? q : T */
   MB_FUNC_NAME(mask_mov_FE521_)(X3, X3, p_at_infinity, q->X);
   MB_FUNC_NAME(mask_mov_FE521_)(Y3, Y3, p_at_infinity, q->Y);
   MB_FUNC_NAME(mask_mov_FE521_)(Z3, Z3, p_at_infinity, q->Z);
   /* T = q_at_infinity? p : T */
   MB_FUNC_NAME(mask_mov_FE521_)(X3, X3, q_at_infinity, p->X);
   MB_FUNC_NAME(mask_mov_FE521_)(Y3, Y3, q_at_infinity, p->Y);
   MB_FUNC_NAME(mask_mov_FE521_)(Z3, Z3, q_at_infinity, p->Z);

   /* r = T */
   MB_FUNC_NAME(mov_FE521_)(r->X, X3);
   MB_FUNC_NAME(mov_FE521_)(r->Y, Y3);
   MB_FUNC_NAME(mov_FE521_)(r->Z, Z3);
}


/* Montgomery(1)
// r = 2^(P521_LEN52*DIGIT_SIZE) mod p521
*/
__ALIGN64 static const int64u p521_r_mb[P521_LEN52][sizeof(U64)/sizeof(int64u)] = {
   { REP8_DECL(0x0008000000000000) },
   { REP8_DECL(0x0000000000000000) },
   { REP8_DECL(0x0000000000000000) },
   { REP8_DECL(0x0000000000000000) },
   { REP8_DECL(0x0000000000000000) },
   { REP8_DECL(0x0000000000000000) },
   { REP8_DECL(0x0000000000000000) },
   { REP8_DECL(0x0000000000000000) },
   { REP8_DECL(0x0000000000000000) },
   { REP8_DECL(0x0000000000000000) },
   { REP8_DECL(0x0000000000000000) }
};


/*
// R(X3:Y3:Z3) = P(X1:Y1:Z1) + Q(X2:Y2:Z2=1)
//
// formulas:
//    A = X1*Z2^2     B = X2*Z1^2     C = Y1*Z2^3     D = Y2*Z1^3
//    E = B-A         F = D-C
//    X3= -E^3 -2*A*E^2 + F^2
//    Y3= -C*E^3 + F*(A*E^2 -X3)
//    Z3= Z1*Z2*E
//
//    if Z2=1, then
//    A = X1          B = X2*Z1^2     C = Y1          D = Y2*Z1^3
//    E = B-X1        F = D-Y1
//    X3= -E^3 -2*X1*E^2 + F^2
//    Y3= -Y1*E^3 + F*(X1*E^2 -X3)
//    Z3= Z1*E
//
// cost: 3S+8M+7A
*/
void MB_FUNC_NAME(ifma_ec_nistp521_add_point_affine_)(P521_POINT* r, const P521_POINT* p, const P521_POINT_AFFINE* q)
{
   /* coordinates of p (projective) */
   const U64* X1 = p->X;
   const U64* Y1 = p->Y;
   const U64* Z1 = p->Z;
   __mb_mask p_at_infinity = MB_FUNC_NAME(is_zero_point_cordinate_)(p->Z);

   /* coordinates of q (affine) */
   const U64* X2 = q->x;
   const U64* Y2 = q->y;
   __mb_mask q_at_infinity = MB_FUNC_NAME(is_zero_point_cordinate_)(q->x)
                           & MB_FUNC_NAME(is_zero_point_cordinate_)(q->y);

   /* coordinates of temp point T(X3:Y3:Z3) */
   __ALIGN64 U64 X3[P521_LEN52];
   __ALIGN64 U64 Y3[P521_LEN52];
   __ALIGN64 U64 Z3[P521_LEN52];

   __ALIGN64 U64 U2[P521_LEN52];
   __ALIGN64 U64 S2[P521_LEN52];
   __ALIGN64 U64 H[P521_LEN52];
   __ALIGN64 U64 R[P521_LEN52];

   sqr(R, Z1);             // R = Z1^2
   mul(S2, Y2, Z1);        // S2 = Y2*Z1
   mul(U2, X2, R);         // U2 = X2*Z1^2
   mul(S2, S2, R);         // S2 = Y2*Z1^3

   sub(H, U2, X1);         // H = U2-X1
   sub(R, S2, Y1);         // R = S2-Y1

   mul(Z3, H, Z1);         // Z3 = H*Z1

   sqr(U2, H);             // U2 = H^2
   sqr(S2, R);             // S2 = R^2
   mul(H, H, U2);          // H = H^3

   mul(U2, U2, X1);        // U2 = X1*H^2

   mul(Y3, H, Y1);         // T = Y1*H^3

   mul2(X3, U2);           // X3 = 2*X1*H^2
   sub(X3, S2, X3);        // X3 = R^2 - 2*X1*H^2
   sub(X3, X3, H);         // X3 = R^2 - 2*X1*H^2 -H^3

   sub(U2, U2, X3);        // U2 = X1*H^2 - X3
   mul(U2, U2, R);         // U2 = R*(X1*H^2 - X3)
   sub(Y3, U2, Y3);      // Y3 = -Y1*H^3 + R*(X1*H^2 - X3)

   /* T = p_at_infinity? q : T */
   MB_FUNC_NAME(mask_mov_FE521_)(X3, X3, p_at_infinity, q->x);
   MB_FUNC_NAME(mask_mov_FE521_)(Y3, Y3, p_at_infinity, q->y);
   MB_FUNC_NAME(mask_mov_FE521_)(Z3, Z3, p_at_infinity, (U64*)p521_r_mb);
   /* T = q_at_infinity? p : T */
   MB_FUNC_NAME(mask_mov_FE521_)(X3, X3, q_at_infinity, p->X);
   MB_FUNC_NAME(mask_mov_FE521_)(Y3, Y3, q_at_infinity, p->Y);
   MB_FUNC_NAME(mask_mov_FE521_)(Z3, Z3, q_at_infinity, p->Z);

   /* r = T */
   MB_FUNC_NAME(mov_FE521_)(r->X, X3);
   MB_FUNC_NAME(mov_FE521_)(r->Y, Y3);
   MB_FUNC_NAME(mov_FE521_)(r->Z, Z3);
}

void MB_FUNC_NAME(get_nistp521_ec_affine_coords_)(U64 x[], U64 y[], const P521_POINT* P)
{
   __ALIGN64 U64 invZ1[P521_LEN52];
   __ALIGN64 U64 invZn[P521_LEN52];

   /* 1/Z and 1/Z^2 */
   MB_FUNC_NAME(ifma_aminv52_p521_)(invZ1, P->Z);
   MB_FUNC_NAME(ifma_ams52_p521_)(invZn, invZ1);

   /* if affine P.x requested */
   if(x)
      MB_FUNC_NAME(ifma_amm52_p521_)(x, P->X, invZn);

   /* if affine P.y requested */
   if(y) {
      MB_FUNC_NAME(ifma_amm52_p521_)(invZn, invZn, invZ1);
      MB_FUNC_NAME(ifma_amm52_p521_)(y, P->Y, invZn);
   }
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////


#define WIN_SIZE  (4)

/*
   s = (Ipp8u)(~((wvalue >> ws) - 1)); //sign
   d = (1 << (ws+1)) - wvalue - 1;     // digit, win size "ws"
   d = (d & s) | (wvaluen & ~s);
   d = (d >> 1) + (d & 1);
   *sign = s & 1;
   *digit = (Ipp8u)d;
*/
__INLINE void MB_FUNC_NAME(booth_recode_)(__mb_mask* sign, U64* dvalue, U64 wvalue)
{
   U64 one = set1(1);
   U64 zero = get_zero64();
   U64 t = srli64(wvalue, WIN_SIZE);
   __mb_mask s = cmp64_mask(t, zero, _MM_CMPINT_NE);
   U64 d = sub64( sub64(set1(1<<(WIN_SIZE+1)), wvalue), one);
   d = mask_mov64(wvalue, s, d);
   U64 odd = and64(d, one);
   d = add64( srli64(d, 1), odd);

   *sign = s;
   *dvalue = d;
}

/* extract point */
static void MB_FUNC_NAME(extract_point_)(P521_POINT* r, const P521_POINT tbl[], U64 idx)
{
   /* decrenent index (the table noes not contain [0]*P */
   U64 idx_target = sub64(idx, set1(1));

   /* assume the point at infinity is what need */
   P521_POINT R;
   MB_FUNC_NAME(set_point_to_infinity_)(&R);

   /* find out what we actually need or just keep original infinity */
   int32u n;
   for(n=0; n<(1<<(WIN_SIZE-1)); n++) {
      U64 idx_curr = set1(n);
      __mb_mask k = cmp64_mask(idx_curr, idx_target, _MM_CMPINT_EQ);

      /* R = k? tbl[] : R */
      MB_FUNC_NAME(secure_mask_mov_FE521_)(R.X, R.X, k, tbl[n].X);
      MB_FUNC_NAME(secure_mask_mov_FE521_)(R.Y, R.Y, k, tbl[n].Y);
      MB_FUNC_NAME(secure_mask_mov_FE521_)(R.Z, R.Z, k, tbl[n].Z);
   }
   MB_FUNC_NAME(mov_FE521_)(r->X, R.X);
   MB_FUNC_NAME(mov_FE521_)(r->Y, R.Y);
   MB_FUNC_NAME(mov_FE521_)(r->Z, R.Z);
}

void MB_FUNC_NAME(ifma_ec_nistp521_mul_point_)(P521_POINT* r, const P521_POINT* p, const U64 scalar[])
{
   /* pre-computed table */
   __ALIGN64 P521_POINT tbl[1<<(WIN_SIZE-1)];

   /*
   // compute tbl[] = [n]P, n=1,..,2^(WIN_SIZE-1):
   //
   // tbl[2*n] = tbl[2*n-1]+p
   // tbl[2*n+1] = [2]*tbl[n]
   */
   /* tbl[0] = p */
   MB_FUNC_NAME(mov_FE521_)(tbl[0].X, p->X);
   MB_FUNC_NAME(mov_FE521_)(tbl[0].Y, p->Y);
   MB_FUNC_NAME(mov_FE521_)(tbl[0].Z, p->Z);
   /* tbl[1] = [2]*p */
   MB_FUNC_NAME(ifma_ec_nistp521_dbl_point_)(&tbl[1], p);

   int n;
   for(n=1; n < (1<<(WIN_SIZE-1))/2; n++) {
      MB_FUNC_NAME(ifma_ec_nistp521_add_point_)(&tbl[2*n], &tbl[2*n-1], p);
      MB_FUNC_NAME(ifma_ec_nistp521_dbl_point_)(&tbl[2*n+1], &tbl[n]);
   }

   P521_POINT R;
   P521_POINT T;
   U64 Ty[P521_LEN52];

   /*
   // point (LR) multiplication
   */
   U64 idx_mask = set1( (1<<(WIN_SIZE+1))-1 );
   int bit = P521_BITSIZE-(P521_BITSIZE % WIN_SIZE);
   int chunk_no = (bit-1)/64;
   int chunk_shift = (bit-1)%64;

   /* first window */
   U64 wvalue = loadu64(&scalar[chunk_no]);
   wvalue = and64( srli64(wvalue, chunk_shift), idx_mask);

   U64  dvalue;
   __mb_mask dsign;
   MB_FUNC_NAME(booth_recode_)(&dsign, &dvalue, wvalue);
   MB_FUNC_NAME(extract_point_)(&R, tbl, dvalue);

   for(bit-=WIN_SIZE; bit>=WIN_SIZE; bit-=WIN_SIZE) {
      /* doubling */
      MB_FUNC_NAME(ifma_ec_nistp521_dbl_point_)(&R, &R);
      MB_FUNC_NAME(ifma_ec_nistp521_dbl_point_)(&R, &R);
      MB_FUNC_NAME(ifma_ec_nistp521_dbl_point_)(&R, &R);
      MB_FUNC_NAME(ifma_ec_nistp521_dbl_point_)(&R, &R);
      #if (WIN_SIZE==5)
      MB_FUNC_NAME(ifma_ec_nistp521_dbl_point_)(&R, &R);
      #endif

      /* extract precomputed []P */
      chunk_no = (bit-1)/64;
      chunk_shift = (bit-1)%64;

      wvalue = loadu64(&scalar[chunk_no]);
      #if (_MSC_VER <= 1916) /* VS 2017 not supported _mm512_shrdv_epi64 */
      {
      __m512i t_lo_ = _mm512_srlv_epi64(wvalue, set64(chunk_shift));
      __m512i t_hi_ = _mm512_sllv_epi64(loadu64(&scalar[chunk_no+1]), set64(64-chunk_shift));
      wvalue = or64(t_lo_, t_hi_);
      }
      #else
      wvalue = _mm512_shrdv_epi64(wvalue, loadu64(&scalar[chunk_no+1]), set1((int32u)chunk_shift));
      #endif
      wvalue = and64(wvalue, idx_mask);

      MB_FUNC_NAME(booth_recode_)(&dsign, &dvalue, wvalue);
      MB_FUNC_NAME(extract_point_)(&T, tbl, dvalue);

      /* T = dsign? -T : T */
      MB_FUNC_NAME(ifma_neg52_p521_)(Ty, T.Y);
      MB_FUNC_NAME(secure_mask_mov_FE521_)(T.Y, T.Y, dsign, Ty);

      /* acumulate T */
      MB_FUNC_NAME(ifma_ec_nistp521_add_point_)(&R, &R, &T);
   }

   /* last window */
   MB_FUNC_NAME(ifma_ec_nistp521_dbl_point_)(&R, &R);
   MB_FUNC_NAME(ifma_ec_nistp521_dbl_point_)(&R, &R);
   MB_FUNC_NAME(ifma_ec_nistp521_dbl_point_)(&R, &R);
   MB_FUNC_NAME(ifma_ec_nistp521_dbl_point_)(&R, &R);
   #if (WIN_SIZE==5)
   MB_FUNC_NAME(ifma_ec_nistp521_dbl_point_)(&R, &R);
   #endif

   wvalue = loadu64(&scalar[0]);
   wvalue = and64( slli64(wvalue, 1), idx_mask);
   MB_FUNC_NAME(booth_recode_)(&dsign, &dvalue, wvalue);
   MB_FUNC_NAME(extract_point_)(&T, tbl, dvalue);

   MB_FUNC_NAME(ifma_neg52_p521_)(Ty, T.Y);
   MB_FUNC_NAME(secure_mask_mov_FE521_)(T.Y, T.Y, dsign, Ty);

   MB_FUNC_NAME(ifma_ec_nistp521_add_point_)(&R, &R, &T);

   /* r = R */
   MB_FUNC_NAME(mov_FE521_)(r->X, R.X);
   MB_FUNC_NAME(mov_FE521_)(r->Y, R.Y);
   MB_FUNC_NAME(mov_FE521_)(r->Z, R.Z);

   /* clear r (to fix potential secutity flaw in case of ecdh */
   MB_FUNC_NAME(zero_)((int64u (*)[8])&R, sizeof(R)/sizeof(U64));

   /* clear stubs of secret scalar */
   wvalue = _mm512_setzero_si512();
   dvalue = _mm512_setzero_si512();
   dsign = 0;
}
#undef WIN_SIZE


//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

#include <internal/ecnist/ifma_ecprecomp4_p521.h>

#define BP_WIN_SIZE  MUL_BASEPOINT_WIN_SIZE  /* defined in the header above */

__INLINE void MB_FUNC_NAME(booth_recode_bp_)(__mb_mask* sign, U64* dvalue, U64 wvalue)
{
   U64 one = set1(1);
   U64 zero = get_zero64();
   U64 t = srli64(wvalue, BP_WIN_SIZE);
   __mb_mask s = cmp64_mask(t, zero, _MM_CMPINT_NE);
   U64 d = sub64( sub64(set1(1<<(BP_WIN_SIZE+1)), wvalue), one);
   d = mask_mov64(wvalue, s, d);
   U64 odd = and64(d, one);
   d = add64( srli64(d, 1), odd);

   *sign = s;
   *dvalue = d;
}

/* extract affine affine point */
__INLINE void MB_FUNC_NAME(extract_point_affine_)(P521_POINT_AFFINE* r, const SINGLE_P521_POINT_AFFINE* tbl, U64 idx)
{
   /* decrement index (the table noes not contain [0]*P */
   U64 targIdx = sub64(idx, set1(1));

   U64 ax0, ax1, ax2, ax3, ax4, ax5, ax6, ax7, ax8, ax9, ax10;
   U64 ay0, ay1, ay2, ay3, ay4, ay5, ay6, ay7, ay8, ay9, ay10;

   /* assume the point at infinity is what need */
   ax0 = ax1 = ax2 = ax3 = ax4 = ax5 = ax6 = ax7 = ax8 = ax9 = ax10 =
   ay0 = ay1 = ay2 = ay3 = ay4 = ay5 = ay6 = ay7 = ay8 = ay9 = ay10 = get_zero64();

   /* find out what we actually need or just keep original infinity */
   int n;
   U64 currIdx = get_zero64();
   for(n=0; n<(1<<(BP_WIN_SIZE-1)); n++, tbl++, currIdx = add64(currIdx, set1(1))) {
      __mb_mask k = cmp64_mask(currIdx, targIdx, _MM_CMPINT_EQ);

      /* R = k? set1( tbl[] ) : R */
      ax0 = mask_add64(ax0, k, ax0, set1( tbl->x[0] ));
      ax1 = mask_add64(ax1, k, ax1, set1( tbl->x[1] ));
      ax2 = mask_add64(ax2, k, ax2, set1( tbl->x[2] ));
      ax3 = mask_add64(ax3, k, ax3, set1( tbl->x[3] ));
      ax4 = mask_add64(ax4, k, ax4, set1( tbl->x[4] ));
      ax5 = mask_add64(ax5, k, ax5, set1( tbl->x[5] ));
      ax6 = mask_add64(ax6, k, ax6, set1( tbl->x[6] ));
      ax7 = mask_add64(ax7, k, ax7, set1( tbl->x[7] ));
      ax8 = mask_add64(ax8, k, ax8, set1( tbl->x[8] ));
      ax9 = mask_add64(ax9, k, ax9, set1( tbl->x[9] ));
      ax10= mask_add64(ax10,k, ax10,set1( tbl->x[10]));

      ay0 = mask_add64(ay0, k, ay0, set1( tbl->y[0] ));
      ay1 = mask_add64(ay1, k, ay1, set1( tbl->y[1] ));
      ay2 = mask_add64(ay2, k, ay2, set1( tbl->y[2] ));
      ay3 = mask_add64(ay3, k, ay3, set1( tbl->y[3] ));
      ay4 = mask_add64(ay4, k, ay4, set1( tbl->y[4] ));
      ay5 = mask_add64(ay5, k, ay5, set1( tbl->y[5] ));
      ay6 = mask_add64(ay6, k, ay6, set1( tbl->y[6] ));
      ay7 = mask_add64(ay7, k, ay7, set1( tbl->y[7] ));
      ay8 = mask_add64(ay8, k, ay8, set1( tbl->y[8] ));
      ay9 = mask_add64(ay9, k, ay9, set1( tbl->y[9] ));
      ay10= mask_add64(ay10,k, ay10,set1( tbl->y[10]));
   }

   r->x[0] = ax0;
   r->x[1] = ax1;
   r->x[2] = ax2;
   r->x[3] = ax3;
   r->x[4] = ax4;
   r->x[5] = ax5;
   r->x[6] = ax6;
   r->x[7] = ax7;
   r->x[8] = ax8;
   r->x[9] = ax9;
   r->x[10]= ax10;

   r->y[0] = ay0;
   r->y[1] = ay1;
   r->y[2] = ay2;
   r->y[3] = ay3;
   r->y[4] = ay4;
   r->y[5] = ay5;
   r->y[6] = ay6;
   r->y[7] = ay7;
   r->y[8] = ay8;
   r->y[9] = ay9;
   r->y[10]= ay10;
}

void MB_FUNC_NAME(ifma_ec_nistp521_mul_pointbase_)(P521_POINT* r, const U64 scalar[])
{
   /* pre-computed table of base powers */
   SINGLE_P521_POINT_AFFINE* tbl = &ifma_ec_nistp521_bp_precomp[0][0];

   P521_POINT R;
   P521_POINT_AFFINE A;
   U64 Ty[P521_LEN52];

   /* R = O */
   MB_FUNC_NAME(set_point_to_infinity_)(&R);


   /*
   // base point (RL) multiplication
   */
   U64  wvalue, dvalue;
   __mb_mask dsign;

   U64 idx_mask = set1( (1<<(BP_WIN_SIZE+1))-1 );
   int bit = 0;

   /* first window - window[0] */
   wvalue = loadu64(&scalar[0]);
   wvalue = and64( slli64(wvalue, 1), idx_mask);
   MB_FUNC_NAME(booth_recode_bp_)(&dsign, &dvalue, wvalue);
   MB_FUNC_NAME(extract_point_affine_)(&A, tbl, dvalue);
   tbl+=BP_N_ENTRY;

   /* A = dsign? -A : A */
   MB_FUNC_NAME(ifma_neg52_p521_)(Ty, A.y);
   MB_FUNC_NAME(secure_mask_mov_FE521_)(A.y, A.y, dsign, Ty);

   /* R += A */
   MB_FUNC_NAME(ifma_ec_nistp521_add_point_affine_)(&R, &R, &A);

   int chunk_no;
   int chunk_shift;
   for(bit+=BP_WIN_SIZE; bit<=P521_BITSIZE; bit+=BP_WIN_SIZE) {
      chunk_no = (bit-1)/64;
      chunk_shift = (bit-1)%64;

      wvalue = loadu64(&scalar[chunk_no]);
      #if (_MSC_VER <= 1916) /* VS 2017 not supported _mm512_shrdv_epi64 */
      {
      __m512i t_lo_ = _mm512_srlv_epi64(wvalue, set64(chunk_shift));
      __m512i t_hi_ = _mm512_sllv_epi64(loadu64(&scalar[chunk_no+1]), set64(64-chunk_shift));
      wvalue = or64(t_lo_, t_hi_);
      }
      #else
      wvalue = _mm512_shrdv_epi64(wvalue, loadu64(&scalar[chunk_no+1]), set1((int32u)chunk_shift));
      #endif
      wvalue = and64(wvalue, idx_mask);

      MB_FUNC_NAME(booth_recode_bp_)(&dsign, &dvalue, wvalue);
      MB_FUNC_NAME(extract_point_affine_)(&A, tbl, dvalue);
      tbl+=BP_N_ENTRY;

      /* A = dsign? -A : A */
      MB_FUNC_NAME(ifma_neg52_p521_)(Ty, A.y);
      MB_FUNC_NAME(secure_mask_mov_FE521_)(A.y, A.y, dsign, Ty);

      /* R += A */
      MB_FUNC_NAME(ifma_ec_nistp521_add_point_affine_)(&R, &R, &A);
   }

   /* r = R */
   MB_FUNC_NAME(mov_FE521_)(r->X, R.X);
   MB_FUNC_NAME(mov_FE521_)(r->Y, R.Y);
   MB_FUNC_NAME(mov_FE521_)(r->Z, R.Z);

   /* clear stubs of secret scalar */
   wvalue = _mm512_setzero_si512();
   dvalue = _mm512_setzero_si512();
   dsign = 0;
}
#undef BP_WIN_SIZE


/* P521 parameters: mont(a), mont(b) */
__ALIGN64 int64u mont_a_p521_mb[P521_LEN52][8] = {
   { REP8_DECL(0x0007ffffffffffff) },
   { REP8_DECL(0x000ffffffffffffe) },
   { REP8_DECL(0x000fffffffffffff) },
   { REP8_DECL(0x000fffffffffffff) },
   { REP8_DECL(0x000fffffffffffff) },
   { REP8_DECL(0x000fffffffffffff) },
   { REP8_DECL(0x000fffffffffffff) },
   { REP8_DECL(0x000fffffffffffff) },
   { REP8_DECL(0x000fffffffffffff) },
   { REP8_DECL(0x000fffffffffffff) },
   { REP8_DECL(0x0000000000000001) }
};
__ALIGN64 int64u mont_b_p521_mb[P521_LEN52][8] = {
   { REP8_DECL(0x00014654fae58638) },
   { REP8_DECL(0x00028fea35a81f80) },
   { REP8_DECL(0x000c41e961a78f7a) },
   { REP8_DECL(0x000dd8df839ab9ef) },
   { REP8_DECL(0x00049bd8b29605e9) },
   { REP8_DECL(0x0000ab0c9ca8f63f) },
   { REP8_DECL(0x0005a44c8c77884f) },
   { REP8_DECL(0x00092dccd98af9dc) },
   { REP8_DECL(0x0005b42a077516d3) },
   { REP8_DECL(0x000e4d0fc94d10d0) },
   { REP8_DECL(0x0000000000000000) }
};

/*
// We have a curve defined by a Weierstrass equation: y^2 = x^3 + a*x + b.
//
// The points are considered in Jacobian projective coordinates
// where  (X, Y, Z)  represents  (x, y) = (X/Z^2, Y/Z^3).
// Substituting this and multiplying by  Z^6  transforms the above equation into
//      Y^2 = X^3 + a*X*Z^4 + b*Z^6
// To test this, we add up the right-hand side in 'rh'.
*/
__mb_mask MB_FUNC_NAME(ifma_is_on_curve_p521_)(const P521_POINT* p, int use_jproj_coords)
{
   U64 rh[P521_LEN52];
   U64 Z4[P521_LEN52], Z6[P521_LEN52], tmp[P521_LEN52];

   /* rh := X^2 */
   MB_FUNC_NAME(ifma_ams52_p521_)(rh, p->X);

   /* if Z!=1, then rh = X^3 + a*X*Z^4 + b*Z^6  = X*(X^2 + a*X*Z^4) + b*Z^6 */
   if(use_jproj_coords) {
      MB_FUNC_NAME(ifma_ams52_p521_)(tmp, p->Z);      /* tmp = Z^2 */
      MB_FUNC_NAME(ifma_ams52_p521_)(Z4, tmp);        /* Z4  = Z^4 */
      MB_FUNC_NAME(ifma_amm52_p521_)(Z6, Z4, tmp);    /* Z6  = Z^6 */

      MB_FUNC_NAME(ifma_add52_p521_)(tmp, Z4, Z4);    /* tmp = 2*Z^4 */
      MB_FUNC_NAME(ifma_add52_p521_)(tmp, tmp, Z4);   /* tmp = 2*Z^4 */
      MB_FUNC_NAME(ifma_sub52_p521_)(rh, rh, tmp);    /* rh = X^2 + a*Z^4 */
      MB_FUNC_NAME(ifma_amm52_p521_)(rh, rh, p->X);   /* rh = (X^2 + a*Z^4)*X */

      MB_FUNC_NAME(ifma_amm52_p521_)(tmp, Z6, (U64*)mont_b_p521_mb);
      MB_FUNC_NAME(ifma_add52_p521_)(rh, rh, tmp);    /* rh = (X^2 + a*Z^4)*X + b*Z^6 */
   }
   /* if Z==1, then rh = X^3 + a*X + b = X*(X^2 +a) b */
   else {
      MB_FUNC_NAME(ifma_add52_p521_)(rh, rh, (U64*)mont_a_p521_mb); /* rh = X^2+a */
      MB_FUNC_NAME(ifma_amm52_p521_)(rh, rh, p->X);                 /* rh = (X^2+a)*X */
      MB_FUNC_NAME(ifma_add52_p521_)(rh, rh, (U64*)mont_b_p521_mb); /* rh = (X^2+a)*X + b */
   }
   MB_FUNC_NAME(ifma_frommont52_p521_)(rh, rh);

   /* rl = tmp = Y^2 */
   MB_FUNC_NAME(ifma_ams52_p521_)(tmp, p->Y);
   MB_FUNC_NAME(ifma_frommont52_p521_)(tmp, tmp);

   /* mask = rl==rh */
   __mb_mask is_on_curve_mask = MB_FUNC_NAME(cmp_eq_FE521_)(tmp, rh);

   return is_on_curve_mask;
}
