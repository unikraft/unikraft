#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023, Unikraft GmbH and the Unikraft Authors.
# Licensed under the BSD-3-Clause License (the "License").
# You may not use this file except in compliance with the License.
IFS_ORIG=$IFS
IFS_NL=$'\n'

usage()
{
	echo "Usage: $0 [OPTION]... [LIBRARY PATH]..."
	echo "Generates a KConfig submenu by looping over Config.uk files"
	echo ""
	echo "  -h                         Display help and exit"
	echo "  -q                         Quiet, do not print warnings to STDERR"
	echo "  -t [TEXT]                  Generates a submenu with TEXT as title"
	echo "  -l [PATH]                  Library directory"
	echo "  -c [PATH:PATH:...]         Colon separated list of library directories"
	echo "  -r [PATH]                  Search for library directories in subfolders"
	echo "  -o [PATH]                  Write to output file instead of STDOUT"
}

ARG_OUT=
ARG_PATHS=()
ARG_TITLE=
OPT_MODE="concat"
OPT_QUIET=1
while getopts :hqt:l:r:c:o: OPT; do
	case ${OPT} in
	h)
		usage
		exit 0
		;;
	q)
		OPT_QUIET=0
		;;
	t)
		ARG_TITLE="${OPTARG}"
		OPT_MODE="menu"
		;;
	l)
		ARG_PATHS+=("${OPTARG}")
		;;
	r)
		if [ ! -z "${OPTARG}" ]; then
			for P in "${OPTARG}/"*; do
				[ -d "${P}" ] && ARG_PATHS+=("${P}")
			done
		fi
		;;
	c)
		IFS=':'
		for P in ${OPTARG}; do
			IFS=$IFS_ORIG
			ARG_PATHS+=("${P}")
			IFS=$IFS_NL
		done
		IFS=$IFS_ORIG
		;;
	o)
		ARG_OUT=("${OPTARG}")
		;;
	\?)
		echo "Unrecognized option -${OPTARG}" 1>&2
		usage 1>&2
		exit 1
		;;
	esac
done
shift $(( OPTIND - 1 ))

#
# Handling OUTPUT
#
if [ ! -z "$ARG_OUT" ]; then
	if [ -e "$ARG_OUT" -a ! -f "$ARG_OUT" ]; then
		echo "$ARG_OUT already exists but is not a regular file" 1>&2
		exit 2
	fi
	[ -f "$ARG_OUT" ] && rm -f "$ARG_OUT"

	# open fd:7
	exec 7<>"$ARG_OUT"
	if [ $? -ne 0 ]; then
		echo "Failed to open $ARG_OUT as output file" 1>&2
		exit 1
	fi

	printf '# Auto generated file. DO NOT EDIT\n' >&7
else
	# duplicate stdout to fd:7
	exec 7>&1
fi

#
# HEADER
#
case "${OPT_MODE}" in
	menu)
		printf "menu \"%s\"\n" "${ARG_TITLE}" >&7
		;;
	*)
		;;
esac

#
# BODY
#
for ARG_PATH in "${ARG_PATHS[@]}" "$@"; do
	CONFIG_UK="${ARG_PATH}/Config.uk"

	if [ ! -f "${CONFIG_UK}" ]; then
		if [ $OPT_QUIET -ne 0 ]; then
			echo "Could not find \"Config.uk\" under \"${ARG_PATH}\". Skipping..." 1>&2
		fi
		continue;
	fi

	printf "source \"%s\"\n" "$( readlink -f "${CONFIG_UK}" )" >&7
done

#
# FOOTER
#
case "${OPT_MODE}" in
	menu)
		printf "endmenu\n" >&7
		;;
	*)
		;;
esac

# close FD:7 and print filename
if [ ! -z "$ARG_OUT" ]; then
	printf '%s\n' "$( readlink -f $ARG_OUT )"
	exec 7>&-
fi
