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
import re

from PyQt5 import QtCore
from PyQt5.QtWidgets import QWidget, QLabel, QPushButton, QGroupBox, QRadioButton, QCheckBox, QGridLayout, \
    QVBoxLayout, QFileDialog, QMessageBox

import tool
from tool import utils


class SettingsPanel(QWidget):
    package_changed = QtCore.pyqtSignal()
    tl_changed      = QtCore.pyqtSignal()

    def __init__(self):
        super().__init__()

        # Initializing GUI elements
        self.current_package = QLabel()
        self.select_package  = QPushButton('Select package')

        self.settings = QGroupBox('Settings')

        self.arch_group = QGroupBox('Architecture')
        self.ia32       = QRadioButton('IA32')
        self.intel64    = QRadioButton('Intel® 64')

        self.thread_group    = QGroupBox('Thread mode')
        self.single_threaded = QRadioButton('Single-threaded')
        self.multi_threaded  = QRadioButton('Multi-threaded')

        self.tl_group = QGroupBox('Threading layer')
        self.tbb      = QRadioButton('TBB')
        self.omp      = QRadioButton('OpenMP')

        self.custom_dispatch = QGroupBox('Custom dispatcher')
        self.sse2            = QCheckBox('SSE2')
        self.sse3            = QCheckBox('SSE3')
        self.ssse3           = QCheckBox('SSSE3')
        self.sse42           = QCheckBox('SSE4.2')
        self.avx             = QCheckBox('AVX')
        self.avx2            = QCheckBox('AVX2')
        self.avx512f         = QCheckBox('AVX512F')
        self.avx512bw        = QCheckBox('AVX512BW')

        self.tl_group.setCheckable(True)
        self.custom_dispatch.setCheckable(True)

        self.select_package.setFixedWidth(130)

        # Setting all widgets in their places
        layout = QGridLayout()
        layout.addWidget(self.select_package, 0, 0)
        layout.addWidget(self.current_package, 0, 1)
        layout.addWidget(self.settings, 1, 0, 1, 3)
        self.setLayout(layout)

        settings_layout = QGridLayout()
        settings_layout.addWidget(self.arch_group, 0, 0)
        settings_layout.addWidget(self.thread_group, 0, 1)
        settings_layout.addWidget(self.tl_group, 0, 2)
        settings_layout.addWidget(self.custom_dispatch, 0, 3)
        self.settings.setLayout(settings_layout)

        arch_layout = QVBoxLayout()
        arch_layout.addWidget(self.intel64)
        arch_layout.addWidget(self.ia32)
        self.arch_group.setLayout(arch_layout)

        thread_layout = QVBoxLayout()
        thread_layout.addWidget(self.single_threaded)
        thread_layout.addWidget(self.multi_threaded)
        self.thread_group.setLayout(thread_layout)

        tl_layout = QVBoxLayout()
        tl_layout.addWidget(self.tbb)
        tl_layout.addWidget(self.omp)
        self.tl_group.setLayout(tl_layout)

        custom_dispatch_layout = QGridLayout()

        custom_dispatch_layout.addWidget(self.sse2, 1, 0)
        custom_dispatch_layout.addWidget(self.sse3, 1, 1)
        custom_dispatch_layout.addWidget(self.ssse3, 1, 2)
        custom_dispatch_layout.addWidget(self.sse42, 1, 3)

        custom_dispatch_layout.addWidget(self.avx, 2, 0)
        custom_dispatch_layout.addWidget(self.avx2, 2, 1)
        custom_dispatch_layout.addWidget(self.avx512f, 2, 2)
        custom_dispatch_layout.addWidget(self.avx512bw, 2, 3)

        self.custom_dispatch.setLayout(custom_dispatch_layout)

        self.select_package.clicked.connect(self.on_select_package)
        self.ia32.toggled.connect(lambda checked: checked and self.on_switch_arch())
        self.intel64.toggled.connect(lambda checked: checked and self.on_switch_arch())
        self.tl_group.clicked.connect(self.on_switch_tl)
        self.custom_dispatch.clicked.connect(self.on_switch_custom_dispatch)

        root = tool.package.get_package_path()
        self.package = tool.package.Package(root)

    def init_settings(self):
        self.current_package.setText('Current package: ' + self.package.name)
        self.disable_widgets()

        if not self.package.broken:
            arch_flags = {arch: any([self.package.features[arch][thread] for thread in utils.THREAD_MODES])
                          for arch in utils.ARCHITECTURES}
            self.refresh_group(self.arch_group, flags_dict=arch_flags)

        self.package_changed.emit()

    def on_switch_arch(self):
        self.current_arch = (utils.IA32 if self.ia32.isChecked() else utils.INTEL64)

        self.refresh_group(self.thread_group, flags_dict=self.package.features[self.current_arch])
        self.refresh_group(self.tl_group,     flags_dict=self.package.features[self.current_arch])
        self.on_switch_custom_dispatch()

    def on_switch_tl(self):
        self.refresh_group(self.tl_group, flags_dict=self.package.features[self.current_arch])
        self.tl_changed.emit()

    def on_switch_custom_dispatch(self):
        self.refresh_group(self.custom_dispatch,
                           search_list=utils.SUPPORTED_CPUS[self.current_arch][utils.HOST_SYSTEM])

    def on_select_package(self):
        while True:
            chosen_path = QFileDialog.getExistingDirectory(self, 'Select package')
            if not chosen_path:
                break

            package = tool.package.Package(chosen_path)
            if package.broken:
                QMessageBox.information(self, 'ERROR!', package.error_message)
            else:
                self.package = package
                self.init_settings()
                break

    def refresh_group(self, group, flags_dict={}, search_list=[]):
        group.setDisabled(True)

        for button in group.findChildren(QRadioButton) + group.findChildren(QCheckBox):
            button_name = self.get_formatted_button_name(button)

            if flags_dict and flags_dict[button_name] or button_name in search_list:
                group.setEnabled(True)
                if not group.isCheckable() or group.isCheckable() and group.isChecked():
                    button.setEnabled(True)
                continue

            button.setChecked(False)
            button.setEnabled(False)

        self.check_group(group)

    def check_group(self, group):
        if not any([button.isChecked() for button in group.findChildren(QRadioButton)]):
            for radiobutton in group.findChildren(QRadioButton):
                if radiobutton.isEnabled():
                    radiobutton.setChecked(True)
                    break

        for checkbox in group.findChildren(QCheckBox):
            checkbox.setChecked(checkbox.isEnabled())

    def disable_widgets(self):
        for group in self.settings.findChildren(QGroupBox):
            for button in group.findChildren(QRadioButton):
                button.setAutoExclusive(False)
                button.setChecked(False)
                button.setAutoExclusive(True)

            for button in group.findChildren(QCheckBox):
                button.setChecked(False)

            group.setChecked(False)
            group.setEnabled(False)

    def get_formatted_button_name(self, button):
        return re.sub('[^\w-]', '', button.text().lower())
