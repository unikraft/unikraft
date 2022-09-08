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

from tool.utils import *

ENV_VAR = {
    WINDOWS : '%{env_var}%',
    LINUX   : '${{{env_var}}}',
    MACOSX  : '${{{env_var}}}'
}

CALL_ENV_SCRIPT_COMMAND = {
    WINDOWS : 'call "{env_script}" {arch}',
    LINUX   : 'source "{env_script}" -arch {arch}',
    MACOSX  : 'source "{env_script}" -arch {arch}'
}

SET_ENV_COMMAND = {
    WINDOWS : 'set "{env_var}={path}"',
    LINUX   : 'export "{env_var}={path}"',
    MACOSX  : 'export "{env_var}={path}"'
}

COMPILERS = {
    WINDOWS : 'cl.exe',
    LINUX   : 'g++',
    MACOSX  : 'clang'
}

LINKERS = {
    WINDOWS : 'link.exe',
    LINUX   : 'g++',
    MACOSX  : 'clang'
}

COMPILERS_FLAGS = {
    WINDOWS: {
        INTEL64 : '/c /MT /GS /sdl /O2',
        IA32    : '/c /MT /GS /sdl /O2'
    },
    LINUX: {
        INTEL64: '-c -m64 -fPIC -fPIE -fstack-protector-strong '
                 + '-fstack-protector -O2 -D_FORTIFY_SOURCE=2 '
                 + '-Wformat -Wformat-security',
        IA32:    '-c -m32 -fPIC -fPIE -fstack-protector-strong '
                 + '-fstack-protector -O2 -D_FORTIFY_SOURCE=2 '
                 + '-Wformat -Wformat-security'
    },
    MACOSX: {
        INTEL64 : '-c -m64',
        IA32    : '-c -m32'
    }
}

COMPILE_COMMAND_FORMAT = {
    WINDOWS: '{compiler} {cmp_flags} '
             + '/I "%{root_type}%\\include" '
             + '"{file_name}.c" /Fo:"{file_name}.obj"\n',
    LINUX: '{compiler} {cmp_flags}'
           + ' -I "${root_type}/include" '
           + '"{file_name}.c" -o "{file_name}.obj"\n',
    MACOSX: '{compiler} {cmp_flags} '
            + '-I "${root_type}/include" '
            + '"{file_name}.c" -o "{file_name}.obj"\n'
}

LINKER_FLAGS = {
    WINDOWS: {
        INTEL64 : '/MACHINE:X64 /NXCompat /DynamicBase ',
        IA32    : '/MACHINE:X86 /SafeSEH /NXCompat /DynamicBase'
    },
    LINUX: {
        INTEL64 : '-z noexecstack -z relro -z now',
        IA32    : '-m32 -z noexecstack -z relro -z now'
    },
    MACOSX: {
        INTEL64: '-dynamiclib -single_module '
                 + '-flat_namespace -headerpad_max_install_names '
                 + '-current_version 2017.0 -compatibility_version 2017.0',
        IA32:    '-dynamical -single_module '
                 + '-flat_namespace -headerpad_max_install_names '
                 + '-current_version 2017.0 -compatibility_version 2017.0'
    }
}

SYS_LIBS_PATH = {
    WINDOWS: {
        INTEL64 : '',
        IA32    : '',
    },
    LINUX: {
        INTEL64 : '$SYSROOT/lib64',
        IA32    : '$SYSROOT/lib'
    },
    MACOSX: {
        INTEL64 : '',
        IA32    : ''
    }
}

EXP_LIBS = {
    WINDOWS: {
        SINGLE_THREADED : '',
        MULTI_THREADED  : 'libiomp5md.lib',
        TBB             : '',
        OPENMP          : 'libiomp5md.lib'
    },
    LINUX: {
        SINGLE_THREADED : '',
        MULTI_THREADED  : '-liomp5',
        TBB             : '-ltbb',
        OPENMP          : '-liomp5'
    },
    MACOSX:  {
        SINGLE_THREADED : '',
        MULTI_THREADED  : '-liomp5',
        TBB             : '-ltbb',
        OPENMP          : '-liomp5'
    }
}

MAIN_FILE = {
    WINDOWS: "#include <Windows.h>\n"
             "#include \"{package_type}.h\"\n\n"
             "int WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)\n"
             "{{\n"
             "    switch (fdwReason)\n"
             "    {{\n"
             "    case DLL_PROCESS_ATTACH:\n"
             "        {package_type}Init(); break;\n"
             "    case DLL_THREAD_ATTACH: break;\n"
             "    case DLL_THREAD_DETACH: break;\n"
             "    case DLL_PROCESS_DETACH: break;\n"
             "    default: break;\n"
             "    }}\n"
             "    return 1;\n"
             "    UNREFERENCED_PARAMETER(hinstDLL);\n"
             "    UNREFERENCED_PARAMETER(lpvReserved);\n"
             "}}\n",
    LINUX: "#include \"{package_type}.h\"\n\n"
           "int _init(void)\n"
           "{{\n"
           "    {package_type}Init();\n"
           "    return 1;\n"
           "}}\n\n"
           "void _fini(void)\n"
           "{{\n"
           "}}\n",
    MACOSX: "#include \"{package_type}.h\"\n\n"
            "__attribute__((constructor)) void initializer( void )\n"
            "{{\n"
            "    static int initialized = 0;\n"
            "    if (!initialized)\n"
            "    {{\n"
            "        initialized = 1;\n"
            "    }}\n\n"
            "    {package_type}Init();\n"
            "    return;\n"
            "}}\n\n"
            "__attribute__((destructor)) void destructor()\n"
            "{{\n"
            "}}\n"
}

CUSTOM_DISPATCHER_FILE = '{include_lines}\n'\
                         '#define IPPFUN(type,name,arg) extern type IPP_CALL name arg\n\n'\
                         '#ifndef NULL\n'\
                         '#ifdef  __cplusplus\n'\
                         '#define NULL    0\n'\
                         '#else\n'\
                         '#define NULL    ((void *)0)\n'\
                         '#endif\n'\
                         '#endif\n\n'\
                         '#define AVX3X_FEATURES ( ippCPUID_AVX512F|ippCPUID_AVX512CD|'\
                         'ippCPUID_AVX512VL|ippCPUID_AVX512BW|ippCPUID_AVX512DQ )\n'\
                         '#define AVX3M_FEATURES ( ippCPUID_AVX512F|ippCPUID_AVX512CD|'\
                         'ippCPUID_AVX512PF|ippCPUID_AVX512ER )\n\n'\
                         '#ifdef __cplusplus\n'\
                         'extern "C" {{\n'\
                         '#endif\n\n'\
                         '{dispatcher}'\
                         '#ifdef __cplusplus\n'\
                         '}}\n'\
                         '#endif\n\n'\

INCLUDE_STR = '#include "{header_name}.h"\n'

FUNCTION_DISPATCHER = '{ippapi}\n'\
                      '{ippfun}\n'\
                      '{{\n'\
                      '    Ipp64u _features;\n'\
                      '    {package_type}GetCpuFeatures( &_features{second_arg} );\n\n'\
                      '{dispatching_scheme}'\
                      '}}\n\n'

DISPATCHING_SCHEME_FORMAT = '    if( {cpuid}  == ( _features & {cpuid}  )) {{\n'\
                            '        return {function}( {args} );\n'\
                            '    }} else \n'

RETURN_VALUES = {
    'IppStatus' : 'ippStsCpuNotSupportedErr',
    'IppiRect'  : '(IppiRect) { IPP_MIN_32S / 2, IPP_MIN_32S / 2, '
                               'IPP_MAX_32S, IPP_MAX_32S }',
    'void'      : '',
    'default'   : 'NULL'
}

BUILD_SCRIPT = {
    WINDOWS: ':: Generates {threading} dynamic library '
             + 'for {architecture} architecture\n'
             + '@echo off\n'
             + '{env_command}\n'
             + 'set "OUTPUT_PATH={output_path}"\n\n'
             + 'if not exist %OUTPUT_PATH% mkdir %OUTPUT_PATH%\n'
             + 'cd /d %OUTPUT_PATH%\n\n'
             + 'if exist "{custom_library}.dll" del "{custom_library}.dll"\n\n'
             + '{compile_commands}\n'
             + '{linker} /DLL {link_flags} /VERBOSE:SAFESEH '
             + '/DEF:"{export_file}" {obj_files}'
             + '/OUT:"{custom_library}.dll" /IMPLIB:"{custom_library}.lib" '
             + '{ipp_libraries} '
             + '{exp_libs}\n\n'
             + 'if exist "{custom_library}.dll" (\n'
             + '    echo Build completed!\n'
             + ') else (\n'
             + '    echo Build failed!\n'
             + '    exit /b 1\n'
             + ')',
    LINUX: '#!/bin/bash\n'
           + '# Generates {threading} dynamic library '
           + 'for {architecture} architecture\n'
           + '{env_command}\n'
           + 'export LIBRARY_PATH=$LD_LIBRARY_PATH\n'
           + 'OUTPUT_PATH="{output_path}"\n\n'
           + 'mkdir -p $OUTPUT_PATH\n'
           + 'cd $OUTPUT_PATH\n\n'
           + 'rm -rf "{custom_library}.so"\n\n'
           + '{compile_commands}\n'
           + '{linker} -shared {link_flags} '
           + '"{export_file}" {obj_files}'
           + '-o "{custom_library}.so" '
           + '{ipp_libraries} '
           + '-L"{sys_libs_path}" -lc -lm {exp_libs}\n\n'
           + 'if [ -f "{custom_library}.so" ]; then\n'
           + '    echo Build completed!\n'
           + 'else\n'
           + '    echo Build failed!\n'
           + '    exit 1\n'
           + 'fi',
    MACOSX: '#!/bin/bash\n'
            + '# Generates {threading} dynamic library '
            + 'for {architecture} architecture\n'
            + '{env_command}\n'
            + 'export LIBRARY_PATH=$DYLD_LIBRARY_PATH\n'
            + 'OUTPUT_PATH="{output_path}"\n\n'
            + 'mkdir -p $OUTPUT_PATH\n'
            + 'cd $OUTPUT_PATH\n\n'
            + 'rm -rf "{custom_library}.dylib"\n\n'
            + '{compile_commands}\n'
            + '{linker} {link_flags} '
            + '-install_name @rpath/{custom_library}.dylib '
            + '-o "{custom_library}.dylib" '
            + '-exported_symbols_list "{export_file}" {obj_files} '
            + '{ipp_libraries} '
            + '-lgcc_s.1 -lm {exp_libs}\n\n'
            + 'if [ -f "{custom_library}.dylib" ]; then\n'
            + '    echo Build completed!\n'
            + 'else\n'
            + '    echo Build failed!\n'
            + '    exit 1\n'
            + 'fi'
}
