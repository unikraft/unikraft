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

use core::alloc::{GlobalAlloc, Layout};
use crate::bindings;
use crate::c_types;

pub struct UkAlloc;

unsafe impl GlobalAlloc for UkAlloc {
    unsafe fn alloc(&self, layout: Layout) -> *mut u8 {
        unsafe { bindings::__ukrust_sys_malloc(layout.size() as u64) as *mut u8 }
    }

    unsafe fn dealloc(&self, ptr: *mut u8, _layout:Layout) {
        unsafe { bindings::__ukrust_sys_free(ptr as *mut c_types::c_void); }
    }
}

#[global_allocator]
static ALLOCATOR: UkAlloc = UkAlloc;

#[no_mangle]
static __rust_no_alloc_shim_is_unstable: u8 = 1;

#[no_mangle]
pub fn __rust_alloc_error_handler(_size: usize, _align: usize) -> ! {
    panic!("Alloc error handler");
}

#[no_mangle]
static __rust_alloc_error_handler_should_panic: u8 = 1;
