CONFIG += \
  release \
  warn_on \
  qt

TARGET = qsvgthemebuilder
DESTDIR = bin
TEMPLATE = app

QT += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

INCLUDEPATH += . ../styleconfig ../thirdparty/svgcleaner

PRE_TARGETDEPS += \
  ../styleconfig/lib/libQSvgStyleConfig.a \
  ../thirdparty/svgcleaner/lib/libsvgcleaner-cli.a

LIBS += \
  ../styleconfig/lib/libQSvgStyleConfig.a \
  ../thirdparty/svgcleaner/lib/libsvgcleaner-cli.a

HEADERS += \
  ThemeBuilderUI.h \
  NewThemeUI.h

SOURCES += \
  main.cpp \
  ThemeBuilderUI.cpp \
  NewThemeUI.cpp

FORMS += \
  ThemeBuilderUIBase.ui \
  NewThemeUIBase.ui

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
  INSTALLS += target
}
