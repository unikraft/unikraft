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

def print_sym_table(table):
	"""
	In : Takes a table of symbols
	Prints what the table contains in a format . | . | . \n or . | . \n depending
	on the col count.
	"""
	for element in table:
		if len(element) == 3:
			print( element[0] + " | " + element[1]+ " | " + str(element[2]))
		else:
			print( element[0] + " | " + element[1])

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

def region_handler(table,sysRand,libList,debug,baseAddr):
	"""
	In : Takes a table of symbols from the linker file, a random engine, 
	the list of libraries contained in the executable, a debug boolean and a baseAddress.
	Returns regionTable which is a table recreating regions based on memory pointers set in the linker script.
	Those pointers are matched based on their names : i.e foo_start = . ; ..... foo_end  = . ; are matched together
	"""
	tableLen = len(table)
	regionTable = []
	textRegion = None
	#For now randomizes current address (. = 0x....) and swap the regions (not subregions)
	for i in range(tableLen): 
		maxCompatIndex = 0
		maxCompat = 0
		compat = 0
		atEnd = False
		tableSize = len(regionTable)
		if table[i][0] == "assign":
			for j in range(i+1,tableLen):
			
				#Qemu expects text to be the first region
				#and bss the last one, so we don't swap them
				#Breaks a second time not to take the ending pointer 
				#for another region
				if table[j][0] == "bss " or table[j-2][0] == "bss ":
					break
				if table[j][0] != "assign":
					continue

				compat = alike(table[i][1],table[j][1]) / (j-i)
				if compat > maxCompat:
					maxCompat = compat
					maxCompatIndex = j
					
			#Checks if the elements that starts the region closes already another one
			for j in range(tableSize):
				if regionTable[tableSize-1-j][1] == i or i < regionTable[tableSize-1-j][1]:
					atEnd = True
					break
			if (tableSize == 0 or not atEnd) and i <= maxCompatIndex:
				#Modifies the lib position
				if table[i][1] == ' _text ' :
					table[i+1] = library_region(table[i+1],sysRand,libList,debug)
				
				#Tries to drag pre-configured memory layout with it
				if i-1 >= 0 and table[i-1][0] == "curAdd" and table[i-1][1].startswith('ALIGN'):
					regionTable.append([i-1,maxCompatIndex])
				else:
					regionTable.append([i,maxCompatIndex])
		
		#Randomizes static addresses
		elif(table[i][0] == "curAdd" and not table[i][1].startswith('ALIGN')):
			#Replaces the starting address
			if i == 0:
				table[i][1] = '0x'+baseAddr
			else:
				table[i][1] = '0x'+''.join('{:02X}'.format(int(table[i][1],16)+(sysRand.randint(0,65536))))
	
	return regionTable, table

def library_region(textRegion,sysRand,libListOrigin,debug):
	"""
	In : textRegion is a string of the whole text segment, a random engine, the list of libraries contained in the executable and a debug boolean.
	Returns a modified version of the textregion with padding and shuffled micro libs.
	"""
	libList = libListOrigin.copy()
	if debug == 'True':
		index = textRegion[1].find("*(.text)")
		padding = '. = . + 0x'+''.join('{:02X}'.format(sysRand.randint(0,65536)))+";\n"
		modfiedRegion = textRegion[1][0:index] +"}"
		nextLib = libList.pop(0)
		modfiedRegion += "  .text."+nextLib+" . :{ "+nextLib+".o (.text);}\n"
		while len(libList) != 0:
			padding = '. = . + 0x'+''.join('{:02X}'.format(sysRand.randint(0,65536)))+";\n"
			nextLib = sysRand.choice(libList)
			libList.remove(nextLib)
			modfiedRegion += "  .text."+nextLib+" . :{ "
			modfiedRegion += padding+"  "
			modfiedRegion += nextLib+".o (.text);}\n"
		
		textRegion[1] = modfiedRegion
		return textRegion
	else:
		index = textRegion[1].find("*(.text)")
		padding = '. = . + 0x'+''.join('{:02X}'.format(sysRand.randint(0,65536)))+";\n"
		modfiedRegion = textRegion[1][0:index]
		while len(libList) != 0:
			padding = '. = . + 0x'+''.join('{:02X}'.format(sysRand.randint(0,65536)))+";\n"
			nextLib = sysRand.choice(libList)
			libList.remove(nextLib)
			modfiedRegion += "  " + padding
			modfiedRegion += nextLib+".o (.text);\n"
		
		textRegion[1] = modfiedRegion+textRegion[1][index-2:]
		return textRegion

def main(openFile,output,debug,libList,baseAddr,dedu):
	
	analyzer = Analyzer(openFile)
	table = analyzer.analyze()
	regionTable = []
	sysRand = SystemRandom()
	if debug == 'True':
		print_sym_table(table)
	
	regionTable, table = region_handler(table,sysRand,libList,debug,baseAddr)
	#Swap memory regions
	table = swap_regions(table,regionTable,sysRand)
		
	print_back(openFile,output,table,debug,dedu)
	
	return 0

def swap_regions(table,regionTable,sysRand):
	"""
	In : Takes a table of symbols from the linker file, the table of the linker's memory segments and a random engine.
	Return a swapped version of the linker script's segment according to some rules specified by Qemu.
	"""
	regionCopy = regionTable.copy()
	iterations = len(regionCopy)
	regionTableIndex = 0
	maxInter = len(table)
	newTable = []
	index = 0
	#Set text,uk_*tab as first regions in order to avoid errors.
	toDel = []
	added = False
	setRoData = False

	for ind in regionCopy :
		if table[ind[1]][1] == " uk_inittab_end " or \
		table[ind[1]][1] == " _etext " or \
		table[ind[1]][1] == " uk_ctortab_end ":
			toDel.append(ind)
	
	if len(toDel) == 0:
		print("[ASLR] {Error} Missing important memory regions : _text, uk_inittab_start, uk_ctortab_start",file=sys.stderr)
		sys.exit()
	
	#if there's only one element there's no point swapping it.
	if iterations > 0:
		while index < maxInter:
		
			if len(toDel) != 0 and index == toDel[0][0] and added:
				index = toDel[0][1]
				toDel.pop(0)
			elif regionTableIndex < iterations and index == regionTable[regionTableIndex][0]:
				#Set text,uk_*tab as first regions in order toclear avoid errors.
				if regionTableIndex == 0 and added != True:
					for el in toDel:
						newTable += table[el[0]:el[1]+1]
						regionCopy.remove(el)
						regionTable.remove(el)
						added = True
						iterations -= 1
					continue
				else:
					#Takes an element at random without replacement
					tmpRegion = None
					while tmpRegion == None or (table[tmpRegion[1]][1] == ' _edata ' and not setRoData):
						tmpRegion = sysRand.choice(regionCopy)

					if table[tmpRegion[1]][1] == " _erodata ":
						setRoData = True
					#Makes the right jump in the table
					index = regionTable[regionTableIndex][1]
					#Append the region to the newtable
					newTable += table[tmpRegion[0]:tmpRegion[1]+1]
					regionCopy.remove(tmpRegion)
					regionTableIndex += 1
			#If it's a singleton
			else :
				newTable.append(table[index])

			index += 1
		return newTable
	return table

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
	
	
	#Writes back
	previous = ""
	if debug:
		#Doesn't set ENTRY in debug mode otherwise causes double import
		# writeFile.write("SECTIONS\n{\n")
		writeFile.write(header[0]+"SECTIONS\n{\n")
	else :
		writeFile.write(header[0]+"SECTIONS\n{\n")
	
	for element in table:
		
		if element[0] == "curAdd":
			#Avoid having ALIGN cascade
			if not (previous.startswith('ALIGN') and element[1].startswith('ALIGN')):
				writeFile.write(". = "+element[1]+";\n")
		elif element[0] == "assign":
			if len(element) == 3:
				writeFile.write(element[1]+"= ."+element[2]+";\n")
			else:
				writeFile.write(element[1]+"= .;\n")
			if element[1] == " _etext " and deduFile:
				conf = extract_conf(deduFile)
				addr = conf.pop(0)[0]
				fill = 0
				for lib in conf:
					fill += int(lib[1],16)
				writeFile.write(". = ALIGN(0x1000);\n.ind "+addr+" : {FILL(0X90);. = . + "+hex(fill)+";BYTE(0X90)}\n")
		else:
			writeFile.write("."+element[0]+":"+element[1]+"\n")
		previous = element[1]
		
		
	writeFile.write("}")
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

	# params.baseAddr = "100000"
	if params.setup != "./":
		openFile = open(params.setup,"r+")
		if(openFile == None):
			print("[ASLR] {Error} Couldn't open the entropy.S file, path may be wrong.")
			sys.exit()
		handleSetup(openFile,params.baseAddr)
		openFile.close()
	
	elif params.baseAddr != '-1':
		# with open(params.path, "rb") as f_src:
		# 	with open(params.output, "wb") as f_dest:
		# 		while True:
		# 			# Read data from source file in chunks
		# 			data = f_src.read(1024)
		# 			if not data:
		# 				break
		# 			# Write data to destination file
		# 			f_dest.write(data)
		# sys.exit()
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

	
	
