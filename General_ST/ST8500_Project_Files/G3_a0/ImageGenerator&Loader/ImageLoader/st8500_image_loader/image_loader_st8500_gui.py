import os
import sys
import serial
from serial.tools import list_ports
from datetime import datetime
import logging
import configparser
from PySide6.QtWidgets import QApplication, QWidget, QFileDialog, QMessageBox
from PySide6.QtCore import QFile, QObject, QThread, Signal, Qt, QEvent
from PySide6.QtGui import QIcon
from image_loader_st8500_core import StImageLoader, LoaderOp
from loadercommon import LoaderCommon, ImageType
from gui_logger import GuiLogger
from qt_custom import set_combobox_alignment, QUiLoaderFixed
import faulthandler
from ctypes import windll  # Only exists on Windows.

# Splash screen
if getattr(sys, 'frozen', False):
    import pyi_splash


# The path built with this expression is independent of the running script location
sys.path.append(os.path.dirname(os.path.realpath(__file__)) + "\\protocol_abstraction_layer")

from protocol_abstraction_layer.global_defs import Protocol    # noqa: E402

# Set application ID
app_id = f'STMicroelectronics.ST8500.image_loader.{StImageLoader.version()}'
windll.shell32.SetCurrentProcessExplicitAppUserModelID(app_id)


class LoaderGui(QWidget):
    ui = None
    cl = None
    port_list = []
    port = None
    description_filter_enable = False
    description_filter_list = []
    check_images = False
    protocol = None
    baudrate = None
    baudrate_program = None
    thread = None
    worker = None
    log = None
    spi_loader_file = "ST8500_SPI_Loader.img"
    spi_loader_path = "imgs\\" + spi_loader_file

    text_select = "Select"
    text_deselect = "Deselect"
    text_execute = "Execute"
    text_abort = "Abort"

    def __init__(self):
        super(LoaderGui, self).__init__()
        self.load_ui()

    def load_ui(self):

        # Window
        self.setWindowIcon(QIcon('image_loader_st8500.ico'))
        self.setWindowTitle("Image Loader ST8500 v" + StImageLoader.version())

        self.setFixedSize(725, 555)

        # Load UI
        loader = QUiLoaderFixed()
        path = "form.ui"
        ui_file = QFile(path)
        ui_file.open(QFile.ReadOnly)

        # Loads all UI items
        self.ui = loader.load(ui_file, self)
        ui_file.close()

        # Define logging format
        logger_fmt = "%(asctime)s %(levelname)s: %(message)s"
        logger_datefmt = "%H:%M:%S"
        logger_formatter = logging.Formatter(fmt=logger_fmt, datefmt=logger_datefmt)

        # Console Handler
        console_handler = logging.StreamHandler(sys.stdout)
        console_handler.setFormatter(logger_formatter)

        # GUI Handler
        gui_handler = GuiLogger(self, self.ui.plainTextEdit_log)
        gui_handler.setFormatter(logger_formatter)

        # Logger
        self.log = logging.getLogger(__name__)
        self.log.setLevel(logging.INFO)

        # Handlers
        self.log.addHandler(console_handler)
        self.log.addHandler(gui_handler)

        # Configuration file parsing
        configuration_parser = configparser.ConfigParser()

        # Read the configuration file
        if configuration_parser.read(StImageLoader.ini_filename):
            try:

                # COM filter
                self.description_filter_enable = int(configuration_parser["COM"]["description_filter_enable"])
                self.description_filter_list = configuration_parser["COM"]["description_filter"].split("\n")[1:]

                # Image checking
                self.check_images = int(configuration_parser["SETTINGS"]["check_images"])
            except KeyError:
                pass

        # Labels
        self.ui.label_info.setText("Not detected")

        # Combo boxes
        self.refresh_port()

        baudrate_list = ["921600", "460800", "115200", "9600"]
        self.ui.comboBox_baudrate_list.setEditable(False)
        self.ui.comboBox_baudrate_list.addItems(baudrate_list)

        operation_list = [LoaderOp.check, LoaderOp.upload, LoaderOp.program, LoaderOp.reset, LoaderOp.erase,
                          LoaderOp.erase_nvm, LoaderOp.erase_full]

        for operation in operation_list:
            self.ui.comboBox_operation_list.addItem(QIcon(), str(operation), userData=operation.value)

        set_combobox_alignment(self.ui.comboBox_port_list, Qt.AlignLeft)
        set_combobox_alignment(self.ui.comboBox_baudrate_list, Qt.AlignCenter)
        set_combobox_alignment(self.ui.comboBox_operation_list, Qt.AlignCenter)

        # Event Filter
        self.ui.comboBox_port_list.installEventFilter(self)

        # GUI signal connections
        self.ui.pushButton_port.clicked.connect(self.select_port)
        self.ui.pushButton_browse_pe.clicked.connect(self.set_image_pe)
        self.ui.pushButton_browse_rte.clicked.connect(self.set_image_rte)
        self.ui.plainTextEdit_log.textChanged.connect(self.change_scroll)
        self.ui.pushButton_execution.clicked.connect(self.execute)
        self.ui.pushButton_clear_log.clicked.connect(self.clear_log)
        self.ui.pushButton_help.clicked.connect(self.show_help)

        # Tooltips
        self.ui.label_info.setToolTip("Detected protocol and baudrate")
        self.ui.pushButton_port.setToolTip("Selects/deselects COM port")
        self.ui.pushButton_browse_pe.setToolTip("Browses for the PE image")
        self.ui.pushButton_browse_rte.setToolTip("Browses for the RTE image")
        self.ui.pushButton_execution.setToolTip("Executes the selected operation")
        self.ui.pushButton_clear_log.setToolTip("Clears the log textbox")
        self.ui.pushButton_help.setToolTip("Shows help information window")
        self.ui.comboBox_port_list.setToolTip("The COM port connected to the platform")
        self.ui.comboBox_baudrate_list.setToolTip("The baudrate to use in upload/programming")
        self.ui.comboBox_operation_list.setToolTip("The operation to perform on the platform")
        self.ui.lineEdit_image_pe.setToolTip("Absolute path to the PE image")
        self.ui.lineEdit_image_rte.setToolTip("Absolute path to the RTE image")
        self.ui.progressBar_progress.setToolTip("Operation progress")
        self.ui.plainTextEdit_log.setToolTip("Operation log")

        # Operations initially disabled
        self.enable_operations(False)

    def eventFilter(self, qt_object, event: QEvent):
        # Update serial ports when clicking the object
        if qt_object == self.ui.comboBox_port_list and event.type() == QEvent.MouseButtonPress:
            self.refresh_port()

        return False  # Continues to process the event by returning False

    def set_progressbar_style(self, color: str, color_back: str = "lightgray"):
        sheet = "QProgressBar { background-color : " + color_back + "; border : 1px; }" \
                "QProgressBar::chunk { background-color: " + color + "; }"
        self.ui.progressBar_progress.setStyleSheet(sheet)

    def enable_operations(self, enable):
        self.ui.pushButton_port.setChecked(enable)
        self.ui.comboBox_port_list.setEnabled(not enable)
        self.ui.comboBox_baudrate_list.setEnabled(not enable)
        self.ui.pushButton_execution.setEnabled(enable)

    def refresh_port(self):

        self.port_list.clear()

        ports = list_ports.comports()

        for port, description, hw_id in sorted(ports):

            add_com = False

            # Depending on the settings
            if self.description_filter_enable:
                for filter_description in self.description_filter_list:
                    if description.startswith(filter_description):  # Only the ports with matching name prefix are added
                        add_com = True
                        break
            else:
                add_com = True  # All ports are added, regardless of the name prefix

            if add_com:
                self.port_list.append([port, description])

        # Saves the old index
        if self.ui.comboBox_port_list.count() > 0:
            current_port = self.ui.comboBox_port_list.currentIndex()
        else:
            current_port = 0

        # Removes the old ports
        for i in range(0, self.ui.comboBox_port_list.count()):
            self.ui.comboBox_port_list.removeItem(0)

        # Adds the new ports
        for port_item in self.port_list:
            self.ui.comboBox_port_list.addItem(port_item[1])

        # Restores the old index, after the refresh
        self.ui.comboBox_port_list.setCurrentIndex(current_port)

    def select_port(self):
        if self.port is None:
            if self.ui.comboBox_port_list.count() > 0:
                port_index = self.ui.comboBox_port_list.currentIndex()
                port_item = self.port_list[port_index]
                com_port = port_item[0]

                try:
                    ser = serial.Serial(com_port)
                    ser.close()
                    self.port = com_port
                    self.baudrate_program = int(self.ui.comboBox_baudrate_list.currentText())
                    self.enable_operations(True)
                    self.ui.pushButton_port.setText(self.text_deselect)
                    self.log.info("Selected {} port, programming baudrate {} bps".format(self.port,
                                                                                         self.baudrate_program))
                except serial.SerialException:
                    self.log.error("Cannot open {} port".format(com_port))
            else:
                self.log.error("No port found")
        else:
            self.ui.pushButton_port.setText(self.text_select)
            self.log.info("Closed {} port".format(self.port))
            self.port = None
            self.baudrate_program = None
            self.enable_operations(False)
            self.refresh_port()
            self.protocol = None
            self.baudrate = None
            self.ui.label_info.setStyleSheet("QLabel { color : black; }")
            self.ui.label_info.setText("Not detected")

    def set_image_pe(self):
        file_filter = "PE image (*.img)"
        file_name = QFileDialog.getOpenFileName(parent=self, caption="Select PE image", dir='imgs', filter=file_filter)
        self.ui.lineEdit_image_pe.setText(file_name[0])

        if self.check_images and file_name[0] != "":
            # Check the image
            lc = LoaderCommon(lg=self.log)
            lc.check_image(file_name[0], ImageType.PE)

    def set_image_rte(self):
        file_filter = "RTE image (*.img)"
        file_name = QFileDialog.getOpenFileName(parent=self, caption="Select RTE image", dir='imgs', filter=file_filter)
        self.ui.lineEdit_image_rte.setText(file_name[0])

        if self.check_images and file_name[0] != "":
            # Check the image
            lc = LoaderCommon(lg=self.log)
            lc.check_image(file_name[0], ImageType.RTE)

    def clear_log(self):
        self.ui.plainTextEdit_log.clear()

    def show_help(self):
        popup = QMessageBox()

        popup.setWindowIcon(QIcon('image_loader_st8500.ico'))
        popup.setWindowTitle("Help")
        popup.setIcon(QMessageBox.Question)
        popup.setStandardButtons(QMessageBox.Ok)
        popup.setText(f"1. Select the COM port connected to the platform from the scrolldown menu.\n\n"
                      f"2. Click on '{self.ui.pushButton_port.text()}'.\n\n"
                      f"3. Select the target operation from the scrolldown menu:\n\n"
                      f"  - {LoaderOp.check}: detects the current running protocol on the platform.\n\n"
                      f"  - {LoaderOp.upload}: writes the PE/RTE images to the platform RAM.\n"
                      f"    NOTE: writing to RAM requires the board in 'Boot from UART' mode.\n\n"
                      f"  - {LoaderOp.program}: writes the PE/RTE images to the platform SPI FLASH.\n"
                      f"    NOTE: writing to SPI FLASH requires the board in 'Boot from SPI FLASH' mode.\n\n"
                      f"  - {LoaderOp.reset}: resets the platform hardware.\n\n"
                      f"  - {LoaderOp.erase}: erases the RTE/PE image sectors on the platform SPI FLASH.\n\n"
                      f"  - {LoaderOp.erase_nvm}: erases the NVM sectors on the platform SPI FLASH.\n\n"
                      f"  - {LoaderOp.erase_full}: erases the entire platform SPI FLASH.\n\n"
                      f"4. For {LoaderOp.upload} or {LoaderOp.program} operations, select the PE and the RTE images, "
                      f"clicking on the {self.ui.pushButton_browse_pe.text()} buttons.\n\n"
                      f"5. Click on '{self.ui.pushButton_execution.text()}'.\n\n"
                      f"Note: it is possible to stop an operation in progress by clicking on '{self.text_abort}'.\n"
                      f"After aborting an operation, please reboot the board with the reset push-button.\n")

        popup.exec()

    def change_scroll(self):
        position = self.ui.plainTextEdit_log.verticalScrollBar().maximum()
        self.ui.plainTextEdit_log.verticalScrollBar().setSliderPosition(position)

    def report_info(self, protocol: Protocol, baudrate: int):
        if protocol is not None:
            self.protocol = protocol
            self.baudrate = baudrate
            info = "{}\n{}bps".format(protocol.dotted_string(), baudrate)
        else:
            self.protocol = None
            self.baudrate = None
            info = "Unknown"

        if "\n" in info:
            self.ui.label_info.setStyleSheet("QLabel { color : green; }")
        else:
            self.ui.label_info.setStyleSheet("QLabel { color : red; }")

        self.ui.label_info.setText(info)

    def report_step(self, step: str):
        self.ui.progressBar_progress.setFormat(step + " %p%")

    def report_progress(self, progress: int):
        self.ui.progressBar_progress.setValue(progress)

    def request_file(self, file):
        file_filter = "File ({})".format(file)
        image_name = QFileDialog.getOpenFileName(parent=self, caption="Select '{}'".format(file), filter=file_filter)

        return image_name[0]

    def operation_complete(self, success: bool):
        self.ui.pushButton_port.setEnabled(True)
        self.ui.pushButton_execution.setText(self.text_execute)

        if success:
            self.set_progressbar_style("light green")
        else:
            self.set_progressbar_style("red")

    def execute(self):
        if self.ui.pushButton_execution.text() == self.text_execute:
            operation = LoaderOp(self.ui.comboBox_operation_list.currentData())

            can_proceed = True

            # Check SPI Loader presence. If absent, prompt the user for its path
            if operation == LoaderOp.program:
                if not os.path.isfile(self.spi_loader_path):
                    self.spi_loader_path = self.request_file(self.spi_loader_file)

                if not os.path.isfile(self.spi_loader_path):
                    can_proceed = False

            if can_proceed:
                self.ui.pushButton_port.setEnabled(False)
                self.ui.pushButton_execution.setText(self.text_abort)

                pe_file = self.ui.lineEdit_image_pe.text()
                rte_file = self.ui.lineEdit_image_rte.text()

                # Create Thread
                self.thread = QThread()
                self.worker = Worker(self.port, operation, self.baudrate_program, pe_file, rte_file,
                                     self.protocol, self.baudrate, self.spi_loader_path, self.log)
                self.worker.moveToThread(self.thread)

                # Connect signals to slots
                self.thread.started.connect(self.worker.run)
                self.worker.finished.connect(self.operation_complete)
                self.worker.finished.connect(self.thread.quit)
                self.worker.finished.connect(self.worker.deleteLater)
                self.thread.finished.connect(self.thread.deleteLater)
                self.worker.detect.connect(self.report_info)
                self.worker.step.connect(self.report_step)
                self.worker.progress.connect(self.report_progress)

                self.set_progressbar_style("#3CB4E6")  # "#0F2343"

                # Start the thread
                self.thread.start()
            else:
                self.log.error("Cannot proceed")
        else:
            self.abort_operation()

    def kill_thread(self):
        if self.thread is not None:
            if self.thread.isRunning():
                if self.worker.image_loader is not None:
                    self.worker.image_loader.aborted[0] = True
                self.thread.quit()
                self.thread.wait()

    def abort_operation(self):
        if self.thread is not None:
            try:
                if self.thread.isRunning():
                    answer = QMessageBox.question(self, "Abort Confirmation", "Abort?", QMessageBox.Yes, QMessageBox.No)
                    if answer == QMessageBox.Yes:
                        self.kill_thread()
            except RuntimeError:  # If object was deleted
                self.log.error("Cannot abort, thread not running")
        else:
            self.log.error("Cannot abort, missing thread")

    def closeEvent(self, event):
        if self.thread is not None:
            try:
                if self.thread.isRunning():
                    answer = QMessageBox.question(self, "Exit Confirmation", "Exit?", QMessageBox.Yes, QMessageBox.No)
                    if answer == QMessageBox.Yes:
                        self.kill_thread()
                        event.accept()
                    else:
                        event.ignore()
                else:
                    event.accept()
            except RuntimeError:  # If object was deleted
                event.accept()
        else:
            event.accept()


class Worker(QObject):
    finished = Signal(bool)
    detect = Signal(Protocol, int)
    step = Signal(str)
    progress = Signal(int)

    def __init__(self, port: str, operation: LoaderOp, baudrate_program: int, pe_file: str, rte_file: str,
                 protocol: Protocol, baudrate: int, spi_ld_file: str, log: logging.Logger):
        super(Worker, self).__init__()
        self.port = port
        self.operation = operation
        self.baudrate_program = baudrate_program
        self.pe_file = pe_file
        self.rte_file = rte_file
        self.protocol = protocol
        self.baudrate = baudrate
        self.spi_ld_file = spi_ld_file
        self.log = log
        self.image_loader = None

    def run(self):
        self.log.info("<BEGIN>----------------------------------------------------------------------------------------")
        self.progress.emit(0)

        try:
            self.image_loader = StImageLoader(self.port, self.baudrate_program, self.log)
            success = self.image_loader.run(self.operation, self.pe_file, self.rte_file, self.protocol, self.baudrate,
                                            self.detect.emit, self.step.emit, self.progress.emit, self.spi_ld_file)
        except Exception as inst:
            self.image_loader = None
            self.log.error(f"Operation failed: {type(inst).__name__}")
            success = False

        self.progress.emit(100)
        self.finished.emit(success)
        self.log.info("<END>------------------------------------------------------------------------------------------")


if __name__ == "__main__":

    enable_fault_handler = False

    config = configparser.ConfigParser()

    # Read the configuration file
    if config.read(StImageLoader.ini_filename):

        try:
            enable_fault_handler = int(config["DEBUG"]["enable_fault_handler"])
        except KeyError:
            pass

    if enable_fault_handler:
        fault_file = open("fault_tracebacks.txt", "a", encoding="utf-8")
        fault_file.write("Executed on {}\n".format(datetime.now().strftime("%H:%M:%S")))
        faulthandler.enable(file=fault_file, all_threads=True)
    else:
        fault_file = None

    app = QApplication([])

    # Splash screen
    if getattr(sys, 'frozen', False):
        pyi_splash.close()

    widget = LoaderGui()
    widget.show()
    exit_code = app.exec()

    if enable_fault_handler:
        fault_file.close()

    sys.exit(exit_code)
