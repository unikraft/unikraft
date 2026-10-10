/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

/*
 * Hyperlight console driver.
 *
 * Two output paths:
 *
 *   1. DebugPrint (port 103) — one VM exit per byte.  Available from
 *      the very first instruction; used during early boot before the
 *      hcall subsystem is initialised.  Also the fallback if HostPrint
 *      fails.
 *
 *   2. HostPrint (host function call) — one VM exit per 4 KiB of the
 *      buffer.  Uses the FlatBuffer-based hcall protocol over the PEB's
 *      shared I/O stacks.  Requires hl_hcall_init() to have been called.
 *      Standard error and kernel messages go through HostPrintErr, the
 *      same call under another name, so the host keeps them apart.
 *
 * The console transparently picks HostPrint when the hcall subsystem
 * is ready, falling back to DebugPrint otherwise.
 *
 * Input uses the ReadStdin host function — the host maintains a
 * byte buffer that the caller populates via GuestConfig::write_stdin().
 * The console_in callback drains from that buffer; when it's empty
 * the read returns 0 (EOF).
 *
 * TODO: Route uk_pr_* output through port 99 (Log / OutBAction::Log)
 * instead of HostPrint (port 101).  Port 99 delivers a GuestLogData
 * FlatBuffer to the host's tracing framework, keeping kernel warnings
 * out of the HostPrint capture path (drain_output).  This requires
 * encoding GuestLogData FlatBuffers (level, source_file, line, source,
 * message) in the kernel — a different schema from hcall FlatBuffers.
 */

#include <uk/console/driver.h>
#include <hyperlight/hcall.h>
#include <hyperlight/outb.h>

/*
 * Send @buf through @host_print (HostPrint or HostPrintErr) when the hcall
 * subsystem is up; what it could not send goes out through DebugPrint.
 */
static __ssz hyperlight_console_write(__ssz (*host_print)(const char *, __sz),
				      const char *buf, __sz len)
{
	__ssz sent = 0;

	/* Try the host function first — one VM exit per 4 KiB */
	if (hl_hcall_ready()) {
		sent = host_print(buf, len);
		if (sent < 0)
			sent = 0;
	}

	/* Fallback: DebugPrint, one byte at a time */
	for (__sz i = (__sz)sent; i < len; i++)
		hyperlight_debug_putc(buf[i]);
	return len;
}

static __ssz hyperlight_console_out(struct uk_console *dev __unused,
				    const char *buf, __sz len)
{
	return hyperlight_console_write(hl_call_host_print, buf, len);
}

/* Standard error, kept apart from standard output by HostPrintErr */
static __ssz hyperlight_console_err(struct uk_console *dev __unused,
				    const char *buf, __sz len)
{
	return hyperlight_console_write(hl_call_host_print_err, buf, len);
}

/*
 * Console input via the ReadStdin host function.
 *
 * The host maintains a stdin buffer that the caller populates via
 * GuestConfig::write_stdin().  Each call here drains up to @len
 * bytes from that buffer.
 *
 * When the buffer is fully drained, we inject an EOT (Ctrl-D, 0x04)
 * byte.  The serial TTY driver recognises EOT as the end of input, so
 * the next read(2) returns 0 -- the POSIX signal for end-of-file; with
 * CONFIG_LIBPOSIX_TTY_SERIAL_EOF_FINAL (on for this platform) every
 * later read does too and stdin stays readable to poll().  Without the
 * EOT, serial_read returns -EAGAIN and the caller busy-loops forever.
 */
static __ssz hyperlight_console_in(struct uk_console *dev __unused,
				   char *buf, __sz len)
{
	int n;

	if (!hl_hcall_ready())
		return 0;

	n = hl_call_read_stdin(buf, len);
	if (n == 0 && len > 0) {
		buf[0] = '\004'; /* EOT: the end of input, see above */
		return 1;
	}
	return (__ssz)n;
}

static struct uk_console_ops hyperlight_console_ops = {
	.out = hyperlight_console_out,
	.in = hyperlight_console_in,
	.err = hyperlight_console_err,
};

static struct uk_console hyperlight_console;

/* Called from setup.c during early boot */
void _ukplat_init_console(void)
{
	uk_console_init(&hyperlight_console, "Hyperlight",
			&hyperlight_console_ops,
			UK_CONSOLE_FLAG_STDOUT | UK_CONSOLE_FLAG_STDIN);
	uk_console_register(&hyperlight_console);
}
