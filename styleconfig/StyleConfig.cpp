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


StyleConfig::StyleConfig() :
  settings(NULL)
{
}

StyleConfig::StyleConfig(const QString& style) :
  settings(NULL)
{
  load(style);
}

StyleConfig::~StyleConfig()
{
  if (settings) {
    settings->sync();
    delete settings;
  }
}

void StyleConfig::load(const QString& style)
{
  if (settings) {
    settings->sync();
    delete settings;
  }

  settings = NULL;

  if (!QFile::exists(style))
    return;


  settings = new QSettings(style,QSettings::NativeFormat);
}

void StyleConfig::sync()
{
  if ( settings )
    settings->sync();
}

QVariant StyleConfig::getRawValue(const QString& group, const QString& key) const
{
  return settings ? settings->value(group+"/"+key) : QVariant();
}

QVariant StyleConfig::getValue(const QString& group, const QString& key, int depth) const
{
  QVariant r;
  QVariant inherit;

  inherit = getRawValue(group,"element.inherits");
  r = getRawValue(group, key);

  if ( r.isNull() && !inherit.isNull() && (depth <= 2) ) {
    r = getValue(inherit.toString(),key,depth+1);
  }

  return r;
}

void StyleConfig::setValue(const QString& group, const QString& key, const QVariant& v) const
{
  if (settings) {
    if ( v.isNull() )
      settings->remove(group+"/"+key);
    else
      settings->setValue(group+"/"+key,v);
  }
}

style_spec_t StyleConfig::getStyleSpec() const
{
  style_spec_t r;

  r.theme = getRawValue("General","theme");
  r.palette = getRawValue("General","palette");

  return r;
}

void StyleConfig::setStyleSpec(const style_spec_t& ss) const
{
  if ( settings ) {
    settings->beginGroup("General");
    settings->remove("");
    settings->endGroup();
  }

  setValue("General","theme", ss.theme);
  setValue("General","palette", ss.palette);
}

QDir StyleConfig::getSystemConfigDir()
{
  return QDir("/usr/share/QSvgStyle");
}

QDir StyleConfig::getUserConfigDir()
{
  // get cfg dir
  QDir cfgDir;

#if QT_VERSION >= 0x050000
  cfgDir = QDir(QStandardPaths::locate(QStandardPaths::ConfigLocation,"",QStandardPaths::LocateDirectory).append("QSvgStyle"));
#else
  cfgDir = QDir(QDir::homePath().append("/.config/QSvgStyle"));
#endif

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

      result.append(ts);
    }
  }


  return result;
}

QList<palette_spec_t> StyleConfig::getPaletteList()
{
  QList<palette_spec_t> result;
  QDir cfgDir;
  QStringList paletteFiles;

  // get user palettes
  cfgDir = getUserConfigDir();
  paletteFiles = cfgDir.entryList(QStringList() << "*.pal",
                                  QDir::Files | QDir::Readable);

  Q_FOREACH(QString f, paletteFiles) {
    QString filename = cfgDir.absolutePath().append("/%1").arg(f);
    PaletteConfig p(filename);
    palette_spec_t ps = p.getPaletteSpec();

    result.append(ps);
  }

  // get system palettes
  cfgDir = getSystemConfigDir();
  paletteFiles = cfgDir.entryList(QStringList() << "*.pal",
                                  QDir::Files | QDir::Readable);

  Q_FOREACH(QString f, paletteFiles) {
    QString filename = cfgDir.absolutePath().append("/%1").arg(f);
    PaletteConfig p(filename);
    palette_spec_t ps = p.getPaletteSpec();

    result.append(ps);
  }

  return result;
}
