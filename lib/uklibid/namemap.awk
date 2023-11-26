# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
# Licensed under the BSD-3-Clause License (the "License").
# You may not use this file except in compliance with the License.
BEGIN {
	print "/* Automatically generated file; DO NOT EDIT */"
	print "const char *namemap[] = {"
}

/[a-zA-Z0-9]+/{
	printf "\t\"%s\",\n", $1;
}

END {
	print "};"
}
