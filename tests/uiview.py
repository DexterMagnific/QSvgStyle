#!/bin/env python3

import sys
from PyQt6 import uic
from PyQt6.QtWidgets import QApplication

def main():
    if len(sys.argv) != 2:
        print(f"Usage: {sys.argv[0]} <file.ui>")
        sys.exit(1)

    ui_file = sys.argv[1]

    app = QApplication(sys.argv)
    window = uic.loadUi(ui_file)
    window.show()
    window.resize(window.sizeHint())
    sys.exit(app.exec())

if __name__ == "__main__":
    main()
