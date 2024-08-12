#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2024, Unikraft GmbH and The Unikraft Authors.
# Licensed under the BSD-3-Clause License (the "License").
# You may not use this file except in compliance with the License.

class Analyzer:
	def __init__(self,openFile):
		self.file = openFile
		self.symbolTable = []
		# A list of the sections that cause crashes if their position is changed in the link step
		self.skippedSections = ["bss", "uk_inittab", "uk_ctortab", "intrstack"]

	def __handlesLine(self,lines):
		buffer = ""
		section_name = ""

		# fsm 1 treats the case of the regions that start with "_*" and end with "_e*".
		# This regions are flattened into a single string as not to overlap in the next steps.
		# For example, the following region:
		#  _rodata = .;
		# .rodata :
		# {
		# *(.rodata)
		# *(.rodata.*)
		# } :rodata
		# _erodata = .;
		# becomes:
		#  _rodata = .; .rodata : { *(.rodata) *(.rodata.*) } :rodata _erodata = .;
		fsm_1 = 0
		# fsm 2 treats the case of the sections that start with "*_start" and end with "*_end"
		# This regions are flattened into a single string as not to overlap in the next steps.
		# For example, the following line:
		# . = ALIGN((1 << 12)); __eh_frame_start = .; .eh_frame : { KEEP(*(.eh_frame)) KEEP(*(.eh_frame.*)) } __eh_frame_end = .; __eh_frame_hdr_start = .; .eh_frame_hdr : { KEEP(*(.eh_frame_hdr)) KEEP(*(.eh_frame_hdr.*)) } __eh_frame_hdr_end = .; __gcc_except_table_start = .; .gcc_except_table : { *(.gcc_except_table) *(.gcc_except_table.*) } __gcc_except_table_end = .;
		# becomes:
		# . = ALIGN((1 << 12));
		# __eh_frame_start = .; .eh_frame : { KEEP(*(.eh_frame)) KEEP(*(.eh_frame.*)) } __eh_frame_end = .;
		# __eh_frame_hdr_start = .; .eh_frame_hdr : { KEEP(*(.eh_frame_hdr)) KEEP(*(.eh_frame_hdr.*)) } __eh_frame_hdr_end = .;
		# __gcc_except_table_start = .; .gcc_except_table : { *(.gcc_except_table) *(.gcc_except_table.*) } __gcc_except_table_end = .;
		fsm_2 = 0

		tmp_lines = list(map(lambda x: [substring + ";" for substring in x.split(";")[:-1]] + [x.split(";")[-1]], lines))
		tmp_lines = [x for sub_tmp_lines in tmp_lines for x in sub_tmp_lines if x != '']
		lines = tmp_lines

		for line in lines:
			stripped_line = line.strip()

			#fsm 1
			if stripped_line.startswith("_") and not stripped_line.startswith("_e") and not stripped_line.startswith("__"):
				if fsm_1 != 0:
					self.symbolTable.append(buffer)
					buffer = ""
				fsm_1 = 1

				# Extract the region name
				region_name = stripped_line[1:stripped_line.index(" ")]
				buffer += line
				continue

			if fsm_1 == 1 and "_e" + region_name in line:
				fsm_1 = 0
				if line not in buffer:
					buffer += line
				if ".text" in buffer:
					self.symbolTable.append(buffer)
				else:
					self.symbolTable.append([buffer, 1])
				buffer = ""
				continue

			#fsm 2
			if fsm_2 == 0 and "_start" in stripped_line and fsm_1 == 0:
				buffer += line
				fsm_2 = 1
				continue

			if fsm_2 == 1 and "_end" in stripped_line:
				buffer += line
				skip = False
				for region in self.skippedSections:
					if region in buffer:
						skip = True
				if skip:
					self.symbolTable.append(buffer)
				else:
					self.symbolTable.append([buffer, 2, section_name])
				buffer = ""
				fsm_2 = 0
				continue

			if fsm_1 != 0 or fsm_2 != 0:
				buffer += line
			elif fsm_1 == 0 and fsm_2 == 0:
				self.symbolTable.append(line)

	def analyze(self):
		lines = self.file.read().split("SECTIONS\n{\n")[1].split("\n")
		self.__handlesLine(lines)

		return self.symbolTable.copy()
