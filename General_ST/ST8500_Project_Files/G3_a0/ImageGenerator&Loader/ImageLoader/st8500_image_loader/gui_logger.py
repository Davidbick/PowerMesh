import logging
from PySide6.QtCore import QObject, Signal
from PySide6.QtGui import QTextCursor
import threading


class GuiLogger(logging.Handler):

    class QtGuiLogEmitter(QObject):
        log_signal = Signal(str)

        def __init__(self, parent, plain_text_edit):
            super().__init__(parent)

            self.thread_id = threading.get_ident()
            self.plain_text_edit = plain_text_edit

            self.log_signal.connect(self.add_log_line)

        def add_log_line(self, log_line):
            if threading.get_ident() != self.thread_id:
                warning = "WARNING, executed out of main Qt thread (ID: {} instead of {})".format(threading.get_ident(),
                                                                                                  self.thread_id)
                print(warning)
                warning += "\n"
                log_line += warning

            text_cursor = self.plain_text_edit.textCursor()
            text_cursor.movePosition(QTextCursor.End)
            text_cursor.insertText(log_line)

    def __init__(self, parent, plain_text_edit):
        super().__init__()  # Init of logging.Handler
        self._emitter = self.QtGuiLogEmitter(parent, plain_text_edit)

    @property
    def emitter(self):
        return self._emitter

    def emit(self, record):
        log_line = self.format(record) + "\n"
        self.emitter.log_signal.emit(log_line)
