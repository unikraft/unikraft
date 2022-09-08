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

import os
from subprocess import call  # nosec

from tool import utils
from tool.generators import main_file_generator, EXPORT_GENERATORS, build_script_generator, custom_dispatcher_generator


def generate_script():
    """
    Generates build script
    """
    host        = utils.HOST_SYSTEM
    output_path = utils.CONFIGS[utils.OUTPUT_PATH]

    if not os.path.exists(output_path):
        os.makedirs(output_path)

    with open(os.path.join(output_path, utils.MAIN_FILE_NAME + '.c'), 'w') as main_file:
        main_file.write(main_file_generator())

    if utils.CONFIGS[utils.CUSTOM_CPU_SET]:
        with open(os.path.join(output_path, utils.CUSTOM_DISPATCHER_FILE_NAME) + '.c', 'w') as custom_dispatcher_file:
            custom_dispatcher_file.write(custom_dispatcher_generator())

    with open(os.path.join(output_path, utils.EXPORT_FILE[host]), 'w') as export_file:
        EXPORT_GENERATORS[host](export_file, utils.CONFIGS[utils.FUNCTIONS_LIST])

    script_path = os.path.join(output_path, utils.CONFIGS[utils.BUILD_SCRIPT_NAME])
    with open(script_path, 'w') as build_script:
        build_script.write(build_script_generator())
    os.chmod(script_path, 0o745)

    return os.path.exists(script_path)


def build():
    """
    Builds dynamic library

    :return: True if build was successful and False in the opposite case
    """
    success = generate_script()
    if not success:
        return False

    output_path = utils.CONFIGS[utils.OUTPUT_PATH]
    error = call([os.path.join(output_path, utils.CONFIGS[utils.BUILD_SCRIPT_NAME])])
    if error:
        return False

    os.remove(os.path.join(output_path, utils.MAIN_FILE_NAME + '.c'))
    os.remove(os.path.join(output_path, utils.MAIN_FILE_NAME + '.obj'))
    os.remove(os.path.join(output_path, utils.EXPORT_FILE[utils.HOST_SYSTEM]))
    os.remove(os.path.join(output_path, utils.CONFIGS[utils.BUILD_SCRIPT_NAME]))

    if utils.CONFIGS[utils.CUSTOM_CPU_SET]:
        os.remove(os.path.join(output_path, utils.CUSTOM_DISPATCHER_FILE_NAME + '.obj'))

    return True
