CONFIG += \
  release \
  warn_on \
  qt \
  staticlib

TARGET   = QSvgStyleConfig
DESTDIR  = lib
TEMPLATE = lib

QT      += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

INCLUDEPATH += ..

SOURCES += \
  groups.cpp \
  ThemeConfig.cpp \
  PaletteConfig.cpp \
  StyleConfig.cpp

HEADERS += \
  specs.h  \
  groups.h \
  ThemeConfig.h \
  PaletteConfig.h \
  StyleConfig.h
