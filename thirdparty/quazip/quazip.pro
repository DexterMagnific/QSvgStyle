CONFIG += \
  release \
  warn_on \
  qt \
  staticlib

QT      += core core5compat
QT      -= gui

TARGET   = quazip
DESTDIR  = lib
TEMPLATE = lib

DEFINES += QUAZIP_BUILD
DEFINES += QT_NO_CAST_FROM_ASCII
DEFINES += QT_NO_CAST_TO_ASCII
DEFINES += QUAZIP_QT_MAJOR_VERSION=6
CONFIG(staticlib): DEFINES += QUAZIP_STATIC

HEADERS += \
    $$PWD/ioapi.h \
    $$PWD/JlCompress.h \
    $$PWD/minizip_crypt.h \
    $$PWD/quaadler32.h \
    $$PWD/quachecksum32.h \
    $$PWD/quacrc32.h \
    $$PWD/quagzipfile.h \
    $$PWD/quaziodevice.h \
    $$PWD/quazipdir.h \
    $$PWD/quazipfile.h \
    $$PWD/quazipfileinfo.h \
    $$PWD/quazip_global.h \
    $$PWD/quazip.h \
    $$PWD/quazipnewinfo.h \
    $$PWD/quazip_qt_compat.h \
    $$PWD/unzip.h \
    $$PWD/zip.h \

SOURCES += \
    $$PWD/unzip.c \
    $$PWD/zip.c \
    $$PWD/JlCompress.cpp \
    $$PWD/qioapi.cpp \
    $$PWD/quaadler32.cpp \
    $$PWD/quachecksum32.cpp \
    $$PWD/quacrc32.cpp \
    $$PWD/quagzipfile.cpp \
    $$PWD/quaziodevice.cpp \
    $$PWD/quazip.cpp \
    $$PWD/quazipdir.cpp \
    $$PWD/quazipfile.cpp \
    $$PWD/quazipfileinfo.cpp \
    $$PWD/quazipnewinfo.cpp \
