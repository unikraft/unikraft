#ifndef _LIBCPP_SUPPORT_SGX_SUPPORT_H
#define _LIBCPP_SUPPORT_SGX_SUPPORT_H

extern "C" {

size_t mbsnrtowcs(wchar_t *dst, const char **src,
                  size_t nmc, size_t len, mbstate_t *ps);
size_t wcsnrtombs(char *dst, const wchar_t **src,
                  size_t nwc, size_t len, mbstate_t *ps);
}

#endif
