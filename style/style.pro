CONFIG += \
  release \
  warn_on \
  qt      \
  dll     \
  plugin

TARGET = qsvgstyle
DESTDIR = plugins
TEMPLATE = lib

QT += svg

HEADERS += \
  ../common/groups.h \
  specs.h            \
  ThemeConfig.h      \
  QSvgStyle.h        \
  QSvgStylePlugin.h

SOURCES += \
  ../common/groups.cpp \
  ThemeConfig.cpp      \
  QSvgStyle.cpp        \
  QSvgStylePlugin.cpp

RESOURCES += \
  defaulttheme.qrc
