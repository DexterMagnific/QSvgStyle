# styleconfig dependencies for projects that link against pre-built styleconfig
# This provides include paths and library linking, but NOT the source files

INCLUDEPATH += $$PWD

# Library linking - adjust path as needed for your build system
# For QSvgStyle internal build (styleconfig built in build dir alongside style):
# From style/ build dir, styleconfig lib is at styleconfig/lib/
# NOTE: This path only works when included from style/ or styleconfig/ build dirs.
# For external projects (themebuilder, thememanager), set STYLECONFIG_LIB manually before including.
!isEmpty(STYLECONFIG_LIB) {
    LIBS += $$STYLECONFIG_LIB
    PRE_TARGETDEPS += $$STYLECONFIG_LIB
} else {
    LIBS += $$OUT_PWD/styleconfig/lib/libQSvgStyleConfig.a
    PRE_TARGETDEPS += $$OUT_PWD/styleconfig/lib/libQSvgStyleConfig.a
}

# For external projects, you may need to adjust the library path:
# LIBS += -L/path/to/styleconfig/lib -lQSvgStyleConfig
# PRE_TARGETDEPS += /path/to/styleconfig/lib/libQSvgStyleConfig.a