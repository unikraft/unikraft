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

#ifndef _SGX_RANDOM_BUFFERS_H_
#define _SGX_RANDOM_BUFFERS_H_

#if !defined(__cplusplus) || __cplusplus < 201100L
#   error This header needs c++11 or later
#endif

#include <utility>
#include <string.h>
#include <stdlib.h>

#if defined(MAXIMAL_CALLSTACK)
extern int EDMM_supported;
#endif

/*
 * This function is equivalent to __builtin_rdrand16/32/64_step() except it
 * returns the random value instead of writing it to memory, which is required
 * in buffer randomization
 */
template <class R = unsigned>
inline R rdrand(void)
{
    R r;
    __asm__ volatile (
        "1: rdrand  %0\n"
        "   jnc     1b\n"
        : "=r" (r)
    );

    return r;

}

/*
 * To invoke somefunc(arg1, arg2,...), use
 *  + random_stack_advance(somefunc, arg1, args2,...)
 * By default, the maximal space to advance is 0x1000 bytes (1 page), which
 * could be customized using the 1st template argument, e.g.
 *  + random_stack_advance<0x2000>(somefunc, arg1, arg2,...)
 * advances the stack randomly between 1 and 0x2000 bytes before calling
 * somefunc(). In practice, the compiler will align the stack so the actual
 * number of possible advances depends on calling convention.
 *
 * Please note that optimizer may inline f, which would make dummy_vla
 * ineffective. Hence it is recommended to declare f with [[gnu::noinline]]
 * attribute.
 */
template <class R, class... Ps, class... Qs>
__attribute__((noinline)) static R _random_stack_noinline_wrapper(R(*f)(Ps...), Qs&&... args)
{
    return f(std::forward<Qs>(args)...);
}
template <unsigned M = 0x1000, class R, class... Ps, class... Qs>
R random_stack_advance(R(*f)(Ps...), Qs&&... args)
{
#if defined(MAXIMAL_CALLSTACK)
    unsigned size = M;
#else
    unsigned size = rdrand() % M + 1;
#endif
    volatile char dummy_vla[size];

#if defined(MAXIMAL_CALLSTACK)
    if (!EDMM_supported)
        memset((void *)dummy_vla, 0, size);
#else
    (void)(dummy_vla);
#endif

    return _random_stack_noinline_wrapper(f, std::forward<Qs>(args)...);
}

template <class T>
inline T *random_integers(T *ptr, std::size_t size)
{
    if (size % alignof(T))
        return nullptr;

    size /= alignof(T);
    while (size > 0)
        ptr[--size] = rdrand<T>();
    return ptr;
}

template <class T>
inline T *random_fill(T *ptr, std::size_t size)
{
    if (alignof(T) >= sizeof(std::size_t))
        return (T*)random_integers(reinterpret_cast<std::size_t*>(ptr), size);
    if (alignof(T) == sizeof(int))
        return (T*)random_integers(reinterpret_cast<int*>(ptr), size);
    if (alignof(T) == sizeof(short))
        return (T*)random_integers(reinterpret_cast<short*>(ptr), size);

    while (size > 0)
        ((char*)ptr)[--size] = char(rdrand());
    return ptr;
}

/*
 * randomly_placed_buffer<T, M> randomly picks an address in a bigger buffer to
 * store an object (or objects) of T.
 */
template <class T, std::size_t A, unsigned M = 0x1000>
struct alignas(A)randomly_placed_buffer
{
    static constexpr std::size_t size(std::size_t count = 1)
    {
        //
        // gives error with gcc 4.8
        //
        //return sizeof(randomly_placed_buffer<T, A, M>) + sizeof(T) * (count - 1);
        return sizeof(T) * count + M - A;
    }

    randomly_placed_buffer<T, A, M>& wipe(std::size_t count = 1)
    {
        return *reinterpret_cast<decltype(this)>(memset(__bigger_, 0, size(count)));
    }

    ~randomly_placed_buffer(void)
    {
        wipe();
    }

    randomly_placed_buffer<T, A, M>& reset(std::size_t count = 1)
    {
        return wipe(count);
    }

    // randomize_object() returns a pointer to an uninitialized object(s). It is
    // used for objects without a constructor.
    T *randomize_object(std::size_t count = 1)
    {
        return (T*)(reset(count).__bigger_ + ((rdrand() % M) & ~(A - 1)));
    }

    // instantiate_object() invokes T's constructor on the object returned by
    // randomize_object()
    //
    // Please note that randomly_placed_buffer<T, M> doesn't keep track of the
    // contained object address to avoid storing it in memory.
    template <class... Ps>
    T *instantiate_object(Ps&&... args)
    {
        auto *t = new(randomize_object()) _T_instantiator_(std::forward<Ps>(args)...);
        return &t->__inst;
    }

    // instantiate_array() invokes T's constructor on the object returned by
    // randomize_object(count).
    //
    // Care must be taken to allocate the underlying buffer explicitly because
    // __bigger_ is big enough for only 1 object.
    // T must be default constructible.
    T *instantiate_array(std::size_t count)
    {
        auto *t = new(randomize_object(count)) _T_instantiator_[count];
        return &t->__inst;
    }

    // destroy() invokes the destructor the object of array of objects.
    // ptr must be the same value returned by either instantiate_* functions.
    // count must be the same value passed to previous instantiate_array().
    std::size_t destroy(T *ptr, std::size_t count = 1)
    {
        if ((char*)ptr >= __bigger_ &&
            (char*)(ptr + 1) <= __bigger_ + sizeof(__bigger_))
            while (count > 0) delete reinterpret_cast<_T_instantiator_*>(ptr + --count);
        return count;
    }

    // This overload requires global operator new() support from libc++
    // If dependence on libc++ is not wanted, use the placement allocation
    // version (below) instead
    static void *operator new (std::size_t, std::size_t count = 1, unsigned nbufs = 16)
    {
        auto sz = size(count);
        void **ppv[nbufs];

        // Buffers returned by malloc() are suitable for all scalar types, thus
        // adjustment is needed only when A is larger.
        if (A > alignof(long double))
            sz += A;

        for (auto i = nbufs; i > 0;)
        {
            ppv[--i] = reinterpret_cast<void**>(malloc(sz));
            auto q = reinterpret_cast<char*>(ppv[i]);
            for (auto r = q + sz; q < r; q += 0x1000)
                *q = '\0';
        }

        auto j = rdrand() % nbufs;
        for (auto i = nbufs; i > 0;)
            if (--i != j)
                free(ppv[i]);

        if (A > alignof(long double))
        {
            auto a = reinterpret_cast<std::size_t>(ppv[j]);
            a += A;
            a &= ~(A - 1);

            auto t = ppv[j];
            ppv[j] = reinterpret_cast<void**>(a);
            ppv[j][-1] = t;
        }
        return ppv[j];
    }

        // This placement allocation overload instantiates
        // randomly_placed_buffer<T, A, M> on a buffer whose address is given by
        // the caller. The buffer may be obtained via malloc() or alloca() functions
        static void *operator new (std::size_t, void *ptr)
    {
        return ptr;
    }

        static void operator delete (void *ptr)
    {
        if (A > alignof(long double))
            ptr = (reinterpret_cast<void**>(ptr))[-1];
        free(ptr);
    }

private:
    struct alignas(A)_T_instantiator_
    {
        T __inst;

        template <class... Ps>
        _T_instantiator_(Ps&&... args) : __inst(std::forward<Ps>(args)...)
        {
        }

        static void *operator new (std::size_t, void *ptr)
        {
            return ptr;
        }

            static void *operator new[](std::size_t, void *ptr)
        {
            return ptr;
        }

            static void operator delete (void*)
        {
        }
    };

    char __bigger_[size()];
};

template <class T, unsigned M = 0x1000>
using randomly_placed_object = randomly_placed_buffer<T, alignof(T), M>;

#endif
