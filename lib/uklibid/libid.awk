# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
# Licensed under the BSD-3-Clause License (the "License").
# You may not use this file except in compliance with the License.
BEGIN {
	print "/* Automatically generated file; DO NOT EDIT */"
	print ""
	count = 0
}

/[a-zA-Z0-9]+/{
	printf "#define __UKLIBID_%s ((__u16) %d)\n", $1, count;
	count += 1
}

END {
	print ""
	printf "#define __UKLIBID_COUNT__ ((__u16) %d)\n", count;
}
