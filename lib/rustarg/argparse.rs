#![no_std]
#![feature(option_result_unwrap_unchecked)]
#![allow(unused_unsafe)]
use core::ops::{Index, IndexMut};
use core::panic::PanicInfo;

unsafe fn left_shift(mut buf: CharPtr, mut index: usize, max_len: usize) {
    while buf[index] != 0 && index < max_len {
        buf[index] = buf[index + 1];
        index += 1;
    }
}
#[derive(Copy, Clone)]
#[repr(transparent)]
pub struct CharPtr(*mut u8);

impl Index<usize> for CharPtr {
    type Output = u8;
    fn index(&self, index: usize) -> &Self::Output {
        unsafe { self.0.add(index).as_ref().unwrap_unchecked() }
    }
}

impl IndexMut<usize> for CharPtr {
    fn index_mut(&mut self, index: usize) -> &mut Self::Output {
        unsafe { self.0.add(index).as_mut().unwrap_unchecked() }
    }
}

/// # Safety
/// This function is not safe
/// like not at all
/// it is basically c code
#[no_mangle]
pub unsafe extern "C" fn uk_argnparse(
    mut argument_buffer: CharPtr,
    max_len: usize,
    indices: *mut CharPtr,
    max_count: i32,
) -> i32 {
    unsafe {
        let mut argc = 0;
        let mut prev_wspace = true;
        let mut in_quote = 0;
        let mut i = 0;
        while i < max_len && argc < max_count {
            match argument_buffer[i] {
                /* end of string */
                0 => break,

                /* white spaces */
                9..=12 | 13 | 32 => {
                    if in_quote == 0 {
                        argument_buffer[i] = 0;
                        prev_wspace = true;
                    }
                }

                /* quotes */
                34 | 39 if in_quote == argument_buffer[i] => {
                    in_quote = 0;
                    left_shift(argument_buffer, i, max_len);
                    i -= 1;
                }
                34 | 39 if in_quote == 0 => {
                    in_quote = argument_buffer[i];
                    left_shift(argument_buffer, i, max_len);
                    i -= 1;
                }

                _ => {
                    if prev_wspace {
                        *indices.offset(argc as isize) = CharPtr(argument_buffer.0.add(i));
                        argc += 1;
                        prev_wspace = false;
                    }
                }
            }
            i += 1;
        }
        argc
    }
}

#[panic_handler]
fn panic(_panic: &PanicInfo<'_>) -> ! {
    loop {}
}
