CONFIG += \
  release \
  warn_on \
  qt

TARGET = qsvgthememanager
DESTDIR = bin
TEMPLATE = app

QT += core gui widgets

# Use build directory path for styleconfig lib
STYLECONFIG_LIB = $$OUT_PWD/../style/styleconfig/lib/libQSvgStyleConfig.a

# Use absolute paths for include directories
# $$PWD is the source directory: /home/dexter/QSvgStyle/thememanager
STYLECONFIG_INCLUDE = $$PWD/../style/styleconfig

INCLUDEPATH += . $$PWD $$STYLECONFIG_INCLUDE

PRE_TARGETDEPS += \
  $$STYLECONFIG_LIB

LIBS += \
  $$STYLECONFIG_LIB

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