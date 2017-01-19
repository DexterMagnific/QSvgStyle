# QSvgStyle

QSvgStyle is a themeable SVG style for Qt 5 applications
(C) Said LANKRI, under the GNU GPL License version 2+

It comes with the style engine (QSvgStyle), a theme builder (QSvgThemeBuilder),
 a theme manager (QSvgThemeManager) and a built-in svg cleaner courtesy of
 SVGCleaner team.

# Compile

You need Qt5 development files

```
$ qmake
$ make
```

If you have both Qt4 and Qt5 installed on your system, you can add `-qt=4`
or `-qt=5` to the `qmake` line.

# Install

```
$ sudo make install
```

# Testing

You can run any Qt5 application with QSvgStyle by adding a `-style` option:

```
$ kwrite -style qsvgstyle
```

If you want to set QSvgStyle as the default style for all Qt applications,
use your favorite desktop's configuration application. For example, use the
systemsettings in KDE.

# Theme management with QSvgThemeManager

QSvgStyle already comes with a default built-in theme courtesy of Richard Kung.
This theme will be used by default unless another theme is explicitly set.

To choose a theme, use the QSvgThemeManager application.

```
$ qsvgthememanager
```

Using this app you can also set theme independant tweaks.

# Theme building with QSvgThemeBuilder

QSvgStyle comes with a nice GUI that helps building themes.
It can generate both an initial SVG file and its companion config file.


```
$ qsvgthemebuilder
```

![QSvgThemeBuilder](qsvgstyle.png)

Use `New` theme button to create a new theme based on the default theme.

You can now open the corresponding SVG file on your favorite SVG editor and
work on it.

QSvgThemeBuilder shows live preview of your theme while you make changes to
the SVG file. You can adjust all the settings supported by QSvgStyle engine.

We recommend that you optimize your SVG file once you have finished editing the
SVG file. This will result in smaller files and less memory footprint.
