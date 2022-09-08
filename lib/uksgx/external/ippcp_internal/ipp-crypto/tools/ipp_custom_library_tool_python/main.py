"""
Copyright 2018-2021 Intel Corporation.

This software and the related documents are Intel copyrighted  materials,  and
your use of  them is  governed by the  express license  under which  they were
provided to you (License).  Unless the License provides otherwise, you may not
use, modify, copy, publish, distribute,  disclose or transmit this software or
the related documents without Intel's prior written permission.

This software and the related documents  are provided as  is,  with no express
or implied  warranties,  other  than those  that are  expressly stated  in the
License.

License:
http://software.intel.com/en-us/articles/intel-sample-source-code-license-agr
eement/
"""
import sys
import os
from argparse import ArgumentParser, RawTextHelpFormatter

import tool.package
from tool import utils
from tool.core import build, generate_script


if __name__ == '__main__':
    utils.set_host_system()

    args_parser = ArgumentParser(formatter_class=RawTextHelpFormatter)
    args_parser.add_argument('-c', '--console',
                             help='Turns on console version.\n'
                                  'Custom Library Tool is running in GUI mode by default',
                             action='store_true')

    console_args = args_parser.add_argument_group('Console mode options')
    console_args.add_argument('-g', '--generate',
                              help='Generate build script without building custom dynamic library',
                              action='store_true')
    console_args.add_argument('-n', '--name',
                              help='Name of custom dynamic library',
                              default=utils.CONFIGS[utils.CUSTOM_LIBRARY_NAME])
    console_args.add_argument('-p', '--output_path',
                              help='Path to output directory',
                              default=utils.CONFIGS[utils.OUTPUT_PATH])
    console_args.add_argument('-root',
                              help='Path to specified ' + utils.PACKAGE_NAME[utils.IPP] +
                                   '\nor ' + utils.PACKAGE_NAME[utils.IPPCP] + ' package')
    console_args.add_argument('-f', '--functions',
                              help='Functions that has to be in dynamic library (appendable)',
                              nargs='+',
                              metavar='FUNCTION')
    console_args.add_argument('-ff', '--functions_file',
                              help='Load custom functions list from text file')
    console_args.add_argument('-arch', '--architecture',
                              help='Target architecture',
                              choices=utils.ARCHITECTURES,
                              default='intel64')
    console_args.add_argument('-mt', '--multi-threaded',
                              help='Build multi-threaded dynamic library',
                              action='store_true')
    console_args.add_argument('-tl', '--threading_layer_type',
                              help='Build dynamic library with selected threading layer type',
                              choices=utils.TL_TYPES)
    console_args.add_argument('-d', '--custom_dispatcher',
                              help='Build dynamic library with custom dispatcher.\n'
                                   'Set of CPUs can be any combination of the following:\n'
                                   'IA32 architecture - ' + ' '.join(utils.SUPPORTED_CPUS[utils.IA32]
                                                                                         [utils.HOST_SYSTEM]) +
                                   '\nIntel 64 architecture - ' + ' '.join(utils.SUPPORTED_CPUS[utils.INTEL64]
                                                                                               [utils.HOST_SYSTEM]),
                              nargs='+',
                              metavar='CPU')

    args = args_parser.parse_args()

    if args.console:
        print('Intel® Integrated Performance Primitives Custom Library Tool console version is on...')

        functions_list = []
        if args.functions_file:
            with open(args.functions_file, 'r') as functions_file:
                functions_list += map(lambda x: x.replace('\n', ''), functions_file.readlines())
        if args.functions:
            functions_list += args.functions
        if not functions_list:
            sys.exit("Please, specify functions that has to be in dynamic library by using -f or -ff options")

        if args.root:
            root = args.root
            if not os.path.exists(root):
                sys.exit("Error: specified package path " + args.root + " doesn't exist")
        else:
            root = tool.package.get_package_path()
            if not os.path.exists(root):
                sys.exit("Error: cannot find " + utils.PACKAGE_NAME[utils.IPP] + ' or ' +
                         utils.PACKAGE_NAME[utils.IPPCP] + " package. "
                         "Please, specify IPPROOT or IPPCRYPTOROOT by using -root option")

        package = tool.package.Package(root)
        print('Current package: ' + package.name)

        architecture = args.architecture
        thread_mode = (utils.MULTI_THREADED if args.multi_threaded else utils.SINGLE_THREADED)
        threading_layer_type = args.threading_layer_type

        error = package.errors[architecture][thread_mode]
        if error:
            sys.exit('Error: ' + error)
        if threading_layer_type:
            error = package.errors[architecture][threading_layer_type]
            if error:
                sys.exit('Error: ' + error)

        custom_cpu_set = []
        if args.custom_dispatcher:
            for cpu in args.custom_dispatcher:
                if cpu not in utils.SUPPORTED_CPUS[architecture][utils.HOST_SYSTEM]:
                    sys.exit("Error: " + cpu + " isn't supported for " + utils.HOST_SYSTEM + ' ' + architecture)
            custom_cpu_set = args.custom_dispatcher

        custom_library_name = args.name
        output_path = os.path.abspath(args.output_path)
        if not os.path.exists(output_path):
            os.makedirs(output_path)

        utils.set_configs_dict(package=package,
                               functions_list=functions_list,
                               architecture=architecture,
                               thread_mode=thread_mode,
                               threading_layer_type=threading_layer_type,
                               custom_library_name=custom_library_name,
                               output_path=output_path,
                               custom_cpu_set=custom_cpu_set)

        if args.generate:
            success = generate_script()
            print('Generation', 'completed!' if success else 'failed!')
        else:
            build()
    else:
        from PyQt5.QtWidgets import QApplication
        from gui.app import MainAppWindow

        app = QApplication(sys.argv)
        ex = MainAppWindow()
        sys.exit(app.exec_())
