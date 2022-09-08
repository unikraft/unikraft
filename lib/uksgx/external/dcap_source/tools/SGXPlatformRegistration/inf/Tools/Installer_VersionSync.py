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

# A python script for sync Multi-package Agent installer's plug-in XML 

import os
import sys
import re
import shutil
import uuid
import zipfile
from xml.dom import minidom

def SyncComponentsVersion(filename, tempFileName, pattern,componentsName, version ):
	f = open(filename, 'r')
	lines = f.readlines()
	f.close()
	
	if os.path.exists(tempFileName):
		os.remove(tempFileName)
	f_tmp = open(tempFileName, 'w')
	for line in lines[0:]:
		match = pattern.search(line)
		if match:
			line = "set PACKAGETNAME=" + componentsName + version + "\n"
		f_tmp.write(line)
	f_tmp.close()
	
	if os.path.exists(filename):
		os.remove(filename)
	os.rename(tempFileName, filename)
	

def SyncNuspecVersion(filename, version):
	vsixmanifest = minidom.parse(filename)
	Metadata = vsixmanifest.getElementsByTagName('metadata')
	Identity = Metadata[0].getElementsByTagName("version")
	Identity[0].firstChild.replaceWholeText(version)
	
	print vsixmanifest.toxml()
	f =  open(filename, "wb")
	vsixmanifest.writexml(f)
	f.close()	

def Main(argv):
	local_path = os.path.split(os.path.realpath(sys.argv[0]))[0]
	local_dir = os.path.basename(local_path)
	
	#get version string from se_version.h
	file = local_path + "\..\..\..\..\QuoteGeneration\common\inc\internal\se_version.h"
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


	#Sync the "MPA_Network_Components.bat" version.
	file = local_path + "\..\MPA_Network_Components.bat"
	file_tmp = local_path + "\..\MPA_Network_Components.bat.tmp"	
	componentsName = "MPA_Network_Components"
	pattern = re.compile(r'^set PACKAGETNAME=MPA_Network_Components.')
	SyncComponentsVersion(file, file_tmp, pattern, componentsName, version)
    
	#Sync the "MPA_UEFI_Components.bat" version.
	file = local_path + "\..\MPA_UEFI_Components.bat"
	file_tmp = local_path + "\..\MPA_UEFI_Components.bat.tmp"	
	componentsName = "MPA_UEFI_Components"
	pattern = re.compile(r'^set PACKAGETNAME=MPA_UEFI_Components.')
	SyncComponentsVersion(file, file_tmp, pattern, componentsName, version)
    
	#Sync the "MPA_Network_Components.nuspec" version
	xmlFile = "../MPA_Network_Components/MPA_Network_Components.nuspec"
	SyncNuspecVersion(xmlFile, version)
    
	#Sync the "MPA_UEFI_Components.nuspec" version
	xmlFile = "../MPA_UEFI_Components/MPA_UEFI_Components.nuspec"
	SyncNuspecVersion(xmlFile, version)

if __name__=="__main__":
	Main(sys.argv)

