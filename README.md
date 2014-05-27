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

* It is recommended to optimize SVG files before they are used by
  QSvgStyle. This will reduce their size and hence the overall memory footprint
  of applications that use QSvgStyle.

  There are various SVG optimizers int the wild. Two have been successfully
  tested, which means:
  - Optimized SVG is still editable inside Inkscape
  - Optimized SVG is correctly shown using Qt SVG module

First optimizer is "scour". It is a Python program:

$ python scour.py -i my_theme.inkscape.svg -o my_theme.svg --shorten-ids --protect-ids-noninkscape --quiet

To use it, you need to download the 'scour' utility at http://codedread.com/scour

Second optimizer is SVGCleaner:

$ svgcleaner-cli --remove-comments 
		--remove-unused-defs 
		--remove-nonsvg-elts 
		--remove-sodipodi-elts 
		--remove-ai-elts 
		--remove-corel-elts 
		--remove-msvisio-elts 
		--remove-sketch-elts 
		--remove-invisible-elts 
		--remove-empty-containers 
		--remove-duplicated-defs 
		--remove-gaussian-blur=0 
		--remove-notappl-atts 
		--remove-default-atts 
		--remove-inkscape-atts 
		--remove-sodipodi-atts 
		--remove-ai-atts 
		--remove-corel-atts 
		--remove-msvisio-atts 
		--remove-sketch-atts 
		--remove-stroke-props 
		--remove-fill-props 
		--remove-unused-xlinks 
		--simplify-transform-matrix 
		--apply-transforms-to-shapes 
		--remove-unneeded-symbols 
		--apply-transforms-to-paths 
		--colors-to-rrggbb 
		--transform-precision=8 
		--coordinates-precision=6 
		--attributes-precision=6 

Download it at https://github.com/RazrFalcon/
