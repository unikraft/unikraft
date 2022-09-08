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
import sys
import re
from platform import system
from collections import defaultdict

WINDOWS = 'Windows'
LINUX   = 'Linux'
MACOSX  = 'MacOS'
HOST_SYSTEM = None
SUPPORTED_SYSTEMS = [WINDOWS, LINUX, MACOSX]

IA32    = 'ia32'
INTEL64 = 'intel64'
ARCHITECTURES = [IA32, INTEL64]

SINGLE_THREADED = 'single-threaded'
MULTI_THREADED  = 'multi-threaded'
THREAD_MODES = [SINGLE_THREADED, MULTI_THREADED]

TBB    = 'tbb'
OPENMP = 'openmp'
TL_TYPES = [TBB, OPENMP]

PATH_TO_PACKAGE_REGEX        = '(?P<path>.*)\Wtools\W.*'
COMPONENTS_INSTALL_DIR_REGEX = '(?P<path>.*)\Wipp.*'
VERSION_REGEX                = '.*VERSION_STR.*"(?P<ver>.*)".*'
FUNCTION_NAME_REGEX          = 'IPPAPI\s*\(\s*(?P<ret_type>.*?)\s*,' \
                               '\s*(?P<function_name>\S*)\s*,' \
                               '\s*\(?(?P<args>.*?)\s*\)?\s*\)?\s*$'
ARGUMENT_REGEX               = '.*\W*\w+\W*\s+\W*(?P<arg>[^\W\d]+\w*)\W*?'

CUSTOM_LIBRARY_NAME = 'Custom library name'
BUILD_SCRIPT_NAME   = 'Build script name'
OUTPUT_PATH         = 'Output path'
FUNCTIONS_LIST      = 'Functions list'
PACKAGE             = 'Package'
ARCHITECTURE        = 'Architecture'
THREAD_MODE         = 'Thread mode'
THREADING_LAYER     = 'Threading layer'
CUSTOM_CPU_SET      = 'Custom CPU set'

CONFIGS = {
    CUSTOM_LIBRARY_NAME : 'custom_library',
    BUILD_SCRIPT_NAME   : '',
    OUTPUT_PATH         : '.',
    FUNCTIONS_LIST      : [],
    PACKAGE             : '',
    ARCHITECTURE        : '',
    THREAD_MODE         : '',
    THREADING_LAYER     : '',
    CUSTOM_CPU_SET      : []
}

IPP   = 'IPP'
IPPCP = 'IPPCP'

IPPROOT       = 'IPPROOT'
IPPCRYPTOROOT = 'IPPCRYPTOROOT'

PACKAGE_NAME = {
    IPP   : 'Intel® Integrated Performance Primitives',
    IPPCP : 'Intel® Integrated Performance Primitives Cryptography'
}

DOMAINS = {
    IPP: {
        'ippcc'   : 'Color Conversion',
        'ippch'   : 'String Operations',
        'ippcv'   : 'Computer Vision',
        'ippdc'   : 'Data Compression',
        'ippe'    : 'Embedded Functionality',
        'ippi'    : 'Image Processing',
        'ipps'    : 'Signal Processing',
        'ippvm'   : 'Vector Math',
        'ippcore' : 'Core'
    },
    IPPCP: {
        'ippcp'   : 'Cryptography'
    },
    THREADING_LAYER: {
        'ippcc'   : 'Color conversion TL',
        'ippi'    : 'Image processing TL',
        'ippcore' : 'Core TL',
    }
}

SSE2     = 'sse2'
SSE3     = 'sse3'
SSSE3    = 'ssse3'
SSE42    = 'sse42'
AVX      = 'avx'
AVX2     = 'avx2'
AVX512F  = 'avx512f'
AVX512BW = 'avx512bw'

CPU = {
    SSE2: {
        IA32    : 'w7',
        INTEL64 : ''
    },
    SSE3: {
        IA32    : '',
        INTEL64 : 'm7'
    },
    SSSE3: {
        IA32    : 's8',
        INTEL64 : 'n8'
    },
    SSE42: {
        IA32    : 'p8',
        INTEL64 : 'y8'
    },
    AVX: {
        IA32    : 'g9',
        INTEL64 : 'e9'
    },
    AVX2: {
        IA32    : 'h9',
        INTEL64 : 'l9'
    },
    AVX512F: {
        IA32    : '',
        INTEL64 : 'n0'
    },
    AVX512BW: {
        IA32    : '',
        INTEL64 : 'k0'
    }
}

SUPPORTED_CPUS = {
    IA32: {
        WINDOWS : [SSE2, SSSE3, SSE42, AVX, AVX2],
        LINUX   : [SSE2, SSSE3, SSE42, AVX, AVX2],
        MACOSX  : []
    },
    INTEL64: {
        WINDOWS : [SSE3, SSSE3, SSE42, AVX, AVX2, AVX512BW],
        LINUX   : [SSE3, SSSE3, SSE42, AVX, AVX2, AVX512F, AVX512BW],
        MACOSX  : [SSE42, AVX, AVX2, AVX512BW]
    }
}

CPUID = {
    AVX512BW : 'AVX3X_FEATURES',
    AVX512F  : 'AVX3M_FEATURES',
    AVX2     : 'ippCPUID_AVX2',
    AVX      : 'ippCPUID_AVX',
    SSE42    : 'ippCPUID_SSE42',
    SSSE3    : 'ippCPUID_SSSE3',
    SSE3     : 'ippCPUID_SSE3',
    SSE2     : 'ippCPUID_SSE2',
}

PATH_TO_LIBRARIES = {
    SINGLE_THREADED : '{libs_arch_dir}',
    MULTI_THREADED  : '{libs_arch_dir}/threaded',
    TBB             : '{libs_arch_dir}/tl/' + TBB,
    OPENMP          : '{libs_arch_dir}/tl/' + OPENMP
}

LIB_PREFIX = {
    WINDOWS : '',
    LINUX   : 'lib',
    MACOSX  : 'lib'
}

STATIC_LIB_POSTFIX = {
    WINDOWS : 'mt',
    LINUX   : '',
    MACOSX  : ''
}

STATIC_LIB_EXTENSION = {
    WINDOWS : '.lib',
    LINUX   : '.a',
    MACOSX  : '.a'
}

DYNAMIC_LIB_EXTENSION = {
    WINDOWS : '.dll',
    LINUX   : '.so',
    MACOSX  : '.dylib'
}

BUILD_SCRIPT_NAME_FORMAT = {
    WINDOWS : 'build_{name}_{arch}.bat',
    LINUX   : 'build_{name}_{arch}.sh',
    MACOSX  : 'build_{name}_{arch}.sh'
}

BATCH_EXTENSIONS = {
    WINDOWS : '.bat',
    LINUX   : '.sh',
    MACOSX  : '.sh'
}

MAIN_FILE_NAME = 'main'
CUSTOM_DISPATCHER_FILE_NAME = 'custom_dispatcher'

EXPORT_FILE = {
    WINDOWS : 'export.def',
    LINUX   : 'export.def',
    MACOSX  : 'export.lib-export'
}

PROJECT_EXTENSION = '.cltproj'

HAVE_PACKAGE   = 'package...'
HAVE_FUNCTIONS = 'functions that has to be in dynamic library... '

ENABLE_GENERATION_RULES = {
    HAVE_PACKAGE   : True,
    HAVE_FUNCTIONS : True,
}


def set_host_system():
    host_system = system()

    if host_system == 'Darwin':
        host_system = MACOSX
    if host_system not in SUPPORTED_SYSTEMS:
        sys.exit("Error: Intel® Integrated Performance Primitives Custom Library Tool isn't supported for OS " +
                 host_system)

    global HOST_SYSTEM
    HOST_SYSTEM = host_system


def set_configs_dict(package,
                     functions_list,
                     architecture,
                     thread_mode,
                     threading_layer_type,
                     custom_library_name=CONFIGS[CUSTOM_LIBRARY_NAME],
                     build_script_name=CONFIGS[BUILD_SCRIPT_NAME],
                     output_path=CONFIGS[OUTPUT_PATH],
                     custom_cpu_set=CONFIGS[CUSTOM_CPU_SET]):
    CONFIGS[CUSTOM_LIBRARY_NAME] = custom_library_name
    CONFIGS[OUTPUT_PATH]         = output_path
    CONFIGS[FUNCTIONS_LIST]      = functions_list
    CONFIGS[PACKAGE]             = package
    CONFIGS[ARCHITECTURE]        = architecture
    CONFIGS[THREAD_MODE]         = thread_mode
    CONFIGS[THREADING_LAYER]     = threading_layer_type
    CONFIGS[CUSTOM_CPU_SET]      = custom_cpu_set

    if not build_script_name:
        build_script_name = BUILD_SCRIPT_NAME_FORMAT[HOST_SYSTEM].format(name=custom_library_name, arch=architecture)
    CONFIGS[BUILD_SCRIPT_NAME] = build_script_name


def get_first_existing_path_in_list(paths_list):
    for path in paths_list:
        if os.path.exists(path):
            return path
    return ''


def get_lines_from_file(file_path):
    if os.path.exists(file_path):
        with open(file_path, 'r') as file:
            return file.readlines()
    else:
        return []


def get_env(env_var):
    return os.environ[env_var] if os.getenv(env_var) and os.path.exists(os.environ[env_var]) else ''


def get_match(regex, string, group):
    return re.match(regex, string).group(group) if re.compile(regex).match(string) else ''


def nested_dict_init():
    return defaultdict(lambda: defaultdict())


def walk_dict(dictionary):
    for key, value in dictionary.items():
        if type(value) == type(dict()) or type(value) == type(defaultdict()):
            for entire in walk_dict(value):
                yield (key,) + entire
        elif type(value) == type(list()):
            for elem in value:
                yield key, elem
        else:
            yield key, value


def get_dict_value(dictionary, key):
    return dictionary[key] if key in dictionary.keys() else dictionary['default']
