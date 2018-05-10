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

ThemeConfig::ThemeConfig()
  : QSvgCachedSettings()
{
}

ThemeConfig::ThemeConfig(const QString& theme)
  : QSvgCachedSettings(theme)
{
}

void ThemeConfig::setFrameSpec(const QString& group, const frame_spec_t& fs)
{
  // In order to get a clean config file, remove all entries starting with
  // "frame" then write the frame config
  removeAllWithPrefix(group, "frame");

  setValue(group, "frame", fs.hasFrame);
  setValue(group, "frame.element", fs.element);
  setValue(group, "frame.width", fs.width);
}

void ThemeConfig::setInteriorSpec(const QString& group, const interior_spec_t& is)
{
  // In order to get a clean config file, remove all entries starting with
  // "interior" then write the frame config
  removeAllWithPrefix(group, "interior");

  setValue(group, "interior", is.hasInterior);
  setValue(group, "interior.element", is.element);
  setValue(group, "interior.xrepeat", is.px);
  setValue(group, "interior.yrepeat", is.py);
}

void ThemeConfig::setIndicatorSpec(const QString& group, const indicator_spec_t& ds)
{
  // In order to get a clean config file, remove all entries starting with
  // "indicator" then write the frame config
  removeAllWithPrefix(group, "indicator");

  setValue(group, "indicator.element", ds.element);
  setValue(group, "indicator.size", ds.size);
}

void ThemeConfig::setLabelSpec(const QString& group, const label_spec_t & ls)
{
  // In order to get a clean config file, remove all entries starting with
  // "label" then write the frame config
  removeAllWithPrefix(group, "label");

  setValue(group, "label.hmargin", ls.hmargin);
  setValue(group, "label.vmargin", ls.vmargin);
  setValue(group, "label.iconspacing", ls.tispace);
}

void ThemeConfig::setPaletteSpec(const QString& group, const palette_spec_t & ps)
{
  // In order to get a clean config file, remove all entries starting with
  // "palette" then write the frame config
  removeAllWithPrefix(group, "palette");

  setValue(group, "palette.normal.fg", ps.normal.fg);
  setValue(group, "palette.normal.bg", ps.normal.bg);

  setValue(group, "palette.hovered.fg", ps.hovered.fg);
  setValue(group, "palette.hovered.bg", ps.hovered.bg);

  setValue(group, "palette.pressed.fg", ps.pressed.fg);
  setValue(group, "palette.pressed.bg", ps.normal.bg);

  setValue(group, "palette.toggled.fg", ps.toggled.fg);
  setValue(group, "palette.toggled.bg", ps.toggled.bg);

  setValue(group, "palette.disabled.fg", ps.disabled.fg);
  setValue(group, "palette.disabled.bg", ps.disabled.bg);

  setValue(group, "palette.disabled-toggled.fg", ps.disabled_toggled.fg);
  setValue(group, "palette.disabled-toggled.bg", ps.disabled_toggled.bg);

  setValue(group, "palette.focused.fg", ps.focused.fg);
  setValue(group, "palette.focused.bg", ps.focused.bg);

  setValue(group, "palette.default.fg", ps.defaultt.fg);
  setValue(group, "palette.default.bg", ps.defaultt.bg);
}

void ThemeConfig::setFontSpec(const QString& group, const font_spec_t &ts)
{
  // In order to get a clean config file, remove all entries starting with
  // "font" then write the frame config
  removeAllWithPrefix(group, "font");

  setValue(group, "font.normal.bold", ts.normal.bold);
  setValue(group, "font.normal.italic", ts.normal.italic);
  setValue(group, "font.normal.underline", ts.normal.underline);

  setValue(group, "font.hovered.bold", ts.hovered.bold);
  setValue(group, "font.hovered.italic", ts.hovered.italic);
  setValue(group, "font.hovered.underline", ts.hovered.underline);

  setValue(group, "font.pressed.bold", ts.pressed.bold);
  setValue(group, "font.pressed.italic", ts.pressed.italic);
  setValue(group, "font.pressed.underline", ts.pressed.underline);

  setValue(group, "font.toggled.bold", ts.toggled.bold);
  setValue(group, "font.toggled.italic", ts.toggled.italic);
  setValue(group, "font.toggled.underline", ts.toggled.underline);

  setValue(group, "font.disabled.bold", ts.disabled.bold);
  setValue(group, "font.disabled.italic", ts.disabled.italic);
  setValue(group, "font.disabled.underline", ts.disabled.underline);

  setValue(group, "font.disabled-toggled.bold", ts.disabled_toggled.bold);
  setValue(group, "font.disabled-toggled.italic", ts.disabled_toggled.italic);
  setValue(group, "font.disabled-toggled.underline", ts.disabled_toggled.underline);

  setValue(group, "font.focused.bold", ts.focused.bold);
  setValue(group, "font.focused.italic", ts.focused.italic);
  setValue(group, "font.focused.underline", ts.focused.underline);

  setValue(group, "font.default.bold", ts.defaultt.bold);
  setValue(group, "font.default.italic", ts.defaultt.italic);
  setValue(group, "font.default.underline", ts.defaultt.underline);
}

void ThemeConfig::setElementSpec(const QString& group, const element_spec_t& es)
{
  setValue(group, "element.inherits", es.inherits);
  setFrameSpec(group,es.frame);
  setInteriorSpec(group,es.interior);
  setIndicatorSpec(group,es.indicator);
  setLabelSpec(group,es.label);
  setPaletteSpec(group,es.palette);
  setFontSpec(group,es.font);
}

void ThemeConfig::setThemeSpec(const theme_spec_t& ts)
{
  removeAllWithPrefix("General", "");

  setValue("General","name", ts.name);
  setValue("General","variant", ts.variant);
  setValue("General","author", ts.author);
  setValue("General","comment", ts.descr);
  setValue("General","keywords", ts.keywords);
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

palette_spec_t ThemeConfig::getRawPaletteSpec(const QString& group) const
{
  palette_spec_t r;

  r.normal.fg = getRawValue(group, "palette.normal.fg");
  r.normal.bg = getRawValue(group, "palette.normal.bg");

  r.hovered.fg = getRawValue(group, "palette.hovered.fg");
  r.hovered.bg = getRawValue(group, "palette.hovered.bg");

  r.pressed.fg = getRawValue(group, "palette.pressed.fg");
  r.pressed.bg = getRawValue(group, "palette.pressed.bg");

  r.toggled.fg = getRawValue(group, "palette.toggled.fg");
  r.toggled.bg = getRawValue(group, "palette.toggled.bg");

  r.disabled.fg = getRawValue(group, "palette.disabled.fg");
  r.disabled.bg = getRawValue(group, "palette.disabled.bg");

  r.disabled_toggled.fg = getRawValue(group, "palette.disabled-toggled.fg");
  r.disabled_toggled.bg = getRawValue(group, "palette.disabled-toggled.bg");

  r.focused.fg = getRawValue(group, "palette.focused.fg");
  r.focused.bg = getRawValue(group, "palette.focused.bg");

  r.defaultt.fg = getRawValue(group, "palette.default.fg");
  r.defaultt.bg = getRawValue(group, "palette.default.bg");

  return r;
}

font_spec_t ThemeConfig::getRawFontSpec(const QString& group) const
{
  font_spec_t r;

  r.normal.bold = getRawValue(group, "font.normal.bold");
  r.normal.italic = getRawValue(group, "font.normal.italic");
  r.normal.underline = getRawValue(group, "font.normal.underline");

  r.hovered.bold = getRawValue(group, "font.hovered.bold");
  r.hovered.italic = getRawValue(group, "font.hovered.italic");
  r.hovered.underline = getRawValue(group, "font.hovered.underline");

  r.pressed.bold = getRawValue(group, "font.pressed.bold");
  r.pressed.italic = getRawValue(group, "font.pressed.italic");
  r.pressed.underline = getRawValue(group, "font.pressed.underline");

  r.toggled.bold = getRawValue(group, "font.toggled.bold");
  r.toggled.italic = getRawValue(group, "font.toggled.italic");
  r.toggled.underline = getRawValue(group, "font.toggled.underline");

  r.disabled.bold = getRawValue(group, "font.disabled.bold");
  r.disabled.italic = getRawValue(group, "font.disabled.italic");
  r.disabled.underline = getRawValue(group, "font.disabled.underline");

  r.disabled_toggled.bold = getRawValue(group, "font.disabled-toggled.bold");
  r.disabled_toggled.italic = getRawValue(group, "font.disabled-toggled.italic");
  r.disabled_toggled.underline = getRawValue(group, "font.disabled-toggled.underline");

  r.focused.bold = getRawValue(group, "font.focused.bold");
  r.focused.italic = getRawValue(group, "font.focused.italic");
  r.focused.underline = getRawValue(group, "font.focused.underline");

  r.defaultt.bold = getRawValue(group, "font.default.bold");
  r.defaultt.italic = getRawValue(group, "font.default.italic");
  r.defaultt.underline = getRawValue(group, "font.default.underline");

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
  r.palette = getRawPaletteSpec(group);
  r.font = getRawFontSpec(group);

  return r;
}


theme_spec_t ThemeConfig::getThemeSpec() const
{
  theme_spec_t r;

  r.name = getRawValue("General","name");
  r.author = getRawValue("General","author");
  r.variant = getRawValue("General","variant");
  r.descr = getRawValue("General","comment");
  r.keywords = getRawValue("General","keywords");
  r.path = filename();

  return r;
}

color_spec_t *ThemeConfig::paletteRef(palette_spec_t *ps,
                                      const QString &status)
{
  color_spec_t *res = &ps->normal;

  if ( status == "hovered" ) {
    res = &ps->hovered;
  } else if ( status == "pressed" ) {
    res = &ps->pressed;
  } else if ( status == "toggled" ) {
    res = &ps->toggled;
  } else if ( status == "disabled" ) {
    res = &ps->disabled;
  } else if ( status == "disabled-toggled" ) {
    res = &ps->disabled_toggled;
  } else if ( status == "focused" ) {
    res = &ps->focused;
  } else if ( status == "default" ) {
    res = &ps->defaultt;
  }

  return res;
}

font_attr_spec_t *ThemeConfig::fontRef(font_spec_t *ts, const QString &status)
{
  font_attr_spec_t *res = &ts->normal;

  if ( status == "hovered" ) {
    res = &ts->hovered;
  } else if ( status == "pressed" ) {
    res = &ts->pressed;
  } else if ( status == "toggled" ) {
    res = &ts->toggled;
  } else if ( status == "disabled" ) {
    res = &ts->disabled;
  } else if ( status == "disabled-toggled" ) {
    res = &ts->disabled_toggled;
  } else if ( status == "focused" ) {
    res = &ts->focused;
  } else if ( status == "default" ) {
    res = &ts->defaultt;
  }

  return res;
}

frame_spec_t ThemeConfig::getFrameSpec(const QString& group) const
{
  frame_spec_t r;

  r.hasFrame = getValue(group, "frame");
  if ( r.hasFrame.present && r.hasFrame ) {
    r.element = getValue(group, "frame.element");
    r.width = getValue(group, "frame.width");
    r.top = r.bottom = r.left = r.right = r.width;
  }

  return r;
}

interior_spec_t ThemeConfig::getInteriorSpec(const QString& group) const
{
  interior_spec_t r;

  r.hasInterior = getValue(group, "interior");
  if ( r.hasInterior.present && r.hasInterior ) {
    r.element = getValue(group, "interior.element");
    r.px = getValue(group, "interior.xrepeat");
    r.py = getValue(group, "interior.yrepeat");
  }

  return r;
}

indicator_spec_t ThemeConfig::getIndicatorSpec(const QString& group) const
{
  indicator_spec_t r;

  r.element = getValue(group, "indicator.element");
  r.size = getValue(group, "indicator.size");

  return r;
}

label_spec_t ThemeConfig::getLabelSpec(const QString& group) const
{
  label_spec_t r;

  r.hmargin = getValue(group, "label.hmargin");
  r.vmargin = getValue(group, "label.vmargin");
  r.tispace = getValue(group, "label.iconspacing");

  r.margin = qMax(r.hmargin,r.vmargin);

  // TODO implement label color,shadow reading
  r.hasShadow = false;
  r.xshift = r.yshift = 0;

  return r;
}

palette_spec_t ThemeConfig::getPaletteSpec(const QString& group) const
{
  palette_spec_t r;

  r.normal.fg = getValue(group, "palette.normal.fg");
  r.normal.bg = getValue(group, "palette.normal.bg");

  r.hovered.fg = getValue(group, "palette.hovered.fg");
  r.hovered.bg = getValue(group, "palette.hovered.bg");

  r.pressed.fg = getValue(group, "palette.pressed.fg");
  r.pressed.bg = getValue(group, "palette.pressed.bg");

  r.toggled.fg = getValue(group, "palette.toggled.fg");
  r.toggled.bg = getValue(group, "palette.toggled.bg");

  r.disabled.fg = getValue(group, "palette.disabled.fg");
  r.disabled.bg = getValue(group, "palette.disabled.bg");

  r.disabled_toggled.fg = getValue(group, "palette.disabled-toggled.fg");
  r.disabled_toggled.bg = getValue(group, "palette.disabled-toggled.bg");

  r.focused.fg = getValue(group, "palette.focused.fg");
  r.focused.bg = getValue(group, "palette.focused.bg");

  r.defaultt.fg = getValue(group, "palette.default.fg");
  r.defaultt.bg = getValue(group, "palette.default.bg");

  return r;
}

font_spec_t ThemeConfig::getFontSpec(const QString& group) const
{
  font_spec_t r;

  r.normal.bold = getValue(group, "font.normal.bold");
  r.normal.italic = getValue(group, "font.normal.italic");
  r.normal.underline = getValue(group, "font.normal.underline");

  r.hovered.bold = getValue(group, "font.hovered.bold");
  r.hovered.italic = getValue(group, "font.hovered.italic");
  r.hovered.underline = getValue(group, "font.hovered.underline");

  r.pressed.bold = getValue(group, "font.pressed.bold");
  r.pressed.italic = getValue(group, "font.pressed.italic");
  r.pressed.underline = getValue(group, "font.pressed.underline");

  r.toggled.bold = getValue(group, "font.toggled.bold");
  r.toggled.italic = getValue(group, "font.toggled.italic");
  r.toggled.underline = getValue(group, "font.toggled.underline");

  r.disabled.bold = getValue(group, "font.disabled.bold");
  r.disabled.italic = getValue(group, "font.disabled.italic");
  r.disabled.underline = getValue(group, "font.disabled.underline");

  r.disabled_toggled.bold = getValue(group, "font.disabled-toggled.bold");
  r.disabled_toggled.italic = getValue(group, "font.disabled-toggled.italic");
  r.disabled_toggled.underline = getValue(group, "font.disabled-toggled.underline");

  r.focused.bold = getValue(group, "font.focused.bold");
  r.focused.italic = getValue(group, "font.focused.italic");
  r.focused.underline = getValue(group, "font.focused.underline");

  r.defaultt.bold = getValue(group, "font.default.bold");
  r.defaultt.italic = getValue(group, "font.default.italic");
  r.defaultt.underline = getValue(group, "font.default.underline");

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
  r.palette = getPaletteSpec(group);
  r.font = getFontSpec(group);

  return r;
}
