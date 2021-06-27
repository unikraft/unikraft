#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uk/alloc.h>
#include <uk/mmio.h>

#define MAX_DEV_STR 255
const char virtio_mmio_identifier[] = "virtio_mmio.device=";

struct uk_alloc *a;
struct uk_mmio_device_list uk_mmio_device_list =
UK_TAILQ_HEAD_INITIALIZER(uk_mmio_device_list);
int uk_mmio_device_count = 0;

unsigned int uk_mmio_dev_count(void)
{
	return (unsigned int) uk_mmio_device_count;
}

struct uk_mmio_device * uk_mmio_dev_get(unsigned int id)
{
	struct uk_mmio_device *dev;

	UK_TAILQ_FOREACH(dev, &uk_mmio_device_list, _list) {
		if (dev->id == id)
			return dev;
	}

	return NULL;
}

__u64 get_token_until(char *str, char c, int base, char **pEnd)
{
	char *p;

	for (p = str; *p && *p != c; p++);
	if (*p) {
		*p = ' ';
	}

	return strtol(str, pEnd, base);
}

int uk_mmio_add_dev(char *device)
{
	__u64 size, base_addr;
	unsigned long irq, plat_dev_id = 0;
	char devStr[MAX_DEV_STR];
	char *pEnd;
	struct uk_mmio_device *dev;

	if (!(a = uk_alloc_get_default())) {
		uk_pr_err("No allocator\n");
		return -1;
	}

	if (strncmp(device, virtio_mmio_identifier, sizeof(virtio_mmio_identifier) - 1)) {
		uk_pr_err("Invalid mmio device cmdline argument\n");
		return -1;
	}

	strcpy(devStr, device + sizeof(virtio_mmio_identifier) - 1);
	
	size = get_token_until(devStr, '@', 0, &pEnd);
	if (!size) {
		uk_pr_err("Couldn't parse mmio device size\n");
		return -1;
	}

	base_addr = get_token_until(pEnd, ':', 0, &pEnd);
	if (!base_addr) {
		uk_pr_err("Couldn't parse mmio device base addr\n");
		return -1;
	}

	irq = get_token_until(pEnd, ':', 10, &pEnd);
	if (!irq) {
		uk_pr_err("Couldn't parse mmio device base irq\n");
		return -1;
	}

	if (*pEnd) {
		plat_dev_id = get_token_until(pEnd, 0, 10, NULL);
	}

	dev = uk_calloc(a, 1, sizeof(*dev));
	if (!dev) {
		return -1;
	}

	dev->id = uk_mmio_device_count++;
	dev->base_addr = base_addr;
	dev->size = size;
	dev->irq = irq;
	dev->dev_id = plat_dev_id;
	UK_TAILQ_INSERT_TAIL(&uk_mmio_device_list, dev, _list);

	uk_pr_info("New mmio device at %#x of size %#x and irq %u\n", base_addr, size, irq);
	return 0;
}
