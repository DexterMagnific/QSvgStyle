CONFIG += \
  release \
  warn_on \
  qt

TARGET = qsvgthememanager
DESTDIR = bin
TEMPLATE = app

QT += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
greaterThan(QT_MAJOR_VERSION, 4):unix:!macx: QT += x11extras

INCLUDEPATH += . ../styleconfig

PRE_TARGETDEPS += \
  ../styleconfig/lib/libQSvgStyleConfig.a

LIBS += \
  ../styleconfig/lib/libQSvgStyleConfig.a

unix:!macx: {
  DEFINES += HAVE_X11
  LIBS += -lX11 -lxcb
}

HEADERS += \
  ThemeManagerUI.h

SOURCES += \
  main.cpp \
  ThemeManagerUI.cpp

FORMS += \
  ThemeManagerUIBase.ui

RESOURCES += \
  ThemeManagerUIBase.qrc

unix {
  isEmpty(PREFIX) {
    PREFIX = /usr
  }
  BINDIR  = $$PREFIX/bin
  DATADIR = $$PREFIX/share

  #MAKE INSTALL
  target.path = $$BINDIR
  INSTALLS += target
}
