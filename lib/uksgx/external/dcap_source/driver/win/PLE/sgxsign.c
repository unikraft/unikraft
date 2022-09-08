/*
 * Copyright (C) 2011-2021 Intel Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of Intel Corporation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#define _GNU_SOURCE
#include <getopt.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <asm/sgx_arch.h>
#include <openssl/err.h>
#include <openssl/pem.h>

static const char *sign_key_pass;

static bool check_crypto_errors(void)
{
	int err;
	bool had_errors = false;
	const char *filename;
	int line;
	char str[256];

	for ( ; ; ) {
		if (ERR_peek_error() == 0)
			break;

		had_errors = true;
		err = ERR_get_error_line(&filename, &line);
		ERR_error_string_n(err, str, sizeof(str));
		fprintf(stderr, "crypto: %s: %s:%d\n", str, filename, line);
	}

	return had_errors;
}

static void exit_usage(const char *program)
{
	fprintf(stderr,
		"Usage: \n\
		\t%s sign <private_key> <enclave> <sigstruct> [intel_signed]\n \
		\t%s gendata <enclave> <signing_material> [intel_signed]\n \
		\t%s usesig <public_key> <enclave> <signing_material> <signature> <sigstruct> [intel_signed]\n \
		", program, program, program);
	exit(1);
}

static int pem_passwd_cb(char *buf, int size, int rwflag, void *u)
{
	if (!sign_key_pass)
		return -1;

	strncpy(buf, sign_key_pass, size);
	/* no retry */
	sign_key_pass = NULL;

	return strlen(buf) >= size ? size - 1 : strlen(buf);
}

static inline const BIGNUM *get_modulus(RSA *key)
{
#if OPENSSL_VERSION_NUMBER < 0x10100000L
	return key->n;
#else
	const BIGNUM *n;

	RSA_get0_key(key, &n, NULL, NULL);
	return n;
#endif
}

static RSA *load_sign_key(const char *path)
{
	FILE *f;
	RSA *key = NULL;
	bool passed = false;

	f = fopen(path, "rb");
	if (!f) {
		fprintf(stderr, "Unable to open %s\n", path);
		goto out;
	}
	key = RSA_new();
	if (!PEM_read_RSAPrivateKey(f, &key, pem_passwd_cb, NULL))
		goto out;

	if (BN_num_bytes(get_modulus(key)) != SGX_MODULUS_SIZE) {
		fprintf(stderr, "Invalid key size %d\n",
			BN_num_bytes(get_modulus(key)));
		goto out;
	}

	passed = true;
out:
	if (f)
		fclose(f);
	if (!passed) {
		RSA_free(key);
		key = NULL;
	}

	return key;
}

static RSA *load_public_key(const char *path)
{
	FILE *f;
	RSA *key = NULL;
	bool passed = false;

	f = fopen(path, "rb");
	if (!f) {
		fprintf(stderr, "Unable to open %s\n", path);
		goto out;
	}
	key = RSA_new();
	if (!PEM_read_RSA_PUBKEY(f, &key, NULL, NULL))
		goto out;

	if (BN_num_bytes(get_modulus(key)) != SGX_MODULUS_SIZE) {
		fprintf(stderr, "Invalid key size %d\n",
			BN_num_bytes(get_modulus(key)));
		goto out;
	}

	passed = true;
out:
	if (f)
		fclose(f);
	if (!passed) {
		RSA_free(key);
		key = NULL;
	}

	return key;
}


static void reverse_bytes(void *data, int length)
{
	int i = 0;
	int j = length - 1;
	uint8_t temp;
	uint8_t *ptr = data;

	while (i < j) {
		temp = ptr[i];
		ptr[i] = ptr[j];
		ptr[j] = temp;
		i++;
		j--;
	}
}

enum mrtags {
	MRECREATE = 0x0045544145524345,
	MREADD = 0x0000000044444145,
	MREEXTEND = 0x00444E4554584545,
};

static bool mrenclave_update(EVP_MD_CTX *ctx, const void *data)
{
	if (!EVP_DigestUpdate(ctx, data, 64)) {
		fprintf(stderr, "digest update failed\n");
		return false;
	}

	return true;
}

static bool mrenclave_commit(EVP_MD_CTX *ctx, uint8_t *mrenclave)
{
	unsigned int size;

	if (!EVP_DigestFinal_ex(ctx, (unsigned char *)mrenclave, &size)) {
		fprintf(stderr, "digest commit failed\n");
		return false;
	}

	if (size != 32) {
		fprintf(stderr, "invalid digest size = %u\n", size);
		return false;
	}

	return true;
}

struct mrecreate {
	uint64_t tag;
	uint32_t ssaframesize;
	uint64_t size;
	uint8_t reserved[44];
} __attribute__((__packed__));


static bool mrenclave_ecreate(EVP_MD_CTX *ctx, uint64_t blob_size)
{
	struct mrecreate mrecreate;
	uint64_t encl_size;

	for (encl_size = 0x1000; encl_size < blob_size; )
		encl_size <<= 1;

	memset(&mrecreate, 0, sizeof(mrecreate));
	mrecreate.tag = MRECREATE;
	mrecreate.ssaframesize = 1;
	mrecreate.size = encl_size;

	if (!EVP_DigestInit_ex(ctx, EVP_sha256(), NULL))
		return false;

	return mrenclave_update(ctx, &mrecreate);
}

struct mreadd {
	uint64_t tag;
	uint64_t offset;
	uint64_t flags; /* SECINFO flags */
	uint8_t reserved[40];
} __attribute__((__packed__));

static bool mrenclave_eadd(EVP_MD_CTX *ctx, uint64_t offset, uint64_t flags)
{
	struct mreadd mreadd;

	memset(&mreadd, 0, sizeof(mreadd));
	mreadd.tag = MREADD;
	mreadd.offset = offset;
	mreadd.flags = flags;

	return mrenclave_update(ctx, &mreadd);
}

struct mreextend {
	uint64_t tag;
	uint64_t offset;
	uint8_t reserved[48];
} __attribute__((__packed__));

static bool mrenclave_eextend(EVP_MD_CTX *ctx, uint64_t offset, uint8_t *data)
{
	struct mreextend mreextend;
	int i;

	for (i = 0; i < 0x1000; i += 0x100) {
		memset(&mreextend, 0, sizeof(mreextend));
		mreextend.tag = MREEXTEND;
		mreextend.offset = offset + i;

		if (!mrenclave_update(ctx, &mreextend))
			return false;

		if (!mrenclave_update(ctx, &data[i + 0x00]))
			return false;

		if (!mrenclave_update(ctx, &data[i + 0x40]))
			return false;

		if (!mrenclave_update(ctx, &data[i + 0x80]))
			return false;

		if (!mrenclave_update(ctx, &data[i + 0xC0]))
			return false;
	}

	return true;
}

/**
 * measure_encl - measure enclave
 * @path: path to the enclave
 * @mrenclave: measurement
 *
 * Calculates MRENCLAVE. Assumes that the very first page is a TCS page and
 * following pages are regular pages. Does not measure the contents of the
 * enclave as the signing tool is used at the moment only for the launch
 * enclave, which is pass-through (everything gets a token).
 */
static bool measure_encl(const char *path, uint8_t *mrenclave, uint32_t *date)
{
	FILE *file;
	struct stat sb;
	EVP_MD_CTX *ctx;
	uint64_t flags;
	uint64_t offset;
	uint8_t data[0x1000];
	int rc;
	char date_str[11];
	struct tm *tm_p;

	ctx = EVP_MD_CTX_create();
	if (!ctx)
		return false;

	file = fopen(path, "rb");
	if (!file) {
		perror("fopen");
		EVP_MD_CTX_destroy(ctx);
		return false;
	}

	rc = stat(path, &sb);
	if (rc) {
		perror("stat");
		goto out;
	}

	if (!sb.st_size || sb.st_size & 0xfff) {
		fprintf(stderr, "Invalid blob size %lu\n", sb.st_size);
		goto out;
	}

	tm_p = gmtime(&sb.st_mtime);
	if (strftime(date_str, 11, "0x%Y%m%d", tm_p) != 10) {
		fprintf(stderr, "Failed to generate date\n");
		goto out;
	}
	*date = strtol(date_str, NULL, 0);

	if (!mrenclave_ecreate(ctx, sb.st_size))
		goto out;

	for (offset = 0; offset < sb.st_size; offset += 0x1000) {
		if (!offset)
			flags = SGX_SECINFO_TCS;
		else
			flags = SGX_SECINFO_REG | SGX_SECINFO_R |
				SGX_SECINFO_W | SGX_SECINFO_X;

		if (!mrenclave_eadd(ctx, offset, flags))
			goto out;

		rc = fread(data, 1, 0x1000, file);
		if (!rc)
			break;
		if (rc < 0x1000)
			goto out;

		if (!mrenclave_eextend(ctx, offset, data))
			goto out;
	}

	if (!mrenclave_commit(ctx, mrenclave))
		goto out;

	fclose(file);
	EVP_MD_CTX_destroy(ctx);
	return true;
out:
	fclose(file);
	EVP_MD_CTX_destroy(ctx);
	return false;
}

static void init_payload(struct sgx_sigstruct_payload *payload, const struct sgx_sigstruct *sigstruct) 
{
    if (!payload) return;
    
	memcpy(&payload->header, &sigstruct->header, sizeof(payload->header));
	memcpy(&payload->body, &sigstruct->body, sizeof(payload->body));
}

/**
 * sign_encl - sign enclave
 * @sigstruct: pointer to SIGSTRUCT
 * @key: 3072-bit RSA key
 * @signature: byte array for the signature
 *
 * Calculates EMSA-PKCSv1.5 signature for the given SIGSTRUCT. The result is
 * stored in big-endian format so that it can be further passed to OpenSSL
 * libcrypto functions.
 */
static bool sign_encl(const struct sgx_sigstruct *sigstruct, RSA *key,
		      uint8_t *signature)
{
	struct sgx_sigstruct_payload payload;
	unsigned int siglen;
	uint8_t digest[SHA256_DIGEST_LENGTH];
	bool ret;

    init_payload(&payload, sigstruct);

	SHA256((unsigned char *)&payload, sizeof(payload), digest);

	ret = RSA_sign(NID_sha256, digest, SHA256_DIGEST_LENGTH, signature,
		       &siglen, key);

	return ret;
}

struct q1q2_ctx {
	BN_CTX *bn_ctx;
	BIGNUM *m;
	BIGNUM *s;
	BIGNUM *q1;
	BIGNUM *qr;
	BIGNUM *q2;
};

static void free_q1q2_ctx(struct q1q2_ctx *ctx)
{
	BN_CTX_free(ctx->bn_ctx);
	BN_free(ctx->m);
	BN_free(ctx->s);
	BN_free(ctx->q1);
	BN_free(ctx->qr);
	BN_free(ctx->q2);
}

static bool alloc_q1q2_ctx(const uint8_t *s, const uint8_t *m,
			   struct q1q2_ctx *ctx)
{
	ctx->bn_ctx = BN_CTX_new();
	ctx->s = BN_bin2bn(s, SGX_MODULUS_SIZE, NULL);
	ctx->m = BN_bin2bn(m, SGX_MODULUS_SIZE, NULL);
	ctx->q1 = BN_new();
	ctx->qr = BN_new();
	ctx->q2 = BN_new();

	if (!ctx->bn_ctx || !ctx->s || !ctx->m || !ctx->q1 || !ctx->qr ||
	    !ctx->q2) {
		free_q1q2_ctx(ctx);
		return false;
	}

	return true;
}

static bool calc_q1q2(const uint8_t *s, const uint8_t *m, uint8_t *q1,
		      uint8_t *q2)
{
	struct q1q2_ctx ctx;
	int q1_len, q2_len;

	if (!alloc_q1q2_ctx(s, m, &ctx)) {
		fprintf(stderr, "Not enough memory for Q1Q2 calculation\n");
		return false;
	}

	if (!BN_mul(ctx.q1, ctx.s, ctx.s, ctx.bn_ctx))
		goto out;

	if (!BN_div(ctx.q1, ctx.qr, ctx.q1, ctx.m, ctx.bn_ctx))
		goto out;

	if (!BN_mul(ctx.q2, ctx.s, ctx.qr, ctx.bn_ctx))
		goto out;

	if (!BN_div(ctx.q2, NULL, ctx.q2, ctx.m, ctx.bn_ctx))
		goto out;

	q1_len = BN_num_bytes(ctx.q1);
	if (q1_len > SGX_MODULUS_SIZE) {
		fprintf(stderr, "Too large Q1 %d bytes\n", q1_len);
		goto out;
	}

	q2_len = BN_num_bytes(ctx.q2);
	if (q2_len > SGX_MODULUS_SIZE) {
		fprintf(stderr, "Too large Q2 %d bytes\n", q2_len);
		goto out;
	}

	BN_bn2bin(ctx.q1, &q1[SGX_MODULUS_SIZE - q1_len]);
	BN_bn2bin(ctx.q2, &q2[SGX_MODULUS_SIZE - q2_len]);

	free_q1q2_ctx(&ctx);
	return true;
out:
	free_q1q2_ctx(&ctx);
	return false;
}

static bool save_output(const void *data, size_t len,
			   const char *path)
{
	FILE *f = fopen(path, "wb");

	if (!f) {
		fprintf(stderr, "Unable to open %s\n", path);
		return false;
	}

	fwrite(data, len, 1, f);
	fclose(f);
	return true;
}

static void init_sigstruct(struct sgx_sigstruct *ss, bool intel_signed)
{
	uint64_t header1[2] = {0x000000E100000006, 0x0000000000010000};
	uint64_t header2[2] = {0x0000006000000101, 0x0000000100000060};

    if (!ss) return;
    
	memset(ss, 0, sizeof(struct sgx_sigstruct));
	ss->header.header1[0] = header1[0];
	ss->header.header1[1] = header1[1];
	ss->header.header2[0] = header2[0];
	ss->header.header2[1] = header2[1];
	ss->exponent = 3;
	ss->body.attributes = SGX_ATTR_MODE64BIT | SGX_ATTR_EINITTOKENKEY;
	ss->body.xfrm = 3;
	ss->body.attributesmask = 0xFFFFFFFFFFFFFFFF;
	ss->body.xfrmmask = 0xFFFFFFFFFFFFFFFF;
	ss->body.miscmask = 0xFFFFFFFF;
	ss->body.isvsvn = 1;

	if (intel_signed)
		ss->header.vendor = 0x8086;
}

static bool load_sig(const char *path, struct sgx_sigstruct *ss)
{
	FILE *f;
    size_t len;

	struct stat sb;
	int rc;
    
	rc = stat(path, &sb);
	if (rc) {
		perror("stat");
		return false;
	}

	if (sb.st_size != SGX_MODULUS_SIZE) {
		fprintf(stderr, "Wrong signature file size %lu\n", sb.st_size);
		return false;
	}

	f = fopen(path, "rb");
	if (!f) {
		fprintf(stderr, "Unable to open %s\n", path);
		return false;
	}

	len = fread(&ss->signature, 1, SGX_MODULUS_SIZE, f);
    fclose(f);

    if (len != SGX_MODULUS_SIZE) {
		fprintf(stderr, "Wrong signature read len %lu\n", len);
        return false;
    }

    return true;
}

// sign <private_key> <enclave> <sigstruct> [intel_signed]
static bool sign(int argc, char **argv, const char *program)
{
	struct sgx_sigstruct ss;
	RSA *sign_key;
	bool intel_signed = false;

	if (argc < 4)
		exit_usage(program);

	/* sanity check only */
	if (check_crypto_errors())
		exit(1);

	sign_key = load_sign_key(argv[1]);
	if (!sign_key)
		goto out;

	if (argc > 4 && !strcmp(argv[4], "intel_signed"))
		intel_signed = true;

	init_sigstruct(&ss, intel_signed);

	BN_bn2bin(get_modulus(sign_key), ss.modulus);

	if (!measure_encl(argv[2], ss.body.mrenclave, &ss.header.date))
		goto out;

	if (!sign_encl(&ss, sign_key, ss.signature))
		goto out;

	if (!calc_q1q2(ss.signature, ss.modulus, ss.q1, ss.q2))
		goto out;

	/* convert to little endian */
	reverse_bytes(ss.signature, SGX_MODULUS_SIZE);
	reverse_bytes(ss.modulus, SGX_MODULUS_SIZE);
	reverse_bytes(ss.q1, SGX_MODULUS_SIZE);
	reverse_bytes(ss.q2, SGX_MODULUS_SIZE);

	if (!save_output(&ss, sizeof(ss), argv[3]))
		goto out;

	return true;
out:
	check_crypto_errors();
	return false;
}

// gendata <enclave> <signing_material> [intel_signed]
static bool gendata(int argc, char **argv, const char *program)
{
	struct sgx_sigstruct_payload payload;
	struct sgx_sigstruct ss;
	bool intel_signed = false;
    
	if (argc < 3)
		exit_usage(program);
    
	if (argc > 3 && !strcmp(argv[3], "intel_signed"))
		intel_signed = true;

	init_sigstruct(&ss, intel_signed);
    
	if (!measure_encl(argv[1], ss.body.mrenclave, &ss.header.date))
		goto out;
    
    init_payload(&payload, &ss);
    
    if (!save_output(&payload, sizeof(payload), argv[2]))
        goto out;

    return true;
    
out:
    return false;
}

// usesig <public_key> <enclave> <signature> <sigstruct> [intel_signed]
static bool usesig(int argc, char **argv, const char *program)
{
	struct sgx_sigstruct ss;
	RSA *pub_key;
	bool intel_signed = false;

	if (argc < 5)
		exit_usage(program);

	/* sanity check only */
	if (check_crypto_errors())
		exit(1);

	pub_key = load_public_key(argv[1]);
	if (!pub_key)
		goto out;

	if (argc > 5 && !strcmp(argv[5], "intel_signed"))
		intel_signed = true;

	init_sigstruct(&ss, intel_signed);

	BN_bn2bin(get_modulus(pub_key), ss.modulus);

	if (!measure_encl(argv[2], ss.body.mrenclave, &ss.header.date))
		goto out;

	if (!load_sig(argv[3], &ss))
		goto out;
    
    if (!calc_q1q2(ss.signature, ss.modulus, ss.q1, ss.q2))
        goto out;
    
    /* convert to little endian */
    reverse_bytes(ss.signature, SGX_MODULUS_SIZE);
    reverse_bytes(ss.modulus, SGX_MODULUS_SIZE);
    reverse_bytes(ss.q1, SGX_MODULUS_SIZE);
    reverse_bytes(ss.q2, SGX_MODULUS_SIZE);

    if (!save_output(&ss, sizeof(ss), argv[4]))
        goto out;

    return true;
out:
    check_crypto_errors();
    return false;
}

int main(int argc, char **argv)
{
	const char *program;
	int opt;
    bool res;


	sign_key_pass = getenv("KBUILD_SGX_SIGN_PIN");
	program = argv[0];

	do {
		opt = getopt(argc, argv, "");
		switch (opt) {
		case -1:
			break;
		default:
			exit_usage(program);
		}
	} while (opt != -1);

	argc -= optind;
	argv += optind;

	res = false;
	if (!strcmp("sign", argv[0]))
		res = sign(argc, argv, program);
	else if (!strcmp("gendata", argv[0]))
		res = gendata(argc, argv, program);
	else if (!strcmp("usesig", argv[0]))
		res = usesig(argc, argv, program);
    
	if (!res)
		exit(1);

	exit(0);

}

