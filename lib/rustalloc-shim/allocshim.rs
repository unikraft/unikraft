#![no_std]
#![allow(unused_unsafe)]

use core::panic::PanicInfo;

use core::alloc::{GlobalAlloc, Layout};

/// USAGE:
/// '''rust
/// #[global_allocator]
/// static A: UkAlloc = UkAlloc;
/// ```
pub struct UkAlloc;

extern "C" {
    pub fn rust_memalign(align: usize, size: usize) -> *mut u8;
    pub fn rust_free(ptr: *mut u8);
    pub fn printf(format: *const u8, ...) -> i32;
}

unsafe impl GlobalAlloc for UkAlloc {
    unsafe fn alloc(&self, layout: Layout) -> *mut u8 {
        let align = layout.align().max(core::mem::size_of::<*mut u8>());
        unsafe { rust_memalign(align, layout.size()) }
    }
    unsafe fn dealloc(&self, ptr: *mut u8, _layout: Layout) {
        unsafe { rust_free(ptr) }
    }
}

pub fn print(message: &str) {
    unsafe { printf(message.as_ptr()) };
}

#[panic_handler]
fn panic(_panic: &PanicInfo<'_>) -> ! {
    unsafe { printf("The rust code panicked!\n\0".as_ptr()) };
    loop {}
}
