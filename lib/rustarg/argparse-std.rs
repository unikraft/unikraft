#![no_std]
use core::panic::PanicInfo;
use core::{slice, str, str::Utf8Error};

fn cs_to_slice(base: *mut u8, maxlen: usize) -> Result<&'static mut str, Utf8Error> {
    let slice = unsafe {
        let len = (base as usize..).position(|c| *(c as *const u8) == 0);
        slice::from_raw_parts_mut(base, len.unwrap_or(maxlen))
    };
    str::from_utf8_mut(slice)
}

#[no_mangle]
pub extern "C" fn uk_argnparse(
    argument_buffer: *mut u8,
    max_len: usize,
    indices: *mut *mut u8,
    max_count: i32,
) -> i32 {
    let mut string = match cs_to_slice(argument_buffer, max_len) {
        Ok(s) => s,
        _ => return -1,
    };
    let mut arguments = unsafe { slice::from_raw_parts_mut(indices, max_count as usize) };
    let max_count = max_count as usize;
    let mut arg_count = 0;
    let mut base_ptr = argument_buffer;
    let mut quote = None;
    let mut previous_whitespace = true;
    let is_quote = |c: char| c == '\'' || c == '"';
    for c in string.chars() {
        match (c, quote, previous_whitespace) {
            (c, None, _) if c.is_whitespace() => {
                for i in 0..c.len_utf8() {
                    unsafe { *base_ptr.offset(i as isize) = 0 };
                }
            }
            (c, None, _) if is_quote(c) => {
                quote = Some(c);
            }
            (c, Some(q), _) if c == q => {
                quote = None;
            }
            ('\0', _, _) => return arg_count as i32,
            (_, _, true) if arg_count < max_count => {
                arg_count += 1;
                arguments[arg_count] = base_ptr;
                previous_whitespace = false;
            }
            _ => (),
        }
        unsafe { base_ptr.offset(c.len_utf8() as isize) };
    }
    0
}

#[panic_handler]
fn panic(_panic: &PanicInfo<'_>) -> ! {
    loop {}
}
