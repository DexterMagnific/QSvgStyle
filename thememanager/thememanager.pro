CONFIG += \
  release \
  warn_on \
  qt

TARGET = qsvgthememanager
DESTDIR = bin
TEMPLATE = app

QT += core gui widgets
#unix:!macx: QT += x11extras

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

  desktop.path = $$DATADIR/applications
  desktop.files = ./desktop/qsvgthememanager.desktop

  INSTALLS += target desktop
}
