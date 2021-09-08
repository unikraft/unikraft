#![no_std]
#![feature(alloc_error_handler)]
#[macro_use]
extern crate unikraft;

pub mod api;
pub mod fs;

#[global_allocator]
static ALLOC: unikraft::alloc::UkAlloc = unikraft::alloc::UkAlloc;

use core::panic::PanicInfo;

#[panic_handler]
fn panic(info: &PanicInfo<'_>) -> ! {
    unikraft::log::panic_handler(info);
    loop {}
}

#[alloc_error_handler]
fn alloc_error(layout: core::alloc::Layout) -> ! {
    println!(
        "Rust memory allocation error. size: {}, align: {}",
        layout.size(),
        layout.align()
    );
    #[allow(clippy::clippy::empty_loop)]
    loop {}
}
