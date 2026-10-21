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
 *   1. DebugPrint (port 103) — one VM exit per character.  Available from
 *      the very first instruction; used during early boot before the
 *      hcall subsystem is initialised, and for whatever HostWrite does
 *      not send.
 *
 *   2. HostWrite (host function call) — one VM exit per host call's
 *      worth of the buffer.  Uses the FlatBuffer-based hcall protocol
 *      over the G2H virtqueue.  Requires hl_hcall_init() to have been
 *      called.  Standard error and kernel messages go through
 *      HostWriteErr, the same call under another name, so the host keeps
 *      them apart.  A host that refuses HostWriteErr is taken to lack it:
 *      from then on standard error goes through HostWrite; one that
 *      refuses HostWrite, likewise, gets standard output, and with it
 *      standard error, through DebugPrint.  A snapshot restore asks the
 *      new host afresh (hyperlight_console_resume()).
 *
 * Input uses the ReadStdin host function, which hands over the host
 * process's stdin, as bytes, a chunk at a time.  The console_in callback
 * drains each chunk; when the host has no more, the read returns 0 (EOF).
 * ConsoleIsTerminal says whether the streams reach a terminal on the host.
 *
 * TODO: Route uk_pr_* output through G2H Log messages instead of
 * HostWriteErr.  A Log message delivers a GuestLogData FlatBuffer to the
 * host's tracing framework, keeping kernel warnings out of the program's
 * standard error.  This requires
 * encoding GuestLogData FlatBuffers (level, source_file, line, source,
 * message) in the kernel — a different schema from hcall FlatBuffers.
 */

#include <uk/console/driver.h>
#include <hyperlight/console.h>
#include <hyperlight/hcall.h>
#include <hyperlight/outb.h>

/* Set once the host refuses HostWrite or HostWriteErr: it lacks it.  A
 * host whose function fails looks the same.  Guest memory, so a snapshot
 * keeps them: hyperlight_console_resume() clears them for the next host.
 */
static __bool no_host_write;
static __bool no_host_write_err;

void hyperlight_console_resume(void)
{
	no_host_write = __false;
	no_host_write_err = __false;
}

/*
 * HostWrite once the hcall subsystem is up; what it did not send, and all
 * of it before, goes out through DebugPrint, which needs nothing.
 */
static __ssz hyperlight_console_out(struct uk_console *dev __unused,
				    const char *buf, __sz len)
{
	__ssz sent = 0;

	if (hl_hcall_ready() && !no_host_write) {
		sent = hl_call_host_write(buf, len);
		if (sent == HL_HCALL_REFUSED)
			no_host_write = __true;
		if (sent < 0)
			sent = 0;
	}

	/* Fallback, for what it did not take: DebugPrint, a character at a
	 * time.  The host prints each word as a code point, so UTF-8 is
	 * decoded first; a byte that does not start a character here (cut
	 * by the write, or not UTF-8) goes as itself, read as Latin-1.
	 */
	for (__sz i = (__sz)sent; i < len;) {
		__u32 cp;
		__sz n = hl_utf8_decode((const __u8 *)buf + i, len - i, &cp);

		if (!n) {
			cp = (__u8)buf[i];
			n = 1;
		}
		hyperlight_debug_putc(cp);
		i += n;
	}
	return len;
}

/*
 * Standard error, kept apart from standard output by HostWriteErr; what
 * that does not send goes the way of standard output.
 */
static __ssz hyperlight_console_err(struct uk_console *dev,
				    const char *buf, __sz len)
{
	__ssz sent = 0;

	if (hl_hcall_ready() && !no_host_write_err) {
		sent = hl_call_host_write_err(buf, len);
		if (sent == HL_HCALL_REFUSED)
			no_host_write_err = __true;
		if (sent < 0)
			sent = 0;
	}
	if ((__sz)sent < len)
		hyperlight_console_out(dev, buf + sent, len - sent);
	return len;
}

/*
 * Console input via the ReadStdin host function.
 *
 * The host hands over its process's stdin a chunk at a time.  Each call
 * here drains up to @len bytes of it.
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

/*
 * Whether a stream reaches a terminal: the host's ConsoleIsTerminal says
 * for its own stdin, stdout and stderr, which the console's streams are.
 * Asked each time, as a restored guest runs for another host process.  A
 * host without it, or before the hcall subsystem is up, is taken to be a
 * terminal, as the console was before hosts said.
 */
static __bool hyperlight_console_terminal(struct uk_console *dev __unused,
					  int stream)
{
	struct hl_param p[1];
	__s32 terminal;

	if (!hl_hcall_ready())
		return __true;
	p[0].type = HL_PV_HLINT;
	p[0].i32_val = stream;
	if (hl_hcall_int("ConsoleIsTerminal", p, 1, &terminal) < 0)
		return __true;
	return terminal != 0;
}

static struct uk_console_ops hyperlight_console_ops = {
	.out = hyperlight_console_out,
	.in = hyperlight_console_in,
	.err = hyperlight_console_err,
	.terminal = hyperlight_console_terminal,
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
