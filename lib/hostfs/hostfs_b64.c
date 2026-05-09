/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors. */

/*
 * Minimal base64 encoder/decoder (RFC 4648 standard alphabet, padded).
 */

#include "hostfs.h"

static const char b64_enc[] =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

size_t hostfs_b64_encoded_len(size_t n)
{
	return ((n + 2) / 3) * 4;
}

size_t hostfs_b64_decoded_cap(size_t n)
{
	return (n / 4) * 3;
}

size_t hostfs_b64_encode(const void *src_, size_t n, char *dst)
{
	const unsigned char *src = src_;
	size_t i, o = 0;

	for (i = 0; i + 3 <= n; i += 3) {
		unsigned v = ((unsigned)src[i] << 16) |
			     ((unsigned)src[i + 1] << 8) |
			     (unsigned)src[i + 2];
		dst[o++] = b64_enc[(v >> 18) & 0x3F];
		dst[o++] = b64_enc[(v >> 12) & 0x3F];
		dst[o++] = b64_enc[(v >> 6) & 0x3F];
		dst[o++] = b64_enc[v & 0x3F];
	}
	if (i < n) {
		unsigned v = (unsigned)src[i] << 16;
		if (i + 1 < n)
			v |= (unsigned)src[i + 1] << 8;
		dst[o++] = b64_enc[(v >> 18) & 0x3F];
		dst[o++] = b64_enc[(v >> 12) & 0x3F];
		dst[o++] = (i + 1 < n) ? b64_enc[(v >> 6) & 0x3F] : '=';
		dst[o++] = '=';
	}
	return o;
}

static int b64_val(char c)
{
	if (c >= 'A' && c <= 'Z') return c - 'A';
	if (c >= 'a' && c <= 'z') return c - 'a' + 26;
	if (c >= '0' && c <= '9') return c - '0' + 52;
	if (c == '+') return 62;
	if (c == '/') return 63;
	return -1;
}

long hostfs_b64_decode(const char *src, size_t n, void *dst_, size_t cap)
{
	unsigned char *dst = dst_;
	size_t i = 0, o = 0;
	int v[4];
	int k;

	/* Trim trailing whitespace/newlines. */
	while (n > 0 && (src[n - 1] == '\n' || src[n - 1] == '\r'
			 || src[n - 1] == ' ' || src[n - 1] == '\t'))
		n--;

	while (i + 4 <= n) {
		for (k = 0; k < 4; k++) {
			char c = src[i + k];
			if (c == '=')
				v[k] = -2;
			else {
				v[k] = b64_val(c);
				if (v[k] < 0)
					return -1;
			}
		}
		i += 4;
		unsigned bits = ((unsigned)(v[0] < 0 ? 0 : v[0]) << 18) |
				((unsigned)(v[1] < 0 ? 0 : v[1]) << 12) |
				((unsigned)(v[2] < 0 ? 0 : v[2]) << 6)  |
				(unsigned)(v[3] < 0 ? 0 : v[3]);
		if (o >= cap) return -1;
		dst[o++] = (bits >> 16) & 0xFF;
		if (v[2] != -2) {
			if (o >= cap) return -1;
			dst[o++] = (bits >> 8) & 0xFF;
		}
		if (v[3] != -2) {
			if (o >= cap) return -1;
			dst[o++] = bits & 0xFF;
		}
	}
	if (i != n) return -1;
	return (long)o;
}
