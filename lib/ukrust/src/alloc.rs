use core::alloc::{GlobalAlloc, Layout};
use unikraft_sys::sys;

/// USAGE:
/// '''rust
/// #[global_allocator]
/// static A: UkAlloc = UkAlloc;
/// ```
pub struct UkAlloc;

unsafe impl GlobalAlloc for UkAlloc {
    unsafe fn alloc(&self, layout: Layout) -> *mut u8 {
        let align = layout.align().max(core::mem::size_of::<*mut u8>());
        unsafe {
            let alloc = sys::_uk_alloc_head;
            sys::uk_memalign_compat(alloc, align as u64, layout.size() as u64) as *mut u8
        }
    }
    unsafe fn dealloc(&self, ptr: *mut u8, _layout: Layout) {
        unsafe {
            let alloc = sys::_uk_alloc_head;
            sys::uk_free_ifpages(alloc, ptr as *mut sys::ffi::c_void)
        }
    }
}
