#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
# Licensed under the BSD-3-Clause License (the "License").
# You may not use this file except in compliance with the License.

import re
from random import SystemRandom
import argparse
import sys

def replaceBaseAddress(filePath,address):
	regex1 = re.compile(".movl.*%edi")
	regex2 = re.compile(".movl.*%edx")

	file = open(filePath, "r")

	newString = ""

	for line in file.readlines():
		detect1 = re.split(regex1,line)
		detect2 = re.split(regex2,line)
		if len(detect1) > 1:
			newString += "	movl    $0x" + str(address) + ", %edi" + detect1[1]
		elif len(detect2) > 1:
			newString += "	movl    $0x" + str(address) + ", %edx" + detect2[1]
		else:
			newString += line
	file.close()

	# Write back the file, with the changed base address
	file = open(filePath, "w")
	file.seek(0)
	file.write(newString)

	return None

if __name__ == "__main__":
	parser = argparse.ArgumentParser(prog="Unikraft ASLR, linker script implementation")
	parser.add_argument('--file_path', dest='path', default='', help="ASLR's base address")

	params , _ = parser.parse_known_args(sys.argv[1:])

	if(params.path == ""):
		print("[BASE_ADDRESS] {Error} Couldn't find the file, path may be wrong.")
		sys.exit()

	# Generate the new random base address
	baseAddr = '{:02X}'.format(SystemRandom().randint(1048576,1048576*4))

	replaceBaseAddress(params.path, baseAddr)

	# Print the address so that we can later use it in the ASLR script, via the BASE_ADDRESS env var
	print(baseAddr)
