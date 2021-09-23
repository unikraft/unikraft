#![allow(non_upper_case_globals)]
#![allow(non_camel_case_types)]
#![allow(non_snake_case)]
pub use crate::ffi;

include!(concat!(env!("OUT_DIR"), "/bindings.rs"));
