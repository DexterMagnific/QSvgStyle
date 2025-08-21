/***************************************************************************
 *   Copyright (C) 2014 by Saïd LANKRI   *
 *   said.lankri@gmail.com   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#include "StyleConfig.h"

#include <stdlib.h>

#include <QString>
#include <QDebug>
#include <QString>
#include <QVariant>
#include <QSettings>
#include <QFile>
#include <QStringList>
#include <QList>
#if QT_VERSION >= 0x050000
#include <QStandardPaths>
#endif
#include <QDir>


StyleConfig::StyleConfig()
  : QSvgCachedSettings()
{
}

StyleConfig::StyleConfig(const QString& style)
  : QSvgCachedSettings(style)
{
}

StyleConfig::~StyleConfig()
{
}

style_spec_t StyleConfig::getStyleSpec() const
{
  style_spec_t r;

  r.theme = getRawValue("General","theme");

  return r;
}

void StyleConfig::setStyleSpec(const style_spec_t& ss)
{
  removeAllWithPrefix("General","");

  setValue("General","theme", ss.theme);
}

QDir StyleConfig::getSystemConfigDir()
{
  return QDir("/usr/share/QSvgStyle");
}

QDir StyleConfig::getUserConfigDir()
{
  // get cfg dir
  QDir cfgDir = QDir(QStandardPaths::locate(QStandardPaths::GenericConfigLocation,"",QStandardPaths::LocateDirectory).append("QSvgStyle"));

  return cfgDir;
}

QString StyleConfig::getUserConfigFile()
{
  return getUserConfigDir().absolutePath().append("/qsvgstyle.cfg");
}

QList<theme_spec_t> StyleConfig::getThemeList()
{
  QList<theme_spec_t> result;
  QDir cfgDir;
  QStringList themeDirs;

  // get user themes
  cfgDir = getUserConfigDir();
  themeDirs = cfgDir.entryList(QStringList() << "*",
                               QDir::Dirs | QDir::NoDotAndDotDot |
                               QDir::Readable | QDir::Executable);

  Q_FOREACH(QString d, themeDirs) {
    QString basename = cfgDir.absolutePath().append("/%1/%1").arg(d);
    if ( QFile::exists(QString(basename).append(".svg")) &&
         QFile::exists(QString(basename).append(".cfg"))
    ) {
      // both SVG and CFG found -> add theme

      ThemeConfig t(QString(basename).append(".cfg"));
      theme_spec_t ts = t.getThemeSpec();
      ts.path = QString(basename).append(".cfg");
      ts.system = false;

      result.append(ts);
    }
  }

  // get system themes
  cfgDir = getSystemConfigDir();
  themeDirs = cfgDir.entryList(QStringList() << "*",
                               QDir::Dirs | QDir::NoDotAndDotDot |
                               QDir::Readable | QDir::Executable);

  Q_FOREACH(QString d, themeDirs) {
    QString basename = cfgDir.absolutePath().append("/%1/%1").arg(d);
    if ( QFile::exists(QString(basename).append(".svg")) &&
         QFile::exists(QString(basename).append(".cfg"))
    ) {
      // both SVG and CFG found -> add theme

      ThemeConfig t(QString(basename).append(".cfg"));
      theme_spec_t ts = t.getThemeSpec();
      ts.path = QString(basename).append(".cfg");
      ts.system = true;
      result.append(ts);
    }
  }

  return result;
}
