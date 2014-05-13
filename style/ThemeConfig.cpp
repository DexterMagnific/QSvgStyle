/***************************************************************************
 *   Copyright (C) 2009 by Saïd LANKRI   *
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

#include <QString>
#include <QVariant>
#include <QSettings>
#include <QFile>
#include <QStringList>

ThemeConfig::ThemeConfig() :
  m_valid(false),
  settings(NULL),
  m_parent(NULL)
{
}

ThemeConfig::ThemeConfig(const QString& theme, ThemeConfig* parent) :
  m_valid(false),
  settings(NULL),
  m_parent(parent)
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
  m_valid = false;

  if (!QFile::exists(theme))
    return;

  settings = new QSettings(theme,QSettings::NativeFormat);

  m_valid = true;
}

QVariant ThemeConfig::getValue(const QString& group, const QString& key, bool* ok) const
{
  QVariant r;
  if (ok)
    *ok = false;

  if (group.isNull() || group.isEmpty() || key.isNull() || key.isEmpty())
    return r;

  if (settings) {
    settings->beginGroup(group);

    bool present = settings->contains(key);
    if (ok)
      *ok = present;

    if (present)
    {
      r = settings->value(key);
      //qDebug() << group << "/" << key << "found";
    }

    settings->endGroup();
  }

  return r;
}

QVariant ThemeConfig::getValue(const QString& group, const QString& key, bool* ok, const QStringList &inherits, const ThemeConfig *parent) const
{
  QVariant r;

  r = getValue(group, key, ok);

  if (ok && !*ok) {
    QString g;
    foreach (g,inherits) {
      r = getValue(g,key, ok);
      if (ok && *ok)
        break;
    }

    if (ok && !*ok && parent) {
      r = parent->getValue(group, key, ok);
    }
  }

  return r;
}

template<typename T>
void ThemeConfig::setValue(const QString& group, const QString& key, const value_t< T >& v) const
{
  if (v.present)
  {
    setValue(group,key,QString("%1").arg(v));
    //qDebug() << group << "/" << key << "written";
  } else {
    removeKey(group,key);
    //qDebug() << group << "/" << key << "removed";
  }
}

void ThemeConfig::setValue(const QString& group, const QString& key, const QString& v) const
{
  if (settings) {
    settings->beginGroup(group);
    if ( (v.isEmpty()) || (v == QString("%1").arg(-9999)) )
      settings->remove(key);
    else
      settings->setValue(key,v);
    settings->endGroup();
  }
}

void ThemeConfig::removeKey(const QString& group, const QString& key) const
{
  if (settings) {
    settings->beginGroup(group);
    settings->remove(key);
    settings->endGroup();
  }
}

frame_spec_t ThemeConfig::getFrameSpec(const QString &elementName, bool expand) const
{
  frame_spec_t r;
  if (expand)
    default_frame_spec(r);

  bool ok;
  QStringList l;
  ThemeConfig *parent = (expand)?m_parent:NULL;

  QVariant v = getValue(elementName, "element.inherits", &ok);
  if (ok && expand)
    l.prepend(v.toString());

  v = getValue(elementName,"frame", &ok);
  if (ok) {
    r.hasFrame = v.toBool();
    if (r.hasFrame) {
      v = getValue(elementName, "frame.inherits", &ok);
      if (ok) {
        r.inherits = v.toString();
        if (expand)
          l.prepend(r.inherits);
      }
      v = getValue(elementName, "frame.element", &ok, l,parent);
      if (ok)
        r.element = v.toString();
      v = getValue(elementName,"frame.top", &ok, l,parent);
      if (ok)
        r.top = v.toInt();
      v = getValue(elementName,"frame.bottom", &ok, l,parent);
      if (ok)
        r.bottom = v.toInt();
      v = getValue(elementName,"frame.left", &ok, l,parent);
      if (ok)
        r.left = v.toInt();
      v = getValue(elementName,"frame.right", &ok, l,parent);
      if (ok)
        r.right = v.toInt();
      v = getValue(elementName,"frame.width", &ok, l,parent);
      if (ok)
        r.width = v.toInt();
      else
        r.width = qMax(qMax(r.top,r.bottom),qMax(r.left,r.right));

      v = getValue(elementName,"frame.capsule", &ok, l,parent);
      if (ok)
        r.hasCapsule = v.toBool();

      v = getValue(elementName,"frame.animation.frames",&ok, l,parent);
      if (ok)
        r.animationFrames = v.toInt();
      v = getValue(elementName,"frame.animation.loop",&ok, l,parent);
      if (ok)
        r.loopAnimation = v.toBool();

      if (r.animationFrames <= 0)
        r.animationFrames = 1;
    }
  }

  return r;
}

void ThemeConfig::setFrameSpec(const QString& elementName, const frame_spec_t& fspec) const
{
  setValue(elementName, "frame", fspec.hasFrame ? "true" : "false");
  setValue(elementName, "frame.element", fspec.element);
  setValue(elementName, "frame.inherits", fspec.inherits);
  setValue(elementName, "frame.top", fspec.top);
  setValue(elementName, "frame.bottom", fspec.bottom);
  setValue(elementName, "frame.left", fspec.left);
  setValue(elementName, "frame.right", fspec.right);
  setValue(elementName, "frame.capsule", fspec.hasCapsule);
  setValue(elementName, "frame.animation.frames", fspec.animationFrames);
  setValue(elementName, "frame.animation.loop", fspec.loopAnimation);
}

interior_spec_t ThemeConfig::getInteriorSpec(const QString &elementName, bool expand) const
{
  interior_spec_t r;
  if (expand)
    default_interior_spec(r);

  bool ok;
  QStringList l;
  ThemeConfig *parent = (expand)?m_parent:NULL;

  QVariant v = getValue(elementName, "element.inherits", &ok);
  if (ok && expand)
    l.prepend(v.toString());

  v = getValue(elementName,"interior", &ok);
  if (ok)
    r.hasInterior = v.toBool();

  if (r.hasInterior) {
    v = getValue(elementName, "interior.inherits", &ok);
    if (ok) {
      r.inherits = v.toString();
      if (expand)
        l.prepend(r.inherits);
    }

    v = getValue(elementName, "interior.element", &ok, l,parent);
    if (ok)
      r.element = v.toString();
  }

  v = getValue(elementName,"interior.repeat.x.patternsize",&ok, l,parent);
  if (ok)
    r.px = v.toInt();
  v = getValue(elementName,"interior.repeat.y.patternsize",&ok, l,parent);
  if (ok)
    r.py = v.toInt();

  v = getValue(elementName,"interior.animation.frames",&ok, l,parent);
  if (ok)
    r.animationFrames = v.toInt();
  v = getValue(elementName,"interior.animation.loop",&ok, l,parent);
  if (ok)
    r.loopAnimation = v.toBool();

  if (r.animationFrames <= 0)
    r.animationFrames = 1;

  return r;
}

void ThemeConfig::setInteriorSpec(const QString& elementName, const interior_spec_t& ispec) const
{
  setValue(elementName, "interior", ispec.hasInterior ? "true" : "false");
  setValue(elementName, "interior.inherits", ispec.inherits);
  setValue(elementName, "interior.element", ispec.element);
  setValue(elementName, "interior.margin", ispec.hasMargin);
  setValue(elementName, "interior.repeat.x.patternsize", ispec.px);
  setValue(elementName, "interior.repeat.y.patternsize", ispec.py);
  setValue(elementName, "interior.animation.frames", ispec.animationFrames);
  setValue(elementName, "interior.animation.loop", ispec.loopAnimation);
}

indicator_spec_t ThemeConfig::getIndicatorSpec(const QString &elementName, bool expand) const
{
  indicator_spec_t r;
  if (expand)
    default_indicator_spec(r);

  bool ok = false;
  QStringList l;
  ThemeConfig *parent = (expand)?m_parent:NULL;

  QVariant v = getValue(elementName, "element.inherits", &ok);
  if (ok && expand)
    l.prepend(v.toString());

  v = getValue(elementName, "indicator.inherits", &ok);
  if (ok) {
    r.inherits = v.toString();
    if (expand)
      l.prepend(r.inherits);
  }

  v = getValue(elementName,"indicator.size", &ok, l,parent);
  if (ok)
    r.size = v.toInt();

  v = getValue(elementName, "indicator.element", &ok, l,parent);
    if (ok)
      r.element = v.toString();

  v = getValue(elementName,"indicator.animation.frames",&ok, l,parent);
  if (ok)
    r.animationFrames = v.toInt();
  v = getValue(elementName,"indicator.animation.loop",&ok, l,parent);
  if (ok)
    r.loopAnimation = v.toBool();

  if (r.animationFrames <= 0)
    r.animationFrames = 1;

  return r;
}

void ThemeConfig::setIndicatorSpec(const QString& elementName, const indicator_spec_t& dspec) const
{
  setValue(elementName, "indicator.inherits", dspec.inherits);
  setValue(elementName, "indicator.element", dspec.element);
  setValue(elementName, "indicator.size", dspec.size);
  setValue(elementName, "indicator.animation.frames", dspec.animationFrames);
  setValue(elementName, "indicator.animation.loop", dspec.loopAnimation);
}

label_spec_t ThemeConfig::getLabelSpec(const QString &elementName, bool expand) const
{
  label_spec_t r;
  if (expand)
    default_label_spec(r);

  bool ok;
  QStringList l;
  ThemeConfig *parent = (expand)?m_parent:NULL;

  QVariant v = getValue(elementName, "element.inherits", &ok);
  if (ok && expand)
    l.prepend(v.toString());

  v = getValue(elementName,"text.shadow", &ok, l,parent);
  if (ok) {
    r.hasShadow = v.toBool();

    if (r.hasShadow) {
      v = getValue(elementName,"text.shadow.xshift", &ok, l,parent);
      if (ok)
        r.xshift = v.toInt();
      v = getValue(elementName,"text.shadow.yshift", &ok, l,parent);
      if (ok)
        r.yshift = v.toInt();
      v = getValue(elementName,"text.shadow.red", &ok, l,parent);
      if (ok)
        r.r = v.toInt();
      v = getValue(elementName,"text.shadow.green", &ok, l,parent);
      if (ok)
        r.g = v.toInt();
      v = getValue(elementName,"text.shadow.blue", &ok, l,parent);
      if (ok)
        r.b = v.toInt();
      v = getValue(elementName,"text.shadow.alpha", &ok, l,parent);
      if (ok)
        r.a = v.toInt();
      v = getValue(elementName,"text.shadow.depth", &ok, l,parent);
      if (ok)
        r.depth = v.toInt();
    }
  }

  v = getValue(elementName,"text.margin", &ok, l,parent);
  if (ok) {
    r.hasMargin = v.toBool();
    if (r.hasMargin) {
      v = getValue(elementName,"text.margin.top", &ok, l,parent);
      if (ok)
        r.top = v.toInt();
      v = getValue(elementName,"text.margin.bottom", &ok, l,parent);
      if (ok)
        r.bottom = v.toInt();
      v = getValue(elementName,"text.margin.left", &ok, l,parent);
      if (ok)
        r.left = v.toInt();
      v = getValue(elementName,"text.margin.right", &ok, l,parent);
      if (ok)
        r.right = v.toInt();
      v = getValue(elementName,"text.margin.h", &ok, l,parent);
      if (ok)
        r.hmargin = v.toInt();
      else
        r.hmargin = qMax(r.left,r.right);
      v = getValue(elementName,"text.margin.v", &ok, l,parent);
      if (ok)
        r.vmargin = v.toInt();
      else
        r.vmargin = qMax(r.top,r.bottom);
    }
  }

  v = getValue(elementName,"text.iconspacing", &ok, l,parent);
  if (ok)
    r.tispace = v.toInt();

  return r;
}

void ThemeConfig::setLabelSpec(const QString& elementName, const label_spec_t & lspec) const
{
  setValue(elementName, "text.shadow", lspec.hasShadow);
  setValue(elementName, "text.shadow.xshift", lspec.xshift);
  setValue(elementName, "text.shadow.yshift", lspec.yshift);
  setValue(elementName, "text.shadow.red", lspec.r);
  setValue(elementName, "text.shadow.green", lspec.g);
  setValue(elementName, "text.shadow.blue", lspec.b);
  setValue(elementName, "text.shadow.alpha", lspec.a);
  setValue(elementName, "text.shadow.depth", lspec.depth);
  setValue(elementName, "text.margin", lspec.hasMargin);
  setValue(elementName, "text.margin.top", lspec.top);
  setValue(elementName, "text.margin.bottom", lspec.bottom);
  setValue(elementName, "text.margin.left", lspec.left);
  setValue(elementName, "text.margin.right", lspec.right);
  setValue(elementName, "text.iconspacing", lspec.tispace);
}

size_spec_t ThemeConfig::getSizeSpec(const QString& elementName, bool expand) const
{
  size_spec_t r;
  if (expand)
    default_size_spec(r);

  bool ok = false;
  QStringList l;
  ThemeConfig *parent = (expand)?m_parent:NULL;

  QVariant v = getValue(elementName, "element.inherits", &ok);
  if (ok && expand)
    l.prepend(v.toString());

  v = getValue(elementName,"size.minheight", &ok, l,parent);
  if (ok)
    r.minH = v.toInt();
  v = getValue(elementName,"size.fixedheight", &ok, l,parent);
  if (ok)
    r.fixedH = v.toInt();

  v = getValue(elementName,"size.minwidth", &ok, l,parent);
  if (ok)
    r.minW = v.toInt();
  v = getValue(elementName,"size.fixedwidth", &ok, l,parent);
  if (ok)
    r.fixedW = v.toInt();

  return r;
}

void ThemeConfig::setSizeSpec(const QString& elementName, const size_spec_t &sspec) const
{
  setValue(elementName, "size.minheight", sspec.minH);
  setValue(elementName, "size.fixedheight", sspec.fixedH);
  setValue(elementName, "size.minwidth", sspec.minW);
  setValue(elementName, "size.fixedwidth", sspec.fixedW);
}

element_spec_t ThemeConfig::getElementSpec(const QString& elementName, bool expand) const
{
  element_spec_t r;
  if (expand)
    default_element_spec(r);

  bool ok = false;
  QVariant v;

  v = getValue(elementName, "element.inherits", &ok);
  if (ok)
    r.inherits = v.toString();

  r.frame = getFrameSpec(elementName, expand);
  r.interior = getInteriorSpec(elementName, expand);
  r.indicator = getIndicatorSpec(elementName, expand);
  r.label = getLabelSpec(elementName, expand);
  r.size = getSizeSpec(elementName, expand);

  return r;
}

void ThemeConfig::setElementSpec(const QString& elementName, const element_spec_t& espec) const
{
  setValue(elementName, "element.inherits", espec.inherits);
  setFrameSpec(elementName,espec.frame);
  setInteriorSpec(elementName,espec.interior);
  setIndicatorSpec(elementName,espec.indicator);
  setLabelSpec(elementName,espec.label);
  setSizeSpec(elementName,espec.size);
}

theme_spec_t ThemeConfig::getThemeSpec() const
{
  theme_spec_t r;
  default_theme_spec(r);

  bool ok = false;

  QVariant v = getValue("General","author", &ok);
  if (ok)
    r.author = v.toString();

  v = getValue("General","comment", &ok);
  if (ok)
    r.descr = v.toString();

  v = getValue("General","animated", &ok);
  if (ok)
    r.animated = v.toBool();

  v = getValue("General","animation.step", &ok);
  if (ok)
    r.step = v.toInt();

  return r;
}

void ThemeConfig::setThemeSpec(const theme_spec_t& tspec) const
{
  setValue("General","author", tspec.author);
  setValue("General","comment", tspec.descr);
  setValue("General","animated", QString("%1").arg(tspec.animated));
  setValue("General","animation.step", QString("%1").arg(tspec.step));
}

QStringList ThemeConfig::getManagedElements()
{
  return
    QStringList()
      << "PanelButtonCommand"
      << "PanelButtonTool"
      << "Dock"
      << "RadioButton"
      << "CheckBox"
      << "Focus"
      << "GenericFrame"
      << "TabFrame"
      << "GroupBox"
      << "LineEdit"
      << "IndicatorSpinBox"
      << "DropDownButton"
      << "IndicatorArrow"
      << "ToolboxTab"
      << "Tab"
      << "TreeExpander"
      << "IndicatorHeaderArrow"
      << "HeaderSection"
      << "SizeGrip"
      << "Toolbar"
      << "ScrollbarSlider"
      << "Slider"
      << "SliderCursor"
      << "Progressbar"
      << "ProgressbarContents"
      << "Splitter"
      << "Scrollbar"
      << "ScrollbarGroove"
      << "Widget"
      << "MenuItem"
      << "Menu"
      << "MenuBarItem"
      << "MenuBar"
      << "ViewItem"
      << "TitleBar"
      << "ComboBox"
  ;
}
