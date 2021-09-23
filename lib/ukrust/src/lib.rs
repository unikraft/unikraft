#![no_std]
#![allow(unused_unsafe)]

pub mod alloc;
pub mod init;
pub mod log;
#[cfg(feature = "log")]
pub use unikraft_sys::ffi;

#[cfg(feature = "log")]
static LOGGER: log::klog::KernelLog = log::klog::KernelLog;

fn init_ukrust() -> Result<(), i16> {
    eprintln!("Hello from rust!!");
    #[cfg(feature = "log")]
    log::set_logger(&LOGGER);
    Ok(())
}

register_initcall!(__uk_inittab69_ukrust_init, crate::init_ukrust());
