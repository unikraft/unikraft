#include <wctype.h>

#undef iswdigit

int iswdigit(wint_t wc)
{
	return (unsigned)wc-'0' < 10;
}

#ifndef _TLIBC_GNU_
int __iswdigit_l(wint_t c, locale_t l)
{
	return iswdigit(c);
}

weak_alias(__iswdigit_l, iswdigit_l);
#endif
