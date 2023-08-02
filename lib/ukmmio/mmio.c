#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uk/alloc.h>
#include <uk/mmio.h>

#include <uk/libparam.h>
#include <uk/essentials.h>

#define VIRTIO_MMIO_DEV_MAX	32 /* arbitrary */
static char *uk_libparam_devices[VIRTIO_MMIO_DEV_MAX] = {0};

UK_LIBPARAM_PARAM_ARR_ALIAS(device, uk_libparam_devices, charp,
			    VIRTIO_MMIO_DEV_MAX, "virtio-mmio devices");

/* Parse size string with suffix (eg "4K") into number */
static char *parse_size(const char* size_str, size_t *sz)
{
	char *endptr;

	*sz = strtoull(size_str, &endptr, 0);

	switch(*endptr) {
	case 'k':
	case 'K':
		*sz *= 1024;
		++endptr;
		break;
	case 'm':
	case 'M':
		*sz *= 1024 * 1024;
		++endptr;
		break;
	case 'g':
	case 'G':
		*sz *= 1024 * 1024 * 1024;
		++endptr;
		break;
	default:
		break;
	}

	return endptr;
}

/* virtio_mmio.base = <size>@<base>:<irq>[:<id>] */
static struct uk_mmio_device *uk_mmio_parse_dev(char *str)
{
	struct uk_mmio_device *dev;
	int chunks = 0;

	dev = uk_calloc(uk_alloc_get_default(), 1, sizeof(*dev));
	if (unlikely(!dev))
		return NULL;

	str = parse_size(str, &dev->size);
	if (unlikely(!dev->size))
		return NULL;

	chunks = sscanf(str, "@%" PRIx64 ":%" PRIx32 ":%" PRIx32,
			&dev->base_addr, &dev->irq, &dev->id);

	if (unlikely(chunks < 2))
		return NULL;

	return dev;
}

unsigned int uk_mmio_dev_count(void)
{
	unsigned int count = 0;

	while (((char **)uk_libparam_devices)[count++])
		count++;

	return count;
}

struct uk_mmio_device *uk_mmio_dev_get(unsigned int id)
{
	if (id >= ARRAY_SIZE(uk_libparam_devices) || !uk_libparam_devices[id])
		return NULL;

	return uk_mmio_parse_dev(((char **)uk_libparam_devices)[id]);
}

