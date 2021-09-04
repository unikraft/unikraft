use core::panic::PanicInfo;
use core::str;


fn cs_to_slice(base: *const u8, maxlen) -> &mut'static str {
    unsafe {
        let len = (base as usize..).position(|c| *(c as *const u8) == 0);
        let slice = core::slice::from_raw_parts(base, len.unwrap_or(maxlen));
        core::str::from_utf8(slice).expect("Passed not utf-8 string")
    }
}


fn uk_argparse(argument_buffer: const* char, max_len, usize, indices: const* mut* char, max_count: u32) -> i32 {
    let args = cs_to_slice(base, maxlen);
    0
}
/*

static void left_shift(char *buf, __sz index, __sz maxlen)
{
	while(buf[index] != '\0' && index < maxlen) {
		buf[index] = buf[index + 1];
		index++;
	}
}

int uk_argnparse(char *argb, __sz maxlen, char *argv[], int maxcount)
{
	int argc = 0;
	int prev_wspace = 1;
	char in_quote = '\0';
	__sz i;

	UK_ASSERT(argb != __NULL);
	UK_ASSERT(argv != __NULL);
	UK_ASSERT(maxcount >= 0);

	for (i = 0; i < maxlen && argc < maxcount; ++i) {
		switch (argb[i]) {
		/* end of string */
		case '\0':
			goto out;

		/* white spaces */
		case ' ':
		case '\r':
		case '\n':
		case '\t':
		case '\v':
			if (!in_quote) {
				argb[i] = '\0';
				prev_wspace = 1;
			}
			break;

		/* quotes */
		case '\'':
		case '"':
			if (!in_quote) {
				in_quote = argb[i];
				left_shift(argb, i, maxlen);
				--i;
				break;
			}
			if (in_quote == argb[i]) {
				in_quote = '\0';
				left_shift(argb, i, maxlen);
				--i;
				break;
			}

			/* Fall through */
		default:
			/* any character */
			if (prev_wspace) {
				argv[argc++] = &argb[i];
				prev_wspace = 0;
			}
			break;
		}
	}

out:
	return argc;
}

fn panic(_info: &PanicInfo) -> ! {
    loop {}
}
*/
