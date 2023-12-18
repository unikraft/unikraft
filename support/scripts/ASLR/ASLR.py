#!/usr/bin/env python3.10
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
#
# foo_start = .;
# ....
# foo_end = .; is considered to be a region.
#
# .foo : { } is also a region if not between the elements defined upper.

from Analyzer import Analyzer
import sys
from random import SystemRandom
from utils import extract_conf,extract_libs
import argparse
import re
import random
import copy

#Takes 2 string and returns the longest prefix/suffix between them
def alike(s1,s2):
	"""
		In : s1,s2 two strings
		returns the number of similar letters either in the prefix or in the suffix.
		Depending on which is the most alike.
	"""
	lenS1 = len(s1)
	lenS2 = len(s2)
	
	lettersPre = 0
	lettersSuf = 0
	
	stopPre = False
	stopSuf = False
	#Prefix
	for i in range(lenS1):
		if i >= lenS2 or (stopSuf and stopPre):
			break
		if s1[lenS1-i-1] == s2[lenS2-i-1] and not stopSuf:
			lettersSuf +=1
		else:
			stopSuf = True
		
		if s1[i]==s2[i] and not stopPre:
			lettersPre +=1
		else:
			stopPre = True
	
	if lettersPre > lettersSuf :
		return lettersPre
	else :
		return lettersSuf

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

def main(openFile,output,debug,libList,baseAddr,dedu):
	
	analyzer = Analyzer(openFile)
	table = analyzer.analyze()
	regionTable = []
	sysRand = SystemRandom()
	
	# Randomize the library list inside the .text region
	for i, line in enumerate(table):
		if ".text" in line:
			newLibs = library_region(sysRand, libList, debug)
			table[i] = table[i].replace("*(.text)", newLibs + "*(.text)")
			break

	#Swap memory regions
	table = swap_regions(table)

	# Change the base address in the table
	table[0] = baseAddr

	print_back(openFile,output,table,debug,dedu)
	
	return 0

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


def print_back(openFile,output,table,debug,dedu):
	"""
	In : openFile is the linker script to modify,output the file in which the programs writes, 
	table is the table containing the modified linker script, debug a boolean and a char activating or not
	deduplication compatibility.
	Writes back the modified linker script into the file openFile.
	"""
	deduFile = None
	writeFile = open(output,"w")
	if not writeFile:
		print("[ASLR] {Error} Can't open the output file.",file=sys.stderr)
		sys.exit()
	if dedu != './':
		deduFile = open(dedu,"r")
		if not deduFile:
			print("[ASLR] {Error} Can't open the deduplication config file.",file=sys.stderr)
			sys.exit()
	#clears the file and save headers
	openFile.seek(0)
	# header = openFile.read().split("SECTIONS\n{\n")
	openFile.seek(0)
	header = openFile.read().split("SECTIONS\n{\n")
	openFile.seek(0)
	
	new_phdrs = randomize_phdrs(header[0])
	
	#Writes back
	writeFile.write(new_phdrs+"SECTIONS\n{\n")

	for element in table:
		if isinstance(element, list):
			writeFile.write(element[0] + "\n")
		else:
			writeFile.write(element + "\n")

	writeFile.close()
	return 0

def handleSetup(file,address):
	regex = re.compile(".long 0[xX][0-9a-fA-F]+")
	newString = ""

	if address == "-1":
		patch = ''.join('{:02X}'.format(SystemRandom().randint(1048576,1048576*4)))
	else:
		patch = address
	for line in file.readlines():
		detect = re.split(regex,line)
		if len(detect) > 1:
			#that int is 0x100000 in hexa 
			print(patch)
			newString += ".long 0x"+ patch + detect [1]
		else:
			newString += line

	file.seek(0)
	file.write(newString)

	return patch
if __name__ == '__main__':
	#Gets back the path of the linker script to modify
	parser = argparse.ArgumentParser(prog="Unikraft ASLR, linker script implementation")

	parser.add_argument('--setup_file', dest='setup', default='./', help="Path leading to the multiboot.S file.")	
	
	parser.add_argument('--base_addr', dest='baseAddr', default='-1', help="ASLR's base address")
	
	parser.add_argument('--file_path', dest='path', default='./', help="Path leading to the file.")

	parser.add_argument('--output_path', dest='output', default='./', help="Path and name of the file to create.")	
	
	parser.add_argument('--lib_list', dest='build', default=None, help="All the libs in the ELF file.")	
	
	parser.add_argument('--deduplication', dest='dedu', default='./', help="Creates the table or not")
	
	parser.add_argument('--debug', dest='debug', default='False', 
		help="Prints on the standard input debug informations.")

	params , _ = parser.parse_known_args(sys.argv[1:])


	if params.baseAddr == "":
		params.baseAddr = '. = 0x' + ''.join('{:02X}'.format(SystemRandom().randint(1048576,1048576*4))) + ";"

	if params.setup != "./":
		openFile = open(params.setup,"r+")
		if(openFile == None):
			print("[ASLR] {Error} Couldn't open the entropy.S file, path may be wrong.")
			sys.exit()
		handleSetup(openFile,params.baseAddr)
		openFile.close()
	elif params.baseAddr != '-1':
		openFile = open(params.path,"r+")
		if(openFile == None):
			print("[ASLR] {Error} Couldn't open the file, path may be wrong.")
			sys.exit()
		
		libList = extract_libs(params.build,False)
		if len(libList) == 0:
			print("[ASLR] {Error} No lib list given to the program abort.",file=sys.stderr)
			sys.exit()
		main(openFile,params.output,params.debug,libList,params.baseAddr,params.dedu)
		openFile.close()
	
	else:
		print("[ASLR] {Error} Base address should be positive or given.",file=sys.stderr)
		sys.exit()

	
	
