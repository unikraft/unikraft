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
import json

from PyQt5.QtCore import QEvent, Qt
from PyQt5.QtGui import QIcon
from PyQt5.QtWidgets import QMainWindow, QWidget, QGridLayout, QFileDialog, QMessageBox, QDesktopWidget

from tool import utils
from gui.selection_panel import SelectionPanel
from gui.custom_functions_panel import CustomFunctionsPanel
from gui.settings_panel import SettingsPanel
from gui.controller import Controller


class MainAppWindow(QMainWindow):
    def __init__(self):
        super(MainAppWindow, self).__init__()
        self.setWindowIcon(QIcon('icon.ico'))
        self.setWindowTitle('Intel® Integrated Performance Primitives Custom Library Tool')

        project_menu = self.menuBar()

        open_action    = project_menu.addAction('Open project')
        save_action    = project_menu.addAction('Save project')
        save_as_action = project_menu.addAction('Save project as...')

        self.settings_panel         = SettingsPanel()
        self.selection_panel        = SelectionPanel(self.settings_panel)
        self.custom_functions_panel = CustomFunctionsPanel(self.settings_panel)

        self.controller = Controller(self,
                                     self.settings_panel,
                                     self.selection_panel,
                                     self.custom_functions_panel)

        widget = QWidget()
        self.setCentralWidget(widget)

        layout = QGridLayout()
        layout.addWidget(self.settings_panel, 0, 0, 1, 3)
        layout.addWidget(self.selection_panel, 1, 0)
        layout.addWidget(self.controller, 1, 1, Qt.AlignCenter)
        layout.addWidget(self.custom_functions_panel, 1, 2)
        widget.setLayout(layout)

        open_action.triggered.connect(self.on_open)
        save_action.triggered.connect(self.on_save)
        save_as_action.triggered.connect(self.on_save_as)

        self.project = ''
        self.show()

    def on_open(self):
        while True:
            extension = 'CLT project (*' + utils.PROJECT_EXTENSION + ')'
            chosen_path = QFileDialog.getOpenFileName(self,
                                                      'Open project',
                                                      '',
                                                      extension,
                                                      options=QFileDialog.DontResolveSymlinks)[0]
            if not chosen_path:
                return
            elif os.path.islink(chosen_path):
                QMessageBox.information(self, 'ERROR!', 'Please, select not a symlink')
                continue
            else:
                self.project = chosen_path
                break

        with open(self.project, 'r') as project_file:
            configs = json.load(project_file)
            self.controller.set_configs(configs)

    def on_save(self):
        return self.on_save_as(self.project)

    def on_save_as(self, project):
        self.controller.get_selected_configs()

        if not project:
            extension = 'CLT project (*' + utils.PROJECT_EXTENSION + ')'
            project = QFileDialog.getSaveFileName(self, 'Save project as...', '', extension)[0]
            if not project:
                return
            else:
                self.project = project

        if utils.CONFIGS[utils.PACKAGE]:
            utils.CONFIGS[utils.PACKAGE] = utils.CONFIGS[utils.PACKAGE].root

        with open(project, 'w') as project_file:
            json.dump(utils.CONFIGS, project_file)

    def event(self, e):
        if e.type() == QEvent.Show:
            qt_rectangle = self.frameGeometry()
            center_point = QDesktopWidget().availableGeometry().center()
            qt_rectangle.moveCenter(center_point)
            self.move(qt_rectangle.topLeft())

        return super().event(e)
