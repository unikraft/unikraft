/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025, Unikraft GmbH and The Unikraft Authors. */

/*
 * JSON request builders + response parsers for hostfs. The wire is the
 * same {"name": "…", "args": {…}} __dispatch shape the /dev/hcall device
 * uses; we just talk to the Rust-side FsSandbox tools (fs_read_bytes,
 * fs_write_bytes, fs_stat, fs_list, fs_mkdir, fs_unlink, fs_truncate)
 * directly rather than going through /dev/hcall.
 */

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <uk/arch/types.h>
#include <uk/print.h>

#include <hyperlight-x86/hcall.h>

#include "hostfs.h"

/* Maximum single RPC payload (matches HCALL_MAX_PAYLOAD in plat/hyperlight). */
#define HOSTFS_RPC_MAX 65536

/* Static per-call buffers. hostfs is not thread-safe; callers serialise
 * via vfscore's locking. */
static char rpc_req_buf[HOSTFS_RPC_MAX];
static char rpc_resp_buf[HOSTFS_RPC_MAX];

/* -------- tiny JSON helpers -------- */

/* JSON-escape the string `s` into `out`, surrounded by quotes, up to
 * `cap - 1` bytes + NUL. Returns the number of bytes written, or -1 on
 * overflow. Handles ", \, \n, \t, \r only — enough for file paths.
 */
static int json_escape(const char *s, char *out, size_t cap)
{
	size_t o = 0;
	if (o >= cap) return -1;
	out[o++] = '"';
	for (; *s; s++) {
		const char *esc = NULL;
		switch (*s) {
		case '"':  esc = "\\\""; break;
		case '\\': esc = "\\\\"; break;
		case '\n': esc = "\\n";  break;
		case '\r': esc = "\\r";  break;
		case '\t': esc = "\\t";  break;
		default:   break;
		}
		if (esc) {
			size_t n = strlen(esc);
			if (o + n >= cap) return -1;
			memcpy(out + o, esc, n);
			o += n;
		} else {
			if (o + 1 >= cap) return -1;
			out[o++] = *s;
		}
	}
	if (o + 2 > cap) return -1;
	out[o++] = '"';
	out[o] = '\0';
	return (int)o;
}

/* Scan `json` for `"key"` (at an object-key position — approximate; we
 * just look for the literal `"key":`). Returns a pointer to the byte
 * after the colon (+ any whitespace), or NULL if not found.
 */
static const char *json_scan_key(const char *json, const char *key)
{
	char needle[64];
	int n = snprintf(needle, sizeof(needle), "\"%s\"", key);
	if (n < 0 || (size_t)n >= sizeof(needle))
		return NULL;
	const char *p = strstr(json, needle);
	if (!p)
		return NULL;
	p += n;
	while (*p == ' ' || *p == '\t' || *p == ':')
		p++;
	return p;
}

/* Return 1 if `json` has an "error" field. */
static int json_has_error(const char *json)
{
	return strstr(json, "\"error\"") != NULL;
}

/* Map a host error string to -errno (best-effort). */
static int json_errno(const char *json)
{
	const char *p = json_scan_key(json, "error");
	if (!p || *p != '"')
		return -EIO;
	p++;
	if (strstr(p, "No such file") || strstr(p, "not found")
	    || strstr(p, "NotFound"))
		return -ENOENT;
	if (strstr(p, "escape"))
		return -ENOENT;  /* map sandbox refusal to ENOENT for POSIX-path callers */
	if (strstr(p, "exists") || strstr(p, "AlreadyExists"))
		return -EEXIST;
	if (strstr(p, "permission") || strstr(p, "Permission")
	    || strstr(p, "denied"))
		return -EACCES;
	if (strstr(p, "Is a directory"))
		return -EISDIR;
	if (strstr(p, "Not a directory"))
		return -ENOTDIR;
	if (strstr(p, "not empty"))
		return -ENOTEMPTY;
	return -EIO;
}

/* Extract a quoted string value. Writes up to `cap - 1` bytes + NUL.
 * Returns bytes written, or -1 on error. Decodes \n \t \r \" \\.
 */
static int json_get_string(const char *json, const char *key,
			   char *out, size_t cap)
{
	const char *p = json_scan_key(json, key);
	if (!p || *p != '"')
		return -1;
	p++;
	size_t o = 0;
	while (*p && *p != '"') {
		char c;
		if (*p == '\\' && p[1]) {
			switch (p[1]) {
			case 'n': c = '\n'; break;
			case 'r': c = '\r'; break;
			case 't': c = '\t'; break;
			case '"': c = '"';  break;
			case '\\': c = '\\'; break;
			default:  c = p[1]; break;
			}
			p += 2;
		} else {
			c = *p++;
		}
		if (o + 1 >= cap)
			return -1;
		out[o++] = c;
	}
	out[o] = '\0';
	return (int)o;
}

/* Extract a JSON integer value. Returns 0 on success, -1 otherwise. */
static int json_get_int(const char *json, const char *key, long long *out)
{
	const char *p = json_scan_key(json, key);
	if (!p)
		return -1;
	char *end;
	long long v = strtoll(p, &end, 10);
	if (end == p)
		return -1;
	*out = v;
	return 0;
}

/* Extract a JSON boolean value. Returns 1 for true, 0 for false, -1 on error. */
static int json_get_bool(const char *json, const char *key)
{
	const char *p = json_scan_key(json, key);
	if (!p)
		return -1;
	if (!strncmp(p, "true", 4))
		return 1;
	if (!strncmp(p, "false", 5))
		return 0;
	return -1;
}

/* -------- one RPC round-trip -------- */

static int rpc_exchange(size_t req_len, size_t *resp_len)
{
	__sz rlen = 0;
	int rc = hyperlight_hcall((const __u8 *)rpc_req_buf, req_len,
				  (__u8 *)rpc_resp_buf,
				  sizeof(rpc_resp_buf) - 1, &rlen);
	if (rc < 0) {
		uk_pr_err("hostfs: hyperlight_hcall failed: %d\n", rc);
		return -EIO;
	}
	rpc_resp_buf[rlen] = '\0';
	if (resp_len)
		*resp_len = rlen;
	if (json_has_error(rpc_resp_buf)) {
		int err = json_errno(rpc_resp_buf);
		uk_pr_debug("hostfs: host returned error: %s -> %d\n",
			    rpc_resp_buf, err);
		return err;
	}
	return 0;
}

/* Like snprintf but returns a negative error on overflow. */
static int build_req(const char *fmt, ...)
{
	va_list ap;
	int n;
	va_start(ap, fmt);
	n = vsnprintf(rpc_req_buf, sizeof(rpc_req_buf), fmt, ap);
	va_end(ap);
	if (n < 0 || (size_t)n >= sizeof(rpc_req_buf))
		return -ENAMETOOLONG;
	return n;
}

/* -------- high-level RPCs -------- */

int hostfs_rpc_stat(const char *path, struct hostfs_stat *out)
{
	char pbuf[HOSTFS_MAX_PATH + 32];
	if (json_escape(path, pbuf, sizeof(pbuf)) < 0)
		return -ENAMETOOLONG;
	int n = build_req(
		"{\"name\":\"fs_stat\",\"args\":{\"path\":%s}}", pbuf);
	if (n < 0) return n;
	int rc = rpc_exchange(n, NULL);
	if (rc < 0) return rc;
	long long sz = 0;
	if (json_get_int(rpc_resp_buf, "size", &sz) < 0)
		return -EIO;
	int id = json_get_bool(rpc_resp_buf, "is_dir");
	int isf = json_get_bool(rpc_resp_buf, "is_file");
	out->size = (uint64_t)sz;
	out->is_dir = (id == 1);
	out->is_file = (isf == 1);
	return 0;
}

int hostfs_rpc_read(const char *path, uint64_t offset,
		    void *buf, size_t cap, size_t *out_len, int *eof)
{
	char pbuf[HOSTFS_MAX_PATH + 32];
	if (json_escape(path, pbuf, sizeof(pbuf)) < 0)
		return -ENAMETOOLONG;
	size_t chunk = cap < HOSTFS_CHUNK ? cap : HOSTFS_CHUNK;
	int n = build_req(
		"{\"name\":\"fs_read_bytes\",\"args\":"
		"{\"path\":%s,\"offset\":%llu,\"len\":%zu}}",
		pbuf, (unsigned long long)offset, chunk);
	if (n < 0) return n;
	int rc = rpc_exchange(n, NULL);
	if (rc < 0) return rc;
	/* Extract base64 data. The "data" string lives in the response
	 * buffer; we need its start and length, then decode in place into
	 * the caller's buf.
	 */
	const char *p = strstr(rpc_resp_buf, "\"data\":\"");
	if (!p) return -EIO;
	p += 8;
	const char *end = strchr(p, '"');
	if (!end) return -EIO;
	long decoded = hostfs_b64_decode(p, (size_t)(end - p), buf, cap);
	if (decoded < 0) return -EIO;
	if (out_len) *out_len = (size_t)decoded;
	if (eof) *eof = json_get_bool(rpc_resp_buf, "eof") == 1;
	return 0;
}

int hostfs_rpc_write(const char *path, uint64_t offset,
		     const void *buf, size_t len, int append)
{
	char pbuf[HOSTFS_MAX_PATH + 32];
	if (json_escape(path, pbuf, sizeof(pbuf)) < 0)
		return -ENAMETOOLONG;
	if (len > HOSTFS_CHUNK)
		return -E2BIG;

	/* Build header, then base64-encode payload directly into the
	 * request buffer, then close the JSON.
	 */
	int n;
	if (append) {
		n = snprintf(rpc_req_buf, sizeof(rpc_req_buf),
			"{\"name\":\"fs_write_bytes\",\"args\":"
			"{\"path\":%s,\"append\":true,\"data\":\"", pbuf);
	} else {
		n = snprintf(rpc_req_buf, sizeof(rpc_req_buf),
			"{\"name\":\"fs_write_bytes\",\"args\":"
			"{\"path\":%s,\"offset\":%llu,\"data\":\"",
			pbuf, (unsigned long long)offset);
	}
	if (n < 0 || (size_t)n >= sizeof(rpc_req_buf))
		return -ENAMETOOLONG;

	size_t enc_len = hostfs_b64_encoded_len(len);
	if ((size_t)n + enc_len + 4 >= sizeof(rpc_req_buf))
		return -E2BIG;
	enc_len = hostfs_b64_encode(buf, len, rpc_req_buf + n);
	n += (int)enc_len;
	memcpy(rpc_req_buf + n, "\"}}", 3);
	n += 3;

	return rpc_exchange((size_t)n, NULL);
}

int hostfs_rpc_mkdir(const char *path)
{
	char pbuf[HOSTFS_MAX_PATH + 32];
	if (json_escape(path, pbuf, sizeof(pbuf)) < 0)
		return -ENAMETOOLONG;
	int n = build_req(
		"{\"name\":\"fs_mkdir\",\"args\":{\"path\":%s}}", pbuf);
	if (n < 0) return n;
	return rpc_exchange(n, NULL);
}

int hostfs_rpc_unlink(const char *path)
{
	char pbuf[HOSTFS_MAX_PATH + 32];
	if (json_escape(path, pbuf, sizeof(pbuf)) < 0)
		return -ENAMETOOLONG;
	int n = build_req(
		"{\"name\":\"fs_unlink\",\"args\":{\"path\":%s}}", pbuf);
	if (n < 0) return n;
	return rpc_exchange(n, NULL);
}

int hostfs_rpc_truncate(const char *path, uint64_t length)
{
	char pbuf[HOSTFS_MAX_PATH + 32];
	if (json_escape(path, pbuf, sizeof(pbuf)) < 0)
		return -ENAMETOOLONG;
	int n = build_req(
		"{\"name\":\"fs_truncate\",\"args\":"
		"{\"path\":%s,\"length\":%llu}}",
		pbuf, (unsigned long long)length);
	if (n < 0) return n;
	return rpc_exchange(n, NULL);
}

int hostfs_rpc_readdir(const char *path, size_t index,
		       char *name, size_t name_cap, int *is_dir_out)
{
	char pbuf[HOSTFS_MAX_PATH + 32];
	if (json_escape(path, pbuf, sizeof(pbuf)) < 0)
		return -ENAMETOOLONG;
	int n = build_req(
		"{\"name\":\"fs_list\",\"args\":{\"path\":%s}}", pbuf);
	if (n < 0) return n;
	int rc = rpc_exchange(n, NULL);
	if (rc < 0) return rc;

	/* Walk "name":"..." occurrences; the N-th is the result. */
	const char *p = rpc_resp_buf;
	size_t seen = 0;
	const char marker[] = "\"name\":\"";
	while ((p = strstr(p, marker)) != NULL) {
		p += sizeof(marker) - 1;
		if (seen == index) {
			size_t o = 0;
			while (*p && *p != '"') {
				char c;
				if (*p == '\\' && p[1]) {
					c = p[1]; p += 2;
				} else {
					c = *p++;
				}
				if (o + 1 >= name_cap)
					return -ENAMETOOLONG;
				name[o++] = c;
			}
			name[o] = '\0';
			/* Also find the matching "is_dir" — scan ahead until
			 * the end of this entry (next "},{" or "}]").
			 */
			const char *dir_marker = strstr(p, "\"is_dir\":");
			*is_dir_out = 0;
			if (dir_marker) {
				const char *v = dir_marker + 9;
				while (*v == ' ') v++;
				if (!strncmp(v, "true", 4))
					*is_dir_out = 1;
			}
			return 0;
		}
		seen++;
	}
	return -ENOENT;
}
