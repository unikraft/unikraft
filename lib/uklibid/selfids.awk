# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
# Licensed under the BSD-3-Clause License (the "License").
# You may not use this file except in compliance with the License.
BEGIN {
	print "/* Automatically generated file; DO NOT EDIT */"
	print "#include <uk/libid.h>"
	print ""
}

/[a-zA-Z0-9]+/{
	printf "const __u16 _uk_libid_self_%s = __UKLIBID_%s;\n", $1, $1;
}
