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

from PyQt5.QtWidgets import QWidget, QPushButton, QVBoxLayout, QFileDialog, QMessageBox, QLabel, QCheckBox

import tool
from tool import utils
from tool.core import build, generate_script


class Controller(QWidget):
    def change_state(function):
        def wrapper(self, *args):
            function(self, *args)
            self.check_current_state()

        return wrapper

    def __init__(self, parent, settings, left_side_menu, right_side_menu):
        super().__init__()
        self.parent     = parent
        self.settings   = settings
        self.left       = left_side_menu
        self.right      = right_side_menu

        self.to_right = QPushButton('>>')
        self.to_left  = QPushButton('<<')

        layout = QVBoxLayout()
        layout.addWidget(self.to_right)
        layout.addWidget(self.to_left)
        self.setLayout(layout)

        self.status_message = QLabel()
        self.parent.statusBar().addWidget(self.status_message)

        self.to_right.pressed.connect(self.on_right_press)
        self.to_left.pressed.connect(self.on_left_press)
        self.settings.package_changed.connect(self.check_current_state)
        self.left.autobuild.connect(self.autobuild)
        self.right.save_script.connect(self.save_build_script)

        self.settings.init_settings()

    @change_state
    def on_right_press(self):
        function = self.left.functions_list.currentItem()
        if function:
            self.left.remove_function(function.text())
            self.right.add_function(function.text())

    @change_state
    def on_left_press(self):
        function = self.right.functions_list.currentItem()
        if function:
            self.right.remove_function(function.text())
            self.left.add_function(function.text())

    def check_current_state(self):
        state = self.get_current_state()

        if state == utils.ENABLE_GENERATION_RULES:
            self.enable_generation(True)
            self.status_message.setText("Ready to build custom library")
        else:
            self.enable_generation(False)
            differences = dict(utils.ENABLE_GENERATION_RULES.items() - state.items())
            self.status_message.setText("Select " + sorted(differences, key=len)[0])

    def enable_generation(self, flag):
        self.left.autobuild_button.setEnabled(flag)
        self.right.save_script_button.setEnabled(flag)

    def autobuild(self):
        self.get_selected_configs()

        extension = 'Dynamic library (*' + utils.DYNAMIC_LIB_EXTENSION[utils.HOST_SYSTEM] + ')'
        chosen_path = QFileDialog.getSaveFileName(self,
                                                  'Save custom library as...',
                                                  utils.CONFIGS[utils.CUSTOM_LIBRARY_NAME],
                                                  extension)[0]
        if not chosen_path:
            return
        else:
            utils.CONFIGS[utils.CUSTOM_LIBRARY_NAME] = os.path.basename(os.path.splitext(chosen_path)[0])
            utils.CONFIGS[utils.OUTPUT_PATH]         = os.path.dirname(chosen_path)

        self.parent.setDisabled(True)
        QMessageBox.information(self,
                                'Build',
                                'Building will start after this window is closed. '
                                'Please, wait until process is done.')
        success = build()
        QMessageBox.information(self,
                                'Success' if success else 'Failure',
                                'Build completed!' if success else 'Build failed!')
        self.parent.setDisabled(False)

    def save_build_script(self):
        self.get_selected_configs()

        extension = 'Script (*' + utils.BATCH_EXTENSIONS[utils.HOST_SYSTEM] + ')'
        script_path = QFileDialog.getSaveFileName(self,
                                                  'Save build script as...',
                                                  utils.CONFIGS[utils.BUILD_SCRIPT_NAME],
                                                  extension)[0]
        if not script_path:
            return
        else:
            utils.CONFIGS[utils.BUILD_SCRIPT_NAME] = os.path.basename(os.path.splitext(script_path)[0] +
                                                                      utils.BATCH_EXTENSIONS[utils.HOST_SYSTEM])
            utils.CONFIGS[utils.OUTPUT_PATH]       = os.path.dirname(script_path)

        success = generate_script()
        QMessageBox.information(self,
                                'Success' if success else 'Failure',
                                'Generation completed!' if success else 'Generation failed!')

    def get_current_state(self):
        return {utils.HAVE_PACKAGE   : not self.settings.package.broken,
                utils.HAVE_FUNCTIONS : bool(self.right.functions_list.count())}

    def get_selected_configs(self):
        """
        Collecting all user-specified information about future dynamic library into dictionary
        """
        if not self.settings.package.broken:
            custom_library_name = (self.right.lib_name.text() if self.right.lib_name.text() else
                                   utils.CONFIGS[utils.CUSTOM_LIBRARY_NAME])
            functions_list = []
            for i in range(self.right.functions_list.count()):
                functions_list.append(self.right.functions_list.item(i).text())

            architecture = (utils.IA32 if self.settings.ia32.isChecked() else utils.INTEL64)
            thread_mode  = (utils.SINGLE_THREADED if self.settings.single_threaded.isChecked() else utils.MULTI_THREADED)

            if self.settings.tbb.isChecked():
                tl_type = utils.TBB
            elif self.settings.omp.isChecked():
                tl_type = utils.OPENMP
            else:
                tl_type = ''

            custom_cpu_set = [self.settings.get_formatted_button_name(cpu)
                              for cpu in self.settings.custom_dispatch.findChildren(QCheckBox) if cpu.isChecked()]

            utils.set_configs_dict(package=self.settings.package,
                                   functions_list=functions_list,
                                   architecture=architecture,
                                   thread_mode=thread_mode,
                                   threading_layer_type=tl_type,
                                   custom_library_name=custom_library_name,
                                   custom_cpu_set=custom_cpu_set)

    @change_state
    def set_configs(self, configs):
        self.settings.package = tool.package.Package(configs[utils.PACKAGE])
        self.settings.init_settings()

        if configs[utils.ARCHITECTURE] == utils.IA32:
            self.settings.ia32.setChecked(True)
        if configs[utils.ARCHITECTURE] == utils.INTEL64:
            self.settings.intel64.setChecked(True)

        if configs[utils.THREAD_MODE] == utils.SINGLE_THREADED:
            self.settings.single_threaded.setChecked(True)
        if configs[utils.THREAD_MODE] == utils.MULTI_THREADED:
            self.settings.multi_threaded.setChecked(True)

        if configs[utils.THREADING_LAYER] == utils.TBB:
            self.settings.tbb.setChecked(True)
        if configs[utils.THREADING_LAYER] == utils.OPENMP:
            self.settings.omp.setChecked(True)

        if configs[utils.CUSTOM_CPU_SET]:
            self.settings.custom_dispatch.setChecked(True)
            self.settings.on_switch_custom_dispatch()
            for cpu in self.settings.custom_dispatch.findChildren(QCheckBox):
                cpu_name = self.settings.get_formatted_button_name(cpu)
                if cpu_name in configs[utils.CUSTOM_CPU_SET]:
                    cpu.setChecked(True)

        self.right.lib_name.setText(configs[utils.CUSTOM_LIBRARY_NAME])

        self.right.reset()
        for function in configs[utils.FUNCTIONS_LIST]:
            self.left.remove_function(function)
            self.right.add_function(function)
