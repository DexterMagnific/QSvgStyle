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
#ifdef WIN32
#include <QProcessEnvironment>
#endif
#if QT_VERSION >= 0x050000
#include <QStandardPaths>
#endif
#include <QDir>

// Include generated built-in themes list
#include "builtin_themes.h"


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

QList<theme_spec_t> StyleConfig::discoverThemes()
{
  QList<theme_spec_t> result;

  // 1. Environment variable (highest priority)
  QString envPaths = qgetenv("QSVGSTYLE_THEME_PATHS");
  if (!envPaths.isEmpty()) {
    QStringList paths = envPaths.split(':', Qt::SkipEmptyParts);  // colon on all platforms
    for (const QString& path : paths) {
      addThemesFromDir(path, ThemeOrigin_Environment, result);
    }
  }

  // 2. User locations
  for (const QString& dir : getUserThemeDirs()) {
    addThemesFromDir(dir, ThemeOrigin_User, result);
  }

  // 3. System locations
  for (const QString& dir : getSystemThemeDirs()) {
    addThemesFromDir(dir, ThemeOrigin_System, result);
  }

  // 4. Built-in themes (lowest priority)
  for (int i = 0; QSVGSTYLE_BUILTIN_THEMES_LIST[i]; ++i) {
    QString name = QString::fromLatin1(QSVGSTYLE_BUILTIN_THEMES_LIST[i]);
    theme_spec_t ts;
    ts.name = name;
    ts.path = QString(":/qsvgstyle/themes/%1/%1.cfg").arg(name);
    ts.origin = ThemeOrigin_Builtin;
    result.append(ts);
  }

  return result;
}

QStringList StyleConfig::getThemeSearchPaths(ThemeOrigin origin)
{
  QStringList dirs;
  switch (origin) {
    case ThemeOrigin_Environment: {
      QString envPaths = qgetenv("QSVGSTYLE_THEME_PATHS");
      if (!envPaths.isEmpty()) {
        dirs = envPaths.split(':', Qt::SkipEmptyParts);
      }
      break;
    }
    case ThemeOrigin_User:
      dirs = getUserThemeDirs();
      break;
    case ThemeOrigin_System:
      dirs = getSystemThemeDirs();
      break;
    case ThemeOrigin_Builtin:
      // Built-in themes don't have filesystem paths
      break;
  }
  return dirs;
}

QStringList StyleConfig::getUserThemeDirs()
{
  QStringList dirs;
  // ~/.config/QSvgStyle (GenericConfigLocation)
  dirs << QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + "/QSvgStyle";
  // ~/.local/usr/share/QSvgStyle (AppLocalDataLocation)
  dirs << QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/QSvgStyle";
  return dirs;
}

QStringList StyleConfig::getSystemThemeDirs()
{
  QStringList dirs;
  // GenericDataLocation returns /usr/share, /usr/local/share on Linux
  // On Windows, returns appropriate system data directories
  QStringList dataDirs = QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation);
  for (const QString& d : dataDirs) {
    dirs << d + "/QSvgStyle";
  }
  return dirs;
}

void StyleConfig::addThemesFromDir(const QString& dirPath, ThemeOrigin origin, QList<theme_spec_t>& result)
{
  QDir dir(dirPath);
  if (!dir.exists()) return;

  QStringList themeDirs = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot | QDir::Readable);
  for (const QString& d : themeDirs) {
    QString base = dir.absoluteFilePath(d + "/" + d);
    if (QFile::exists(base + ".svg") && QFile::exists(base + ".cfg")) {
      ThemeConfig t(base + ".cfg");
      theme_spec_t ts = t.getThemeSpec();
      ts.path = base + ".cfg";
      ts.origin = origin;
      result.append(ts);
    }
  }
}

QDir StyleConfig::getUserConfigDir()
{
  // get cfg dir
  QDir cfgDir = QDir(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation).append("/QSvgStyle"));
  return cfgDir;
}

QString StyleConfig::getUserConfigFile()
{
  return getUserConfigDir().absolutePath().append("/qsvgstyle.cfg");
}