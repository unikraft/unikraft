/*
 * Copyright (C) 2011-2021 Intel Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of Intel Corporation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef _SL_SIGLINES_H_
#define _SL_SIGLINES_H_

/*
 * sl_siglines - Signal lines is an efficient, bi-directional event
 * notification mechanism between the inside and outside of an enclave.
 *
 * sl_siglines has the following features:
 *
 *  Signal-based. A signal (not to be confused with POSIX signal) is just
 *  occurance of some event. A object of sl_siglines maintains a fixed number
 *  of signal lines. Signal lines can be allocated and freed. Depending upon
 *  the usage, each signal line can represent a different type of event or
 *  event source.
 *
 *  General-purpose. Users can register signal handlers. And, user data can be
 *  associated with each signal sent. Thus, how to handle a signal and its
 *  associated data is completely up to its users.
 *
 *  Cross-enclave. sl_siglines is a cross-enclave data structure (see the
 *  explanation below). The non-enclave code can send signals via an
 *  (untrusted)
 *  sl_siglines object, whose (trusted) cloned object can be used by the
 *  enclave code to receive signals. The other direction is also true.
 *
 *
 * What is cross-enclave data structures?
 *
 * A cross-enclave data structure is a data structure that are designed to be
 * shared by both non-enclave and enclave code, and to be used securely by the
 * enclave code.
 *
 * The life cycle of a cross-enclave data structure is as follows. First, the
 * non-enclave code allocates and initializes an object of the cross-enclave
 * data structure and then passes the pointer of the object to the enclave
 * code. Then, the enclave code creates an trusted object out of the given,
 * untrusted object; this is called "clone". This clone operation will do all
 * proper security checks. Finally, the enclave code can access and manipuate
 * its cloned object securely. 
 *
 */

#include <sl_types.h>
#include <sl_debug.h>
#include <sl_bitops.h>

#pragma pack(push, 1)

/* ID of a signal line */
typedef int_type              sl_sigline_t;

#define NBITS_PER_LINE        ((uint32_t)(sizeof(sl_sigline_t) * 8))

/* ID of a signal line */


#define SL_INVALID_SIGLINE   (uint32_t)(-1)

#define SL_FREE_LINE_INIT   ((sl_sigline_t)(-1))

typedef enum {
    SL_SIGLINES_DIR_T2U,
    SL_SIGLINES_DIR_U2T
} sl_siglines_dir_t;

struct sl_siglines;
typedef void (*sl_sighandler_t)(struct sl_siglines* /*siglines*/,
                                uint32_t /*line*/);

struct sl_siglines {
    sl_siglines_dir_t               direction;
    uint32_t                        num_lines;
    sl_sigline_t*                   event_lines; /* bitmap: 1 - event, 0 - no event  */
    sl_sigline_t*                   free_lines; /* bitmap: 1 - free, 0 - occupied */
    sl_sighandler_t                 handler;
};

#pragma pack(pop)

__BEGIN_DECLS

#if !defined(SL_INSIDE_ENCLAVE) /* Untrusted */


uint32_t sl_siglines_init(struct sl_siglines* sglns,
                          sl_siglines_dir_t direction,
                          uint32_t num_lines,
                          sl_sighandler_t handler);

void sl_siglines_destroy(struct sl_siglines* sglns);

#else /* Trusted */

uint32_t sl_siglines_clone(struct sl_siglines* sglns,
                           const struct sl_siglines* untrusted,
                           sl_sighandler_t handler);

#endif /* SL_INSIDE_ENCLAVE */


static inline uint32_t sl_siglines_size(const struct sl_siglines* sglns) {
    return sglns->num_lines;
}

static inline sl_siglines_dir_t sl_siglines_get_direction(const struct sl_siglines* sglns) {
    return sglns->direction;
}


static inline int is_direction_sender(sl_siglines_dir_t direction)
{
#ifdef SL_INSIDE_ENCLAVE /* trusted */
    return direction == SL_SIGLINES_DIR_T2U;
#else /* untrusted */
    return direction == SL_SIGLINES_DIR_U2T;
#endif
}

static inline int is_direction_receiver(sl_siglines_dir_t direction)
{
#ifdef SL_INSIDE_ENCLAVE /* trusted */
    return direction == SL_SIGLINES_DIR_U2T;
#else /* untrusted */
    return direction == SL_SIGLINES_DIR_T2U;
#endif
}

static inline int is_line_valid(struct sl_siglines* sglns, uint32_t line)
{
    return (line < sglns->num_lines);
}

static inline uint32_t sl_siglines_alloc_line(struct sl_siglines* sglns)
{
    BUG_ON(!is_direction_sender(sglns->direction));

    uint32_t i, max_i = (sglns->num_lines / NBITS_PER_LINE);
    sl_sigline_t* bits_p;

    for (i = 0; i < max_i; i++)
    {
        bits_p = &sglns->free_lines[i];

        int32_t j = extract_one_bit(bits_p);
        if (j < 0) continue;

        uint32_t free_line = NBITS_PER_LINE * i + (uint32_t)j;
        return free_line;

    }

    return SL_INVALID_SIGLINE;
}

static inline void sl_siglines_free_line(struct sl_siglines* sglns, uint32_t line)
{
    BUG_ON(!is_direction_sender(sglns->direction));
    BUG_ON(!is_line_valid(sglns, line));
    uint32_t i = line / NBITS_PER_LINE;
    uint32_t j = line % NBITS_PER_LINE;
    set_bit(&sglns->free_lines[i], j);
}


static inline int sl_siglines_trigger_signal(struct sl_siglines* sglns, uint32_t line)
{
    BUG_ON(!is_direction_sender(sglns->direction));
    BUG_ON(!is_line_valid(sglns, line));
	uint32_t i = line / NBITS_PER_LINE;
	uint32_t j = line % NBITS_PER_LINE;
    set_bit(&sglns->event_lines[i], j);
    return 0;
}

static inline int sl_siglines_revoke_signal(struct sl_siglines* sglns, uint32_t line)
{
    BUG_ON(!is_direction_sender(sglns->direction));
    BUG_ON(!is_line_valid(sglns, line));
	uint32_t i = line / NBITS_PER_LINE;
	uint32_t j = line % NBITS_PER_LINE;
    return test_and_clear_bit(&sglns->event_lines[i], j) == 0;
}

static inline uint32_t sl_siglines_process_signals(struct sl_siglines* sglns)
{
    // function checks bits in array of bitmaps
    // if it detects a 1 bit (which indicates a pending task) and succeeded to change it atomically to 0,
    // the handler function is called
    
    BUG_ON(!is_direction_receiver(sglns->direction));

    uint32_t nprocessed = 0;
    uint32_t i, bit_n, start_bit, end_bit, max_i = (sglns->num_lines / NBITS_PER_LINE);
    sl_sigline_t* bits_p;
    sl_sigline_t  bits_value;

    // for each bitmap in array
    for (i = 0; i < max_i; i++)
    {
        bits_p = &sglns->event_lines[i];
        bits_value = *bits_p;

        if (bits_value != 0)
        {
            // a small optimization to check only "on" bits
			start_bit = count_tailing_zeroes(bits_value);
			end_bit = NBITS_PER_LINE - count_leading_zeroes(bits_value);

            for (bit_n = start_bit; bit_n < end_bit; bit_n++)
            {
                if (unlikely(test_and_clear_bit(bits_p, bit_n) == 0)) continue;

				uint32_t line = NBITS_PER_LINE * i + bit_n;
				sglns->handler(sglns, line);

                nprocessed++;
            }
        }
    }

    return nprocessed;
}


__END_DECLS



#endif /* _SL_SIGLINES_H_ */
