CONFIG += \
  release \
  warn_on \
  qt      \
  dll     \
  plugin

TARGET = qsvgstyle
DESTDIR = plugins
TEMPLATE = lib

QT += core gui svg
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

INCLUDEPATH += ../styleconfig

PRE_TARGETDEPS += \
  ../styleconfig/lib/libQSvgStyleConfig.a

LIBS += \
  ../styleconfig/lib/libQSvgStyleConfig.a

HEADERS += \
  QSvgThemableStyle.h \
  QSvgStylePlugin.h

SOURCES += \
  QSvgThemableStyle.cpp \
  QSvgStylePlugin.cpp

RESOURCES += \
  defaulttheme.qrc

unix {
  isEmpty(PREFIX) {
    PREFIX = /usr
  }

  #MAKE INSTALL
  target.path = $$[QT_INSTALL_PLUGINS]/styles
  INSTALLS += target
}
