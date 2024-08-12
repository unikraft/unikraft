#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
# Licensed under the BSD-3-Clause License (the "License").
# You may not use this file except in compliance with the License.

from Analyzer import Analyzer
import sys
from random import SystemRandom
import argparse
import re
import random
import copy

def library_region(sysRand,libListOrigin,debug):
	"""
	In : A random engine, the list of libraries contained in the executable and a debug boolean.
	Returns a randomized version of the libListOrigin with padding and shuffled micro libs.
	"""
	libList = libListOrigin.copy()
	newRegions = ""
	while len(libList) != 0:
		padding = '. = . + 0x'+''.join('{:02X}'.format(sysRand.randint(0,65536)))+";\n"
		nextLib = sysRand.choice(libList)
		libList.remove(nextLib)
		newRegions += "  " + padding
		newRegions += nextLib+".o (.text);\n"

	return newRegions

def swap_regions(table):
	"""
	In : Takes a table of symbols from the linker file, and randomizez the order of the regions and segments.
	Return a swapped version of the linker script's regions and segments.
	"""

	for index in range(1,3):
		region_index = []
		indices_dict = {}
		for i, item in enumerate(table):
			if isinstance(item, list):
				if item[1] == index:
					region_index.append(i)
			if len(item) == 3:
				key = tuple(item[2]) if isinstance(item[2], list) else item[2]
				if key not in indices_dict:
					indices_dict[key] = [i]
				else:
					indices_dict[key].append(i)

		if indices_dict != {}:
			for key, value in indices_dict.items():
				tmp_shuffle = copy.deepcopy(value)
				random.shuffle(tmp_shuffle)

				tmp_table = copy.deepcopy(table)
				for i, item in enumerate(value):
					tmp_table[value[i]] = table[tmp_shuffle[i]]
				table = tmp_table
		else:
			tmp_shuffle = copy.deepcopy(region_index)
			random.shuffle(tmp_shuffle)

			tmp_table = copy.deepcopy(table)
			for i, item in enumerate(region_index):
				tmp_table[region_index[i]] = table[tmp_shuffle[i]]
		table = tmp_table

	return table

def randomize_phdrs(file_content):
    # Use regular expression to find lines between '{' and '}'
    pattern = re.compile(r'{(.*?)}', re.DOTALL)

    def randomize(match):
        # Split the lines inside the braces
        lines = match.group(1).strip().split('\n')
        # Randomize the order of lines
        random.shuffle(lines)
        # Join the lines back together
        return '{\n' + '\n'.join(lines) + '\n}'

    # Apply the randomize function to each match
    result = pattern.sub(randomize, file_content, count=1)

    return result

def print_back(openFile,output,table,debug):
	"""
	In : openFile is the linker script to modify,
		 output the file in which the programs writes,
		 table is the table containing the modified linker script,
		 debug a boolean
	Writes back the modified linker script into the file openFile.
	"""
	writeFile = open(output,"w")
	if not writeFile:
		print("[ASLR] {Error} Can't open the output file.",file=sys.stderr)
		sys.exit()
	# Clears the file and save headers
	openFile.seek(0)
	headers = openFile.read().split("SECTIONS\n{\n")

	new_phdrs = randomize_phdrs(headers[0])

	# Writes back randomized phdrs
	writeFile.write(new_phdrs+"SECTIONS\n{\n")

	# Write back the sections
	for element in table:
		if isinstance(element, list):
			writeFile.write(element[0] + "\n")
		else:
			writeFile.write(element + "\n")

	writeFile.close()
	return 0

def extract_libs(string,onlyLibs):
	"""
	In : string, a string
	onlyLibs, a boolean
	"""
	wordList = []
	returnList = []
	if string is not None:
		wordList = string.split()

	if onlyLibs:
		for libs in wordList:
			if not libs.startswith("lib"):
				continue
			else:
				if isinstance(libs, list):
					print("[ERROR] - the lib list contains something unexpected")
					sys.exit()
				returnList.append(libs)
		return returnList
	else:
		return wordList

def main(openFile,output,debug,libList,baseAddr):
	analyzer = Analyzer(openFile)
	table = analyzer.analyze()
	sysRand = SystemRandom()

	# Randomize the library list inside the .text region
	for i, line in enumerate(table):
		if ".text" in line:
			newLibs = library_region(sysRand, libList, debug)
			table[i] = table[i].replace("*(.text)", newLibs + "*(.text)")
			break

	# Swap memory regions
	table = swap_regions(table)

	# Change the base address in the table
	table[0] = baseAddr

	print_back(openFile,output,table,debug)

	return 0

if __name__ == '__main__':
	#Gets back the path of the linker script to modify
	parser = argparse.ArgumentParser(prog="Unikraft ASLR, linker script implementation")

	parser.add_argument('--base_addr', dest='baseAddr', default='', help="ASLR's base address")

	parser.add_argument('--file_path', dest='path', default='./', help="Path leading to the file.")

	parser.add_argument('--output_path', dest='output', default='./', help="Path and name of the file to create.")

	parser.add_argument('--lib_list', dest='build', default=None, help="All the libs in the ELF file.")

	parser.add_argument('--debug', dest='debug', default='False',
		help="Prints on the standard input debug information.")

	params , _ = parser.parse_known_args(sys.argv[1:])

	# If no baseAddr is passed, generate a random one
	if params.baseAddr == "":
		params.baseAddr = '{:02X}'.format(SystemRandom().randint(1048576,1048576*4))

	baseAddr = '. = 0x' + ''.join(params.baseAddr) + ";"

	openFile = open(params.path,"r+")
	if openFile is None:
		print("[ASLR] {Error} Couldn't open the file, path may be wrong.")
		sys.exit()

	libList = extract_libs(params.build,False)
	if len(libList) == 0:
		print("[ASLR] {Error} No lib list given to the program abort.",file=sys.stderr)
		sys.exit()
	main(openFile,params.output,params.debug,libList,baseAddr)
	openFile.close()
