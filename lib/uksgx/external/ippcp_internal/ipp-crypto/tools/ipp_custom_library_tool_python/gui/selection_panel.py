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
import copy

from PyQt5 import QtCore
from PyQt5.QtWidgets import QWidget, QComboBox, QLineEdit, QListWidget, QPushButton, QVBoxLayout, QListWidgetItem

from tool import utils


class SelectionPanel(QWidget):
    autobuild = QtCore.pyqtSignal()

    def __init__(self, settings):
        super().__init__()
        self.settings = settings

        # Initializing GUI elements
        self.domains_list      = QComboBox(self)
        self.search            = QLineEdit(self)
        self.functions_list    = QListWidget(self)
        self.autobuild_button  = QPushButton('Autobuild')

        # Preparing elements by giving initial values, etc
        self.setMinimumHeight(500)
        self.search.setPlaceholderText('Search...')

        # Setting all widgets in their places
        layout = QVBoxLayout()
        layout.addWidget(self.domains_list)
        layout.addWidget(self.search)
        layout.addWidget(self.functions_list)
        layout.addWidget(self.autobuild_button)
        self.setLayout(layout)

        self.domains_list.activated[str].connect(self.on_select_domain)
        self.search.textEdited.connect(self.on_search)
        self.autobuild_button.clicked.connect(self.on_autobuild)
        self.settings.package_changed.connect(self.init_selection_panel)
        self.settings.tl_changed.connect(self.refresh)

    def init_selection_panel(self):
        if not self.settings.package.broken:
            self.functions_dict = copy.deepcopy(self.settings.package.functions)
            self.search.setEnabled(True)
            self.refresh()
        else:
            self.search.setEnabled(False)
            self.reset()

    def refresh(self):
        self.domains_type = (self.settings.package.type if not self.settings.tl_group.isChecked()
                             else utils.THREADING_LAYER)
        domains_list = self.functions_dict[self.domains_type].keys()
        self.set_widget_items(self.domains_list, domains_list)
        self.on_select_domain()

    def on_select_domain(self):
        self.current_domain = self.domains_list.currentText()
        self.on_search(self.search.text())

    def on_search(self, search_request):
        self.set_widget_items(self.functions_list,
                              [entry for entry in self.functions_dict[self.domains_type][self.current_domain]
                               if search_request.upper() in entry.upper()])

    def on_autobuild(self):
        self.autobuild.emit()

    def set_widget_items(self, widget, items):
        """
        Adds items to widget
        :param widget: widget
        :param items: list of strings
        """
        widget.clear()
        widget.addItems(items)

    def reset(self):
        self.domains_list.clear()
        self.functions_list.clear()

    def add_function(self, function):
        """
        Adds new function to required list

        :param function: name if function
        """
        domain_type, domain, index = self.find_function(function)
        self.functions_dict[domain_type][domain].insert(index, function)
        if domain == self.current_domain:
            self.functions_list.insertItem(index, QListWidgetItem(function))
            self.on_search(self.search.text())

    def remove_function(self, function):
        """
        Removes function from left list
        """
        domain_type, domain, index = self.find_function(function)
        self.functions_dict[domain_type][domain].remove(function)
        if self.current_domain == domain:
            item = self.functions_list.findItems(function, QtCore.Qt.MatchExactly)
            if item:
                self.functions_list.takeItem(self.functions_list.row(item[0]))

    def find_function(self, function_name):
        previous_domain = ''
        initial_functions_dict = self.settings.package.functions

        for domain_type, domain, function in utils.walk_dict(initial_functions_dict):
            if domain != previous_domain:
                index = 0

            if function_name == function:
                return domain_type, domain, index
            elif function in self.functions_dict[domain_type][domain]:
                previous_domain = domain
                index = self.functions_dict[domain_type][domain].index(function) + 1
