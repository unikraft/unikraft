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
HOST_SYSTEM = ''

WINDOWS = 'Windows'
LINUX = 'Linux'
MACOSX = 'MacOSX'

TEMPORARY_FOLDER = './tmp'

INTEL64 = 'intel64'
IA32 = 'ia32'

BUILD_SCRIPT = {
    INTEL64: {
        WINDOWS: 'intel64.bat',
        LINUX: 'intel64.sh',
        MACOSX: 'intel64.sh'
    },
    IA32: {
        WINDOWS: 'ia32.bat',
        LINUX: 'ia32.sh',
        MACOSX: 'ia32.sh'
    }
}

EXPORT_FILES = {
    WINDOWS: 'export.def',
    LINUX: 'export.def',
    MACOSX: 'export.lib-export'
}

LIBRARIES_EXTENSIONS = {
    WINDOWS: '.dll',
    LINUX: '.so',
    MACOSX: '.dy'
}

LIBRARIES_PREFIX = {
    WINDOWS: '',
    LINUX: 'lib',
    MACOSX: 'lib'
}
