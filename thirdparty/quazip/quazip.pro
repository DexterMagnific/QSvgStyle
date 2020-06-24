CONFIG += \
  release \
  warn_on \
  qt \
  staticlib

QT      += core
QT      -= gui

TARGET   = quazip
DESTDIR  = lib
TEMPLATE = lib

DEFINES += QUAZIP_BUILD
DEFINES += QT_NO_CAST_FROM_ASCII
DEFINES += QT_NO_CAST_TO_ASCII
CONFIG(staticlib): DEFINES += QUAZIP_STATIC

HEADERS += \
    $$PWD/minizip_crypt.h \
    $$PWD/ioapi.h \
    $$PWD/JlCompress.h \
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
    $$PWD/unzip.h \
    $$PWD/zip.h

SOURCES += $$PWD/qioapi.cpp \
    $$PWD/JlCompress.cpp \
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
    $$PWD/unzip.c \
    $$PWD/zip.c
