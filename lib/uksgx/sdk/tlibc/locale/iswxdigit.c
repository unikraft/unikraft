#include <wctype.h>

int iswxdigit(wint_t wc)
{
	return (unsigned)(wc-'0') < 10 || (unsigned)((wc|32)-'a') < 6;
}

#ifndef _TLIBC_GNU_
int __iswxdigit_l(wint_t c, locale_t l)
{
	return iswxdigit(c);
}

weak_alias(__iswxdigit_l, iswxdigit_l);
#endif
