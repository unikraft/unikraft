#ifndef __UK_MMIODEV__
#define __UK_MMIODEV__

#include <sys/types.h>
#include <uk/list.h>

#ifdef __cplusplus
extern "C" {
#endif

struct uk_mmio_device {
	__u64 base_addr;
	__sz size;
	__u32 irq;
	__u32 id;
};

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

#ifdef __cplusplus
}
#endif

#endif /* __UK_MMIODEV__ */
