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
from PyQt5 import QtCore
from PyQt5.QtWidgets import QWidget, QLineEdit, QListWidget, QPushButton, QVBoxLayout, QListWidgetItem


class CustomFunctionsPanel(QWidget):
    save_script = QtCore.pyqtSignal()

    def __init__(self, settings):
        super().__init__()
        self.settings = settings

        # Initializing GUI elements
        self.lib_name           = QLineEdit()
        self.functions_list     = QListWidget()
        self.save_script_button = QPushButton('Save build script')

        # Preparing elements by giving initial values and etc
        self.lib_name.setPlaceholderText('Custom library name...')

        # Setting all widgets in their places
        layout = QVBoxLayout()
        layout.addWidget(self.lib_name)
        layout.addWidget(self.functions_list)
        layout.addWidget(self.save_script_button)
        self.setLayout(layout)

        self.save_script_button.clicked.connect(self.on_save_script)
        self.settings.package_changed.connect(self.reset)

    def on_save_script(self):
        self.save_script.emit()

    def reset(self):
        self.functions_list.clear()

    def add_function(self, function):
        """
        Adds new function to required list

        :param function: name if function
        """
        self.functions_list.addItem(QListWidgetItem(function))

    def remove_function(self, function):
        """
        Removes function from left list
        """
        item = self.functions_list.findItems(function, QtCore.Qt.MatchExactly)
        if item:
            self.functions_list.takeItem(self.functions_list.row(item[0]))
