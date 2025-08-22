TEMPLATE = aux

unix:isEmpty(PREFIX) {
  PREFIX = /usr
}

windows:isEmpty(PREFIX) {
  PREFIX = $$(MSYSTEM_PREFIX)
}

DATADIR = $$PREFIX/share/QSvgStyle

#MAKE INSTALL
QMAKE_INSTALL_DIR = cp -f -R --no-preserve=mode

themes.path = $$DATADIR
themes.files += \
  $$PWD/Arrongin \
  $$PWD/Telinkrin \
  $$PWD/FlatShadowed \
  $$PWD/godot2 \
  $$PWD/godot-catppuccin-latte

INSTALLS += themes
