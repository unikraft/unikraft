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

#include <utility>
#include <string.h>
#include <stdlib.h>
#include <se_cdefs.h>

extern "C" void* _alloca(size_t);
extern "C" unsigned do_rdrand_reg(void);

extern SE_DECLSPEC_EXPORT bool g_checked_by_emmt;

template <class R, class... Ps, class... Qs>
__declspec(noinline) static R _random_stack_noinline_wrapper(R(*f)(Ps...), Qs&&... args)
{
    return f(std::forward<Qs>(args)...);
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
 */

template <unsigned M = 0x1000, class R, class... Ps, class... Qs>
R random_stack_advance(R(*f)(Ps...), Qs&&... args)
{
#if defined(MAXIMAL_CALLSTACK) 

    volatile char dummy[M];
    if (g_checked_by_emmt) {
        memset_s((void*)dummy, sizeof(dummy), 0, sizeof(dummy));
    }

#elif defined(_WIN64)

    if (g_checked_by_emmt) {
        std::size_t size_advance = do_rdrand_reg() % M + 1;
        volatile char* pdummy = (volatile char*)_alloca(size_advance);
        //
        // touch this part of the stack for measurement purposes
        //
        memset_s((void*)pdummy, size_advance, 0, size_advance);
    }
    else {
        volatile char* pdummy = (volatile char*)_alloca(do_rdrand_reg() % M + 1);
        pdummy;

    }



#endif

    //return f(std::forward<Qs>(args)...);
    return _random_stack_noinline_wrapper(f, std::forward<Qs>(args)...);
}

/*
 * randomly_placed_buffer<T, M> randomly picks an address in a bigger buffer to
 * store an object (or objects) of T.
 */

template <class T, std::size_t A, unsigned M = 0x1000>
struct alignas(A) randomly_placed_buffer
{
    static constexpr std::size_t size(std::size_t count = 1)
    {
        return sizeof(randomly_placed_buffer<T, A, M>) + sizeof(T) * (count - 1);
    }

    randomly_placed_buffer<T, A, M>& wipe(std::size_t count = 1)
    {
        return *reinterpret_cast<decltype(this)>(memset(__bigger_, 0, size(count)));
    }

    randomly_placed_buffer<T, A, M>& reset(std::size_t count = 1)
    {
        return wipe(count);
    }

    // randomize_object() returns a pointer to an uninitialized object(s). It is
    // used for objects without a constructor.
    T *randomize_object(std::size_t count = 1)
    {
        return (T*)(reset(count).__bigger_ + ((do_rdrand_reg() % M) & ~(A - 1)));
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

        void*** pppv = (void***)malloc(nbufs * sizeof(void**));
        if (!pppv) return nullptr;

        // Buffers returned by malloc() are suitable for all scalar types, thus
        // adjustment is needed only when A is larger.
        if (A > alignof(long double))
            sz += A;

        for (auto i = nbufs; i > 0;)
        {
            pppv[--i] = reinterpret_cast<void**>(malloc(sz));
            auto q = reinterpret_cast<char*>(pppv[i]);
            for (auto r = q + sz; q < r; q += 0x1000)
                *q = '\0';
        }

        auto j = do_rdrand_reg() % nbufs;
        for (auto i = nbufs; i > 0;)
            if (--i != j)
                free(pppv[i]);

        if (A > alignof(long double))
        {
            auto a = reinterpret_cast<std::size_t>(pppv[j]);
            a += A;
            a &= ~(A - 1);

            auto t = pppv[j];
            pppv[j] = reinterpret_cast<void**>(a);
            pppv[j][-1] = t;
        }
        void** randomBuf = pppv[j];
        free(pppv);
        return randomBuf;
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

    template <unsigned C = 1>
    using storage = char alignas(A)[size(C)];

private:
    struct alignas(A) _T_instantiator_
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

    static_assert(M >= A, "Randomization range, M, cannot be smaller than alignment, A.");

    char __bigger_[sizeof(T) + M - A];
};

template <class T, unsigned M = 0x1000>
using randomly_placed_object = randomly_placed_buffer<T, alignof(T), M>;

#endif
