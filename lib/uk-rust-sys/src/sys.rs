#[allow(non_camel_case_types)]
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
    pub type c_long = i64;
    pub type c_ulong = u64;
    pub type c_void = usize;
}

include!(concat!(env!("OUT_DIR"), "/bindings.rs"));
