/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
 * Licensed under the BSD-3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 */

/*
 * Hyperlight host-call (hcall) API.
 *
 * Allows the guest to call functions registered on the host side.
 * Communication uses the PEB's shared I/O stacks with Hyperlight's
 * FlatBuffer-based protocol:
 *
 *   1. Guest encodes a FunctionCall FlatBuffer and pushes it onto
 *      the PEB output stack.
 *   2. Guest writes to port 101 (CallFunction) → VM exit.
 *   3. Host reads the output stack, dispatches the call, and writes
 *      the result (FunctionCallResult FlatBuffer) to the input stack.
 *   4. Guest resumes and pops the result from the input stack.
 */

#ifndef __HYPERLIGHT_HCALL_H__
#define __HYPERLIGHT_HCALL_H__

#include <uk/arch/types.h>
#include <hyperlight-x86/peb.h>

/**
 * Initialise the hcall subsystem with a copy of the PEB.
 *
 * Must be called before any hl_call_host_* function.  The PEB's
 * input_stack and output_stack pointers are cached internally.
 */
void hl_hcall_init(const struct hyperlight_peb *peb);

/**
 * Check whether the hcall subsystem has been initialised.
 */
int hl_hcall_ready(void);

/**
 * Largest payload one host call can carry, in either direction: a string
 * or vecbytes parameter on the way out, a string or vecbytes result on the
 * way back.  Derived from the PEB I/O stack sizes the host chose, less a
 * reserve for the FlatBuffer framing and the other parameters, so a
 * transfer of this size always fits and a buffer of this size never
 * truncates a result.  Valid once hl_hcall_init() has run; 0 before.
 */
__u64 hl_hcall_max_payload(void);

/**
 * Low-level PEB I/O stack push/pop.
 *
 * Shared by the hcall subsystem (guest→host calls) and the dispatch
 * subsystem (host→guest calls).  Both operate on the same PEB I/O
 * stacks using the same blob-stack layout.
 */
int hl_stack_push(volatile __u8 *stack, __u64 stack_size,
		  const __u8 *data, __u64 data_len);
int hl_stack_pop(volatile __u8 *stack,
		 const __u8 **out_data, __u64 *out_len);

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
 * hl_hcall_max_payload() bytes always suffices.
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
 * Call HostPrint — send a string to the host for printing.
 *
 * This calls the "HostPrint" host function with one string parameter.
 * Used by the console driver as an alternative to DebugPrint (port 103).
 * DebugPrint does one VM exit per byte; HostPrint does one VM exit for
 * the entire string.
 *
 * @param msg   String to print (not necessarily NUL-terminated)
 * @param len   Length of the string
 * @return      0 on success, -1 on error
 */
int hl_call_host_print(const char *msg, __sz len);

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
#define HL_PV_HLVECBYTES 9

/*
 * Additional ReturnType / ReturnValue constants not in original set.
 */
#define HL_RT_VECBYTES		9	/* ReturnType for Vec<u8> */
#define HL_RV_HLSIZEPREFIXED	10	/* ReturnValue discriminant for Vec<u8> */

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

/**
 * Call a host function returning i32.
 *
 * @param func_name  Host function name
 * @param params     Array of typed parameters
 * @param nparams    Number of parameters (0..HL_MAX_PARAMS)
 * @param out        Output: the i32 return value
 * @return           0 on success, -1 on error
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
 * @param out_len    Output: length of string (excluding NUL), or NULL
 * @return           0 on success, -1 on error
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
 * @return           0 on success, -1 on error
 */
int hl_hcall_vecbytes(const char *func_name,
		      const struct hl_param *params, int nparams,
		      __u8 *out_buf, __sz buf_sz, __sz *out_len);

#endif /* __HYPERLIGHT_HCALL_H__ */
