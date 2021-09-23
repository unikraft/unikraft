//! Implement the rust print!, println!, printk! and printlnk!
//! macros to write to the unikraft kernel console
use core::panic::PanicInfo;
#[cfg(feature = "log")]
pub use log::*;
use unikraft_sys::{ukplat_coutd, ukplat_coutk};

pub struct UkCoutD;

impl core::fmt::Write for UkCoutD {
    #[inline]
    fn write_str(&mut self, msg: &str) -> core::fmt::Result {
        unsafe {
            match ukplat_coutd(msg.as_ptr() as *const i8, msg.len() as u32) {
                x if x < 0 => Err(core::fmt::Error::default()),
                _ => Ok(()),
            }
        }
    }
}

pub struct UkCoutK;

impl core::fmt::Write for UkCoutK {
    #[inline]
    fn write_str(&mut self, msg: &str) -> core::fmt::Result {
        unsafe {
            match ukplat_coutk(msg.as_ptr() as *const i8, msg.len() as u32) {
                x if x < 0 => Err(core::fmt::Error::default()),
                _ => Ok(()),
            }
        }
    }
}

/// Macro for printing to the standard output, with a newline.
///
/// Does not panic on failure to write - instead silently ignores errors.
///
/// See [`println!`](https://doc.rust-lang.org/std/macro.println.html) for
/// full documentation.
#[macro_export]
macro_rules! println {
    () => { $crate::println!("") };
    ($($arg:tt)*) => {
        #[allow(unused_must_use)]
        {
            let stm = &mut $crate::log::UkCoutD;
            core::fmt::Write::write_fmt(stm, format_args!($($arg)*));
            use core::fmt::Write;
            stm.write_str("\n");
        }
    };
}

/// Macro for printing to the standard output.
///
/// Does not panic on failure to write - instead silently ignores errors.
///
/// See [`print!`](https://doc.rust-lang.org/std/macro.print.html) for
/// full documentation.
#[macro_export]
macro_rules! print {
    ($($arg:tt)*) => {
        #[allow(unused_must_use)]
        {
            let stm = &mut $crate::log::UkCoutD;
            core::fmt::Write::write_fmt(stm, format_args!($($arg)*));
        }
    };
}

/// Macro for printing to the standard error, with a newline.
///
/// Does not panic on failure to write - instead silently ignores errors.
///
/// See [`eprintln!`](https://doc.rust-lang.org/std/macro.eprintln.html) for
/// full documentation.
#[macro_export]
macro_rules! eprintln {
    () => { $crate::eprintln!("") };
    ($($arg:tt)*) => {
        #[allow(unused_must_use)]
        {
            let stm = &mut $crate::log::UkCoutK;
            core::fmt::Write::write_fmt(stm, format_args!($($arg)*));
            use core::fmt::Write;
            stm.write_str("\n");
        }
    };
}

/// Macro for printing to the standard error.
///
/// Does not panic on failure to write - instead silently ignores errors.
///
/// See [`eprint!`](https://doc.rust-lang.org/std/macro.eprint.html) for
/// full documentation.
#[macro_export]
macro_rules! eprint {
    ($($arg:tt)*) => {
        #[allow(unused_must_use)]
        {
            let stm = &mut $crate::log::UkCoutK;
            core::fmt::Write::write_fmt(stm, format_args!($($arg)*));
        }
    };
}

#[cfg(feature = "log")]
pub mod klog {
    pub fn set() {
        let _ = unsafe { log::set_logger_racy(&KernelLog) };
        log::set_max_level(log::LevelFilter::Trace);
    }

    #[derive(Default)]
    pub struct KernelLog;

    impl log::Log for KernelLog {
        fn enabled(&self, _metadata: &log::Metadata) -> bool {
            true
        }

        fn log(&self, record: &log::Record) {
            use super::{UkCoutD, UkCoutK};

            let (debug, name): (bool, &str) = match record.level() {
                log::Level::Trace => (true, "trace"),
                log::Level::Debug => (true, "debug"),
                log::Level::Info => (false, "info"),
                log::Level::Warn => (false, "warn"),
                log::Level::Error => (false, "error"),
            };
            use core::fmt::Write;
            let _ = match debug {
                true => UkCoutD.write_fmt(format_args!("{}:\t{}\n", name, record.args())),
                false => UkCoutK.write_fmt(format_args!("{}:\t{}\n", name, record.args())),
            };
        }

        fn flush(&self) {}
    }
}

pub fn panic_handler(info: &PanicInfo<'_>) {
    let (file, line, column) = if let Some(loc) = info.location() {
        (loc.file(), loc.line() as i32, loc.column() as i32)
    } else {
        ("<unknown>", 0, 0)
    };
    let payload = info.payload().downcast_ref::<&str>().unwrap_or(&"");
    eprintln!("{} {}:{}/{}", payload, file, line, column,)
}
