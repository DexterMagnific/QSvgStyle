CONFIG += \
  release \
  warn_on \
  qt

TARGET = qsvgthemebuilder
DESTDIR = bin
TEMPLATE = app

QT += core gui xml widgets core5compat

# Use build directory path for styleconfig lib (built in build dir)
STYLECONFIG_LIB = $$OUT_PWD/../style/styleconfig/lib/libQSvgStyleConfig.a

# Use absolute paths for include directories
# $$PWD is the source directory of this .pro file: /home/dexter/QSvgStyle/themebuilder
# To reach style/styleconfig: ../style/styleconfig (one level up to QSvgStyle)
STYLECONFIG_INCLUDE = $$PWD/../style/styleconfig
THIRDPARTY_SVGCLEANER_INCLUDE = $$PWD/../thirdparty/svgcleaner
THIRDPARTY_QUAZIP_INCLUDE = $$PWD/../thirdparty/quazip

INCLUDEPATH += . $$PWD $$STYLECONFIG_INCLUDE $$THIRDPARTY_SVGCLEANER_INCLUDE $$THIRDPARTY_QUAZIP_INCLUDE

PRE_TARGETDEPS += \
  $$STYLECONFIG_LIB \
  $$OUT_PWD/../thirdparty/svgcleaner/lib/libsvgcleaner-cli.a

LIBS += \
  $$STYLECONFIG_LIB \
  $$OUT_PWD/../thirdparty/svgcleaner/lib/libsvgcleaner-cli.a \
  $$OUT_PWD/../thirdparty/quazip/lib/libquazip.a \
  -lz

DEFINES += QUAZIP_STATIC

HEADERS += \
  SvgGen.h \
  ThemeBuilderUI.h \
  NewThemeUI.h \
  GenSubFramePropUI.h \
  ThemeScreenshotUI.h

SOURCES += \
  main.cpp \
  SvgGen.cpp \
  ThemeBuilderUI.cpp \
  NewThemeUI.cpp \
  GenSubFramePropUI.cpp \
  ThemeScreenshotUI.cpp

FORMS += \
  ThemeBuilderUIBase.ui \
  NewThemeUIBase.ui \
  GenSubFramePropUIBase.ui \
  ThemeScreenshotUIBase.ui

RESOURCES += \
  ThemeBuilderUIBase.qrc

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
  desktop.files = ./desktop/qsvgthemebuilder.desktop

  INSTALLS += desktop
}