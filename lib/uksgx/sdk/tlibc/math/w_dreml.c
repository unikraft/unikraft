/*
 * dreml() wrapper for remainderl().
 * 
 * Written by Qingchao, Qiu, <qingchao.qiu@intel.com>
 * Placed into the Public Domain, 2012.
 */

#include <math.h>
#include <float.h>

#ifdef _TLIBC_GNU_

#if (LDBL_MANT_DIG > DBL_MANT_DIG)

long double dreml(long double x, long double y)
{
	return remainderl(x, y);
}

#endif
#endif
