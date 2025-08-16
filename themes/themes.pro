TEMPLATE = aux

unix {
  unix {
    isEmpty(PREFIX) {
      PREFIX = /usr
    }
    DATADIR = $$PREFIX/share/QSvgStyle

    #MAKE INSTALL
    QMAKE_INSTALL_DIR = cp -f -R --no-preserve=mode

    target.path = $$DATADIR
    target.files += \
        $$PWD/Arrongin \
        $$PWD/Telinkrin \
        $$PWD/FlatShadowed \
        $$PWD/godot2 \
        $$PWD/godot-catppuccin-latte

    INSTALLS += target
  }
}
