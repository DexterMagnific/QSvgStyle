CONFIG += \
  warn_on \
  thread  \
  qt      \
  dll     \
  plugin

TARGET = qsvgstyle
DESTDIR = plugins
TEMPLATE = lib

VERSION = 0.1

QT += svg

HEADERS += \
  specs.h       \
  ThemeConfig.h \
  QSvgStyle.h                  \
  QSvgStylePlugin.h

SOURCES += \
  ThemeConfig.cpp \
  QSvgStyle.cpp                  \
  QSvgStylePlugin.cpp

RESOURCES += \
  defaulttheme.qrc
