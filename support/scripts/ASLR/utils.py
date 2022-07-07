def check_file(elfFile):

	if len(elfFile) > 4 and elfFile[1:4] == b'ELF':
		if elfFile[4] == 2:
			return True
		else:
			print("[ERROR] - The file should be a x86 binary file.")
	else:
			print("[ERROR] - Entered file is not an ELF file.")
	return False
def str_to_hex(op):
	try:
		#It's an address
		tmp = int(op,16)
	except ValueError:
		#it's a register
		tmp = None
	
	return tmp
def extract_libs(string,onlyLibs):
	"""
	In : string, a string
	onlyLibs, a boolean
	"""
	wordList = []
	returnList = []
	if string != None:
		wordList = string.split()
		
	if onlyLibs:
		for libs in wordList:
			if not libs.startswith("lib"):
				continue
			else:
				if(type(libs) is list):
					print("[ERROR] - the lib list contains something unexpected")
					sys.exit()
				returnList.append(libs)
		return returnList
	else:
		return wordList
def extract_conf(openConf):
	'''
	In : openConf is the config file.
	Returns a dictionnary that maps the name of the lib with an address.
	Skips comments : #This is a comment
	'''
	content = openConf.readlines()
	conf = []
	default = 0
	first = True
	i = 0
	for line in content:
		if not line.startswith("#"):
			words = line.split(" : ")
			if len(words) == 2 and words[1].rstrip("\n") != '':
				conf.append([words[0],words[1].rstrip("\n")])
				if first:
					default = words[1].rstrip("\n")
					first = False
			elif len(words) == 2 and not first:
				conf.append([words[0].rstrip("\n"),default])
			else:
				print("[Error] - line "+str(i)+" of the config file doesn't respect the format.")
		i += 1
	return conf

