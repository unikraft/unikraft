/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <uk/essentials.h>
#include <uk/alloc.h>
#include <uk/arch/types.h>
#include <uk/list.h>
#include <uk/console.h>
#include <uk/console/driver.h>
#include <uk/spinlock.h>
#include <errno.h>

#if CONFIG_LIBUKDEBUG_PRINTK
#include <uk/print.h>
#endif /* CONFIG_LIBUKDEBUG_PRINTK */

/* List of dynamically registered devices */
static struct uk_spinlock cons_dev_list_lock = UK_SPINLOCK_INITIALIZER();
static UK_LIST_HEAD(cons_dev_list);
static __u16 uk_console_device_count;

static __bool uk_console_set_stdout_once;
static __bool uk_console_set_stdin_once;
static __bool uk_console_set_emerg_stdout_once;

struct uk_console *uk_console_get(__u16 id)
{
	struct uk_console *dev = __NULL;

	uk_spin_lock(&cons_dev_list_lock);
	uk_list_for_each_entry(dev, &cons_dev_list, _list) {
		if (dev->id == id) {
			uk_spin_unlock(&cons_dev_list_lock);
			return dev;
		}
	}
	uk_spin_unlock(&cons_dev_list_lock);

	return __NULL;
}

__u16 uk_console_count(void)
{
	return uk_console_device_count;
}

__ssz uk_console_out(const char *buf, __sz len)
{
	struct uk_console *dev = __NULL;

	if (unlikely(!len))
		return 0;

	if (unlikely(!buf))
		return -EINVAL;

	/* Output to all STDOUT devices */
	uk_spin_lock(&cons_dev_list_lock);
	uk_list_for_each_entry(dev, &cons_dev_list, _list) {
		if ((dev->flags & UK_CONSOLE_FLAG_STDOUT) && dev->ops->out)
			uk_console_out_direct(dev, buf, len);
	}
	uk_spin_unlock(&cons_dev_list_lock);

	return len;
}

__ssz uk_console_in(char *buf, __sz len)
{
	struct uk_console *dev = __NULL;
	__sz leftover = len;
	__ssz rc = 0;

	if (unlikely(!len))
		return 0;

	if (unlikely(!buf))
		return -EINVAL;

	/* Collect input from all STDIN devices and append it */
	/*
	 * FIXME: This code works well if we do not expect input
	 *        from multiple devices at the same time. Due to
	 *        the restart of the iteration each time
	 *        `uk_console_in()` is called, devices with lower
	 *        registration IDs are given higher priority.
	 *        We could solve this by remembering the iteration
	 *        point between calls.
	 */
	uk_spin_lock(&cons_dev_list_lock);
	uk_list_for_each_entry(dev, &cons_dev_list, _list) {
		UK_ASSERT(dev->ops);
		if ((dev->flags & UK_CONSOLE_FLAG_STDIN) && dev->ops->in) {
			rc = uk_console_in_direct(dev, buf, leftover);
			if (rc >= 0 && (__sz)rc <= leftover) {
				leftover -= rc;
				buf += rc;
			}
			if (leftover == 0)
				break;
		}
	}
	uk_spin_unlock(&cons_dev_list_lock);

	return len - leftover;
}

__isr __ssz uk_console_emerg_out(const char *buf, __sz len)
{
	struct uk_console *dev = __NULL;

	if (unlikely(!len))
		return 0;

	if (unlikely(!buf))
		return -EINVAL;

	/* Output to all EMERG_STDOUT devices */
	uk_spin_lock(&cons_dev_list_lock);
	uk_list_for_each_entry(dev, &cons_dev_list, _list) {
		if ((dev->flags & UK_CONSOLE_FLAG_EMERG_STDOUT) &&
		    dev->ops->emerg_out)
			uk_console_emerg_out_direct(dev, buf, len);
	}
	uk_spin_unlock(&cons_dev_list_lock);

	return len;
}

__ssz uk_console_out_direct(struct uk_console *dev, const char *buf, __sz len)
{
	UK_ASSERT(dev && dev->ops);

	if (unlikely(!len))
		return 0;

	if (unlikely(!buf))
		return -EINVAL;

	if (unlikely(!dev->ops->out))
		return -EIO;

	return dev->ops->out(dev, buf, len);
}

__ssz uk_console_in_direct(struct uk_console *dev, char *buf, __sz len)
{
	UK_ASSERT(dev && dev->ops);

	if (unlikely(!len))
		return 0;

	if (unlikely(!buf))
		return -EINVAL;

	if (unlikely(!dev->ops->in))
		return -EIO;

	return dev->ops->in(dev, buf, len);
}

__isr __ssz uk_console_emerg_out_direct(struct uk_console *dev,
					const char *buf, __sz len)
{
	UK_ASSERT(dev && dev->ops);

	if (unlikely(!len))
		return 0;

	if (unlikely(!buf))
		return -EINVAL;

	if (unlikely(!dev->ops->emerg_out))
		return -EIO;

	return dev->ops->emerg_out(dev, buf, len);
}

int uk_console_out_direct_all(struct uk_console *dev,
			      const char *buf, __sz len)
{
	__sz bytes_written = 0;
	__ssz rc;

	UK_ASSERT(dev && dev->ops);

	if (unlikely(!len))
		return 0;

	if (unlikely(!buf))
		return -EINVAL;

	if (unlikely(!dev->ops->out))
		return -EIO;

	while (bytes_written < len) {
		rc = dev->ops->out(dev, buf + bytes_written,
				   len - bytes_written);
		if (unlikely(rc < 0))
			return (int)rc;

		bytes_written += (__sz)rc;
	}

	return 0;
}

int uk_console_in_direct_all(struct uk_console *dev,
			     char *buf, __sz len)
{
	__sz bytes_read = 0;
	__ssz rc;

	UK_ASSERT(dev && dev->ops);

	if (unlikely(!len))
		return 0;

	if (unlikely(!buf))
		return -EINVAL;

	if (unlikely(!dev->ops->in))
		return -EIO;

	while (bytes_read < len) {
		rc = dev->ops->in(dev, buf + bytes_read,
				  len - bytes_read);
		if (unlikely(rc < 0))
			return (int)rc;

		bytes_read += (__sz)rc;
	}

	return 0;
}

__isr int uk_console_emerg_out_direct_all(struct uk_console *dev,
					  const char *buf, __sz len)
{
	__sz bytes_written = 0;
	__ssz rc;

	UK_ASSERT(dev && dev->ops);

	if (unlikely(!len))
		return 0;

	if (unlikely(!buf))
		return -EINVAL;

	if (unlikely(!dev->ops->emerg_out))
		return -EIO;

	while (bytes_written < len) {
		rc = dev->ops->emerg_out(dev, buf + bytes_written,
					 len - bytes_written);
		if (unlikely(rc < 0))
			return (int)rc;

		bytes_written += (__sz)rc;
	}

	return 0;
}

void uk_console_register(struct uk_console *dev)
{
	struct uk_console *known_dev __maybe_unused = __NULL;

	UK_ASSERT(dev);
	UK_ASSERT(dev->ops);

#if CONFIG_LIBUKDEBUG_ENABLE_ASSERT
	uk_spin_lock(&cons_dev_list_lock);
	uk_list_for_each_entry(known_dev, &cons_dev_list, _list)
		UK_ASSERT(dev != known_dev);
	uk_spin_unlock(&cons_dev_list_lock);
#endif /* CONFIG_LIBUKDEBUG_ENABLE_ASSERT */

	/* We want to make sure that one of the registered devices has the
	 * STDOUT or STDIN flags set. `uk_console_set_std[out|in]_once` is
	 * used to track that. If a device already has these flags set is
	 * registered, we're happy
	 */
	if (dev->flags & UK_CONSOLE_FLAG_STDOUT)
		uk_console_set_stdout_once = __true;

	if (dev->flags & UK_CONSOLE_FLAG_STDIN)
		uk_console_set_stdin_once = __true;

	if (dev->flags & UK_CONSOLE_FLAG_EMERG_STDOUT)
		uk_console_set_emerg_stdout_once = __true;

	/* Otherwise, if the current device doesn't have any flags set and
	 * there has not yet been another device with any flags set, we give
	 * the current device flags. Now we have at least one device with flags
	 */
	if (!uk_console_set_stdout_once &&
	    !(dev->flags & UK_CONSOLE_FLAG_STDOUT) &&
	    dev->ops->out) {
		uk_console_set_stdout_once = __true;
		dev->flags |= UK_CONSOLE_FLAG_STDOUT;
	}

	if (!uk_console_set_stdin_once &&
	    !(dev->flags & UK_CONSOLE_FLAG_STDIN) &&
	    dev->ops->in) {
		uk_console_set_stdin_once = __true;
		dev->flags |= UK_CONSOLE_FLAG_STDIN;
	}

	if (!uk_console_set_emerg_stdout_once &&
	    !(dev->flags & UK_CONSOLE_FLAG_EMERG_STDOUT) &&
	    dev->ops->emerg_out) {
		uk_console_set_emerg_stdout_once = __true;
		dev->flags |= UK_CONSOLE_FLAG_EMERG_STDOUT;
	}

	uk_spin_lock(&cons_dev_list_lock);
	uk_list_add_tail(&dev->_list, &cons_dev_list);
	uk_spin_unlock(&cons_dev_list_lock);

	dev->id = uk_console_device_count++;

#if CONFIG_LIBUKDEBUG_PRINTK
	uk_pr_info("Registered con%" __PRIu16 ": %s, flags: %c%c\n",
		   dev->id, dev->name ? dev->name : "<anon>",
		   (dev->flags & UK_CONSOLE_FLAG_STDIN) ? 'I' : '-',
		   (dev->flags & UK_CONSOLE_FLAG_STDOUT) ? 'O' : '-');
#endif /* CONFIG_LIBUKDEBUG_PRINTK */
}

struct uk_console_async_callback {
	void *cookie;
	uk_console_async_handler_func handler;
	__u32 evflags;
	struct uk_list_head _list;
};

int uk_console_async_register_callback(struct uk_console *dev,
				       uk_console_async_handler_func handler,
				       void *cookie, __u32 event)
{
	struct uk_console_async_callback *cb;
	struct uk_console_async *async_dev;

	UK_ASSERT(dev);
	UK_ASSERT(dev->ops);
	UK_ASSERT(handler);

	if (unlikely(!event ||
		     (event & ~(UK_CONSOLE_ASYNC_EVENT_IN |
				UK_CONSOLE_ASYNC_EVENT_OUT))))
		return -EINVAL;

	if (unlikely((event & UK_CONSOLE_ASYNC_EVENT_IN) &&
		     !(dev->flags & UK_CONSOLE_FLAG_ASYNC_RX)))
		return -ENOTSUP;

	if (unlikely((event & UK_CONSOLE_ASYNC_EVENT_OUT) &&
		     !(dev->flags & UK_CONSOLE_FLAG_ASYNC_TX)))
		return -ENOTSUP;

	cb = uk_malloc(uk_alloc_get_default(), sizeof(*cb));
	if (unlikely(!cb)) {
		uk_pr_err("Failed to allocate memory for the callback.\n");
		return -ENOMEM;
	}

	cb->cookie = cookie;
	cb->handler = handler;
	cb->evflags = event;
	UK_INIT_LIST_HEAD(&cb->_list);

	async_dev = __containerof(dev, struct uk_console_async, cons);

	uk_spin_lock(&async_dev->_cb_list_lock);
	uk_list_add_tail(&cb->_list, &async_dev->_cb_list);
	uk_spin_unlock(&async_dev->_cb_list_lock);

	return 0;
}

int uk_console_async_unregister_callback(struct uk_console *dev,
					 uk_console_async_handler_func handler,
					 void *cookie, __u32 event)
{
	struct uk_console_async_callback *cb, *tmp;
	struct uk_console_async *async_dev;

	UK_ASSERT(dev);
	UK_ASSERT(dev->ops);
	UK_ASSERT(handler);

	if (unlikely(!event ||
		     (event & ~(UK_CONSOLE_ASYNC_EVENT_IN |
				UK_CONSOLE_ASYNC_EVENT_OUT))))
		return -EINVAL;

	if (unlikely(!(dev->flags & (UK_CONSOLE_FLAG_ASYNC_RX |
				     UK_CONSOLE_FLAG_ASYNC_TX))))
		return -ENOTSUP;

	async_dev = __containerof(dev, struct uk_console_async, cons);

	uk_spin_lock(&async_dev->_cb_list_lock);
	uk_list_for_each_entry_safe(cb, tmp, &async_dev->_cb_list, _list) {
		if (cb->handler != handler ||
		    cb->cookie != cookie  ||
		    cb->evflags != event)
			continue;

		uk_list_del(&cb->_list);
		uk_spin_unlock(&async_dev->_cb_list_lock);
		uk_free(uk_alloc_get_default(), cb);
		return 0;
	}
	uk_spin_unlock(&async_dev->_cb_list_lock);

	return -ENOENT;
}

static inline void _uk_console_async_evhandle(struct uk_console_async *dev,
					      __u32 event)
{
	struct uk_console_async_callback *cb;

	uk_spin_lock(&dev->_cb_list_lock);
	uk_list_for_each_entry(cb, &dev->_cb_list, _list) {
		if (!(cb->evflags & event))
			continue;

		cb->handler(&dev->cons, cb->cookie);
	}
	uk_spin_unlock(&dev->_cb_list_lock);
}

void uk_console_async_in_handle(struct uk_console_async *dev)
{
	_uk_console_async_evhandle(dev, UK_CONSOLE_ASYNC_EVENT_IN);
}

void uk_console_async_out_handle(struct uk_console_async *dev)
{
	_uk_console_async_evhandle(dev, UK_CONSOLE_ASYNC_EVENT_OUT);
}
