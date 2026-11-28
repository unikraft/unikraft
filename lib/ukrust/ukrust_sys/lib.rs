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
#![no_std]
#![feature(
    allocator_api,
    alloc_error_handler,
    associated_type_defaults,
    const_mut_refs,
    lang_items,
    receiver_trait,
)]
use core;

mod allocator;
pub mod bindings;
pub mod c_types;

#[alloc_error_handler]
#[no_mangle]
fn on_oom(_layout: core::alloc::Layout) -> ! {
    panic!("Alloc error");
}

extern "C" {
    fn __ukrust_sys_crash() -> !;
}

#[panic_handler]
#[no_mangle]
pub fn panic(_info: &core::panic::PanicInfo<'_>) -> ! {
    unsafe {
        __ukrust_sys_crash();
    }
}

#[lang = "eh_personality"]
#[no_mangle]
pub extern fn rust_eh_personality() {}

#[no_mangle]
unsafe extern "C" fn _Unwind_Resume() {
}

