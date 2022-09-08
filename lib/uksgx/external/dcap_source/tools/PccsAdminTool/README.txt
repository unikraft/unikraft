Administrator tool for PCCS

Prerequisites
  Install python3 and pip3 first, then install required packages using pip3
    sudo apt install python3
    sudo apt install python3-pip
    pip3 install -r requirements.txt

Usage: ./pccsadmin.py [-h] {get,put,fetch,collect,refresh} ...

positional arguments:
  {get,put,fetch,collect,refresh}

optional arguments:
  -h, --help       show this help message and exit

1. Get registration data from PCCS service
  ./pccsadmin.py get [-h] [-u URL] [-o OUTPUT_FILE] [-s SOURCE]

  optional arguments:
          -h, --help            show this help message and exit
          -u URL, --url URL     The URL of the PCCS's GET platforms API; default: https://localhost:8081/sgx/certification/v3/platforms
          -o OUTPUT_FILE, --output_file OUTPUT_FILE
                                The output file name for platform list; default: platform_list.json
          -s SOURCE, --source SOURCE
                                reg - Get platforms from registration table.(default)
                                reg_na - Get platforms whose PCK certs are currently not available from registration table.
                                [FMSPC1,FMSPC2,...] - Get platforms from cache based on the fmspc values. [] to get all cached platforms.

2. Fetch platform collateral data from Intel PCS based on the registration data
  ./pccsadmin.py fetch [-h] [-u URL] [-i INPUT_FILE] [-o OUTPUT_FILE]

  optional arguments:
          -h, --help            show this help message and exit
          -i INPUT_FILE, --input_file INPUT_FILE
                                The input file name for platform list; default: platform_list.json
          -o OUTPUT_FILE, --output_file OUTPUT_FILE
                                The output file name for platform collaterals; default: platform_collaterals.json
          -u URL, --url URL     The URL of the Intel PCS service; default: https://api.trustedservices.intel.com/sgx/certification/v3/

3. Put platform collateral data to PCCS cache db 
  ./pccsadmin.py put [-h] [-u URL] [-i INPUT_FILE]

  optional arguments:
          -h, --help            show this help message and exit
          -u URL, --url URL     The URL of the PCCS's PUT collateral API; default: https://localhost:8081/sgx/certification/v3/platformcollateral
          -i INPUT_FILE, --input_file INPUT_FILE
                                The input file name for platform collaterals; default: platform_collaterals.json

4. Collect platform data that was retrieved by PCK ID retrieval tool into one json file. This file can be used as input of "fetch" command.
  ./pccsadmin.py collect [-h] [-d DIRECTORY] [-o OUTPUT_FILE]

  optional arguments:
          -h, --help            show this help message and exit
          -d DIRECTORY, --directory DIRECTORY
                                The directory which stores the platform data(*.csv) retrieved by PCK ID retrieval tool; default: ./
          -o OUTPUT_FILE, --output_file OUTPUT_FILE
                                The output json file name; default: platform_list.json

5. Request PCCS to refresh certificates or collateral in cache database
  ./pccsadmin.py refresh [-h] [-u URL] [-f fmspc]

  optional arguments:
          -h, --help            show this help message and exit
          -u URL, --url URL     The URL of the PCCS's refresh API; default: https://localhost:8081/sgx/certification/v3/refresh
          -f FMSPCs, --fmspc FMSPCs
                                If this argument is not provided, then it will require PCCS to refresh quote verification collateral.
                                all - Refresh all cached certificates.
                                FMSPC1,FMSPC2,... - Refresh certificates of specified fmspc values.
