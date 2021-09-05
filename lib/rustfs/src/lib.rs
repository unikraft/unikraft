#![no_std]
#![feature(alloc_error_handler)]

pub mod api;
pub mod fs;

#[global_allocator]
static ALLOC: alloc_shim::UkAlloc = alloc_shim::UkAlloc;

#[alloc_error_handler]
fn alloc_error(_layout: core::alloc::Layout) -> ! {
    alloc_shim::print("allocation error\n\0");
    #[allow(clippy::empty_loop)]
    loop {}
}
