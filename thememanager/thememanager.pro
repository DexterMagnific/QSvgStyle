CONFIG += \
  release \
  warn_on \
  qt

TARGET = qsvgthememanager
DESTDIR = bin
TEMPLATE = app

QT += core gui widgets

INCLUDEPATH += . ../styleconfig

PRE_TARGETDEPS += \
  ../styleconfig/lib/libQSvgStyleConfig.a

LIBS += \
  ../styleconfig/lib/libQSvgStyleConfig.a

HEADERS += \
  ThemeManagerUI.h

SOURCES += \
  main.cpp \
  ThemeManagerUI.cpp

FORMS += \
  ThemeManagerUIBase.ui

RESOURCES += \
  ThemeManagerUIBase.qrc

unix:isEmpty(PREFIX) {
  PREFIX = /usr
}

windows:isEmpty(PREFIX) {
  PREFIX = $$(MSYSTEM_PREFIX)
}

BINDIR  = $$PREFIX/bin
DATADIR = $$PREFIX/share

#MAKE INSTALL
target.path = $$BINDIR
INSTALLS += target

unix {
  desktop.path = $$DATADIR/applications
  desktop.files = ./desktop/qsvgthememanager.desktop

  INSTALLS += desktop
}
