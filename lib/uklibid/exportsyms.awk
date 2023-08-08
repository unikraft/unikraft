# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
# Licensed under the BSD-3-Clause License (the "License").
# You may not use this file except in compliance with the License.
BEGIN {
	print "# Automatically generated file; DO NOT EDIT"
	print "uk_libname"
	print "uk_libid"
	print "uk_libid_count"
}

/[a-zA-Z0-9]+/{
	printf "_uk_libid_self_%s\n", $1;
}
