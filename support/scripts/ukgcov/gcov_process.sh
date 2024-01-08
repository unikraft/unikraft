#!/bin/bash

# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023, Unikraft GmbH and The Unikraft Authors.
# Licensed under the MIT License (the "License", see COPYING.md).
# You may not use this file except in compliance with the License.

set -e

if [[ $# -ne 5 ]]; then
	echo "Usage: $0 -o <result_directory> \
[-b <binary_dump> | -c <console_text>] <build_dir>" 1>&2
	exit 1
fi

if [ -n "${CROSS_COMPILE}" ]; then
	GCOV=${CROSS_COMPILE}/gcov
else
	GCOV=$(which gcov)
fi

if [ -z "$GCOV" ]; then
	echo "$0: Could not find gcov" 1>&2
	exit 1
fi

SCRIPT_DIR=$(dirname "$0")
SCRIPT_DIR=$(realpath "$SCRIPT_DIR")
PARSE_SCRIPT=$(realpath "$SCRIPT_DIR"/gcov_serial_parse.py)
RESULT_DIRECTORY=$(realpath "$2")
OUTPUT_FILE=$(realpath "$4")
BUILD_DIR=$(realpath "$5")
export OBJ_DIR="$RESULT_DIRECTORY"/objs/

mkdir -p "$RESULT_DIRECTORY"
cd "$RESULT_DIRECTORY"

touch dump.mem
CONVERTED_FILE=$(realpath dump.mem)

mkdir -p objs
mkdir -p results

rm -rf objs/*
rm -rf results/*

find "$BUILD_DIR" -type d -name objs \
	-prune -o -name \*.gcno -exec cp {} objs/ \;

lcov --gcov-tool "$CROSS_COMPILE"gcov --capture --initial --directory objs \
	-o "$RESULT_DIRECTORY"/results/baseline.info 2> /dev/null

if [[ $3 == "-c" ]]; then
	"$PARSE_SCRIPT" --filename "$OUTPUT_FILE" --output "$CONVERTED_FILE"
	gcov-tool merge-stream < "$CONVERTED_FILE"
else
	gcov-tool merge-stream < "$OUTPUT_FILE"
fi

find "$BUILD_DIR" -type d -name objs \
	-prune -o -name \*.gcda -exec cp {} objs/ \;

echo

cd "$RESULT_DIRECTORY"

# Capture converage info from this run
lcov --gcov-tool gcov --capture --directory objs \
	--ignore-errors gcov,source -o results/uk.info

# # Combine baseline and coverage info from this run
lcov -a results/baseline.info -a results/uk.info -o results/run.info

# # Generate HTML coverage report
genhtml -o results/html results/run.info --show-details --legend

echo -e "\nlcov report in file://$(realpath results/html)/index.html\n"
