#!/bin/sh
# SPDX-License-Identifier: GPL-2.0
#
# Print the C compiler name and its version in a 5 or 6-digit form.
# Also, perform the minimum version check.

set -e

# Print the C compiler name and some version components.
get_c_compiler_info()
{
	cat <<- EOF | "$@" -E -P -x c - 2>/dev/null
	#if defined(__clang__)
	Clang	__clang_major__  __clang_minor__  __clang_patchlevel__
	#elif defined(__GNUC__)
	GCC	__GNUC__  __GNUC_MINOR__  __GNUC_PATCHLEVEL__
	#else
	unknown
	#endif
	EOF
}

# $@ instead of $1 because multiple words might be given, e.g. CC="ccache gcc".
orig_args="$@"
set -- $(get_c_compiler_info "$@")

name=$1

case "$name" in
GCC)
	version=$2.$3.$4
	;;
Clang)
	version=$2.$3.$4
	;;
*)
	echo "$orig_args: unknown C compiler" >&2
	exit 1
	;;
esac

echo $name $version
