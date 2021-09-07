#![no_std]
#![feature(alloc_error_handler)]
#[macro_use]
extern crate unikraft;

pub mod api;
pub mod fs;

#[global_allocator]
static ALLOC: unikraft::alloc::UkAlloc = unikraft::alloc::UkAlloc;

use core::panic::PanicInfo;

extern "C" {
    pub fn printf(format: *const u8, ...) -> i32;
}

#[panic_handler]
fn panic(info: &PanicInfo<'_>) -> ! {
    println!("Rust panic: {}", info);
    loop {}
}

#[alloc_error_handler]
fn alloc_error(layout: core::alloc::Layout) -> ! {
    println!(
        "Rust memory allocation error. size: {}, align: {}",
        layout.size(),
        layout.align()
    );
    loop {}
}
