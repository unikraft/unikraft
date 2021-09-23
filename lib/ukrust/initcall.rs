//! Forces the usage of this symbol in the final binary, otherwise it will be optimized away

extern "C" {
    pub static __uk_inittab69_ukrust_init: usize;
}

pub fn use_init() {
    unsafe { core::ptr::read_volatile(__uk_inittab69_ukrust_init as *const u8) };
}
