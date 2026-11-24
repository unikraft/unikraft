/* SPDX-License-Identifier: BSD-3-Clause */

#include <errno.h>
#include <string.h>
#include <uk/alloc.h>
#include <uk/binfmt.h>
#include <uk/print.h>
#include <uk/test.h>

#define BINFMT_TEST_TAG 0x62696e666d747374ULL

struct binfmt_test_metadata {
	struct uk_alloc *alloc;
	__u64 *image;
};

static __u8 binfmt_test_stack[64] __align(16);
static const __u64 binfmt_static_image[] = { BINFMT_TEST_TAG };
static unsigned int metadata_live, image_live, released, unloaded;

static void binfmt_test_args(struct uk_binfmt_loader_args *args,
			     const char *path)
{
	memset(args, 0, sizeof(*args));
	args->alloc = uk_alloc_get_default();
	args->pathname = path;
	args->stack_size = sizeof(binfmt_test_stack);
	args->ctx.sp = (__uptr)(binfmt_test_stack + sizeof(binfmt_test_stack));
}

static int binfmt_test_load(struct uk_binfmt_loader_args *args,
			    const char *path)
{
	struct binfmt_test_metadata *metadata;

	if (strcmp(args->pathname, path))
		return UK_BINFMT_NOT_HANDLED;
	metadata = uk_malloc(args->alloc, sizeof(*metadata));
	if (!metadata)
		return -ENOMEM;
	metadata->alloc = args->alloc;
	metadata->image = uk_malloc(args->alloc, sizeof(*metadata->image));
	if (!metadata->image) {
		uk_free(args->alloc, metadata);
		return -ENOMEM;
	}
	*metadata->image = BINFMT_TEST_TAG;
	args->ctx.ip = (__uptr)metadata->image;
	args->user = metadata;
	metadata_live++;
	image_live++;
	return UK_BINFMT_HANDLED;
}

static int binfmt_load_release(struct uk_binfmt_loader_args *args)
{
	return binfmt_test_load(args, "/uktest/binfmt/release");
}

static int binfmt_load_script(struct uk_binfmt_loader_args *args)
{
	return binfmt_test_load(args, "/uktest/binfmt/script");
}

static int binfmt_load_stateless(struct uk_binfmt_loader_args *args)
{
	if (strcmp(args->pathname, "/uktest/binfmt/stateless"))
		return UK_BINFMT_NOT_HANDLED;
	args->ctx.ip = (__uptr)binfmt_static_image;
	return UK_BINFMT_HANDLED;
}

static void binfmt_free_metadata(struct uk_binfmt_loader_args *args)
{
	struct binfmt_test_metadata *metadata = args->user;

	uk_free(metadata->alloc, metadata);
	metadata_live--;
	released++;
}

static int binfmt_test_unload(struct uk_binfmt_loader_args *args)
{
	struct binfmt_test_metadata *metadata = args->user;

	uk_free(metadata->alloc, metadata->image);
	uk_free(metadata->alloc, metadata);
	args->user = NULL;
	image_live--;
	metadata_live--;
	unloaded++;
	return UK_BINFMT_HANDLED;
}

static struct uk_binfmt_loader binfmt_release_loader = {
	.name = "metadata ownership test",
	.type = UK_BINFMT_LOADER_TYPE_EXEC,
	.ops = {
		.load = binfmt_load_release,
		.unload = binfmt_test_unload,
		.release_metadata = binfmt_free_metadata,
	},
};

static struct uk_binfmt_loader binfmt_script_loader = {
	.name = "script ownership test",
	.type = UK_BINFMT_LOADER_TYPE_SCRIPT,
	.ops = {
		.load = binfmt_load_script,
		.unload = binfmt_test_unload,
	},
};

static int binfmt_stateless_unload(struct uk_binfmt_loader_args *args __unused)
{
	return UK_BINFMT_HANDLED;
}

static struct uk_binfmt_loader binfmt_stateless_loader = {
	.name = "stateless ownership test",
	.type = UK_BINFMT_LOADER_TYPE_EXEC,
	.ops = {
		.load = binfmt_load_stateless,
		.unload = binfmt_stateless_unload,
	},
};

UK_TESTCASE(binfmt_metadata, release_preserves_loaded_image)
{
	struct uk_binfmt_loader_args args;
	unsigned int before = released;
	__u64 *image;
	int rc;

	binfmt_test_args(&args, "/uktest/binfmt/release");
	rc = uk_binfmt_load(&args);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		return;
	image = (__u64 *)args.ctx.ip;
	UK_TEST_EXPECT_PTR_EQ(args.loader, &binfmt_release_loader);
	rc = uk_binfmt_release_metadata(&args);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc) {
		UK_TEST_EXPECT_ZERO(uk_binfmt_unload(&args));
		return;
	}
	UK_TEST_EXPECT_NULL(args.user);
	UK_TEST_EXPECT_NULL(args.loader);
	UK_TEST_EXPECT_ZERO(metadata_live);
	UK_TEST_EXPECT_SNUM_EQ(image_live, 1);
	UK_TEST_EXPECT_SNUM_EQ(released, before + 1);
	UK_TEST_EXPECT_SNUM_EQ(args.ctx.ip, (__uptr)image);
	if (image_live) {
		UK_TEST_EXPECT_SNUM_EQ(*image, BINFMT_TEST_TAG);
		uk_free(args.alloc, image);
		image_live--;
	}
	UK_TEST_EXPECT_SNUM_EQ(uk_binfmt_release_metadata(&args), -EINVAL);
	UK_TEST_EXPECT_ZERO(image_live);
	uk_pr_info("BINFMT_METADATA_PASS release image_live=0\n");
}

UK_TESTCASE(binfmt_metadata, script_fallback_and_stateless_image)
{
	struct uk_binfmt_loader_args args;
	unsigned int before = unloaded;
	void *metadata;
	int rc;

	binfmt_test_args(&args, "/uktest/binfmt/script");
	rc = uk_binfmt_load(&args);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		return;
	metadata = args.user;
	UK_TEST_EXPECT_PTR_EQ(args.loader, &binfmt_script_loader);
	if (!args.loader) {
		binfmt_test_unload(&args);
		return;
	}
	rc = uk_binfmt_release_metadata(&args);
	UK_TEST_EXPECT_SNUM_EQ(rc, -ENOTSUP);
	UK_TEST_EXPECT_PTR_EQ(args.user, metadata);
	UK_TEST_EXPECT_PTR_EQ(args.loader, &binfmt_script_loader);
	UK_TEST_EXPECT_SNUM_EQ(metadata_live, 1);
	UK_TEST_EXPECT_SNUM_EQ(image_live, 1);
	UK_TEST_EXPECT_ZERO(uk_binfmt_unload(&args));
	UK_TEST_EXPECT_SNUM_EQ(unloaded, before + 1);
	UK_TEST_EXPECT_ZERO(metadata_live);
	UK_TEST_EXPECT_ZERO(image_live);
	binfmt_test_args(&args, "/uktest/binfmt/stateless");
	rc = uk_binfmt_load(&args);
	UK_TEST_EXPECT_ZERO(rc);
	if (rc)
		return;
	UK_TEST_EXPECT_PTR_EQ(args.loader, &binfmt_stateless_loader);
	UK_TEST_EXPECT_ZERO(uk_binfmt_release_metadata(&args));
	UK_TEST_EXPECT_NULL(args.loader);
	UK_TEST_EXPECT_NULL(args.user);
	UK_TEST_EXPECT_SNUM_EQ(*(__u64 *)args.ctx.ip, BINFMT_TEST_TAG);
	uk_pr_info("BINFMT_METADATA_PASS fallback metadata_live=0 image_live=0\n");
}

_UK_TESTSUITE(binfmt_metadata, NULL);

static void __constructor run_binfmt_metadata(void)
{
	uk_binfmt_register(&binfmt_release_loader);
	uk_binfmt_register(&binfmt_script_loader);
	uk_binfmt_register(&binfmt_stateless_loader);
	uk_testsuite_run(&_UK_TESTSUITE_NAME(binfmt_metadata));
}
