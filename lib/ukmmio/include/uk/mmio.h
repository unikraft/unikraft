#ifndef __UK_MMIODEV__
#define __UK_MMIODEV__

#include <sys/types.h>
#include <uk/list.h>

#ifdef __cplusplus
extern "C" {
#endif

struct uk_mmio_device {
	int id;
	UK_TAILQ_ENTRY(struct uk_mmio_device) _list;

	__u64 size;
	__u64 base_addr;
	unsigned long irq;
	unsigned long dev_id;
};

/* List of MMIO devices */
UK_TAILQ_HEAD(uk_mmio_device_list, struct uk_mmio_device);

/**
 * Get number of mmio devices.
 *
 * @return
 *	- int: total number of mmio devices
 */
unsigned int uk_mmio_dev_count(void);

/**
 * Get a reference to a Unikraft MMIO Device, based on its ID.
 *
 * @param id
 *	The identifier of the Unikraft MMIO device.
 * @return
 *	- NULL: device not found in list
 *	- (struct uk_mmio_device *): reference to an Unikraft MMIO Device
 */
struct uk_mmio_device * uk_mmio_dev_get(unsigned int id);

/**
 * Add a Unikraft MMIO device
 *
 * @param device
 *	Cmdline argument represnting a virtio_mmio device.
 * @return
 *	- 0: succesfully registered the device
 *	- != 0: error on registering the device
 */
int uk_mmio_add_dev(char *device);

#ifdef __cplusplus
}
#endif

#endif /* __UK_MMIODEV__ */
