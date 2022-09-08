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
from collections import defaultdict

from tool import utils


class Package:
    def __init__(self, path):
        self.root          = path
        self.headers_dir   = os.path.join(self.root, 'include')
        self.libraries_dir = os.path.join(self.root, 'lib')

        self.headers      = utils.nested_dict_init()
        self.functions    = utils.nested_dict_init()
        self.declarations = defaultdict()

        self.libraries   = utils.nested_dict_init()
        self.features    = utils.nested_dict_init()
        self.errors      = utils.nested_dict_init()

        self.functions_without_dispatcher = []

        self.set_type()
        self.set_env_script()

        self.set_headers_functions_declarations_dicts()
        self.set_libraries_features_errors_dicts(self.type,             utils.THREAD_MODES, '')
        self.set_libraries_features_errors_dicts(utils.THREADING_LAYER, utils.TL_TYPES,     '_tl')

        self.package_validation()
        self.set_name()

    def set_type(self):
        header = os.path.join(self.headers_dir, 'ippcp.h')
        self.type = (utils.IPP if not os.path.exists(header) else utils.IPPCP)

    def set_name(self):
        version = ''
        self.name = 'None'
        header = os.path.join(self.headers_dir, 'ippversion.h')

        for line in utils.get_lines_from_file(header):
            version = utils.get_match(utils.VERSION_REGEX, line, 'ver')
            if version:
                break
        if not version:
            version = 'None'

        if not self.broken:
            self.name = utils.PACKAGE_NAME[self.type] + ' Version ' + version

    def set_env_script(self):
        paths_to_search = []
        batch_extension = utils.BATCH_EXTENSIONS[utils.HOST_SYSTEM]

        components_install_dir = utils.get_match(utils.COMPONENTS_INSTALL_DIR_REGEX, self.root, 'path')
        if components_install_dir:
            paths_to_search.append(os.path.join(components_install_dir, 'setvars' + batch_extension))
            paths_to_search.append(os.path.join(components_install_dir, 'bin', 'compilervars' + batch_extension))
        paths_to_search.append(os.path.join(self.root, 'bin', self.type.lower() + 'vars' + batch_extension))
        paths_to_search.append(os.path.join(self.root, 'env', 'vars' + batch_extension))

        self.env_script = utils.get_first_existing_path_in_list(paths_to_search)

    def set_headers_functions_declarations_dicts(self):
        if not os.path.exists(self.headers_dir):
            return

        headers = [header for header in os.listdir(self.headers_dir) if '.h' in header]

        for header in headers:
            domain_type, domain = self.get_type_and_domain(header)
            if not domain:
                continue

            functions_list = []
            incomplete_function = ''
            for line in utils.get_lines_from_file(os.path.join(self.headers_dir, header)):
                if incomplete_function:
                    self.declarations[incomplete_function] += ' ' + " ".join(line.split())
                    incomplete_function = self.get_function_if_incomplete(incomplete_function)
                    continue

                function = utils.get_match(utils.FUNCTION_NAME_REGEX, line, 'function_name')
                if not function:
                    continue
                functions_list.append(function)
                self.declarations[function] = " ".join(line.split())
                incomplete_function = self.get_function_if_incomplete(function)

            if not functions_list:
                continue

            self.headers[domain_type][domain] = header

            domain_name = utils.DOMAINS[domain_type][domain]
            if domain_name not in self.functions[domain_type]:
                self.functions[domain_type][domain_name] = functions_list
            else:
                self.functions[domain_type][domain_name] += functions_list

            if header == 'ippcpdefs.h' or \
               domain_type == utils.THREADING_LAYER or \
               domain == 'ippcore':
                self.functions_without_dispatcher += functions_list

    def set_libraries_features_errors_dicts(self, domains_type, thread_types, domains_postfix):
        host = utils.HOST_SYSTEM

        for arch in utils.ARCHITECTURES:
            for thread_type in thread_types:
                self.libraries[arch][thread_type] = []
                path_to_libraries = self.get_path_to_libraries(arch, thread_type)

                for domain in utils.DOMAINS[domains_type].keys():
                    lib_name = utils.LIB_PREFIX[host] + \
                               domain + \
                               utils.STATIC_LIB_POSTFIX[host] + \
                               domains_postfix + \
                               utils.STATIC_LIB_EXTENSION[host]

                    lib_path = os.path.join(path_to_libraries,
                                            lib_name)

                    if os.path.exists(lib_path):
                        self.libraries[arch][thread_type].append(lib_path)
                    elif not domain == 'ippe':
                        self.libraries[arch][thread_type].clear()
                        break

                self.features[arch][thread_type] = bool(self.libraries[arch][thread_type])
                self.errors[arch][thread_type]   = self.check_headers_and_libs(arch, domains_type, thread_type)

    def package_validation(self):
        self.broken        = True
        self.error_message = self.errors[utils.INTEL64][utils.SINGLE_THREADED]

        for arch in utils.ARCHITECTURES:
            for thread in utils.THREAD_MODES:
                if self.features[arch][thread]:
                    self.broken = False
                    return

    def get_type_and_domain(self, header):
        domain_type = (self.type if '_tl' not in header else utils.THREADING_LAYER)
        for domain in utils.DOMAINS[domain_type].keys():
            if domain in header:
                return domain_type, domain
        return '', ''

    def get_function_if_incomplete(self, function):
        if self.declarations[function].count('(') != self.declarations[function].count(')'):
            return function
        return ''

    def get_path_to_libraries(self, arch, thread_type):
        paths_to_search = [os.path.join(self.libraries_dir, arch),
                           os.path.join(self.libraries_dir, arch + '_' + utils.HOST_SYSTEM[:3].lower()),
                           self.libraries_dir]
        paths_to_search = [utils.PATH_TO_LIBRARIES[thread_type].format(libs_arch_dir=path)
                           for path in paths_to_search]

        return utils.get_first_existing_path_in_list(paths_to_search)

    def check_headers_and_libs(self, arch, domains_type, thread_type):
        for domain in utils.DOMAINS[domains_type].keys():
            if domains_type not in self.headers.keys() or \
                    (domain not in self.headers[domains_type].keys() and not domain == 'ippe'):
                return "Broken package - cannot find header files for " + domains_type + " functions"

        if not self.libraries[arch][thread_type]:
            return "Cannot find " + thread_type + " libraries for " + arch + " architecture"

        return ''


def get_package_path():
    current_path = os.path.realpath(__file__)
    paths_to_search = [utils.get_match(utils.PATH_TO_PACKAGE_REGEX, current_path, 'path'),
                       utils.get_env(utils.IPPROOT),
                       utils.get_env(utils.IPPCRYPTOROOT)]

    return utils.get_first_existing_path_in_list(paths_to_search)
