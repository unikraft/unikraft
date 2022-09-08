/*******************************************************************************
* Copyright 2019-2021 Intel Corporation
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

#include <internal/rsa/ifma_div_104_by_52.h>


static ALIGNSPEC const UINT64 __mask52[] = { DUP8_DECL(0x00ffffffffffffffull) };
static ALIGNSPEC const UINT64 __one[] = { DUP8_DECL(0x3ff0000000000000ull) };
static ALIGNSPEC const UINT64 __two52[] = { DUP8_DECL(0x4330000000000000ull) };
//static ALIGNSPEC const UINT64 __zero[] = { DUP8_DECL(0x0000000000000000ull) };
static ALIGNSPEC const UINT64 __one_l[] = { DUP8_DECL(0x0000000000000001ull) };



// each 64-bit SIMD element should be in the [0, 2^52) range
// perform division of (Ah[k]*2^52 + Al[k])/B[k], k=0..7
// Return quotients (as well as remainders via pointer)
// Note:  There is no checking for B==0
VUINT64 __div_104_by_52(VUINT64 Ah, VUINT64 Al, VUINT64 B, VUINT64* prem)
{
	VUINT64 msk52, Rem, one;
	VUINT64 sgn_mask, B_corr, Qh, Ql;
	VDOUBLE db, dah, dal, dbr, d_one;
	VDOUBLE dqh, dql, two52, dqh2;
	VMASK_L corr_gt_mask;

    // limit range of Ah, Al, B inputs to 52 bits (optional?)
	msk52 = VLOAD_L(__mask52);
	B  = VAND_L(B, msk52);
	Ah = VAND_L(Ah, msk52);
	Al = VAND_L(Al, msk52);

	// convert inputs to double precision
	// conversion will be exact
	db  = VCVT_L2D(B);
	dah = VCVT_L2D(Ah);
	dal = VCVT_L2D(Al);

	d_one = VLOAD_D(__one);
	// get reciprocal of B, RZ mode
	dbr = VDIV_RU_D(d_one, db);

	// estimate quotient Qh = (dah*dbr)_RZ ~ Ah/B
	dqh = VMUL_RZ_D(dah, dbr);
	// truncate dqh to integral
	dqh = VROUND_RZ_D(dqh);
	// get remainder term:  dah_new -= dqh*dbh
	// 0 <= dah_new <= B  (due to rounding direction and rounding error magnitude in dbr, dqh)
	dah = VQFMR_D(db, dqh, dah);

	// estimate quotient Ql = (dal*dbr)_RZ ~ Al/B
	dql = VMUL_RZ_D(dal, dbr);
	// truncate dqh to integral
	dql = VROUND_RZ_D(dql);
	// get remainder term:  dal_new -= dql*dbh
	// 0 <= dal_new <= B  (similar to above)
	dal = VQFMR_D(db, dql, dal);
	// convert remainder term back to (signed) 64-bit integer
	Al = VCVT_D2L(dal);

	// scale dah by 2^52
	two52 = VLOAD_D(__two52);
	dah = VMUL_D(dah, two52);
	// estimate quotient Qh2 = ((2^52)*dah)_RU/db
	dqh2 = VMUL_RZ_D(dah, dbr);
	// truncate dqh2 to integral
	dqh2 = VROUND_RZ_D(dqh2);
	// remainder term:  dah_new2 = dah - db*dqh2
	// dah_new2 in (-B, 2B)
	dah = VQFMR_D(db, dqh2, dah);
	// convert remainder term back to (signed) 64-bit integer
	Ah = VCVT_D2L(dah);

	// add and convert low quotients: Qh2+Ql
	dql = VADD_D(dql, dqh2);
	Ql = VCVT_D2L(dql);
	// convert high quotient
	Qh = VCVT_D2L(dqh);

	// remainder term in (-B, 3B)
	Rem = VADD_L(Ah, Al);

	// Rem < 0?
	sgn_mask = VSAR_L(Rem, 63);
	// Rem > B?
	corr_gt_mask = VCMP_GE_L(Rem, B);
	B_corr = VAND_L(B, sgn_mask);
	// apply correction if B<0
	Rem = VADD_L(Rem, B_corr);
	Ql = VADD_L(Ql, sgn_mask);

	// if Rem> B, apply correction
	Rem = VMSUB_L(corr_gt_mask, Rem, B);
	one = VLOAD_L(__one_l);
	Ql = VMADD_L(corr_gt_mask, Ql, one);

	// Rem > B?
	corr_gt_mask = VCMP_GE_L(Rem, B);
	// if Rem> B, apply correction
	// this is the final remainder
	*prem = VMSUB_L(corr_gt_mask, Rem, B);
	Ql = VMADD_L(corr_gt_mask, Ql, one);

	// now add the high part of the quotient
	Qh = VSHL_L(Qh, 52);
	Ql = VADD_L(Ql, Qh);

	return Ql;

}
