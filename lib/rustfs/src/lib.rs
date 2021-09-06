#![no_std]
#![feature(alloc_error_handler)]

pub mod api;
pub mod fs;

#[global_allocator]
static ALLOC: ukalloc::UkAlloc = ukalloc::UkAlloc;

use core::panic::PanicInfo;

extern "C" {
    pub fn printf(format: *const u8, ...) -> i32;
}

#[panic_handler]
fn panic(_panic: &PanicInfo<'_>) -> ! {
    unsafe { printf("Rust panic\0\n".as_ptr()) };
    loop {}
}

#[alloc_error_handler]
fn alloc_error(_layout: core::alloc::Layout) -> ! {
    unsafe { printf("Rust memory allocation error\0\n".as_ptr()) };
    loop {}
}
