CONFIG += \
  release \
  warn_on \
  qt \
  staticlib

TARGET   = QSvgStyleConfig
DESTDIR  = lib
TEMPLATE = lib

QT      += core gui widgets

INCLUDEPATH += ..

SOURCES += \
  groups.cpp \
  ThemeConfig.cpp \
  StyleConfig.cpp \
  QSvgCachedSettings.cpp

HEADERS += \
  specs.h  \
  groups.h \
  ThemeConfig.h \
  StyleConfig.h \
  QSvgCachedSettings.h
