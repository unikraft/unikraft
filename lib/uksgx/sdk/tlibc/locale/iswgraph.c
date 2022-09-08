#include <wctype.h>

int iswgraph(wint_t wc)
{
	/* ISO C defines this function as: */
	return !iswspace(wc) && iswprint(wc);
}

#ifndef _TLIBC_GNU_
int __iswgraph_l(wint_t c, locale_t l)
{
	return iswgraph(c);
}

weak_alias(__iswgraph_l, iswgraph_l);
#endif
