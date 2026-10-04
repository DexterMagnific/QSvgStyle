TEMPLATE = subdirs

# Minimal build mode: only build style plugin (and its dependency styleconfig)
# Usage: qmake CONFIG+=minimal
CONFIG(minimal) {
  message("Building in MINIMAL mode - style plugin only")
  SUBDIRS = style/styleconfig style
} else {
  SUBDIRS = style/styleconfig style thirdparty/svgcleaner thirdparty/quazip themebuilder thememanager
}

CONFIG += ordered

# Generate all theme resources at configure time
system($$shell_path($$PWD/style/generate_theme_resources.sh) $$PWD/style/themes/themes_list.txt $$PWD/style $$PWD/style/themes)

# Define install target for non-built-in themes
include(style/themes/themes_install.pri)

# Ensure nonbuiltin_themes is installed
!contains(INSTALLS, nonbuiltin_themes) {
    INSTALLS += nonbuiltin_themes
}