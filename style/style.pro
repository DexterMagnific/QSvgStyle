CONFIG += warn_on \
	  thread \
          qt \
 dll \
 plugin
TARGET = qsvgstyle
DESTDIR = plugins
TEMPLATE = lib

VERSION = 0.1

QT += svg

HEADERS += ../themeconfig/specs.h \
 ../themeconfig/ThemeConfig.h \
 QSvgStyle.h \
 QSvgStylePlugin.h

SOURCES += ../themeconfig/ThemeConfig.cpp \
 QSvgStyle.cpp \
 QSvgStylePlugin.cpp

RESOURCES += ../themeconfig/defaulttheme.qrc

