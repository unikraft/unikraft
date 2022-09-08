#!/usr/bin/env python
#
# Copyright (C) 2011-2021 Intel Corporation. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
#   * Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
#   * Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in
#     the documentation and/or other materials provided with the
#     distribution.
#   * Neither the name of Intel Corporation nor the names of its
#     contributors may be used to endorse or promote products derived
#     from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
#

# A python script for sync SGX SDK installer's plug-in XML 

import os
import sys
import re
import shutil
import uuid
import zipfile
from xml.dom import minidom

def Main(argv):
	local_path = os.path.split(os.path.realpath(sys.argv[0]))[0]
	local_dir = os.path.basename(local_path)
	
	#get version string from se_version.h
	file = local_path + "\..\..\..\common\inc\internal\se_version.h"
	f = open(file, 'r+')
	lines = f.readlines()
	f.close()
	VersionStr = re.compile(r'^#define STRPRODUCTVER')
	for line in lines[1:]:
		if line == "\n":
			continue
		match = VersionStr.search(line)
		if match:
			version = (line.split(' \"')[1]).split('\"')[0]
			break


	#Sync the "DCAP_Components.bat" version.
	file = local_path + "\..\DCAP_Components.bat"
	file_tmp = local_path + "\..\DCAP_Components.bat.tmp"
	f = open(file, 'r')
	lines = f.readlines()
	f.close()
	
	if os.path.exists(file_tmp):
		os.remove(file_tmp)
	f_tmp = open(file_tmp, 'w')
	Str = re.compile(r'^set PACKAGETNAME=DCAP_Components.')
	for line in lines[0:]:
		match = Str.search(line)
		if match:
			line = "set PACKAGETNAME=DCAP_Components." + version + "\n"
		f_tmp.write(line)
	f_tmp.close()
	
	if os.path.exists(file):
		os.remove(file)
	os.rename(file_tmp, file)
	
	
	#Sync the "DCAP_Components.nuspec" version
	xmlFile = "../DCAP_Components/DCAP_Components.nuspec"
	vsixmanifest = minidom.parse(xmlFile)
	Metadata = vsixmanifest.getElementsByTagName('metadata')
	Identity = Metadata[0].getElementsByTagName("version")
	Identity[0].firstChild.replaceWholeText(version)
	
	print vsixmanifest.toxml()
	f =  open(xmlFile, "wb")
	vsixmanifest.writexml(f)
	f.close()	

if __name__=="__main__":
	Main(sys.argv)

