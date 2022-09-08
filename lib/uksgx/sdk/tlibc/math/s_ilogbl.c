/*	$OpenBSD: s_ilogbl.c,v 1.2 2016/09/12 19:47:02 guenther Exp $	*/
/*
 * From: @(#)s_ilogb.c 5.1 93/09/24
 * ====================================================
 * Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
 *
 * Developed at SunPro, a Sun Microsystems, Inc. business.
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice
 * is preserved.
 * ====================================================
 */

#include <sys/types.h>
#include <sys/ieee.h>
#include <limits.h>
#include <math.h>

#ifdef _TLIBC_GNU_

int
ilogbl(long double x)
{
	struct ieee_ext *p = (struct ieee_ext *)&x;
	unsigned long m;
	int b;

	if (p->ext_exp == 0) {
		if ((p->ext_fracl
#ifdef EXT_FRACLMBITS
			| p->ext_fraclm
#endif /* EXT_FRACLMBITS */
#ifdef EXT_FRACHMBITS
			| p->ext_frachm
#endif /* EXT_FRACHMBITS */
			| p->ext_frach) == 0)
			return (FP_ILOGB0);
		/* denormalized */
		if (p->ext_frach == 0
#ifdef EXT_FRACHMBITS
			&& p->ext_frachm == 0
#endif
			) {
			m = 1lu << (EXT_FRACLBITS - 1);
			for (b = EXT_FRACHBITS; !(p->ext_fracl & m); m >>= 1)
				b++;
#if defined(EXT_FRACHMBITS) && defined(EXT_FRACLMBITS)
			m = 1lu << (EXT_FRACLMBITS - 1);
			for (b += EXT_FRACHMBITS; !(p->ext_fraclm & m); m >>= 1)
				b++;
#endif /* defined(EXT_FRACHMBITS) && defined(EXT_FRACLMBITS) */
		} else {
			m = 1lu << (EXT_FRACHBITS - 1);
			for (b = 0; !(p->ext_frach & m); m >>= 1)
				b++;
#ifdef EXT_FRACHMBITS
			m = 1lu << (EXT_FRACHMBITS - 1);
			for (; !(p->ext_frachm & m); m >>= 1)
				b++;
#endif /* EXT_FRACHMBITS */
		}
#ifdef EXT_IMPLICIT_NBIT
		b++;
#endif
		return (LDBL_MIN_EXP - b - 1);
	} else if (p->ext_exp < (LDBL_MAX_EXP << 1) - 1)
		return (p->ext_exp - LDBL_MAX_EXP + 1);
	else if (p->ext_fracl != 0
#ifdef EXT_FRACLMBITS
		|| p->ext_fraclm != 0
#endif /* EXT_FRACLMBITS */
#ifdef EXT_FRACHMBITS
		|| p->ext_frachm != 0
#endif /* EXT_FRACHMBITS */
		|| p->ext_frach != 0)
		return (FP_ILOGBNAN);
	else
		return (INT_MAX);
}
#endif
