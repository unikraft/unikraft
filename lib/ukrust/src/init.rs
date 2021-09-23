//! Used to register an initializer function that hooks into the boot process
//! Due to rust being compiled to a static library, we have to add an external
//! script that references our initcall;
//!
//! # Example
//! register a new initcall in your library:
//! ```rust
//! register_initcall(_uk_inittab69_ukrust_init, my_init_function);
//! ```
//! An some rust file that is compiled to an object file add:
//! ```rust
//! extern "C" {
//!     pub static __uk_inittab69_ukrust_init: usize;
//! }
//!
//! pub fn use_init() {
//!     unsafe { core::ptr::read_volatile(__uk_inittab69_ukrust_init as *const u8) };
//! }
//! ```

pub type InitFunc = extern "C" fn() -> Result<(), i16>;

#[macro_export]
macro_rules! register_initcall {
    ($name:ident, $function:expr) => {
        mod __initcall {
            pub(super) extern "C" fn wrapper() -> $crate::ffi::c_int {
                match $function {
                    Ok(_) => 0,
                    Err(x) => x as $crate::ffi::c_int,
                }
            }
        }
        #[no_mangle]
        #[link_section = ".uk_inittab"]
        pub static $name: extern "C" fn() -> $crate::ffi::c_int = __initcall::wrapper;
    };
}
