/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Vlad-Andrei Badoiu <vlad_andrei.badoiu@upb.ro>
 *
 * Copyright (c) 2021, University Politehnica of Bucharest. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

#![feature(compiler_builtins)]
#![compiler_builtins]
#![no_builtins]
#![no_std]

macro_rules! define_panicking_intrinsics(
    ($reason: tt, { $($ident: ident, )* }) => {
        $(
            #[doc(hidden)]
            #[no_mangle]
            pub extern "C" fn $ident() {
                panic!($reason);
            }
        )*
    }
);

/* TODO
 * In order to build the core crate, we need some intrinsics that are
 * normally define either in compiler-rt or a special rust crate, compiler_builtins.
 * Since we never use them in this context, we use the define_panicking_intrinsics
 * macro to define them
 * */
define_panicking_intrinsics!("`f32` should not be used", {
    __addsf3,
    __addsf3vfp,
    __aeabi_fcmpeq,
    __aeabi_ul2f,
    __divsf3,
    __divsf3vfp,
    __eqsf2,
    __eqsf2vfp,
    __fixsfdi,
    __fixsfsi,
    __fixsfti,
    __fixunssfdi,
    __fixunssfsi,
    __fixunssfti,
    __floatdisf,
    __floatsisf,
    __floattisf,
    __floatundisf,
    __floatunsisf,
    __floatuntisf,
    __gesf2,
    __gesf2vfp,
    __gtsf2,
    __gtsf2vfp,
    __lesf2,
    __lesf2vfp,
    __ltsf2,
    __ltsf2vfp,
    __mulsf3,
    __mulsf3vfp,
    __nesf2,
    __nesf2vfp,
    __powisf2,
    __subsf3,
    __subsf3vfp,
    __unordsf2,
});

define_panicking_intrinsics!("`f64` should not be used", {
    __adddf3,
    __adddf3vfp,
    __aeabi_dcmpeq,
    __aeabi_ul2d,
    __divdf3,
    __divdf3vfp,
    __eqdf2,
    __eqdf2vfp,
    __fixdfdi,
    __fixdfsi,
    __fixdfti,
    __fixunsdfdi,
    __fixunsdfsi,
    __fixunsdfti,
    __floatdidf,
    __floatsidf,
    __floattidf,
    __floatundidf,
    __floatunsidf,
    __floatuntidf,
    __gedf2,
    __gedf2vfp,
    __gtdf2,
    __gtdf2vfp,
    __ledf2,
    __ledf2vfp,
    __ltdf2,
    __ltdf2vfp,
    __muldf3,
    __muldf3vfp,
    __nedf2,
    __nedf2vfp,
    __powidf2,
    __subdf3,
    __subdf3vfp,
    __unorddf2,
});

define_panicking_intrinsics!("`i128` should not be used", {
    __ashrti3,
    __muloti4,
    __multi3,
});

define_panicking_intrinsics!("`u128` should not be used", {
    __ashlti3,
    __lshrti3,
    __udivmodti4,
    __udivti3,
    __umodti3,
});

#[cfg(target_arch = "arm")]
define_panicking_intrinsics!("`u64` division/modulo should not be used", {
    __aeabi_uldivmod,
    __mulodi4,
});
