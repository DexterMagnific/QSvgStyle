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
#include "ThemeConfig.h"

#include <stdlib.h>
#include <limits.h>

#include <QDebug>
#include <QString>
#include <QVariant>
#include <QSettings>
#include <QFile>
#include <QStringList>

ThemeConfig::ThemeConfig() :
  settings(NULL)
{
}

ThemeConfig::ThemeConfig(const QString& theme) :
  settings(NULL)
{
  load(theme);
}

ThemeConfig::~ThemeConfig()
{
  if (settings) {
    settings->sync();
    delete settings;
  }
}

void ThemeConfig::load(const QString& theme)
{
  if (settings) {
    settings->sync();
    delete settings;
  }

  settings = NULL;

  if (!QFile::exists(theme))
    return;

  settings = new QSettings(theme,QSettings::NativeFormat);
}

void ThemeConfig::sync()
{
  if ( settings )
    settings->sync();
}

QVariant ThemeConfig::getRawValue(const QString& group, const QString& key) const
{
  return settings ? settings->value(group+"/"+key) : QVariant();
}

QVariant ThemeConfig::getValue(const QString& group, const QString& key, int depth) const
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

void ThemeConfig::setValue(const QString& group, const QString& key, const QVariant& v) const
{
  if (settings) {
    if ( v.isNull() )
      settings->remove(group+"/"+key);
    else
      settings->setValue(group+"/"+key,v);
  }
}

void ThemeConfig::setFrameSpec(const QString& group, const frame_spec_t& fs) const
{
  // In order to get a clean config file, remove all entries starting with
  // "frame" then write the frame config
  if ( settings ) {
    settings->beginGroup(group);
    foreach(QString s, settings->childKeys()) {
      if ( s.startsWith("frame") )
        settings->remove(s);
    }
    settings->endGroup();
  }

  setValue(group, "frame", fs.hasFrame);
  setValue(group, "frame.element", fs.element);
  setValue(group, "frame.width", fs.width);
}

void ThemeConfig::setInteriorSpec(const QString& group, const interior_spec_t& is) const
{
  // In order to get a clean config file, remove all entries starting with
  // "interior" then write the frame config
  if ( settings ) {
    settings->beginGroup(group);
    foreach(QString s, settings->childKeys()) {
      if ( s.startsWith("interior") )
        settings->remove(s);
    }
    settings->endGroup();
  }

  setValue(group, "interior", is.hasInterior);
  setValue(group, "interior.element", is.element);
  setValue(group, "interior.xrepeat", is.px);
  setValue(group, "interior.yrepeat", is.py);
}

void ThemeConfig::setIndicatorSpec(const QString& group, const indicator_spec_t& ds) const
{
  // In order to get a clean config file, remove all entries starting with
  // "indicator" then write the frame config
  if ( settings ) {
    settings->beginGroup(group);
    foreach(QString s, settings->childKeys()) {
      if ( s.startsWith("indicator") )
        settings->remove(s);
    }
    settings->endGroup();
  }

  setValue(group, "indicator.element", ds.element);
  setValue(group, "indicator.size", ds.size);
}

void ThemeConfig::setLabelSpec(const QString& group, const label_spec_t & ls) const
{
  // In order to get a clean config file, remove all entries starting with
  // "indicator" then write the frame config
  if ( settings ) {
    settings->beginGroup(group);
    foreach(QString s, settings->childKeys()) {
      if ( s.startsWith("label") )
        settings->remove(s);
    }
    settings->endGroup();
  }

  setValue(group, "label.hmargin", ls.hmargin);
  setValue(group, "label.vmargin", ls.vmargin);
  setValue(group, "label.iconspacing", ls.tispace);
}

void ThemeConfig::setElementSpec(const QString& group, const element_spec_t& es) const
{
  setValue(group, "element.inherits", es.inherits);
  setFrameSpec(group,es.frame);
  setInteriorSpec(group,es.interior);
  setIndicatorSpec(group,es.indicator);
  setLabelSpec(group,es.label);
}

void ThemeConfig::setThemeSpec(const theme_spec_t& ts) const
{
  if ( settings ) {
    settings->beginGroup("General");
    settings->remove("");
    settings->endGroup();
  }

  setValue("General","name", ts.name);
  setValue("General","author", ts.author);
  setValue("General","comment", ts.descr);
}

frame_spec_t ThemeConfig::getRawFrameSpec(const QString& group) const
{
  frame_spec_t r;

  r.hasFrame = getRawValue(group, "frame");
  r.element = getRawValue(group, "frame.element");
  r.width = getRawValue(group, "frame.width");

  r.top = r.bottom = r.left = r.right = r.width;

  return r;
}

interior_spec_t ThemeConfig::getRawInteriorSpec(const QString& group) const
{
  interior_spec_t r;

  r.hasInterior = getRawValue(group, "interior");
  r.element = getRawValue(group, "interior.element");
  r.px = getRawValue(group, "interior.xrepeat");
  r.py = getRawValue(group, "interior.yrepeat");

  return r;
}

indicator_spec_t ThemeConfig::getRawIndicatorSpec(const QString& group) const
{
  indicator_spec_t r;

  r.element = getRawValue(group, "indicator.element");
  r.size = getRawValue(group, "indicator.size");

  return r;
}

label_spec_t ThemeConfig::getRawLabelSpec(const QString& group) const
{
  label_spec_t r;

  r.hmargin = getRawValue(group, "label.hmargin");
  r.vmargin = getRawValue(group, "label.vmargin");
  r.tispace = getRawValue(group, "label.iconspacing");

  r.margin = qMax(r.hmargin,r.vmargin);

  return r;
}

element_spec_t ThemeConfig::getRawElementSpec(const QString& group) const
{
  element_spec_t r;

  r.inherits = getRawValue(group, "element.inherits");

  r.frame = getRawFrameSpec(group);
  r.interior = getRawInteriorSpec(group);
  r.indicator = getRawIndicatorSpec(group);
  r.label = getRawLabelSpec(group);

  return r;
}


theme_spec_t ThemeConfig::getThemeSpec() const
{
  theme_spec_t r;

  r.name = getRawValue("General","name");
  r.author = getRawValue("General","author");
  r.descr = getRawValue("General","comment");
  r.path = settings->fileName();

  return r;
}

frame_spec_t ThemeConfig::getFrameSpec(const QString& group) const
{
  frame_spec_t r;

  r.hasFrame = getValue(group, "frame");
  r.element = getValue(group, "frame.element");
  r.width = getValue(group, "frame.width");

  // fill unset values with default values but leave their status to unset
  if ( !r.hasFrame.present ) {
    r.hasFrame = false;
    r.hasFrame.present = false;
  }
  if ( !r.width.present ) {
    r.width = 0;
    r.width.present = false;
  }

  r.top = r.bottom = r.left = r.right = r.width;

  return r;
}

interior_spec_t ThemeConfig::getInteriorSpec(const QString& group) const
{
  interior_spec_t r;

  r.hasInterior = getValue(group, "interior");
  r.element = getValue(group, "interior.element");
  r.px = getValue(group, "interior.xrepeat");
  r.py = getValue(group, "interior.yrepeat");

  // fill unset values with default values but leave their status to unset
  if ( !r.hasInterior.present ) {
    r.hasInterior = false;
    r.hasInterior.present = false;
  }
  if ( !r.px.present ) {
    r.px = 0;
    r.px.present = false;
  }
  if ( !r.py.present ) {
    r.py = 0;
    r.py.present = false;
  }

  return r;
}

indicator_spec_t ThemeConfig::getIndicatorSpec(const QString& group) const
{
  indicator_spec_t r;

  r.element = getValue(group, "indicator.element");
  r.size = getValue(group, "indicator.size");

  // fill unset values with default values but leave their status to unset
  if ( !r.size.present ) {
    r.size = 7;
    r.size.present = false;
  }

  return r;
}

label_spec_t ThemeConfig::getLabelSpec(const QString& group) const
{
  label_spec_t r;

  r.hmargin = getValue(group, "label.hmargin");
  r.vmargin = getValue(group, "label.vmargin");
  r.tispace = getValue(group, "label.iconspacing");

  // fill unset values with default values but leave their status to unset
  if ( !r.hmargin.present ) {
    r.hmargin = 0;
    r.hmargin.present = false;
  }
  if ( !r.vmargin.present ) {
    r.vmargin = 0;
    r.vmargin.present = false;
  }
  if ( !r.tispace.present ) {
    r.tispace = 0;
    r.tispace.present = false;
  }

  r.margin = qMax(r.hmargin,r.vmargin);

  // TODO implement label color,shadow reading
  r.hasShadow = false;
  r.xshift = r.yshift = 0;
  r.r = r.g = r.b = r.a = 255;

  return r;
}

element_spec_t ThemeConfig::getElementSpec(const QString& group) const
{
  element_spec_t r;

  r.inherits = getValue(group, "element.inherits");

  r.frame = getFrameSpec(group);
  r.interior = getInteriorSpec(group);
  r.indicator = getIndicatorSpec(group);
  r.label = getLabelSpec(group);

  return r;
}

