CONFIG += \
  release \
  warn_on \
  qt \
  staticlib

QT      += core
QT      -= gui

TARGET   = svgcleaner-cli
DESTDIR  = lib
TEMPLATE = lib
DEFINES *= QT_USE_QSTRINGBUILDER

SOURCES += \
    basecleaner.cpp \
    keys.cpp \
    paths.cpp \
    remover.cpp \
    replacer.cpp \
    svgelement.cpp \
    tinyxml2.cpp \
    tools.cpp \
    transform.cpp

HEADERS += \
    basecleaner.h \
    keys.h \
    paths.h \
    remover.h \
    replacer.h \
    svgelement.h \
    tinyxml2.h \
    tools.h \
    transform.h
