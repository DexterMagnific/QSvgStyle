TEMPLATE = app
TARGET = qsvgthemebuilder
DEPENDPATH += .
INCLUDEPATH += .
DESTDIR = bin

# Input
PRE_TARGETDEPS += ../style/plugins/libqsvgstyle.so
HEADERS += ThemeBuilderUI.h NewThemeUI.h ../style/ThemeConfig.h ../style/specs.h ../common/groups.h
FORMS += ThemeBuilderUIBase.ui NewThemeUIBase.ui
SOURCES += main.cpp ThemeBuilderUI.cpp NewThemeUI.cpp ../style/ThemeConfig.cpp ../common/groups.cpp
RESOURCES += ThemeBuilderUIBase.qrc ../style/defaulttheme.qrc
