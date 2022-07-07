#!/usr/bin/python

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
