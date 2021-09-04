#![no_std]
#![feature(alloc_error_handler)]

pub mod api;
pub mod fs;

#[global_allocator]
static ALLOC: wee_alloc::WeeAlloc = wee_alloc::WeeAlloc::INIT;

use core::panic::PanicInfo;

#[no_mangle]
pub unsafe extern "C" fn hello_rust() {
    printf("Hi!\n\0".as_ptr());
}

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
    loop {}
}
