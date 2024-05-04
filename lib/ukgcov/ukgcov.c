/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the MIT License (the "License", see COPYING.md).
 * You may not use this file except in compliance with the License.
 */

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <uk/essentials.h>
#include <uk/gcov.h>
#include <uk/config.h>

#if CONFIG_LIBUKGCOV_OUTPUT_BINARY_MEMORY
static unsigned char __used *gcov_output_buffer;
static size_t __used gcov_output_buffer_pos;
static size_t gcov_output_buffer_size = CONFIG_LIBUKGCOV_OUTPUT_BUFFER_SIZE;
#endif /* CONFIG_LIBUKGCOV_OUTPUT_BINARY_MEMORY */

extern const struct gcov_info *const __gcov_info_start[];
extern const struct gcov_info *const __gcov_info_end[];

/* gcov declarations taken from
 * https://github.com/gcc-mirror/gcc/blob/master/libgcc/gcov.h
 */
struct gcov_info;
typedef void (*__filename_fn)(const char *, void *);
typedef void (*__dump_fn)(const void *, unsigned int, void *);
typedef void *(*__allocate_fn)(unsigned int, void *);

void __gcov_info_to_gcda(const struct gcov_info *__info,
			 __filename_fn filename_fn, __dump_fn dump_fn,
			 __allocate_fn allocate_fn, void *__arg);
void __gcov_filename_to_gcfn(const char *__filename, __dump_fn dump_fn,
			     void *__arg);

#if CONFIG_LIBUKGCOV_OUTPUT_BINARY_MEMORY
static void dump_memory(const void *source, unsigned int n, void *arg __unused)
{
	const unsigned char *c = source;
	unsigned char *buf;

	if (gcov_output_buffer_pos + n >= gcov_output_buffer_size) {
		buf = (unsigned char *)realloc(gcov_output_buffer,
						gcov_output_buffer_size <<= 1);

		if (unlikely(!buf)) {
			free(gcov_output_buffer);
			free(buf);
			return;
		}
		gcov_output_buffer = buf;
	}
	memcpy(gcov_output_buffer + gcov_output_buffer_pos, c, n);
	gcov_output_buffer_pos += n;
}
#endif /* CONFIG_LIBUKGCOV_OUTPUT_BINARY_MEMORY */

#if CONFIG_LIBUKGCOV_OUTPUT_BINARY_FILE
static void dump_file(const void *source, unsigned int n, void *arg)
{
	FILE *file = arg;
	const unsigned char *c = source;

	fwrite(c, n, 1, file);
}
#endif /* CONFIG_LIBUKGCOV_OUTPUT_BINARY_FILE */

#if CONFIG_LIBUKGCOV_OUTPUT_SERIAL_HEXDUMP
static inline char e(unsigned int c)
{
	return (c % 16) < 10 ? (c % 16) + '0' : (c % 16) - 10 + 'a';
}

static inline unsigned char *encode(unsigned char c, unsigned char buf[2])
{
	buf[0] = e(c);
	buf[1] = e(c >> 4);
	return buf;
}

static void dump_serial(const void *source, unsigned int n, void *arg __unused)
{
	const unsigned char *c = source;
	unsigned char buf[2];
	char *encoded;

	for (unsigned int i = 0; i < n; ++i) {
		encoded = encode(c[i], buf);

		putchar(encoded[0]);
		putchar(encoded[1]);
	}
}
#endif /* CONFIG_LIBUKGCOV_OUTPUT_SERIAL_HEXDUMP */

/*
 * Generate a consistent byte stream in order to transmit
 * the gcov data from the target system to the host.
 */
static void dump(const void *d, unsigned int n, void *arg)
{
#if CONFIG_LIBUKGCOV_OUTPUT_BINARY_MEMORY
	dump_memory(d, n, arg);
#endif /* CONFIG_LIBUKGCOV_OUTPUT_BINARY_MEMORY */
#if CONFIG_LIBUKGCOV_OUTPUT_BINARY_FILE
	dump_file(d, n, arg);
#endif /* CONFIG_LIBUKGCOV_OUTPUT_BINARY_FILE */
#if CONFIG_LIBUKGCOV_OUTPUT_SERIAL_HEXDUMP
	dump_serial(d, n, arg);
#endif /* CONFIG_LIBUKGCOV_OUTPUT_SERIAL_HEXDUMP */
}

/*
 * Serializes the filename into a gcfn data stream.
 * The "gcov-tool" utilizes this gcfn data in its "
 * merge-stream" subcommand to determine
 * the filename linked to the gcov details.
 */
static void filename(const char *f, void *arg)
{
	__gcov_filename_to_gcfn(f, dump, arg);
}

/*
 * Specifies the memory allocation function to be used by the gcov library.
 */
static void *allocate(unsigned int length, void *arg __unused)
{
	return malloc(length);
}

#if CONFIG_LIBUKGCOV_OUTPUT_BINARY_MEMORY
static int gcov_dump_info_memory(void)
{
	const struct gcov_info *const *info = __gcov_info_start;
	const struct gcov_info *const *end = __gcov_info_end;

	gcov_output_buffer = (unsigned char *)malloc(gcov_output_buffer_size);
	if (unlikely(!gcov_output_buffer))
		return -ENOMEM;
	while (info != end) {
		__gcov_info_to_gcda(*info, filename, dump, allocate, NULL);
		++info;
	}
	return 0;
}
#endif /* CONFIG_LIBUKGCOV_OUTPUT_BINARY_MEMORY */

#if CONFIG_LIBUKGCOV_OUTPUT_BINARY_FILE
static int gcov_dump_info_file(void)
{
	const struct gcov_info *const *info = __gcov_info_start;
	const struct gcov_info *const *end = __gcov_info_end;

	FILE *file = fopen(UKGCOV_OUTPUT_BINARY_FILENAME, "w");

	if (unlikely(unlikely(!file)))
		return -ENOENT;

	while (info != end) {
		__gcov_info_to_gcda(*info, filename,
				    dump, allocate, (void *)file);
		++info;
	}
	fclose(file);
	return 0;
}
#endif /* CONFIG_LIBUKGCOV_OUTPUT_BINARY_FILE */

#if CONFIG_LIBUKGCOV_OUTPUT_SERIAL_HEXDUMP
static int gcov_dump_info_serial(void)
{
	const struct gcov_info *const *info = __gcov_info_start;
	const struct gcov_info *const *end = __gcov_info_end;

	printf("info_start: %p\n", info);
	printf("info_end: %p\n", end);

	puts("\n");
	puts("GCOV_DUMP_INFO_SERIAL:");
	while (info != end) {
		__gcov_info_to_gcda(*info, filename, dump, allocate, NULL);
		putchar('\n');
		++info;
	}
	puts("GCOV_DUMP_INFO_SERIAL_END\n");
	return 0;
}
#endif /* CONFIG_LIBUKGCOV_OUTPUT_SERIAL_HEXDUMP */

/*
 * Dump the coverage information using the selected method
 */
int ukgcov_dump_info(void)
{
#if CONFIG_LIBUKGCOV_OUTPUT_BINARY_MEMORY
	return gcov_dump_info_memory();
#endif /* CONFIG_LIBUKGCOV_OUTPUT_BINARY_MEMORY */

#if CONFIG_LIBUKGCOV_OUTPUT_BINARY_FILE
	return gcov_dump_info_file();
#endif /* CONFIG_LIBUKGCOV_OUTPUT_BINARY_FILE */

#if CONFIG_LIBUKGCOV_OUTPUT_SERIAL_HEXDUMP
	printf("GCOV_DUMP_INFO_SERIAL:\n");
	return gcov_dump_info_serial();
#endif /* CONFIG_LIBUKGCOV_OUTPUT_SERIAL_HEXDUMP */

	/* Should never reach this point */
	return -1;
}
