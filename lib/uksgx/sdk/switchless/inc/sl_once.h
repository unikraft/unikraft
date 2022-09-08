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


#ifndef _SL_ONCE_H_
#define _SL_ONCE_H_

/*
 * sl_once - Make sure something (e.g., initialisation) is done only once.
 */

#include <sl_types.h>

#define STATE_NO_CALL       0
#define STATE_CALLING       1
#define STATE_CALLED        2

#define SL_ONCE_INITIALIZER     { STATE_NO_CALL, 0}

typedef struct 
{
    int_type   state;
    int_type   ret;
} sl_once_t;

typedef int_type(*sl_once_func_t)(void* param);

__BEGIN_DECLS

/*
 * Call a function only once for a given sl_once_t object.
 *
 * @param once The point to a sl_once_t object.
 * @param func The function to be called.
 * @return The return value of this function is always the return value of the
 * first call to the given function.
 *
 * This function guarantees thread-safety.
 */
static inline int_type sl_call_once(sl_once_t* once, sl_once_func_t func, void* param)
{
    if (once->state == STATE_NO_CALL)
    {
        if (lock_cmpxchg(&once->state, STATE_NO_CALL, STATE_CALLING) == STATE_NO_CALL)
        {
            once->ret = func(param);
            once->state = STATE_CALLED;
        }
    }
    while (once->state == STATE_CALLING) asm_pause();
    return once->ret;
}


__END_DECLS

#endif /* _SL_ONCE_H_ */
