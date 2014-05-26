TEMPLATE = app
TARGET = qsvgthemebuilder
DEPENDPATH += .
INCLUDEPATH += .
DESTDIR = bin

# Input
PRE_TARGETDEPS += ../style/plugins/libqsvgstyle.so
HEADERS += ThemeBuilderUI.h ../style/ThemeConfig.h ../style/specs.h ../common/groups.h
FORMS += ThemeBuilderUIBase.ui
SOURCES += main.cpp ThemeBuilderUI.cpp ../style/ThemeConfig.cpp ../common/groups.cpp
RESOURCES += ThemeBuilderUIBase.qrc ../style/defaulttheme.qrc
