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

PaletteConfig::PaletteConfig()
  : QSvgCachedSettings()
{
}

PaletteConfig::PaletteConfig(const QString& palette)
  : QSvgCachedSettings(palette)
{
}

PaletteConfig::~PaletteConfig()
{
}


void PaletteConfig::setColorSpec(const QString& group, const color_spec_t& cs)
{
  // In order to get a clean config file, remove all entries starting with
  // "color" then write the color config
  removeAllWithPrefix(group, "color");

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
  r.path = filename();

  return r;
}
