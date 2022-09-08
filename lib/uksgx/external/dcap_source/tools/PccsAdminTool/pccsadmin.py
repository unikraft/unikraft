#!/usr/bin/env python3
# encoding: utf-8

import argparse
import requests
import os
import csv
import json
import re
from lib.intelsgx.pckcert import SgxPckCertificateExtensions
from lib.intelsgx.pcs import PCS
from lib.intelsgx.credential import Credentials
from urllib.parse import unquote
import traceback

def main():
    parser = argparse.ArgumentParser(description="Administrator tool for PCCS")
    #parser.add_argument('action', help='Choose your action')
    subparsers = parser.add_subparsers()

    #  subparser for get
    parser_get = subparsers.add_parser('get', formatter_class=argparse.RawTextHelpFormatter)
    # add optional arguments for get
    parser_get.add_argument("-u", "--url", help="The URL of the PCCS's GET platforms API; default: https://localhost:8081/sgx/certification/v4/platforms")
    parser_get.add_argument("-o", "--output_file", help="The output file name for platform list; default: platform_list.json")
    parser_get.add_argument("-s", "--source", help=
              "reg - Get platforms from registration table.(default)\n"
              "reg_na - Get platforms whose PCK certs are currently not available from registration table.\n"
            + "[FMSPC1,FMSPC2,...] - Get platforms from cache based on the fmspc values. [] to get all cached platforms.")
    parser_get.set_defaults(func=pccs_get)

    #  subparser for put
    parser_put = subparsers.add_parser('put')
    # add optional arguments for put
    parser_put.add_argument("-u", "--url", help="The URL of the PCCS's PUT collateral API; default: https://localhost:8081/sgx/certification/v4/platformcollateral")
    parser_put.add_argument("-i", "--input_file", help="The input file name for platform collaterals; default: platform_collaterals.json")
    parser_put.set_defaults(func=pccs_put)

    #  subparser for fetch
    parser_fetch = subparsers.add_parser('fetch')
    # add optional arguments for fetch
    parser_fetch.add_argument("-u", "--url", help="The URL of the Intel PCS service; default: https://api.trustedservices.intel.com/sgx/certification/v4/")
    parser_fetch.add_argument("-i", "--input_file", help="The input file name for platform list; default: platform_list.json")
    parser_fetch.add_argument("-o", "--output_file", help="The output file name for platform collaterals; default: platform_collaterals.json")
    parser_fetch.set_defaults(func=pcs_fetch)

    #  subparser for collect 
    parser_collect = subparsers.add_parser('collect')
    # add optional arguments for collect
    parser_collect.add_argument("-d", "--directory", help="The directory where platform CSV files are saved; default: ./")
    parser_collect.add_argument("-o", "--output_file", help="The output file name for platform list; default: platform_list.json")
    parser_collect.set_defaults(func=pcs_collect)

    #  subparser for refresh
    parser_refresh = subparsers.add_parser('refresh')
    # add optional arguments for refresh
    parser_refresh.add_argument("-u", "--url", help="The URL of the PCCS's refresh API; default: https://localhost:8081/sgx/certification/v4/refresh")
    parser_refresh.add_argument("-f", "--fmspc", help="Only refresh certificates for specified FMSPCs. Format: [FMSPC1, FMSPC2, ..., FMSPCn]")
    parser_refresh.set_defaults(func=pccs_refresh)

    args = parser.parse_args()
    if len(args.__dict__) <= 1:
        # No arguments or subcommands were given.
        parser.print_help()
        parser.exit()

    args.func(args)

def is_file_writable(filename):
    fullpath = os.path.join(os.getcwd(), filename)
    if os.path.isfile(fullpath):
        while True:
            overwrite = input('File %s already exists. Overwrite? (y/n) ' %(filename))
            if overwrite.lower() == "y":
                break
            if overwrite.lower() == "n":
                print("Aborted.")
                return False
    return True

def pccs_get(args):
    try :
        url = "https://localhost:8081/sgx/certification/v4/platforms"
        if args.url:
            url = args.url
        output_file = "platform_list.json"
        if args.output_file:
            output_file = args.output_file
        if args.source:
            url += '?source=' + args.source

        # Get administrator token from keyring
        credential = Credentials()
        token = credential.get_admin_token()

        HEADERS = {'user-agent': 'pccsadmin/0.1', 'admin-token': token}
        PARAMS = {}
        r = requests.get(url = url, headers=HEADERS, params = PARAMS, verify=False)
        if r.status_code == 200:
            # write output file
            if is_file_writable(output_file):
                with open(output_file, "w") as ofile:
                    json.dump(r.json(), ofile)
                print(output_file, " saved successfully.")
        elif r.status_code == 401: #Authentication error
            credential.set_admin_token('')
            print("Authentication failed.")
        else:
            # print error
            print("Failed to get platforms list from the PCCS.")
            print("\tStatus code is : %d" % r.status_code)
            print("\tMessage : " , r.text)
    except Exception as e:
        print(e)

def pccs_put(args):
    try :
        url = "https://localhost:8081/sgx/certification/v4/platformcollateral"
        if args.url:
            url = args.url
        input_file = "platform_collaterals.json"
        if args.input_file:
            input_file = args.input_file

        # Get administrator token from keyring
        credential = Credentials()
        token = credential.get_admin_token()

        HEADERS = {'user-agent': 'pccsadmin/0.1', 
                'Content-Type': 'application/json', 
                'admin-token': token}
        PARAMS = {}
        fullpath = os.path.join(os.getcwd(), input_file)
        with open(fullpath) as inputfile:
            data = inputfile.read()
        r = requests.put(url = url, data=data, headers=HEADERS, params = PARAMS, verify=False)
        if r.status_code == 200:
            print("Collaterals uploaded successfully.")
        elif r.status_code == 401: #Authentication error
            credential.set_admin_token('')
            print("Authentication failed.")
        else:
            # print error
            print("Failed to put platform collaterals to the PCCS.")
            print("\tStatus code is : %d" % r.status_code)
            print("\tMessage : " , r.text)
    except Exception as e:
        print(e)

def get_api_version_from_url(url):
    version = 4
    regex = re.compile('/v[1-9][0-9]*/')
    match = regex.search(url)
    if match is not None:
        verstr = match[0]
        if len(verstr) >= 4:
            version = int(verstr[2:-1])
    return version           

def pcs_fetch(args):
    try :
        url = 'https://api.trustedservices.intel.com/sgx/certification/v4/'
        ApiVersion = 4

        if args.url:
            url = args.url
        ApiVersion = get_api_version_from_url(url)
        input_file = "platform_list.json"
        if args.input_file:
            input_file = args.input_file
        output_file = "platform_collaterals.json"
        if args.output_file:
            output_file = args.output_file

        # Get PCS ApiKey from keyring
        credential = Credentials()
        apikey = credential.get_pcs_api_key()

        # prompt for overwriting output file
        if not is_file_writable(output_file):
            return

        # Initialize PCS object
        pcsclient = PCS(url,ApiVersion,apikey)
        sgxext= SgxPckCertificateExtensions()

        input_fullpath = os.path.join(os.getcwd(), input_file)
        with open(input_fullpath) as ifile:
            plaformlist = json.load(ifile)

        # construct output data
        output_json={}
        output_json["platforms"] = plaformlist
        output_json["collaterals"] = {
            "version" : ApiVersion,
            "pck_certs" : [],
            "tcbinfos" : [],
            "pckcacrl" : {
            },
            "qeidentity" : "",
            "tdqeidentity" : "",
            "qveidentity" : "",
            "certificates" : {
                PCS.HDR_PCK_Certificate_Issuer_Chain: {},
                PCS.HDR_TCB_INFO_ISSUER_CHAIN: "",
                PCS.HDR_Enclave_Identity_Issuer_Chain : ""
            },
            "rootcacrl": ""
        }

        # output.collaterals.pck_certs
        platform_dict = {}
        fmspc_set = set()
        for platform in plaformlist:
            platform_dict[(platform["qe_id"], platform["pce_id"])] = {"enc_ppid" : platform["enc_ppid"], 
                                                                      "platform_manifest" : platform["platform_manifest"]}
        certs_not_available = []
        for platform_id in platform_dict:
            enc_ppid = platform_dict[platform_id]["enc_ppid"]
            platform_manifest = platform_dict[platform_id]["platform_manifest"]
            pce_id = platform_id[1]

            # get pckcerts from Intel PCS, return value is [certs, chain]
            pckcerts = pcsclient.get_pck_certs(enc_ppid, pce_id, platform_manifest, 'ascii')
            if pckcerts == None:
                print("Failed to get PCK certs for platform enc_ppid:%s, pce_id:%s" %(enc_ppid,pce_id))
                return

            # Get the first property
            pckcerts_json = pckcerts[0]

            # parse the first cert to get FMSPC value and put it into a set
            cert = pckcerts_json[0]["cert"]
            sgxext.parse_pem_certificate(unquote(cert).encode('utf-8'))
            fmspc_set.add(sgxext.get_fmspc())

            # set pck-certificate-issuer-chain
            ca = sgxext.get_ca()
            if ca is None:
                print("Wrong PCK certificate format!")
                return

            pckchain = output_json["collaterals"]["certificates"][PCS.HDR_PCK_Certificate_Issuer_Chain]
            if not hasattr(pckchain, ca) or pckchain[ca] == '':
                pckchain[ca] = pckcerts[2]

            output_json["collaterals"]["pck_certs"].append({
                "qe_id" : platform_id[0],
                "pce_id" : pce_id,
                "enc_ppid": enc_ppid,
                "platform_manifest": platform_dict[platform_id]["platform_manifest"],
                "certs": pckcerts_json
            })
            certs_not_available.extend(pckcerts[1])

        if len(certs_not_available) > 0:
            # Found 'Not available' platforms
            while True:
                save_to_file = input("Some certificates are 'Not available'. Do you want to save the list?(y/n)")
                if save_to_file.lower() == "y":
                    file_na = input("Please input file name (Press enter to use default name not_available.json):")
                    if file_na.strip() == '' :
                        file_na = 'not_available.json'
                    # write output file
                    if is_file_writable(file_na):
                        with open(file_na, "w") as ofile:
                            json.dump(certs_not_available, ofile)
                        print("Please check " + file_na + " for 'Not available' certificates.")
                    else:
                        print('Unable to save file. ')

                    break
                if save_to_file.lower() == "n":
                    break

        # output.collaterals.tcbinfos
        for fmspc in fmspc_set:
            # tcbinfo : [tcbinfo, chain]
            sgx_tcbinfo = pcsclient.get_tcb_info(fmspc, 'sgx', 'ascii')
            tcbinfoJson = {"fmspc" : fmspc}
            if sgx_tcbinfo != None:
                if ApiVersion >= 4:    
                    tcbinfoJson['sgx_tcbinfo'] = json.loads(sgx_tcbinfo[0])
                else:
                    tcbinfoJson['tcbinfo'] = json.loads(sgx_tcbinfo[0])
            else:
                print("Failed to get SGXtcbinfo for FMSPC:%s" %(fmspc))
                return
            # TDX tcbinfo is optional
            if ApiVersion >= 4:
                tdx_tcbinfo = pcsclient.get_tcb_info(fmspc, 'tdx', 'ascii')
                if tdx_tcbinfo != None:
                    tcbinfoJson['tdx_tcbinfo'] = json.loads(tdx_tcbinfo[0])
            output_json["collaterals"]["tcbinfos"].append(tcbinfoJson)
            if output_json["collaterals"]["certificates"][PCS.HDR_TCB_INFO_ISSUER_CHAIN] == '':
                output_json["collaterals"]["certificates"][PCS.HDR_TCB_INFO_ISSUER_CHAIN] = sgx_tcbinfo[1]
            
        # output.collaterals.pckcacrl
        processorCrl = pcsclient.get_pck_crl('processor', 'ascii')
        if processorCrl == None:
            print("Failed to get processor PCK CRL.")
            return
        output_json["collaterals"]["pckcacrl"]["processorCrl"] = processorCrl[0]

        if ApiVersion >= 3:
            platformCrl = pcsclient.get_pck_crl('platform', 'ascii')
            if platformCrl == None:
                print("Failed to get platform PCK CRL.")
                return
            output_json["collaterals"]["pckcacrl"]["platformCrl"] = platformCrl[0]

        # output.collaterals.qeidentity
        qe_identity = pcsclient.get_enclave_identity('qe', 'ascii')
        if qe_identity == None:
            print("Failed to get QE identity")
            return
        output_json["collaterals"]["qeidentity"] = qe_identity[0]
        output_json["collaterals"]["certificates"][PCS.HDR_Enclave_Identity_Issuer_Chain] = qe_identity[1]

        # output.collaterals.tdqeidentity (Api Version >= 4)
        if ApiVersion >= 4:
            tdqe_identity = pcsclient.get_enclave_identity('tdqe', 'ascii')
            if tdqe_identity == None:
                print("Failed to get TDQE identity")
                return
            output_json["collaterals"]["tdqeidentity"] = tdqe_identity[0]

        # output.collaterals.qveidentity
        qve_identity = pcsclient.get_enclave_identity('qve', 'ascii')
        if qve_identity == None:
            print("Failed to get QvE identity")
            return
        output_json["collaterals"]["qveidentity"] = qve_identity[0]

        # output.collaterals.rootcacrl
        spos = qe_identity[1].rfind('-----BEGIN%20CERTIFICATE-----')
        root_cert = qe_identity[1][spos:]
        cdp = sgxext.get_root_ca_crl(unquote(root_cert).encode('utf-8'))
        rootcacrl = pcsclient.getFileFromUrl(cdp)
        output_json["collaterals"]["rootcacrl"] = rootcacrl

        with open(output_file, "w") as ofile:
            json.dump(output_json, ofile)
        print(output_file, " saved successfully.")

    except Exception as e:
        print(e)
        traceback.print_exc()

def pcs_collect(args):
    try :
       csv_dir = '.'
       output_file = "platform_list.json"
       if args.directory:
           csv_dir = args.directory
       if args.output_file:
           output_file = args.output_file

       if not is_file_writable(output_file):
           return

       fieldnames = ("enc_ppid", "pce_id", "cpu_svn", "pce_svn", "qe_id", "platform_manifest")
       platform_list = list()
       jsonfile = open(output_file, 'w')
       arr = os.listdir(csv_dir)
       if len(arr) < 2:
           print("At least 2 csv files are needed. Please make sure this is an administrator platform.")
           return
       for file in arr:
           if file.endswith(".csv"):
               csvfile = open(os.path.join(csv_dir,file), 'r')
               reader = csv.DictReader(csvfile, fieldnames)
               for row in reader:
                   platform_list.append(row)
               csvfile.close()
       json.dump(platform_list, jsonfile)
        
    except Exception as e:
        print(e)
        traceback.print_exc()

def pccs_refresh(args):
    try :
        url = "https://localhost:8081/sgx/certification/v4/refresh"
        if args.url:
            url = args.url
        fmspc = None 
        if args.fmspc:
            fmspc = args.fmspc

        # Get administrator token from keyring
        credential = Credentials()
        token = credential.get_admin_token()

        HEADERS = {'user-agent': 'pccsadmin/0.1', 
                'admin-token': token}
        PARAMS = {}
        if fmspc == 'all':
            PARAMS = {'type': 'certs',
                      'fmspc':''}
        elif fmspc != None:
            PARAMS = {'type': 'certs',
                      'fmspc': fmspc}
        r = requests.post(url = url, headers=HEADERS, params = PARAMS, verify=False)
        if r.status_code == 200:
            print("The cache database was refreshed successfully.")
        elif r.status_code == 401: #Authentication error
            credential.set_admin_token('')
            print("Authentication failed.")
        else:
            # print error
            print("Failed to refresh the cache database.")
            print("\tStatus code is : %d" % r.status_code)
            print("\tMessage : " , r.text)
    except Exception as e:
        print(e)

main()
