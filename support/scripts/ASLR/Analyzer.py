#!/usr/bin/env python3
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

import re

class Analyzer:
	
	def __init__(self,openFile):
		self.file = openFile;
		self.keywords = re.compile("SECTIONS")
		self.symbolTable = [];
		self.buffer = '';
		self.braceCounter = 0
		self.address = re.compile("\.\s*=\s*(.*)\s*;")
		self.assign = re.compile("(.*)=\s*\.(.*);")
		self.region = re.compile("\.\s*(.*)\s*:\s*")
		self.skippedBraces = 0
	
	def __handlesLine(self,string):
		#If that's header elements we skip it
		strLen = len(string)
		string = re.sub(self.keywords,'',string)
		if(len(string) != strLen):
			self.skippedBraces += 1
		
		#Proceed with analysis
		for letter in string:
			self.buffer += letter
			#Checks for current address assignement for outside pointers
			#They are usually around memoryblocks
			secSplit = re.split(self.assign, self.buffer)[1:]
			if len(secSplit) > 0 and self.braceCounter==0:
				if(secSplit[1] != ''):
					self.symbolTable.append(["assign",secSplit[0],secSplit[1]])
				else:
					self.symbolTable.append(["assign",secSplit[0]])
				self.buffer = ''
			
			#Checks for fixed addresses
			secSplit = re.split(self.address, self.buffer)[1:]
			if len(secSplit) > 0 and self.braceCounter==0:
				self.symbolTable.append(["curAdd",secSplit[0]])
				self.buffer = ''
			
			match letter:
				case '{':
					if(self.skippedBraces == 0):
						self.braceCounter += 1
					elif len(self.buffer)  == 1:
						letter = ''
				case '}':
					self.braceCounter -= 1
					#Means that we skipped an opening brace before
					if(self.braceCounter < 0):
						self.skippedBraces -= 1
						self.braceCounter += 1
					#Flushes the section in the table
					if( self.braceCounter == 0):
						secSplit = re.split(self.region, self.buffer)[1:]

						if len(secSplit) > 0:
							self.symbolTable.append([secSplit[0],secSplit[1]])
							self.buffer = ''
				#Wildcard
				case _:
					continue
	def analyze(self):
		
		lines = self.file.readlines()
		for line in lines:
			self.__handlesLine(line)
		
		return self.symbolTable.copy()
