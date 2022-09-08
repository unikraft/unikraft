############################################################################
# Copyright 2016-2017 Intel Corporation
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
############################################################################

"""use scons -k to invoke all builds regardless of unit test failures
"""
import string
import sys
import SCons.Script
import os.path
import subprocess
from subprocess import Popen, PIPE
from parts import *
import re
import tempfile
import shutil
from collections import OrderedDict

def get_parts_versions(env):
    """Get Parts related versions given SCons environment env"""
    return OrderedDict({'python': string.split(sys.version, " ", 1)[0],
                        'scons': str(SCons.__version__),
                        'parts': str(PartsExtensionVersion())})

def get_toolchain_versions(env):
    """Get version of compilation toolchain given SCons environment env"""
    versions = OrderedDict()
    if 'MSVC_VERSION' in env:
        versions['compiler'] = 'MSVC ' + env['MSVC_VERSION']
        cmd = env.subst('echo int main(){return 0;} > a.cpp'
                        ' | $CXX $CCFLAGS a.cpp /link /verbose')
        defaultlib_regexp = r'.*Searching (.*\.lib).*'
    elif 'GCC_VERSION' in env:
        versions['compiler'] = 'GCC ' + env['GCC_VERSION']
        if 'GXX_VERSION' in env:
            versions['compiler'] += ' and GXX ' + env['GXX_VERSION']
            if os.name == 'nt':
                cmd = env.subst('echo int main(){return 0;}'
                                ' | $CXX $CCFLAGS -xc++ -Wl,--verbose -')
            else:
                cmd = env.subst('echo "int main(){return 0;}"'
                                ' | $CXX $CCFLAGS -xc++ -Wl,--verbose -')
        else:
            if os.name == 'nt':
                cmd = env.subst('echo int main(){return 0;}'
                                ' | $CXX $CCFLAGS -xc++ -Wl,--verbose -')
            else:
                cmd = env.subst('echo "int main(){return 0;}"'
                                ' | $CC  $CCFLAGS -xc   -Wl,--verbose -')
        if os.name == 'nt':
            defaultlib_regexp = r'\n.* open (.*) succeeded'
        else:
            defaultlib_regexp = r'[\n(](/.*\.so[-.\da-fA-F]*).*'

    # Intel C compiler always depends from base toolchain
    if 'INTELC_VERSION' in env:
        versions['compiler'] = 'INTELC {0} with {1}'.format(
            env['INTELC_VERSION'],
            versions['compiler'])

    env['ENV']['PATH'] = str(env['ENV']['PATH'])
    temp_dir = tempfile.mkdtemp()
    try:
        proc = subprocess.Popen(cmd,
                                cwd=temp_dir,
                                env=env['ENV'],
                                shell=True,
                                stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        stdout, _ = proc.communicate()
        if proc.returncode != 0:
            versions['default_libs'] = 'failure executing: "{0}"'.format(cmd)
        else:
            default_libs = list(
                set(re.findall(defaultlib_regexp, stdout, re.M)))
            if 'MSVC_VERSION' in env:
                # for windows additionally report versions of Windows Kit used
                runtime_version_set = set()
                for lib_path in default_libs:
                    path_components = os.path.realpath(lib_path).split(os.sep)
                    if 'Windows Kits' in path_components:
                        i = path_components.index('Windows Kits')
                        runtime_version_set.add(
                            'Windows Kits {0} {1}'.format(path_components[i + 1],
                                                          path_components[i + 3]))
                versions['sdk_or_libc'] = '; '.join(list(runtime_version_set))
            else:
                # for posix additionally report versions of libc used
                versions['sdk_or_libc'] = os.path.split(os.path.realpath(
                    next((lib for lib in default_libs if
                          'libc' in lib.lower() and 'libcilk' not in lib.lower()), None)))[1]
            versions['default_libs'] = default_libs
    finally:
        shutil.rmtree(temp_dir)

    return versions


def log_versions(env, include_toolchain=True):
    """Log tools and libraries versions given SCons environment env

    Args:
        env: Scons environment.
        include_toolchain: Log version of compilation toolchain if True.
    """

    versions = get_parts_versions(env)
    if include_toolchain:
        versions.update(get_toolchain_versions(env))

    print "**************** VERSIONS *************"
    long_names = {
        'python': 'Python Version',
        'scons': 'SCons  Version',
        'parts': 'Parts  Version',
        'compiler': 'Compiler Version',
        'sdk_or_libc': 'Libc/SDK',
        'default_libs': 'Default Libs'
    }
    for name, value in versions.iteritems():
        if not isinstance(value, list):
            print '* {0}: {1}'.format(long_names.get(name, name), value)
        else:
            print '* {0}:\n* \t{1}'.format(long_names.get(name, name),
                                           '\n* \t'.join(sorted(value)))
    print "***************************************"


def include_parts(part_list, **kwargs):
    for parts_file in part_list:
        if os.path.isfile(DefaultEnvironment().subst(parts_file)):
            Part(parts_file=parts_file, **kwargs)


######## Part groups ####################################################
ipp_parts = ['ext/ipp/ippcp.parts']
utest_parts = ['ext/gtest/gtest.parts',
               'epid/common-testhelper/common-testhelper.parts']
common_parts = ['epid/common/common.parts']
member_parts = ['epid/member/member.parts']
verifier_parts = ['epid/verifier/verifier.parts']
util_parts = ['example/util/util.parts']
example_parts = ['ext/argtable3/argtable3.parts',
                 'example/verifysig/verifysig.parts',
                 'example/signmsg/signmsg.parts',
                 'example/data/data.parts',
                 'example/compressed_data/compressed_data.parts']
sizing_parts = ['example/util/util_static.parts',
                'example/signmsg/signmsg_shared.parts',
                'example/verifysig/verifysig_shared.parts',
                'example/verifysig/verifysig11_shared.parts']
example_static_parts = ['example/util/util_static.parts',
                        'example/signmsg/signmsg_static.parts',
                        'example/verifysig/verifysig_static.parts']
tools_parts = ['tools/revokegrp/revokegrp.parts',
               'tools/revokekey/revokekey.parts',
               'tools/revokesig/revokesig.parts',
               'tools/extractkeys/extractkeys.parts',
               'tools/extractgrps/extractgrps.parts']
testbot_test_parts = ['test/testbot/testbot.parts',
                      'test/testbot/signmsg/signmsg_testbot.parts',
                      'test/testbot/verifysig/verifysig_testbot.parts',
                      'test/testbot/integration/integration_testbot.parts',
                      'test/testbot/ssh_remote/ssh_remote_testbot.parts',
                      'test/testbot/revokegrp/revokegrp_testbot.parts',
                      'test/testbot/revokekey/revokekey_testbot.parts',
                      'test/testbot/revokesig/revokesig_testbot.parts',
                      'test/testbot/extractkeys/extractkeys_testbot.parts',
                      'test/testbot/extractgrps/extractgrps_testbot.parts',
                      'tools/reports/reports.parts']
tss_test_parts = ['test/tss/tss.parts']
package_parts = ['ext/gtest/gtest.parts',
                 'ext/ipp/ippcp.parts',
                 'package.parts']
memory_profiler_parts = ['tools/memory_profiler/memory_profiler.parts']
internal_tools_parts = ['ext/argtable3/argtable3.parts',
                        'tools/ikgfwrapper/ikgfwrapper.parts']
epid_data = ['test/epid_data/epid_data.parts']
perf_benchmark_parts = ['ext/google_benchmark/google_benchmark.parts',
                        'test/performance/performance.parts']
memory_benchmark_parts = ['test/dynamic_memory/dynamic_memory.parts']
######## End Part groups ###############################################
######## Commandline option setup #######################################
product_variants = [
    'production',
    'internal-test',
    'package-epid-sdk',
    'internal-tools',
    'benchmark',
    'tiny',
    'internal-test-tiny'
]

default_variant = 'production'


def is_production():
    return GetOption("product-variant") == 'production'


def is_internal_test():
    return GetOption("product-variant") == 'internal-test'


def is_internal_tools():
    return GetOption("product-variant") == 'internal-tools'


def is_package():
    return GetOption("product-variant") == 'package-epid-sdk'


def is_benchmark():
    return GetOption("product-variant") == 'benchmark'

def is_tiny():
    return GetOption("product-variant") == 'tiny'

def is_internal_test_tiny():
    return GetOption("product-variant") == 'internal-test-tiny'


def use_commercial_ipp():
    return GetOption("use-commercial-ipp")


def use_tss():
    return GetOption("use-tss")


def config_has_instrumentation():
    return any(DefaultEnvironment().isConfigBasedOn(config_name)
               for config_name in ['instr_release'])


def variant_dirname():
    s = GetOption("product-variant")
    if s == 'production':
        return 'epid-sdk'
    elif s == 'package-epid-sdk':
        return 'epid-sdk'
    elif s == 'tiny':
        return 'epid-sdk'
    else:
        return s


AddOption("--product-variant", "--prod-var", nargs=1,
          help=("Select product variant to build. Possible "
                "options are: {0}. The default is {1} if no option "
                "is specified").format(", ".join(product_variants),
                                       default_variant),
          action='store', dest='product-variant', type='choice',
          choices=product_variants, default=default_variant)

AddOption("--use-commercial-ipp",
          help=("Link with commercial IPP. The IPPROOT environment variable "
                "must be set."),
          action='store_true', dest='use-commercial-ipp',
          default=False)

AddOption("--use-tss",
          help=("Link with TPM TSS. The TSSROOT environment variable "
                "must be set."),
          action='store_true', dest='use-tss',
          default=False)

AddOption("--ipp-shared",
          help=("Build /ext/ipp as shared library."),
          action='store_true', dest='ipp-shared',
          default=False)

AddOption("--enable-sanitizers",
          help=("Build with sanitizers (https://github.com/google/sanitizers)."),
          action='store_true', dest='sanitizers',
          default=False)

AddOption("--sanitizers-recover",
          help=("Configure sanititzers to recover and continue execution "
                "on error found. Only applicable when sanitizers are enabled."
                "See --enable-sanitizers option."),
          action='store_true', dest='sanitizers-recover',
          default=False)


SetOptionDefault("PRODUCT_VARIANT", variant_dirname())

######## End Commandline option setup ###################################


# fix for parts 0.10.8 until we get better logic to extract ${CC}
SetOptionDefault('PARTS_USE_SHORT_TOOL_NAMES', 1)


def enable_sanitizers(recover):
    """
        Configures compiler to enable sanitizers.
        Adds sanitizer options to default scons environment such
        that it affects all parts.
    Args:
        recover: Enable sanitizers recovery from errors found when True.
    """
    env = DefaultEnvironment()
    error_msg = None
    try:
       major = int(env.subst('$GCC_VERSION').partition('.')[0])
    except ValueError:
       major = 0

    if major >= 6 and env['TARGET_OS'] == 'posix':
        if 'INTELC_VERSION' not in env:
            ccflags = ['-fsanitize=address,undefined', '-fno-sanitize=alignment',
                       '-fno-sanitize=shift', '-fno-omit-frame-pointer']
            if recover:
                ccflags = ccflags + ['-fsanitize-recover=all', '-fsanitize-recover=address']
            else:
                ccflags = ccflags + ['-fno-sanitize-recover']
            # Extends default flags with sanitizer options
            SetOptionDefault('CCFLAGS', ccflags)
            SetOptionDefault('LIBS', ['asan', 'ubsan'])
        else:
            error_msg = """
                Build with sanitizers is not supported for Intel(R) C++ Compiler.
                Try scons --toolchain=gcc_6 --target=posix
                """
    else:
        # User experience with sanitizers in GCC 4.8 is not great. Use at least GCC 6.x.
        error_msg = """
            Build with sanitizers is only supported for GCC version greater than
            6.x targeting posix OS. Current GCC version is "{0}" and OS target is "{1}".
            Try scons --toolchain=gcc_6 --target=posix
            """.format(env.get('GCC_VERSION', 'unknown'), env.get('TARGET_OS', 'unknown'))
    if error_msg is not None:
        env.PrintError(error_msg)


def set_default_production_options():
    SetOptionDefault('CONFIG', 'release')

    SetOptionDefault('TARGET_VARIANT', '${TARGET_OS}-${TARGET_ARCH}')

    SetOptionDefault('INSTALL_ROOT',
                     '#_install/${PRODUCT_VARIANT}')

    SetOptionDefault('INSTALL_TOOLS_BIN',
                     '$INSTALL_ROOT/tools')

    SetOptionDefault('INSTALL_SAMPLE_BIN',
                     '$INSTALL_ROOT/example')

    SetOptionDefault('INSTALL_EPID_INCLUDE',
                     '$INSTALL_ROOT/include/epid')

    SetOptionDefault('INSTALL_IPP_INCLUDE',
                     '$INSTALL_ROOT/include/ext/ipp/include')

    SetOptionDefault('INSTALL_TEST_BIN',
                     '$INSTALL_ROOT/test')

    SetOptionDefault('INSTALL_LIB',
                     '$INSTALL_ROOT/lib/${TARGET_VARIANT}')

    SetOptionDefault('INSTALL_SAMPLE_DATA',
                     '$INSTALL_ROOT/example')

    SetOptionDefault('INSTALL_TOOLS_DATA',
                     '$INSTALL_ROOT/tools')

    SetOptionDefault('PACKAGE_DIR',
                     '#_package')

    SetOptionDefault('PACKAGE_ROOT',
                     '#_package/${PRODUCT_VARIANT}')

    SetOptionDefault('ROOT',
                     '#')

    SetOptionDefault('PACKAGE_NAME',
                     '{PRODUCT_VARIANT}')


if GetOption("sanitizers"):
    enable_sanitizers(GetOption("sanitizers-recover"))

if is_production():
    set_default_production_options()
    ipp_mode = ['install_lib']
    if use_commercial_ipp():
        ipp_mode.append('use_commercial_ipp')
    sdk_mode = ['install_lib']
    if use_tss():
        sdk_mode.append('use_tss')
    if GetOption('ipp-shared'):
        ipp_mode.append('build_ipp_shared')
    include_parts(ipp_parts, mode=ipp_mode,
                  INSTALL_INCLUDE='${INSTALL_IPP_INCLUDE}')
    include_parts(utest_parts + common_parts +
                  member_parts + verifier_parts,
                  mode=sdk_mode,
                  INSTALL_INCLUDE='${INSTALL_EPID_INCLUDE}')
    include_parts(util_parts + example_parts,
                  INSTALL_INCLUDE='${INSTALL_EPID_INCLUDE}',
                  INSTALL_BIN='${INSTALL_SAMPLE_BIN}',
                  INSTALL_DATA='${INSTALL_SAMPLE_DATA}')
    include_parts(tools_parts,
                  INSTALL_BIN='${INSTALL_TOOLS_BIN}',
                  INSTALL_DATA='${INSTALL_TOOLS_DATA}')
    Default('all')
    Default('utest::')
    if not use_tss():
        Default('run_utest::')

if is_internal_test():
    set_default_production_options()
    sdk_mode = []
    if use_tss():
        sdk_mode.append('use_tss')
        include_parts(tss_test_parts)
    include_parts(ipp_parts)
    include_parts(utest_parts + common_parts +
                  member_parts + verifier_parts,
                  mode=sdk_mode)
    include_parts(util_parts + example_parts,
                  INSTALL_BIN='${INSTALL_SAMPLE_BIN}',
                  INSTALL_DATA='${INSTALL_SAMPLE_DATA}')
    include_parts(sizing_parts,
                  INSTALL_BIN='${INSTALL_SAMPLE_BIN}')
    include_parts(tools_parts, INSTALL_BIN='${INSTALL_TOOLS_BIN}')
    include_parts(testbot_test_parts)
    Default('all')

if is_internal_tools():
    set_default_production_options()
    include_parts(ipp_parts + utest_parts + common_parts + verifier_parts + member_parts + util_parts)
    include_parts(internal_tools_parts + memory_profiler_parts,
                  INSTALL_BIN='${INSTALL_TOOLS_BIN}')
    Default('ikgfwrapper', 'memory_profiler')
    Default('run_utest::memory_profiler::')

if is_benchmark():
    set_default_production_options()
    MODE = []
    if config_has_instrumentation():
        MODE.append('use_memory_profiler')
    ipp_mode = []
    if use_commercial_ipp():
        ipp_mode.append('use_commercial_ipp')

    # install ipp static and ipp shared builds into separate locations
    if GetOption('ipp-shared'):
        ipp_mode.append('build_ipp_shared')
        SetOptionDefault('INSTALL_TEST_BIN',
                         '$INSTALL_ROOT/test_ipp_shared')
        SetOptionDefault('INSTALL_LIB',
                         '$INSTALL_ROOT/lib_ipp_shared')
    else:
        SetOptionDefault('INSTALL_LIB',
                         '$INSTALL_ROOT/lib')

    # do not allow file links to keep previous builds intact
    SetOptionDefault('CCOPY_LOGIC', 'copy')

    include_parts(ipp_parts, config_independent=True, mode=MODE + ipp_mode,
                  INSTALL_BIN='${INSTALL_TEST_BIN}')
    include_parts(example_static_parts + utest_parts + perf_benchmark_parts +
                  common_parts + verifier_parts +
                  sizing_parts + epid_data,
                  config_independent=True,
                  mode=MODE,
                  INSTALL_BIN='${INSTALL_TEST_BIN}')

    member_mode = ['install_lib']
    member_cfg = ('embedded' if not DefaultEnvironment().isConfigBasedOn(
        'debug') and not config_has_instrumentation() else DefaultEnvironment().subst('$CONFIG'))
    Part(parts_file='epid/common/tinycommon.parts', CONFIG=member_cfg)
    Part(parts_file='epid/member/tinymember.parts', CONFIG=member_cfg,
         config_independent=True, mode=MODE + member_mode, INSTALL_BIN='${INSTALL_TEST_BIN}')

    if config_has_instrumentation():
        include_parts(memory_benchmark_parts + memory_profiler_parts,
                      config_independent=True,
                      mode=MODE,
                      INSTALL_BIN='${INSTALL_TEST_BIN}')

    Default('build::')

if is_package():
    set_default_production_options()
    include_parts(package_parts,
                  mode=['install_package'],
                  INSTALL_TOP_LEVEL='${PACKAGE_ROOT}')
    Default('package')

if is_tiny():
    set_default_production_options()
    ### Member
    Part(parts_file='ext/gtest/gtest.parts')
    member_mode = ['install_lib']
    member_cfg = ('embedded'
                  if not DefaultEnvironment().isConfigBasedOn('debug')
                  else DefaultEnvironment().subst('$CONFIG'))
    Part(parts_file='epid/common/tinycommon.parts', CONFIG=member_cfg)
    Part(parts_file='epid/member/tinymember.parts', CONFIG=member_cfg,
         config_independent=True, mode=member_mode)
    Default('member::')
    Default('run_utest::member::')
    ### Verifier, samples and tools
    verifier_mode = ['install_lib']
    ipp_mode = ['install_lib']
    if use_commercial_ipp():
        ipp_mode.append('use_commercial_ipp')
    if GetOption('ipp-shared'):
        ipp_mode.append('build_ipp_shared')
    include_parts(ipp_parts, mode=ipp_mode,
                  INSTALL_INCLUDE='${INSTALL_IPP_INCLUDE}')
    Part(parts_file='epid/common-testhelper/common-testhelper.parts',
         config_independent=True)
    include_parts(common_parts + verifier_parts,
                  mode=verifier_mode,
                  INSTALL_INCLUDE='${INSTALL_EPID_INCLUDE}')
    include_parts(util_parts + example_parts,
                  INSTALL_INCLUDE='${INSTALL_EPID_INCLUDE}',
                  INSTALL_BIN='${INSTALL_SAMPLE_BIN}',
                  INSTALL_DATA='${INSTALL_SAMPLE_DATA}')
    include_parts(tools_parts,
                  INSTALL_BIN='${INSTALL_TOOLS_BIN}',
                  INSTALL_DATA='${INSTALL_TOOLS_DATA}')
    Default('all')
    Default('utest::')

if is_internal_test_tiny():
    set_default_production_options()
    sdk_mode = []
    ### Member
    Part(parts_file='ext/gtest/gtest.parts')
    member_cfg = ('embedded'
                  if not DefaultEnvironment().isConfigBasedOn('debug')
                  else DefaultEnvironment().subst('$CONFIG'))
    Part(parts_file='epid/common/tinycommon.parts', CONFIG=member_cfg)
    Part(parts_file='epid/member/tinymember.parts', CONFIG=member_cfg,
         config_independent=True, mode=sdk_mode)
    ### Verifier, samples and tools
    include_parts(ipp_parts)
    Part(parts_file='epid/common-testhelper/common-testhelper.parts',
         config_independent=True)
    include_parts(common_parts + verifier_parts,
                  mode=sdk_mode)
    include_parts(util_parts + example_parts,
                  INSTALL_BIN='${INSTALL_SAMPLE_BIN}',
                  INSTALL_DATA='${INSTALL_SAMPLE_DATA}')
    include_parts(tools_parts, INSTALL_BIN='${INSTALL_TOOLS_BIN}')
    include_parts(testbot_test_parts)
    Default('build::')

log_versions(DefaultEnvironment(), not is_package())
