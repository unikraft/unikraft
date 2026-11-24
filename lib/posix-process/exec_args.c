/* SPDX-License-Identifier: BSD-3-Clause */

#include <errno.h>
#include <string.h>
#include <uk/alloc.h>
#include <uk/lcpu.h>
#include <uk/vmem/process.h>

#include "process.h"

struct exec_args_cursor {
	struct uk_vas *vas;
	char *text;
	const char **top;
};

static bool exec_args_range(struct uk_vas *vas, __vaddr_t addr, __sz len)
{
	return addr >= vas->range_start && addr < vas->range_end &&
		len <= vas->range_end - addr;
}

static int exec_args_string(struct exec_args_cursor *cursor, __vaddr_t src,
			    const char **out)
{
	char *start = cursor->text, *end;
	__sz room, chunk;
	int rc;

	while (cursor->text < (char *)cursor->top) {
		if (!exec_args_range(cursor->vas, src, 1))
			return -EFAULT;
		room = (char *)cursor->top - cursor->text;
		chunk = MIN(room, UK_PAGING_PAGE_SIZE -
			    (src & (UK_PAGING_PAGE_SIZE - 1)));
		chunk = MIN(chunk, cursor->vas->range_end - src);
		rc = uk_vas_copy_from(cursor->vas, cursor->text, src, chunk);
		if (rc)
			return rc;
		end = memchr(cursor->text, 0, chunk);
		if (end) {
			cursor->text = end + 1;
			*out = start;
			return 0;
		}
		cursor->text += chunk;
		src += chunk;
	}
	return -E2BIG;
}

static int exec_args_pointer(struct exec_args_cursor *cursor, __vaddr_t src,
			     __vaddr_t *out)
{
	if (!exec_args_range(cursor->vas, src, sizeof(*out)))
		return -EFAULT;
	return uk_vas_copy_from(cursor->vas, out, src, sizeof(*out));
}

static int exec_args_vector(struct exec_args_cursor *cursor, __vaddr_t src,
			    const char ***out, unsigned int *count)
{
	const char **first, *text, *swap;
	__vaddr_t string;
	unsigned int n = 0;
	int rc;

	if ((__sz)((char *)cursor->top - cursor->text) < sizeof(text))
		return -E2BIG;
	*--cursor->top = NULL;
	while (src) {
		rc = exec_args_pointer(cursor, src, &string);
		if (rc)
			return rc;
		if (!string)
			break;
		if ((__sz)((char *)cursor->top - cursor->text) <= sizeof(text))
			return -E2BIG;
		--cursor->top;
		rc = exec_args_string(cursor, string, &text);
		if (rc)
			return rc;
		*cursor->top = text;
		n++;
		if (src > __VADDR_MAX - sizeof(string))
			return -EFAULT;
		src += sizeof(string);
	}
	first = cursor->top;
	/* Entries grow downward; keep the terminating NULL at the upper end. */
	for (unsigned int i = 0; i < n / 2; i++) {
		swap = first[i];
		first[i] = first[n - i - 1];
		first[n - i - 1] = swap;
	}
	*out = first;
	*count = n;
	return 0;
}

int pprocess_exec_args_copy(struct uk_alloc *a, struct uk_vas *vas,
			    __vaddr_t pathname, __vaddr_t argv, __vaddr_t envp,
			    struct pprocess_exec_args **out)
{
	struct pprocess_exec_args *args;
	struct exec_args_cursor cursor;
	const char *path;
	int rc;

	if (!out)
		return -EINVAL;
	*out = NULL;
	if (!a || !vas)
		return -EINVAL;
	if (!uk_vas_is_owned(vas))
		return -ENOTSUP;
	if (uk_lcpu_irqs_disabled())
		return -EWOULDBLOCK;
	args = uk_malloc(a, sizeof(*args) + PPROCESS_EXEC_ARG_MAX);
	if (!args)
		return -ENOMEM;
	/* The destination must be resident before guarded VAS copies start. */
	memset(args, 0, sizeof(*args) + PPROCESS_EXEC_ARG_MAX);
	args->_a = a;
	cursor.vas = vas;
	cursor.text = (char *)(args + 1);
	cursor.top = (const char **)(cursor.text + PPROCESS_EXEC_ARG_MAX);
	rc = exec_args_string(&cursor, pathname, &path);
	if (!rc) {
		args->pathname = path;
		rc = exec_args_vector(&cursor, argv, &args->argv, &args->argc);
	}
	if (!rc)
		rc = exec_args_vector(&cursor, envp, &args->envp, &args->envc);
	if (rc) {
		pprocess_exec_args_free(args);
		return rc;
	}
	*out = args;
	return 0;
}

void pprocess_exec_args_free(struct pprocess_exec_args *args)
{
	if (args)
		uk_free(args->_a, args);
}
