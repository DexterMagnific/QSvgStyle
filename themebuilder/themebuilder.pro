CONFIG += \
  release \
  warn_on \
  qt

TARGET = qsvgthemebuilder
DESTDIR = bin
TEMPLATE = app

QT += core gui xml widgets core5compat

INCLUDEPATH += . ../styleconfig ../thirdparty/svgcleaner ../thirdparty/quazip

PRE_TARGETDEPS += \
  ../styleconfig/lib/libQSvgStyleConfig.a \
  ../thirdparty/svgcleaner/lib/libsvgcleaner-cli.a

LIBS += \
  ../styleconfig/lib/libQSvgStyleConfig.a \
  ../thirdparty/svgcleaner/lib/libsvgcleaner-cli.a \
  ../thirdparty/quazip/lib/libquazip.a \
  -lz

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

unix {
  isEmpty(PREFIX) {
    PREFIX = /usr
  }
  BINDIR  = $$PREFIX/bin
  DATADIR = $$PREFIX/share

  #MAKE INSTALL
  target.path = $$BINDIR

  desktop.path = $$DATADIR/applications
  desktop.files = ./desktop/qsvgthemebuilder.desktop

  INSTALLS += target desktop
}
