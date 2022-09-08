"""
Copyright 2019-2021 Intel Corporation.

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
import shutil
import unittest
from sys import platform

import tests.utils
from tool.core import generate_script, build


class FunctionalityTests(unittest.TestCase):
    functions = ['ippccGetLibVersion',
                 'ippdcGetLibVersion',
                 'ippchGetLibVersion',
                 'ippsGetLibVersion']
    domains = ['ippcc',
               'ippdc',
               'ippch',
               'ipps',
               'ippcore',
               'ippvm']

    def generator_assertion(self):
        self.assertTrue(os.path.exists('./tmp'), 'Temporary folder does not exist')
        self.assertTrue(os.path.exists('./tmp/main.c'), 'main.c file was not generated')
        self.assertTrue(os.path.exists('./tmp/' + tests.utils.EXPORT_FILES[tests.utils.HOST_SYSTEM]),
                        'Export file was not generated')
        self.assertTrue(os.path.exists('./tmp/' +
                                       tests.utils.BUILD_SCRIPT[tests.utils.INTEL64][tests.utils.HOST_SYSTEM]),
                        'Build script was not generated')

    @classmethod
    def setUpClass(cls):
        if platform == "linux" or platform == "linux2":
            tests.utils.HOST_SYSTEM = tests.utils.LINUX
        elif platform == "darwin":
            tests.utils.HOST_SYSTEM = tests.utils.MACOSX
        elif platform == "win32":
            tests.utils.HOST_SYSTEM = tests.utils.WINDOWS

    @classmethod
    def tearDown(self):
        if os.path.exists(tests.utils.TEMPORARY_FOLDER):
            shutil.rmtree(tests.utils.TEMPORARY_FOLDER, ignore_errors=True)

    def test_generation(self):
        generate_script(tests.utils.HOST_SYSTEM,
                        tests.utils.HOST_SYSTEM,
                        self.functions,
                        tests.utils.TEMPORARY_FOLDER,
                        'tmp_dll',
                        self.domains,
                        tests.utils.INTEL64,
                        False)
        self.generator_assertion()

    def test_build(self):
        self.assertTrue(build(tests.utils.HOST_SYSTEM,
                              tests.utils.HOST_SYSTEM,
                              self.functions,
                              os.path.abspath(tests.utils.TEMPORARY_FOLDER),
                              'tmp_dll',
                              self.domains,
                              tests.utils.INTEL64,
                              False,
                              os.environ['COMPILERS_AND_LIBRARIES']))


if __name__ == '__main__':
    unittest.main()
