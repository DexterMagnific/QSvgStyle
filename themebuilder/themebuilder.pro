TEMPLATE = app
TARGET = qsvgthemebuilder
DEPENDPATH += .
INCLUDEPATH += .
DESTDIR = bin

HEADERS += ThemeBuilderUI.h ../style/ThemeConfig.h ../style/specs.h
FORMS += ThemeBuilderUIBase.ui
SOURCES += main.cpp ThemeBuilderUI.cpp ../style/ThemeConfig.cpp
