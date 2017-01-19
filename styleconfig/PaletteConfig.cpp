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
#include "PaletteConfig.h"

#include <stdlib.h>
#include <limits.h>

#include <QDebug>
#include <QString>
#include <QVariant>
#include <QSettings>
#include <QFile>
#include <QStringList>

PaletteConfig::PaletteConfig() :
  settings(NULL)
{
}

PaletteConfig::PaletteConfig(const QString& palette) :
  settings(NULL)
{
  load(palette);
}

PaletteConfig::~PaletteConfig()
{
  if (settings) {
    settings->sync();
    delete settings;
  }
}

void PaletteConfig::load(const QString& palette)
{
  if (settings) {
    settings->sync();
    delete settings;
  }

  settings = NULL;

  if (!QFile::exists(palette))
    return;


  settings = new QSettings(palette,QSettings::NativeFormat);
}

void PaletteConfig::sync()
{
  if ( settings )
    settings->sync();
}

QVariant PaletteConfig::getRawValue(const QString& group, const QString& key) const
{
  return settings ? settings->value(group+"/"+key) : QVariant();
}

QVariant PaletteConfig::getValue(const QString& group, const QString& key, int depth) const
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

void PaletteConfig::setValue(const QString& group, const QString& key, const QVariant& v) const
{
  if (settings) {
    if ( v.isNull() )
      settings->remove(group+"/"+key);
    else
      settings->setValue(group+"/"+key,v);
  }
}

void PaletteConfig::setColorSpec(const QString& group, const color_spec_t& cs) const
{
  // In order to get a clean config file, remove all entries starting with
  // "color" then write the color config
  if ( settings ) {
    settings->beginGroup(group);
    foreach(QString s, settings->childKeys()) {
      if ( s.startsWith("color") )
        settings->remove(s);
    }
    settings->endGroup();
  }

  setValue(group, "color.foreground", cs.fg);
  setValue(group, "color.background", cs.bg);
}

color_spec_t PaletteConfig::getRawColorSpec(const QString& group) const
{
  color_spec_t r;

  r.fg = getRawValue(group, "color.foreground");
  r.bg = getRawValue(group, "color.background");

  return r;
}

color_spec_t PaletteConfig::getColorSpec(const QString& group) const
{
  color_spec_t r;

  r.fg = getValue(group, "color.foreground");
  r.bg = getValue(group, "color.background");

  // fill unset values with default values but leave their status to unset
  if ( !r.fg.present ) {
    r.fg = 0xFFFFFFFF;
    r.fg.present = false;
  }
  if ( !r.bg.present ) {
    r.bg = 0x000000FF;
    r.bg.present = false;
  }

  return r;
}

palette_spec_t PaletteConfig::getPaletteSpec() const
{
  palette_spec_t r;

  r.name = getRawValue("General","name");
  r.author = getRawValue("General","author");
  r.descr = getRawValue("General","comment");
  r.path = settings->fileName();

  return r;
}
