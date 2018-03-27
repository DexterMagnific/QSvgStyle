.. highlight:: shell

.. _quick-start:

Quick start guide
=================

This is the documentation of the **QSvgStyle distribution**. The
distribution includes the following components:

- :doc:`qsvgstyle-engine`: A themeable `Qt`_ style engine for desktop
  applications

- :doc:`qsvgthemebuilder`: A GUI for building configuration files for
  themes

- :doc:`qsvgthememanager`: A GUI for managing themes

The QSvgStyle project is a complete rewrite of a former project called
*Quantum Style* which I also initiated. Quantum Style is no longer
available.

- The QSvgStyle distribution is (C) Saïd LANKRI, and is distributed
  under the `GNU General Public License version 3.0
  <https://www.gnu.org/licenses/gpl.html>`_.

- QSvgStyleBuilder incorporates a built-in **SVG cleaner** component
  which is (C) SVGCleaner team.

- QSvgStyleBuilder and QSvgStyleManager use icons from Qt Designer
  which are (C) The Qt Company Ltd.

The latest QSvgStyle distribution documentation is always available at
`Read the Docs <http://qsvgstyle.readthedocs.io>`_.

.. _download:

Download
--------

The QSvgStyle distribution is hosted as a Github project `here
<https://github.com/DexterMagnific/QSvgStyle>`_.

It can be retrieved as a Git repository using the following command::

  git clone https://github.com/DexterMagnific/QSvgStyle.git

Alternatively, a tarball can be downloaded using::

  wget https://github.com/DexterMagnific/QSvgStyle/archive/master.zip


.. _build-n-run:

Build & Run
-----------

Prerequisites
~~~~~~~~~~~~~

The distribution expects to run on top of Qt **version 5**.
To use it, you need the following runtime Qt modules:

- Qt Core
- Qt Widgets
- Qt SVG
- Qt XML

As the distribution comes in source form, you will need the
**development** packages of the modules above in order to compile it.

On a Debian based system, you can get them using the following::

  sudo apt install libqt5gui5-dev libqt5svg5-dev

Compile
~~~~~~~

Run the following commands to compile the distribution::

  cd QSvgStyle
  qmake -qt=5
  make

.. note:: QSvgStyle is a pure Qt5 style engine. It does not depend on KDE.

.. _install:
   
Install
~~~~~~~

Run the following command to install the distribution::

  sudo make install

This command will install:

- The QSvgStyle Engine, which includes a default theme in::

    $PluginsPath/styles/libqsvgstyle.so
    
- The QSvgThemeBuilder application in::

    $PrefixPath/bin/qsvgthemebuilder
    
- The QSvgThemeManager application in::

    $PrefixPath/bin/qsvgthememanager
    
- Some few system themes in::

    $PrefixPath/share/QSvgStyle/<theme_name>

Where ``PluginsPath`` and ``PrefixPath`` are the paths output by the
command::

  qtdiag

.. _run:

Run
~~~

In order to test your install, you can run any Qt5 application with
the ``-style QSvgStyle`` argument.

As an example, if your are working on KDE_, you can launch the
:program:`dolphin` file manager::

   dolphin -style qsvgstyle

.. note:: The style name is not case sensitive when supplied to the
   ``-style`` option

Build the documentation
~~~~~~~~~~~~~~~~~~~~~~~

QSvgStyle distribution comes with its documentation in source
format. The documentation is not built as part of the source build
described above.

The documentation relies on the Sphinx_ documentation builder, which
must be installed prior to building it::

  sudo apt install sphinx-common

To build it, just type::

  cd doc
  make html

This will build the documentation in HTML format. The main entry point
is located in::

  build/html/index.html

Remember that this documentation is always available online at `Read
the Docs`_.

.. _Sphinx: http://www.sphinx-doc.org

Set as default
--------------

Now that you can make individual applications run with QSvgStyle
Engine, you can also globally set all Qt5 application to use QSvgStyle
Engine.

When running KDE, you can go to ``System Settings -> Application
Style`` and change the ``Widget style`` to QSvgStyle.

Change theme
------------

You can change the current theme by running the ``Utilities ->
QSvgStyle Theme Manager`` application from your desktop
menu. Alternatively, you can type in a terminal::

  qsvgthememanager


.. _Qt: http://qt.io
.. _KDE: http://www.kde.org
