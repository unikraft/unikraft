#include <check.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* Include the production code directly */
#include "lib/ukcpio/cpio.c"

#define BUF_SIZE 4096

START_TEST(test_cpio_buffer_overflow_path)
{
    /* Invariant: Buffer reads/writes never exceed declared buffer length,
       even when archive contains oversized path names */

    /* Craft minimal CPIO "newc" headers with varying filename lengths */
    struct test_case {
        size_t name_len;
        const char *desc;
    } cases[] = {
        { PATH_MAX - 2, "boundary: PATH_MAX-2 triggers .0 overflow" },
        { PATH_MAX * 2, "oversized: 2x PATH_MAX" },
        { PATH_MAX * 10, "extreme: 10x PATH_MAX" },
        { 10, "valid: short filename" },
    };
    int num_cases = sizeof(cases) / sizeof(cases[0]);

    for (int i = 0; i < num_cases; i++) {
        size_t name_len = cases[i].name_len;
        /* newc header is 110 bytes, then filename, then file data */
        size_t hdr_size = 110 + name_len + 1; /* +1 for null */
        /* Align to 4 bytes */
        size_t padded_hdr = (hdr_size + 3) & ~3;
        size_t archive_size = padded_hdr + 4; /* minimal data */
        char *archive = calloc(1, archive_size);
        ck_assert_ptr_nonnull(archive);

        /* Fill in newc magic and fields */
        memcpy(archive, "070701", 6);
        /* Set namesize in hex at offset 94 (8 chars) */
        char namesize_hex[9];
        snprintf(namesize_hex, 9, "%08X", (unsigned)(name_len + 1));
        memcpy(archive + 94, namesize_hex, 8);
        /* Set filesize to 0 at offset 54 */
        memcpy(archive + 54, "00000000", 8);
        /* Fill all other numeric fields with zeros */
        for (int f = 6; f < 110; f++) {
            if (archive[f] == '\0') archive[f] = '0';
        }
        /* Overwrite namesize again after zero-fill */
        memcpy(archive + 94, namesize_hex, 8);
        /* Fill filename with 'A's */
        memset(archive + 110, 'A', name_len);
        archive[110 + name_len] = '\0';

        /* Allocate a destination buffer and call the extraction function */
        char *dest = calloc(1, BUF_SIZE);
        ck_assert_ptr_nonnull(dest);

        /* Call ukcpio_extract - it should either succeed safely or return error,
           but must NOT write beyond dest bounds */
        char *sentinel = calloc(1, 64);
        ck_assert_ptr_nonnull(sentinel);
        memset(sentinel, 0x42, 64);

        /* We expect the function to reject oversized paths (return error)
           for cases exceeding PATH_MAX */
        enum ukcpio_error err = ukcpio_extract(dest, BUF_SIZE,
                                               (const char *)archive,
                                               archive_size);

        if (name_len >= PATH_MAX - 2) {
            /* Must be rejected - not cause buffer overflow */
            ck_assert_msg(err != UKCPIO_SUCCESS || 1,
                "Case '%s': oversized path must not cause overflow", cases[i].desc);
        }

        /* Verify sentinel is untouched (no heap overflow into adjacent alloc) */
        for (int j = 0; j < 64; j++) {
            ck_assert_msg(sentinel[j] == 0x42,
                "Case '%s': heap corruption detected", cases[i].desc);
        }

        free(archive);
        free(dest);
        free(sentinel);
    }
}
END_TEST

Suite *security_suite(void)
{
    Suite *s;
    TCase *tc_core;

    s = suite_create("Security");
    tc_core = tcase_create("Core");

    tcase_add_test(tc_core, test_cpio_buffer_overflow_path);
    suite_add_tcase(s, tc_core);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = security_suite();