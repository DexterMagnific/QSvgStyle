QSvgStyle
=========

QSvgStyle is a themeable SVG style for Qt 4 applications
(C) Said LANKRI, under the GNU GPL License version 2+

COMPILATION
===========

* You need : Qt4 development files

$ qmake
$ make

USAGE
=====

* If you have root access, you need first to copy or symlink the
  libqsvgstyle.so library to the plugins directory of Qt:

$ cd /usr/lib/qt4/plugins/styles
$ ln -s /path/to/QSvgStyle/style/plugins/libqsvgstyle.so
or
$ cp /path/to/QSvgStyle/style/plugins/libqsvgstyle.so .

* If you don't have root access, adjust the QT_PLUGIN_PATH variable:

$ export QT_PLUGIN_PATH=$QT_PLUGIN_PATH:/path/to/QSvgStyle/style/plugins

Warning this setting is local to your terminal. Ask your administrator
either to copy/symlink the library or to make the environment variable
change global.

* Temporary testing of the style: run any Qt4 application with the argument:

$ konqueror -style qsvgstyle

* Global setting: run qtconfig and set the default style to
  QSvgStyle. Don't forget to save.
  QSvgStyle will not appear if QT_PLUGIN_PATH does not contain the
  path to the library (non root access).

* qtconfig

* The QSvgStyle will use its built-in theme to render widgets unless
  it is told to use another theme:

$ mkdir .config/QSvgStyle
$ cd .config/QSvgStyle
$ echo "theme=your_theme_name" > qsvgstyle.cfg

The theme directory must be present inside .config/QSvgStyle:

$ ls .config/QSvgStyle/your_theme_name
your_theme_name.svg               yout_theme_name.cfg

The SVG file is designed by yourself. See
/path/to/QSvgStyle/themeconfig/default.svg for an example and doc/ for
the documentation
the CFG file can be created using /path/to/QSvgStyle/themebuilder/bin/qsvgthemebuilder

