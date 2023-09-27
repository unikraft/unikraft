# uklibid: Library identifiers and information

## Library information

Library information is stored in a special section called `.uk_libinfo`.
The `support/scripts/uk-libinfo.py` tool can be used to inspect this library information.
This can be done directly on a library object (e.g., `build/libukboot.o`) or on a final unikernel (debug) image.
The format was directed by the following design principles:

- The format should be able to get changed in the future while preserving binary backwards compatibility.
- Older tools/parsers should be able to handle newer metadata layout versions somehow, e.g. by skipping unsupported metadata layout when parsing.
- Support mixing metadata layout versions within the same unikernel image.
- Adding a new record type (= metadata field) should be as easy as reserving a new record type number.
  Binary backward compatibility must be preserved.
- Externally created libraries can contain different information.
  The use of pre-built libraries with different sets of information must be supported and any information must be preserved in the final image.

For each library the build system will generate one header (`struct uk_libid_info_hdr`).
Due to binary cross-compatibility, the sizes, offsets, data types and meanings must remain the same regardless of the layout version.
All other fields/bytes/data formats (including `struct uk_libid_info_rec`) are defined by the layout version.

```c
struct uk_libid_info_hdr {
	unsigned int   len;     /* incl. hdr and total size of records */
	unsigned short version; /* layout version */
	/*
	 * Any following bytes are defined by the layout version.
	 */
} __packed __align(1);
```

When a final Unikraft image is linked, the individual sections of each library are merged into one section.
Since each header can be of different size, a parser (like `support/scripts/uk-libinfo.py`) needs the `len` field to skip parsing unsupported header layout versions.

Example of a final `.uk_libinfo` section with 3 libraries:

```text
 .uk_libinfo
 +---------------+
 | hdr library A |
 +---------------+
 | hdr library B |
 |               |
 +---------------+
 | hdr library C |
 +---------------+
```

NOTE: This standard assumes that endianness and actual size of `int` and `short` are defined by the target architecture.
External tools can derive this information from the image (e.g., ELF header).

### Layout version 1

#### Library information

In addition to a minimal header that satisfies the minimum requirements (field `len` and `version`), version 1 introduces a record structure (`struct uk_libid_info_rec`) that is used for each individual library value.
The structure requires an record `type` code, the embedded data, and the total `len` of the record header and data bytes of the entry.
The `type` codes are defined in `lib/uklibid/include/uk/libinfo.h` and specify parameter name, description and data type.

```c
struct uk_libid_info_rec {
	unsigned short type;    /* record type */
	unsigned int   len;     /* including record header */
	char           data[];  /* raw data */
} __packed __align(1);
```

Each record is appended to the corresponding library header:

```text
                             _____________
 +--------------------------+             \
 | struct uk_libid_info_hdr |              |
 |                          |               > hdr->len
 +--------------------------+              |
 | struct uk_libid_info_rec |              |
 +--------------------------+ -.           |
 | struct uk_libid_info_rec |   > rec->len |
 |                          |  |           |
 +--------------------------+ -`           |
 :          . . .           :              :
 +--------------------------+              |
 | struct uk_libid_info_rec |              |
 |                          |              |
 +--------------------------+ ____________/
```

The library name (`LIBNAME`, `0x0001`) is the only mandatory record type for each library.

#### Global information

Layout version 1 also supports storing of information related to the image as a whole and not to a specific library.
A single header is created for global information, which differs from the library headers in that it does not contain a record for a library name (`LIBNAME`).
