TEMPLATE = app
TARGET = qsvgthemebuilder
DEPENDPATH += .
INCLUDEPATH += .
DESTDIR = bin

# Input
PRE_TARGETDEPS += ../style/plugins/libqsvgstyle.so
LIBS += -L../style/plugins -lqsvgstyle
HEADERS += ThemeBuilderUI.h ../style/ThemeConfig.h ../style/specs.h
FORMS += ThemeBuilderUIBase.ui
SOURCES += main.cpp ThemeBuilderUI.cpp ../style/ThemeConfig.cpp
RESOURCES += ThemeBuilderUIBase.qrc ../style/defaulttheme.qrc
