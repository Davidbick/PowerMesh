from PySide6.QtWidgets import QStyledItemDelegate, QStyleOptionViewItem, QProxyStyle, QCommonStyle
from PySide6.QtCore import Qt, QObject, QModelIndex, QRect, QFile
from PySide6.QtGui import QPainter, QPalette
from PySide6.QtUiTools import QUiLoader


class AlignDelegate(QStyledItemDelegate):
    def __init__(self, alignment: Qt.Alignment, parent: QObject):
        super(AlignDelegate, self).__init__(parent)
        self.alignment = alignment

    def initStyleOption(self, option: QStyleOptionViewItem, index: QModelIndex):
        super(AlignDelegate, self).initStyleOption(option, index)
        option.displayAlignment = self.alignment


class AlignComboBoxProxy(QProxyStyle):

    def __init__(self, alignment: Qt.Alignment, style: QCommonStyle):
        super(AlignComboBoxProxy, self).__init__(style)
        self.alignment = alignment
    def drawItemText(self, painter: QPainter, rect: QRect, flags: int, pal: QPalette, enabled: bool, text: str,
                     text_role: QPalette.ColorRole):  # Ignore warning

        super(AlignComboBoxProxy, self).drawItemText(painter, rect, self.alignment, pal, enabled, text, text_role)


def set_combobox_alignment(combobox, alignment: Qt.Alignment):
    # combobox.setStyle(AlignComboBoxProxy(alignment, combobox.style()))

    combobox.setEditable(True)

    # getting the line edit of combo box
    line_edit = combobox.lineEdit()

    # setting line edit alignment to the center
    line_edit.setAlignment(alignment)

    # setting line edit to read only
    line_edit.setReadOnly(True)

    combobox.setItemDelegate(AlignDelegate(alignment, combobox))


# avoids warnings on QWidgets
class QUiLoaderFixed(QUiLoader):
    def load(self, file: QFile, parent: QObject):
        return super(QUiLoaderFixed, self).load(file, parent)
