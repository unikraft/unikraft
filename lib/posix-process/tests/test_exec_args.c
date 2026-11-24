/* SPDX-License-Identifier: BSD-3-Clause */

#include <uk/config.h>

#if CONFIG_LIBPOSIX_PROCESS_FORK
#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include <uk/alloc.h>
#include <uk/falloc.h>
#include <uk/isr/string.h>
#include <uk/lcpu.h>
#include <uk/print.h>
#include <uk/test.h>
#include <uk/vmem/process.h>
#include <uk/vmem/vma_types.h>

#include "../process.h"

#define ARGS_PATH (UK_PAGING_PAGE_SIZE - 3)
#define ARGS_VECTOR (2 * UK_PAGING_PAGE_SIZE - 7)
#define ARGS_VALUE (3 * UK_PAGING_PAGE_SIZE - 4)

struct args_alloc_probe {
	struct uk_alloc *real;
	unsigned int allocated;
	unsigned int freed;
	bool fail;
};

struct args_source {
	struct uk_vas *vas;
	__vaddr_t base;
	char *buffer;
};

static void *args_probe_malloc(struct uk_alloc *a, __sz len)
{
	struct args_alloc_probe *probe = (void *)a->priv;
	void *memory;

	if (probe->fail)
		return NULL;
	memory = uk_malloc(probe->real, len);
	if (memory)
		probe->allocated++;
	return memory;
}

static void args_probe_free(struct uk_alloc *a, void *memory)
{
	struct args_alloc_probe *probe = (void *)a->priv;

	probe->freed++;
	uk_free(probe->real, memory);
}

static struct uk_alloc *args_probe_allocator(void)
{
	struct uk_alloc *real = uk_vas_get_kernel()->a;
	struct uk_alloc *a = uk_malloc(real, sizeof(*a) +
				     sizeof(struct args_alloc_probe));
	struct args_alloc_probe *probe;

	if (!a)
		return NULL;
	memset_isr(a, 0, sizeof(*a) + sizeof(*probe));
	a->malloc = args_probe_malloc;
	a->free = args_probe_free;
	probe = (void *)a->priv;
	probe->real = real;
	return a;
}

static int args_source_init(struct args_source *source)
{
	struct uk_alloc *a = uk_vas_get_kernel()->a;
	__vaddr_t argv[3], envp[3];
	int rc;

	memset_isr(source, 0, sizeof(*source));
	source->base = UK_PAGING_VADDR_ANY;
	source->buffer = uk_malloc(a, 4 * UK_PAGING_PAGE_SIZE);
	if (!source->buffer)
		return -ENOMEM;
	rc = uk_vas_create_process(a, &source->vas);
	if (rc)
		return rc;
	rc = uk_vma_map_anon(source->vas, &source->base,
			     4 * UK_PAGING_PAGE_SIZE,
			     UK_PAGING_PAGE_ATTR_PROT_RW, 0, NULL);
	if (rc)
		return rc;
	memset_isr(source->buffer, 0, 4 * UK_PAGING_PAGE_SIZE);
	memcpy_isr(source->buffer + ARGS_PATH, "/bin/test", 10);
	memcpy_isr(source->buffer + ARGS_VALUE, "child value", 12);
	memcpy_isr(source->buffer + 256, "A=1", 4);
	memcpy_isr(source->buffer + 272, "B=2", 4);
	argv[0] = source->base + ARGS_PATH;
	argv[1] = source->base + ARGS_VALUE;
	argv[2] = 0;
	envp[0] = source->base + 256;
	envp[1] = source->base + 272;
	envp[2] = 0;
	memcpy_isr(source->buffer + ARGS_VECTOR, argv, sizeof(argv));
	memcpy_isr(source->buffer + 128, envp, sizeof(envp));
	return uk_vas_copy_to(source->vas, source->base, source->buffer,
			      4 * UK_PAGING_PAGE_SIZE);
}

static void args_source_free(struct args_source *source)
{
	uk_vas_put(source->vas);
	uk_free(uk_vas_get_kernel()->a, source->buffer);
	source->vas = NULL;
	source->buffer = NULL;
}

UK_TESTCASE(process_exec_args, deep_copy_after_source_teardown)
{
	struct uk_alloc *a = args_probe_allocator();
	struct pprocess_exec_args *args = NULL;
	struct args_alloc_probe *probe;
	struct args_source source;
	__vaddr_t path, argv, envp;
	unsigned int cycle;
	int rc;

	UK_TEST_EXPECT_NOT_NULL(a);
	if (!a)
		return;
	probe = (void *)a->priv;
	for (cycle = 0; cycle < 16; cycle++) {
		rc = args_source_init(&source);
		UK_TEST_EXPECT_ZERO(rc);
		if (rc) {
			args_source_free(&source);
			break;
		}
		path = source.base + ARGS_PATH;
		argv = source.base + ARGS_VECTOR;
		envp = source.base + 128;
		rc = pprocess_exec_args_copy(a, source.vas, path, argv,
					     envp, &args);
		UK_TEST_EXPECT_ZERO(rc);
		UK_TEST_EXPECT_NOT_NULL(args);
		if (!rc) {
			rc = uk_vas_copy_to(source.vas, path, "/changed", 9);
			UK_TEST_EXPECT_ZERO(rc);
		}
		args_source_free(&source);
		if (rc || !args)
			break;
		UK_TEST_EXPECT_SNUM_EQ(args->argc, 2);
		UK_TEST_EXPECT_SNUM_EQ(args->envc, 2);
		UK_TEST_EXPECT_ZERO(strcmp(args->pathname, "/bin/test"));
		UK_TEST_EXPECT_ZERO(strcmp(args->argv[0], "/bin/test"));
		UK_TEST_EXPECT_ZERO(strcmp(args->argv[1], "child value"));
		UK_TEST_EXPECT_NULL(args->argv[2]);
		UK_TEST_EXPECT_ZERO(strcmp(args->envp[0], "A=1"));
		UK_TEST_EXPECT_ZERO(strcmp(args->envp[1], "B=2"));
		UK_TEST_EXPECT_NULL(args->envp[2]);
		pprocess_exec_args_free(args);
		args = NULL;
		UK_TEST_EXPECT_SNUM_EQ(probe->allocated, probe->freed);
	}
	pprocess_exec_args_free(args);
	if (cycle == 16)
		uk_pr_info("PROCESS_EXEC_ARGS_PASS copied cycles=16\n");
	uk_free(uk_vas_get_kernel()->a, a);
}

static int args_check_error(struct uk_alloc *a, struct args_source *source,
			    __vaddr_t path, __vaddr_t argv, __vaddr_t envp,
			    int expected)
{
	struct pprocess_exec_args *args = (void *)1;
	struct args_alloc_probe *probe = (void *)a->priv;
	int rc;

	rc = pprocess_exec_args_copy(a, source->vas, path, argv, envp, &args);

	if (!rc) {
		pprocess_exec_args_free(args);
		return -EIO;
	}
	if (rc != expected)
		return rc;
	if (args || probe->allocated != probe->freed)
		return -EIO;
	return 0;
}

UK_TESTCASE(process_exec_args, bounded_error_cleanup)
{
	struct uk_alloc *a = args_probe_allocator();
	struct args_alloc_probe *probe;
	struct pprocess_exec_args *args = NULL;
	struct args_source source;
	struct uk_vas *kernel = uk_vas_get_kernel();
	__vaddr_t large = UK_PAGING_VADDR_ANY;
	__vaddr_t bad = UK_PAGING_VADDR_ANY, path;
	unsigned long free_bytes, irqf;
	int rc;

	UK_TEST_EXPECT_NOT_NULL(a);
	if (!a)
		return;
	probe = (void *)a->priv;
	rc = args_source_init(&source);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		goto out;
	path = source.base + ARGS_PATH;
	rc = uk_vma_map_anon(source.vas, &large, PPROCESS_EXEC_ARG_MAX,
			     UK_PAGING_PAGE_ATTR_PROT_RW, 0, NULL);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		goto out;
	/* Leave an explicit hole, independently of the VMA placement policy. */
	rc = uk_vma_map_anon(source.vas, &bad, UK_PAGING_PAGE_SIZE,
			     UK_PAGING_PAGE_ATTR_PROT_RW, 0, NULL);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		goto out;
	rc = uk_vma_unmap(source.vas, bad, UK_PAGING_PAGE_SIZE, 0);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		goto out;
	memset_isr(source.buffer, 'x', UK_PAGING_PAGE_SIZE);
	for (__sz offset = 0; offset < PPROCESS_EXEC_ARG_MAX;
	     offset += UK_PAGING_PAGE_SIZE) {
		rc = uk_vas_copy_to(source.vas, large + offset, source.buffer,
				    UK_PAGING_PAGE_SIZE);
		UK_TEST_EXPECT_ZERO(rc);
		if (rc)
			goto out;
	}
	rc = pprocess_exec_args_copy(a, source.vas, path, 0, 0, &args);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		goto out;
	UK_TEST_EXPECT_ZERO(args->argc);
	UK_TEST_EXPECT_ZERO(args->envc);
	UK_TEST_EXPECT_NULL(args->argv[0]);
	UK_TEST_EXPECT_NULL(args->envp[0]);
	pprocess_exec_args_free(args);
	args = NULL;
	free_bytes = kernel->pt->fa->free_memory;
	for (unsigned int i = 0; i < 16; i++) {
		rc = args_check_error(a, &source, bad, 0, 0, -EFAULT);
		UK_TEST_EXPECT_ZERO(rc);
		rc = args_check_error(a, &source, path, bad, 0, -EFAULT);
		UK_TEST_EXPECT_ZERO(rc);
		rc = args_check_error(a, &source, path,
				      source.base + ARGS_VECTOR, bad, -EFAULT);
		UK_TEST_EXPECT_ZERO(rc);
		rc = args_check_error(a, &source, large, 0, 0, -E2BIG);
		UK_TEST_EXPECT_ZERO(rc);
		UK_TEST_EXPECT_SNUM_EQ(kernel->pt->fa->free_memory, free_bytes);
	}
	probe->fail = true;
	rc = args_check_error(a, &source, path, 0, 0, -ENOMEM);
	UK_TEST_EXPECT_ZERO(rc);
	probe->fail = false;
	irqf = uk_lcpu_save_irqf();
	rc = pprocess_exec_args_copy(a, source.vas, path, 0, 0, &args);
	uk_lcpu_restore_irqf(irqf);
	UK_TEST_EXPECT_SNUM_EQ(rc, -EWOULDBLOCK);
	UK_TEST_EXPECT_NULL(args);
	UK_TEST_EXPECT_SNUM_EQ(probe->allocated, probe->freed);
	uk_pr_info("PROCESS_EXEC_ARGS_PASS rollback rounds=16 errors=64\n");
out:
	pprocess_exec_args_free(args);
	args_source_free(&source);
	uk_free(kernel->a, a);
}

_UK_TESTSUITE(process_exec_args, NULL);

static void __constructor run_process_exec_args(void)
{
	uk_testsuite_run(&_UK_TESTSUITE_NAME(process_exec_args));
}
#endif
