#![no_std]
#![allow(unused_unsafe)]
#![allow(non_camel_case_types)]
pub mod sys;

pub use sys::*;

#[cfg(any(target_arch = "x86_64", target_arch = "arm64"))]
pub mod ffi {
    pub type c_char = i8;
    pub type c_schar = i8;
    pub type c_uchar = u8;
    pub type c_short = i16;
    pub type c_ushort = u16;
    pub type c_int = i32;
    pub type c_uint = u32;
    pub type c_long = i64;
    pub type c_ulong = u64;
    pub type c_longlong = i64;
    pub type c_ulonglong = u64;
    pub type c_void = usize;
}
#[cfg(any(target_arch = "x86", target_arch = "arm"))]
pub mod ffi {
    pub type c_char = i8;
    pub type c_schar = i8;
    pub type c_uchar = u8;
    pub type c_short = i16;
    pub type c_ushort = u16;
    pub type c_int = i32;
    pub type c_uint = u32;
    pub type c_long = i32;
    pub type c_ulong = u32;
    pub type c_longlong = i64;
    pub type c_ulonglong = u64;
    pub type c_void = usize;
}
