#!/usr/bin/python
#
# Copyright (c) 2022, NEC Laboratories Europe GmbH, NEC Corporation.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# General Public License for more details.

import sys

if __name__ == '__main__':
	if len(sys.argv) != 2:
		print(sys.argv)
		print("[Error] bad number of args fed to the script.")
		sys.exit()
	string = sys.argv[1]
	if type(string) is not str:
		print("[Error] the argument should be a string containing lib names.")
		sys.exit()
		
	splittedString = string.split()
	libString = ""
	for index,words in enumerate(splittedString):
		if words.startswith("lib"):
			libString += (words+" ")
		elif index+1 < len(splittedString) and index-1 > 0 and \
			splittedString[index-1].startswith("lib") and \
			splittedString[index+1].startswith("lib"):
			libString += (words+" ")
	print(libString)
