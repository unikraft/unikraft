#![no_std]
#![feature(libc)]
use core::panic::PanicInfo;

extern "C" {
    pub fn printf(format: *const u8, ...) -> i32;
}

pub fn do_panic() -> ! {
    panic!("test")
}

#[panic_handler]
fn panic(_panic: &PanicInfo<'_>) -> ! {
    unsafe { printf("Rust panic\0\n".as_ptr()) };
    loop {}
}
