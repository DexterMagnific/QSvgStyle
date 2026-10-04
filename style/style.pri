CONFIG += \
  release \
  warn_on \
  qt \
  dll \
  plugin

TARGET = qsvgstyle
DESTDIR = plugins
TEMPLATE = lib

QT += core gui widgets svg

# Styleconfig from new location
include(styleconfig/styleconfig_dependencies.pri)

# Include pre-generated built-in themes resource
RESOURCES += builtinthemes.qrc

# Include path for generated header
INCLUDEPATH += .

# Default theme is defined in builtin_themes.h (from themes_list.txt)
# C++ code uses QSVGSTYLE_BUILTIN_DEFAULT_THEME from builtin_themes.h

HEADERS += \
  QSvgThemableStyle.h \
  QSvgStylePlugin.h \
  QSvgCachedRenderer.h

SOURCES += \
  QSvgThemableStyle.cpp \
  QSvgStylePlugin.cpp \
  QSvgCachedRenderer.cpp

unix:isEmpty(PREFIX) {
  PREFIX = /usr
}

windows:isEmpty(PREFIX) {
  PREFIX = $$(MSYSTEM_PREFIX)
}

# MAKE INSTALL
target.path = $$[QT_INSTALL_PLUGINS]/styles
INSTALLS += target