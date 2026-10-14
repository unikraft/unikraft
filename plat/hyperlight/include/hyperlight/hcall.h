/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

/*
 * Hyperlight host-call (hcall) API.
 *
 * Allows the guest to call functions registered on the host side.
 * Communication uses the G2H virtqueue (transport.h) with Hyperlight's
 * FlatBuffer-based protocol:
 *
 *   1. Guest encodes a FunctionCall FlatBuffer, followed by the bytes of
 *      any VecBytes parameter, into the request buffer.
 *   2. Guest publishes the request with a writable reply buffer in one
 *      chain and writes to port 109 (VirtqNotify) → VM exit.
 *   3. Host reads the request, dispatches the call, and writes the
 *      result (FunctionCallResult FlatBuffer, then any VecBytes bytes)
 *      to the reply buffer.
 *   4. Guest resumes and decodes the reply.
 */

#ifndef __HYPERLIGHT_HCALL_H__
#define __HYPERLIGHT_HCALL_H__

#include <uk/arch/types.h>

/**
 * Initialise the hcall subsystem: attach to the virtqueue transport the
 * host published in scratch.
 *
 * Must be called before any hl_call_host_* function.
 */
void hl_hcall_init(void);

/**
 * Check whether host calls can be made: the hcall subsystem has been
 * initialised, and the transport has not been marked lost (see
 * hl_transport_ready()).
 * Asked before each call, not cached: the console falls back to
 * DebugPrint when it turns false.
 */
int hl_hcall_ready(void);

/**
 * Largest payload one host call can carry, in either direction: a string
 * or vecbytes parameter on the way out, a string or vecbytes result on the
 * way back.  Derived from the G2H pool size the host chose, less a
 * reserve for the FlatBuffer framing and the other parameters, so a
 * transfer of this size always fits.  A result can be up to the reserve
 * larger: hluk caps its data transfers at this same figure (fs_*,
 * net_recvfrom, HostCall), so a buffer of this size holds them; a
 * larger result of any other (the environment, the mount table) is
 * refused by hl_hcall_string() and hl_hcall_vecbytes().  Valid once
 * hl_hcall_init() has run; 0 before.
 */
__u64 hl_hcall_max_payload(void);

/**
 * Whether @a len bytes at @a s are well-formed UTF-8: no overlong forms,
 * no surrogates, nothing past U+10FFFF.  A string parameter must be: the
 * host decodes it as a string, and refuses the whole message, and the
 * guest function call in flight, on one that is not.  The hl_hcall_*
 * calls check theirs and fail.
 */
int hl_utf8_valid(const __u8 *s, __u64 len);

/**
 * Decode the well-formed UTF-8 character (as hl_utf8_valid() takes it)
 * at the start of the @a len bytes at @a s into @a cp.
 *
 * @return its length in bytes, 0 if the bytes do not start one.
 */
__u64 hl_utf8_decode(const __u8 *s, __u64 len, __u32 *cp);

/**
 * Queue the result of guest function call @a cid (see
 * hl_transport_respond()), under the lock host calls take, so it cannot
 * meet one half made.
 */
int hl_hcall_respond(__u32 cid, const __u8 *payload, __u64 len);

/**
 * Call GetExnStackTop() to query the scratch exception stack top GVA.
 *
 * The host reads the value from hyperlight-common so the guest does
 * not need to hardcode layout constants.
 *
 * @return  Exception stack top GVA, or 0 on failure.
 */
__u64 hl_call_get_exn_stack_top(void);

/**
 * Call the GetCmdLine host function to retrieve the command line.
 *
 * @param out_buf   Buffer to receive the null-terminated string
 * @param buf_sz    Size of out_buf
 * @return          Length of the returned string (excluding NUL),
 *                  or -1 on error.
 */
int hl_call_get_cmdline(char *out_buf, __sz buf_sz);

/**
 * Call GetEnvVars() to retrieve host-provided environment variables.
 *
 * Stores KEY=VALUE entries separated by NUL, followed by a terminating
 * NUL, and returns the length not counting that terminator -- with
 * snprintf() semantics: a return value of @a buf_sz or more means the
 * data did not fit and nothing was stored.  A buffer of
 * hl_hcall_max_payload() bytes holds what the host is documented to
 * send (under that many bytes, separators included).
 *
 * @param out_buf   Buffer to receive the NUL-separated entries
 * @param buf_sz    Size of out_buf
 * @return          Length of the data, or -1 on error; 0 if no
 *                  variables are set.
 */
int hl_call_get_env_vars(char *out_buf, __sz buf_sz);

/**
 * Call ReadStdin() to read bytes from the host's stdin buffer.
 *
 * Used by the console input driver.  The host returns up to
 * @a buf_sz bytes from its stdin buffer; when the buffer is
 * drained, returns 0 (EOF).
 *
 * @param out_buf   Buffer to receive stdin data
 * @param buf_sz    Maximum bytes to read
 * @return          Number of bytes read, or 0 on EOF/error.
 */
int hl_call_read_stdin(char *out_buf, __sz buf_sz);

/**
 * Call GetPagingBudget() to query how many bytes the guest should
 * allocate from scratch for the paging frame allocator.
 *
 * @return  Number of bytes (page-aligned), or 0 if the host function
 *          is not registered (caller should compute a dynamic default).
 */
__u64 hl_call_get_paging_budget(void);

/**
 * Call GetInitrdBase() to query the GPA where the host mapped the
 * initrd via map_file_cow.
 *
 * @return  GPA of the initrd, or 0 if no initrd is mapped.
 */
__u64 hl_call_get_initrd_base(void);

/**
 * Call GetInitrdSize() to query the size of the mapped initrd.
 *
 * @return  Size in bytes, or 0 if no initrd is mapped.
 */
__u64 hl_call_get_initrd_size(void);

/**
 * Call GetWallClockNs() to query the host's wall-clock time.
 *
 * @return  Nanoseconds since the Unix epoch, or 0 if the host function
 *          is not registered (wall clock will fall back to monotonic).
 */
__u64 hl_call_get_wall_clock_ns(void);

/**
 * Call GetRandomBytes() to fill @a out_buf with @a len bytes of the host's
 * entropy.  Usable from early boot: it needs no heap.
 *
 * @return  0 on success, -1 if the host has no such function or could not
 *          provide @a len bytes.
 */
int hl_call_get_random_bytes(__u8 *out_buf, __sz len);

/**
 * Call HostWrite -- send standard output to the host, as bytes.
 *
 * Used by the console driver as an alternative to DebugPrint (port 103).
 * DebugPrint does one VM exit per character; HostWrite does one VM exit per
 * hl_hcall_max_payload() bytes.  Bytes, not a string: a program's output
 * need not be UTF-8, and a write can end inside a character.
 *
 * @param buf   Bytes to write
 * @param len   How many
 * @return      How many bytes, from the start, the host took: all of
 *              them, unless a call failed part way or the host wrote
 *              fewer than it was given; or if none, HL_HCALL_REFUSED
 *              when the host answered with an error (such as not having
 *              the function) and -1 otherwise.
 */
__ssz hl_call_host_write(const char *buf, __sz len);

/**
 * Call HostWriteErr -- HostWrite for the guest's standard error.
 *
 * The same as hl_call_host_write(), through the "HostWriteErr" host
 * function, so the host can keep standard error apart.
 */
__ssz hl_call_host_write_err(const char *buf, __sz len);

/*
 * FlatBuffer ReturnType constants (from function_types.fbs).
 * Used as expected_return_type in FunctionCall.
 */
#define HL_RT_INT	0
#define HL_RT_UINT	1
#define HL_RT_LONG	2
#define HL_RT_ULONG	3
#define HL_RT_FLOAT	4
#define HL_RT_DOUBLE	5
#define HL_RT_STRING	6
#define HL_RT_BOOL	7
#define HL_RT_VOID	8

/*
 * FlatBuffer FunctionCallType constants.
 */
#define HL_FCT_GUEST	1
#define HL_FCT_HOST	2

/*
 * FlatBuffer ReturnValue union discriminants.
 */
#define HL_RV_NONE	0
#define HL_RV_HLINT	1
#define HL_RV_HLUINT	2
#define HL_RV_HLLONG	3
#define HL_RV_HLULONG	4
#define HL_RV_HLFLOAT	5
#define HL_RV_HLDOUBLE	6
#define HL_RV_HLSTRING	7
#define HL_RV_HLBOOL	8
#define HL_RV_HLVOID	9

/*
 * FlatBuffer ParameterValue union discriminants.
 */
#define HL_PV_HLINT	1
#define HL_PV_HLUINT	2
#define HL_PV_HLLONG	3
#define HL_PV_HLULONG	4
#define HL_PV_HLFLOAT	5
#define HL_PV_HLDOUBLE	6
#define HL_PV_HLSTRING	7
#define HL_PV_HLBOOL	8
/* A Vec<u8> parameter, as callers name it in struct hl_param.  On the
 * wire it is an hlexternalbytes: tag 9 is reserved since Hyperlight 0.18.
 */
#define HL_PV_HLVECBYTES 9
#define HL_PV_HLEXTERNALBYTES 10

/*
 * Additional ReturnType / ReturnValue constants not in original set.
 */
#define HL_RT_VECBYTES		9	/* ReturnType for Vec<u8> */
#define HL_RV_HLEXTERNALBYTES	11	/* ReturnValue discriminant for Vec<u8> */

/*
 * FlatBuffer FunctionCallResultType discriminants.
 */
#define HL_FCRT_NONE		0
#define HL_FCRT_RETURN_VALUE	1
#define HL_FCRT_GUEST_ERROR	2

/* ── Generic host function call API ──────────────────────────────── */

/**
 * Maximum parameters per host function call.
 */
#define HL_MAX_PARAMS	11

/**
 * Typed parameter for generic host function calls.
 */
struct hl_param {
	__u8 type;		/* HL_PV_* discriminant */
	union {
		__s32 i32_val;
		__u32 u32_val;
		__s64 i64_val;
		__u64 u64_val;
		struct { const char *ptr; __u32 len; } str;
		struct { const __u8 *ptr; __u32 len; } vec;
	};
};

/* hl_hcall_* failures besides -1, the call did not reach the host. */
#define HL_HCALL_REFUSED	(-2)	/* it answered with an error */
#define HL_HCALL_BAD_RESULT	(-3)	/* it ran it; the result is unusable */

/**
 * Call a host function returning i32.
 *
 * @param func_name  Host function name
 * @param params     Array of typed parameters
 * @param nparams    Number of parameters (0..HL_MAX_PARAMS)
 * @param out        Output: the i32 return value
 * @return           0 on success; HL_HCALL_REFUSED if the host answered
 *                   with an error (such as not having the function),
 *                   HL_HCALL_BAD_RESULT if it ran the call but the
 *                   result does not decode, or does not fit, -1 if the
 *                   call did not reach it.  The same for every hl_hcall_*.
 */
int hl_hcall_int(const char *func_name,
		 const struct hl_param *params, int nparams,
		 __s32 *out);

/**
 * Call a host function returning u64.
 */
int hl_hcall_ulong(const char *func_name,
		   const struct hl_param *params, int nparams,
		   __u64 *out);

/**
 * Call a host function returning String.
 *
 * @param out_buf    Buffer to receive the string (NUL-terminated)
 * @param buf_sz     Size of out_buf
 * @param out_len    Output: length of string (excluding NUL), or NULL;
 *                   also set, with HL_HCALL_BAD_RESULT, when it does not
 *                   fit
 * @return           as hl_hcall_int()
 */
int hl_hcall_string(const char *func_name,
		    const struct hl_param *params, int nparams,
		    char *out_buf, __sz buf_sz, __sz *out_len);

/**
 * Call a host function returning Vec<u8>.
 *
 * @param out_buf    Buffer to receive the bytes
 * @param buf_sz     Size of out_buf
 * @param out_len    Output: number of bytes written
 * @return           as hl_hcall_int(); HL_HCALL_BAD_RESULT if the result
 *                   is larger than @a buf_sz
 */
int hl_hcall_vecbytes(const char *func_name,
		      const struct hl_param *params, int nparams,
		      __u8 *out_buf, __sz buf_sz, __sz *out_len);

#endif /* __HYPERLIGHT_HCALL_H__ */
