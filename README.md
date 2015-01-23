QSvgStyle
=========

QSvgStyle is a themeable SVG style for Qt 4 and Qt 5 applications
(C) Said LANKRI, under the GNU GPL License version 2+

It comes with the style (QSvgStyle), a theme builder (QSvgThemeBuilder)
and a built-in svg cleaner courtesy of SVGCleaner team.

COMPILATION
===========

* You need : Qt4 or Qt5 development files

$ qmake
$ make

USAGE
=====

* If you have root access, you need to copy or symlink the
  libqsvgstyle.so library to the plugins directory of Qt:

(example here for Qt 4)

$ cd /usr/lib/qt4/plugins/styles
$ ln -s /path/to/QSvgStyle/style/plugins/libqsvgstyle.so
or
$ cp /path/to/QSvgStyle/style/plugins/libqsvgstyle.so .

* If you don't have root access, adjust the QT_PLUGIN_PATH variable:

$ export QT_PLUGIN_PATH=$QT_PLUGIN_PATH:/path/to/QSvgStyle/style/plugins

Warning this setting is local to your terminal.

* Temporary testing of the style: run any Qt4 application with the argument:

$ konqueror -style qsvgstyle

* Global setting: run qtconfig and set the default style to
  QSvgStyle. Don't forget to save.
  QSvgStyle will not appear if QT_PLUGIN_PATH does not contain the
  path to the library (non root access only).

* qtconfig

* The QSvgStyle will use its built-in theme to render widgets unless
  it is told to use another theme:

$ mkdir .config/QSvgStyle
$ cd .config/QSvgStyle
$ echo "theme=your_theme_name" > qsvgstyle.cfg

The theme directory must be present inside .config/QSvgStyle:

$ ls .config/QSvgStyle/your_theme_name
your_theme_name.svg               your_theme_name.cfg

The SVG file is designed by yourself. See
/path/to/QSvgStyle/style/default.svg for a complete example and doc/ for
the documentation
the CFG file can be created using /path/to/QSvgStyle/themebuilder/bin/qsvgthemebuilder

* It is recommended to optimize the SVG file each time you modify
  it in an SVG editor. You can use the QSvgThemeBuilder built-in
  optimizer got this purpose.