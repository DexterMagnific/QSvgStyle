CONFIG += \
  release \
  warn_on \
  qt      \
  dll     \
  plugin

TARGET = qsvgstyle
DESTDIR = plugins
TEMPLATE = lib

QT += core gui widgets svg

INCLUDEPATH += ../styleconfig

PRE_TARGETDEPS += \
  ../styleconfig/lib/libQSvgStyleConfig.a

LIBS += \
  ../styleconfig/lib/libQSvgStyleConfig.a

HEADERS += \
  QSvgThemableStyle.h \
  QSvgStylePlugin.h \
  QSvgCachedRenderer.h

SOURCES += \
  QSvgThemableStyle.cpp \
  QSvgStylePlugin.cpp \
  QSvgCachedRenderer.cpp

RESOURCES += \
  defaulttheme.qrc

#MAKE INSTALL
target.path = $$[QT_INSTALL_PLUGINS]/styles
INSTALLS += target
