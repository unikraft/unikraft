#ifndef __UK_VSOCKDEV_DRIVER__
#define __UK_VSOCKDEV_DRIVER__

#include <uk/vsockdev_core.h>
#include <uk/assert.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Assign Unikraft vsock device.
 * This should be called whenever a driver adds a new device.
 *
 * @param dev
 *	Struct to unikraft vsock device that shall be registered
 * @return
 *	- (-ENOMEM): Allocation of private
 *	- (>=0): Vsock device ID on success
 */
int uk_vsockdev_drv_register(struct uk_vsockdev *dev);

/**
 * Forwards an RX queue event to the API user
 * Can (and should) be called from device interrupt context
 *
 * @param dev
 *   Unikraft vsock device to which the event relates to
 * @param queue_id
 *   receive queue ID to which the event relates to
 */
static inline void uk_vsockdev_drv_rx_event(struct uk_vsockdev *dev,
					  uint16_t queue_id)
{
	struct uk_vsockdev_event_handler *rxq_handler;

	UK_ASSERT(dev);

	rxq_handler = &dev->rx_handler;

#ifdef CONFIG_LIBUKVSOCKDEV_DISPATCHERTHREADS
	uk_semaphore_up(&rxq_handler->events);
#else
	if (rxq_handler->callback)
		rxq_handler->callback(dev, queue_id, rxq_handler->cookie);
#endif
}

#ifdef __cplusplus
}
#endif

#endif /* __UK_VSOCKDEV_DRIVER__ */
