/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

#include <uk/essentials.h>
#include <uk/arch/types.h>
#include <uk/list.h>
#include <uk/console.h>
#include <uk/console/driver.h>
#include <errno.h>
#if CONFIG_LIBUKCONSOLE_ONLCR
#include <string.h>
#endif /* CONFIG_LIBUKCONSOLE_ONLCR */

#if CONFIG_LIBUKDEBUG_PRINTK
#include <uk/print.h>
#endif /* CONFIG_LIBUKDEBUG_PRINTK */

/* List of dynamically registered devices */
static UK_LIST_HEAD(uk_console_device_list);
static __u16 uk_console_device_count;

static __bool uk_console_set_stdout_once;
static __bool uk_console_set_stdin_once;

struct uk_console *uk_console_get(__u16 id)
{
	struct uk_console *dev = __NULL;

	uk_list_for_each_entry(dev, &uk_console_device_list, _list) {
		if (dev->id == id)
			return dev;
	}

	return __NULL;
}

__u16 uk_console_count(void)
{
	return uk_console_device_count;
}

enum console_stream { CONSOLE_OUT, CONSOLE_ERR, CONSOLE_ERR_UNLOGGED };

/* There is no STDERR flag: standard error goes to the STDOUT devices */
static __ssz console_write(const char *buf, __sz len,
			   enum console_stream stream)
{
	struct uk_console *dev = __NULL;

	if (unlikely(!len))
		return 0;

	if (unlikely(!buf))
		return -EINVAL;

	uk_list_for_each_entry(dev, &uk_console_device_list, _list) {
		if (!(dev->flags & UK_CONSOLE_FLAG_STDOUT))
			continue;
		if (stream == CONSOLE_ERR_UNLOGGED && dev->ops->log &&
		    !dev->_log_declined)
			continue;
		if (stream != CONSOLE_OUT)
			uk_console_err_direct(dev, buf, len);
		else
			uk_console_out_direct(dev, buf, len);
	}

	return len;
}

__bool uk_console_is_terminal(int stream)
{
	struct uk_console *dev = __NULL;

	uk_list_for_each_entry(dev, &uk_console_device_list, _list) {
		if (!(dev->flags & (UK_CONSOLE_FLAG_STDOUT |
				    UK_CONSOLE_FLAG_STDIN)))
			continue;
		if (dev->ops->terminal && !dev->ops->terminal(dev, stream))
			return __false;
	}
	return __true;
}

__ssz uk_console_out(const char *buf, __sz len)
{
	return console_write(buf, len, CONSOLE_OUT);
}

__ssz uk_console_err(const char *buf, __sz len)
{
	return console_write(buf, len, CONSOLE_ERR);
}

__ssz uk_console_err_unlogged(const char *buf, __sz len)
{
	return console_write(buf, len, CONSOLE_ERR_UNLOGGED);
}

void uk_console_log(const struct uk_console_log_msg *msg)
{
	struct uk_console *dev = __NULL;

	UK_ASSERT(msg);

	uk_list_for_each_entry(dev, &uk_console_device_list, _list) {
		if ((dev->flags & UK_CONSOLE_FLAG_STDOUT) && dev->ops->log)
			dev->_log_declined = dev->ops->log(dev, msg) != 0;
	}
}

/* One place for the CR-LF serial consoles need and byte-keeping hosts do not */
static __ssz console_write_onlcr(__ssz (*write)(const char *, __sz),
				 const char *buf, __sz len)
{
#if CONFIG_LIBUKCONSOLE_ONLCR
	const char *next_nl;
	__sz l = len;
	__sz off;
#endif /* CONFIG_LIBUKCONSOLE_ONLCR */
	__ssz rc;

	if (unlikely(!len))
		return 0;
	if (unlikely(!buf))
		return -EINVAL;

#if CONFIG_LIBUKCONSOLE_ONLCR
	while (l > 0) {
		next_nl = memchr(buf, '\n', l);
		if (!next_nl) {
			if ((rc = write(buf, l)) < 0)
				return rc;
			break;
		}
		off = next_nl - buf;
		if ((rc = write(buf, off)) < 0)
			return rc;
		if ((rc = write("\r\n", 2)) < 0)
			return rc;
		buf = next_nl + 1;
		l -= off + 1;
	}
#else /* !CONFIG_LIBUKCONSOLE_ONLCR */
	if ((rc = write(buf, len)) < 0)
		return rc;
#endif /* !CONFIG_LIBUKCONSOLE_ONLCR */

	return len;
}

__ssz uk_console_out_onlcr(const char *buf, __sz len)
{
	return console_write_onlcr(uk_console_out, buf, len);
}

__ssz uk_console_err_onlcr(const char *buf, __sz len)
{
	return console_write_onlcr(uk_console_err, buf, len);
}

__ssz uk_console_err_unlogged_onlcr(const char *buf, __sz len)
{
	return console_write_onlcr(uk_console_err_unlogged, buf, len);
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
	uk_list_for_each_entry(dev, &uk_console_device_list, _list) {
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

	return len - leftover;
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

__ssz uk_console_err_direct(struct uk_console *dev, const char *buf, __sz len)
{
	UK_ASSERT(dev && dev->ops);

	if (unlikely(!len))
		return 0;

	if (unlikely(!buf))
		return -EINVAL;

	return dev->ops->err ? dev->ops->err(dev, buf, len)
			     : uk_console_out_direct(dev, buf, len);
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

void uk_console_register(struct uk_console *dev)
{
	struct uk_console *known_dev __maybe_unused = __NULL;

	UK_ASSERT(dev);
	UK_ASSERT(dev->ops);

#if CONFIG_LIBUKDEBUG_ENABLE_ASSERT
	uk_list_for_each_entry(known_dev, &uk_console_device_list, _list)
		UK_ASSERT(dev != known_dev);
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

	uk_list_add_tail(&dev->_list, &uk_console_device_list);
	dev->id = uk_console_device_count++;

#if CONFIG_LIBUKDEBUG_PRINTK
	uk_pr_info("Registered con%" __PRIu16 ": %s, flags: %c%c\n",
		   dev->id, dev->name ? dev->name : "<anon>",
		   (dev->flags & UK_CONSOLE_FLAG_STDIN) ? 'I' : '-',
		   (dev->flags & UK_CONSOLE_FLAG_STDOUT) ? 'O' : '-');
#endif /* CONFIG_LIBUKDEBUG_PRINTK */

}
