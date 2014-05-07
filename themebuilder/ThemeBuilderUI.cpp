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
#include "ThemeBuilderUI.h"

#include <QComboBox>
#include <QSettings>
#include <QFile>
#include <QListWidget>
#include <QFileDialog>

#include "../style/ThemeConfig.h"
#include <QLineEdit>

/* Helper macros */
#define readInt(group,id,key) \
  group ## id ## Cb->setChecked(espec.group.key.present); \
  if (espec.group.key.present) \
    group ## id ## Sb->setValue(espec.group.key ); \
  else \
    group ## id ## Sb->setValue(0);

#define writeInt(group,id,key) \
  if ( group ## id ## Cb->isChecked() ) \
    espec.group.key = group ## id ## Sb->value(); \
  else \
    espec.group.key.present = false;

ThemeBuilderUI::ThemeBuilderUI(QWidget* parent)
 : QWidget(parent), config(NULL)
{
  setupUi(this);

  config = new ThemeConfig();
  defaultConfig = new ThemeConfig(":default.cfg");

  QObject::connect(elementList,SIGNAL(currentTextChanged(const QString &)), this,SLOT(slot_ElementChanged(const QString &)));
  QObject::connect(quitButton,SIGNAL(clicked()), this,SLOT(close()));
  QObject::connect(loadButton,SIGNAL(clicked()), this,SLOT(slot_open()));
  QObject::connect(newButton,SIGNAL(clicked()), this,SLOT(slot_new()));

  elementList->addItems(ThemeConfig::getManagedElements());
  inheritElementCombo->addItems(ThemeConfig::getManagedElements());
  inheritFrameCombo->addItems(ThemeConfig::getManagedElements());
  inheritInteriorCombo->addItems(ThemeConfig::getManagedElements());
  inheritIndicatorCombo->addItems(ThemeConfig::getManagedElements());
}

ThemeBuilderUI::~ThemeBuilderUI()
{
  delete config;
  delete defaultConfig;
}

void ThemeBuilderUI::slot_quit()
{
  slot_save(lastElement);
}

void ThemeBuilderUI::slot_loadTheme(const QString& theme)
{
  slot_save(lastElement);
  elementList->clear();
  svgElements.clear();
  config->load(theme);

  if (config->isValid()) {
    QStringList f = theme.split("/");
    if (f.size() > 0)
      themeLabel->setText(f[f.size()-1]);

    QStringList l = config->getManagedElements();
    QStringList::iterator it;
    elementList->addItems(l);

    for (it=l.begin(); it!=l.end(); ++it) {
      element_spec_t espec = config->getElementSpec(*it, false);
      if (!((QString) espec.frame.element).isNull())
        if (!svgElements.contains(espec.frame.element))
          svgElements.insert(0,espec.frame.element);

      if (!((QString)espec.interior.element).isNull())
        if (!svgElements.contains(espec.interior.element))
          svgElements.insert(0,espec.interior.element);

      if (!((QString)espec.indicator.element).isNull())
        if (!svgElements.contains(espec.indicator.element))
          svgElements.insert(0,espec.indicator.element);
    }

    // general
    theme_spec_t tspec = config->getThemeSpec();
    authorLineEdit->setText(tspec.author);
    commentLineEdit->setText(tspec.comment);
    animateThemeCb->setChecked(tspec.animated);
    stepSb->setValue(tspec.step);

    mainTab->setEnabled(true);
  } else {
    mainTab->setEnabled(false);
  }
}

void ThemeBuilderUI::slot_ElementChanged(const QString& element)
{
  if (!config->isValid())
    return;

  slot_save(lastElement);

  lastElement = QString();

  if (element.isEmpty())
    return;

  lastElement = element;

  theme_spec_t tspec = config->getThemeSpec();

  // theme
  authorLineEdit->setText(tspec.author);
  commentLineEdit->setText(tspec.comment);
  animateThemeCb->setChecked(tspec.animated);
  stepSb->setValue(tspec.step);

  element_spec_t espec = config->getElementSpec(element, false);

  // general
  if ( !((QString)espec.inherits).isNull() && !((QString)espec.inherits).isEmpty() )
  {
    inheritElementCb->setChecked(true);
    inheritElementCombo->setCurrentIndex(inheritElementCombo->findText(espec.inherits));
  } else {
    inheritElementCb->setChecked(false);
    inheritElementCombo->setCurrentIndex(-1);
  }

  printf("load %s\n",(const char *)(element.toAscii()));

  if (espec.frame.hasCapsule.present && espec.frame.hasCapsule)
    capsuleCb->setChecked(true);
  else
    capsuleCb->setChecked(false);

  // frame
  if ( !((QString)espec.frame.inherits).isNull() && !((QString)espec.frame.inherits).isEmpty() )
  {
    inheritFrameCb->setChecked(true);
    inheritFrameCombo->setCurrentIndex(inheritFrameCombo->findText(espec.frame.inherits));
  } else {
    inheritFrameCb->setChecked(false);
    inheritFrameCombo->setCurrentIndex(-1);
  }

  frameSvgCombo->clear();
  frameSvgCombo->addItems(svgElements);
  frameCb->setChecked(espec.frame.hasFrame);
  frameSvgCombo->setCurrentIndex(frameSvgCombo->findText(espec.frame.element));

  readInt(frame, Top, top);
  readInt(frame, Bottom, bottom);
  readInt(frame, Left, left);
  readInt(frame, Right, right);

  if (espec.frame.animationFrames.present)
    frameAnimateCb->setChecked(true);
  else
    frameAnimateCb->setChecked(false);

  readInt(frame, Frames, animationFrames);

  // interior
  if ( !((QString)espec.interior.inherits).isNull() && !((QString)espec.interior.inherits).isEmpty() )
  {
    inheritInteriorCb->setChecked(true);
    inheritInteriorCombo->setCurrentIndex(inheritInteriorCombo->findText(espec.interior.inherits));
  } else {
    inheritInteriorCb->setChecked(false);
    inheritInteriorCombo->setCurrentIndex(-1);
  }

  interiorSvgCombo->clear();
  interiorSvgCombo->addItems(svgElements);
  interiorCb->setChecked(espec.interior.hasInterior);
  interiorSvgCombo->setCurrentIndex(interiorSvgCombo->findText(espec.interior.element));

  readInt(interior, Rx, px);
  readInt(interior, Ry, py);

  if (espec.interior.animationFrames.present)
    interiorAnimateCb->setChecked(true);
  else
    interiorAnimateCb->setChecked(false);

  readInt(interior, Frames, animationFrames);

  // indicator
  if ( !((QString)espec.indicator.inherits).isNull() && !((QString)espec.indicator.inherits).isEmpty() )
  {
    inheritIndicatorCb->setChecked(true);
    inheritIndicatorCombo->setCurrentIndex(inheritIndicatorCombo->findText(espec.indicator.inherits));
  } else {
    inheritIndicatorCb->setChecked(false);
    inheritIndicatorCombo->setCurrentIndex(-1);
  }

  indicatorSvgCombo->clear();
  indicatorSvgCombo->addItems(svgElements);
  indicatorSvgCombo->setCurrentIndex(indicatorSvgCombo->findText(espec.indicator.element));

  readInt(indicator, Size, size);

  // text
  readInt(label, ShadowX, xshift);
  readInt(label, ShadowY, yshift);
  readInt(label, ShadowDepth, depth);

  if (espec.label.hasShadow.present && espec.label.hasShadow)
    shadowCb->setChecked(true);
  else
    shadowCb->setChecked(false);

  readInt(label, ShadowRed, r);
  readInt(label, ShadowGreen, g);
  readInt(label, ShadowBlue, b);
  readInt(label, ShadowAlpha, a);

  if (espec.label.hasMargin.present && espec.label.hasMargin)
    textMarginCb->setChecked(true);
  else
    textMarginCb->setChecked(false);

  readInt(label, TextTop, top);
  readInt(label, TextBottom, bottom);
  readInt(label, TextLeft, left);
  readInt(label, TextRight, right);

  readInt(label, TextIconSpacing, tispace);

  // size
  heightSb->setValue(0);
  widthSb->setValue(0);

  if (espec.size.minH.present) {
    heightSb->setValue(espec.size.minH);
    minHeightRadio->setChecked(true);
  }
  if (espec.size.fixedH.present) {
    heightSb->setValue(espec.size.fixedH);
    fixedHeightRadio->setChecked(true);
  }
  if (espec.size.minH.present || espec.size.fixedH.present)
    forceHeightCb->setChecked(true);
  else
    forceHeightCb->setChecked(false);

  if (espec.size.minW.present) {
    widthSb->setValue(espec.size.minW);
    minWidthRadio->setChecked(true);
  }
  if (espec.size.fixedW.present) {
    widthSb->setValue(espec.size.fixedW);
    fixedWidthRadio->setChecked(true);
  }
  if (espec.size.minW.present || espec.size.fixedW.present)
    forceWidthCb->setChecked(true);
    else
      forceWidthCb->setChecked(false);
}

void ThemeBuilderUI::slot_save(const QString& widget)
{
  if (!config->isValid())
    return;

  theme_spec_t tspec;
  tspec.author = authorLineEdit->text();
  tspec.comment = commentLineEdit->text();
  tspec.animated = animateThemeCb->isChecked();
  tspec.step = stepSb->value();

  config->setThemeSpec(tspec);

  if (widget.isEmpty())
    return;

  element_spec_t espec;

  if (inheritElementCb->isChecked())
    espec.inherits = inheritElementCombo->currentText();
  if (inheritFrameCb->isChecked())
    espec.frame.inherits = inheritFrameCombo->currentText();
  espec.frame.hasFrame = frameCb->isChecked();
  espec.frame.hasCapsule = capsuleCb->isChecked();
  espec.frame.element = frameSvgCombo->currentText();
  if (!svgElements.contains(espec.frame.element))
    svgElements.append(espec.frame.element);

  writeInt(frame, Top, top);
  writeInt(frame, Bottom, bottom);
  writeInt(frame, Left, left);
  writeInt(frame, Right, right);

  if (frameAnimateCb->isChecked()) {
    writeInt(frame, Frames, animationFrames);
  } else {
    espec.frame.animationFrames = 0;
  }

  if (inheritInteriorCb->isChecked())
    espec.interior.inherits = inheritInteriorCombo->currentText();
  espec.interior.hasInterior = interiorCb->isChecked();
  espec.interior.element = interiorSvgCombo->currentText();
  if (!svgElements.contains(espec.interior.element))
    svgElements.append(espec.interior.element);

  writeInt(interior, Rx, px);
  writeInt(interior, Ry, py);

  if (interiorAnimateCb->isChecked()) {
    writeInt(interior, Frames, animationFrames);
  } else {
    espec.interior.animationFrames = 0;
  }

  if (inheritIndicatorCb->isChecked())
    espec.indicator.inherits = inheritIndicatorCombo->currentText();
  espec.indicator.element = indicatorSvgCombo->currentText();
  if (!svgElements.contains(espec.indicator.element))
    svgElements.append(espec.indicator.element);

  writeInt(indicator, Size, size);

  espec.label.hasShadow = shadowCb->isChecked();

  writeInt(label, ShadowX, xshift);
  writeInt(label, ShadowY, yshift);
  writeInt(label, ShadowDepth, depth);

  writeInt(label, ShadowRed, r);
  writeInt(label, ShadowGreen, g);
  writeInt(label, ShadowBlue, b);
  writeInt(label, ShadowAlpha, a);

  espec.label.hasMargin = textMarginCb->isChecked();

  writeInt(label, TextTop, top);
  writeInt(label, TextBottom, bottom);
  writeInt(label, TextLeft, left);
  writeInt(label, TextRight, right);

  writeInt(label, TextIconSpacing, tispace);

  if (forceHeightCb->isChecked()) {
    if (minHeightRadio->isChecked()) {
      espec.size.minH = heightSb->value();
    }
    if (fixedHeightRadio->isChecked()) {
      espec.size.fixedH = heightSb->value();
    }
  }

  if (forceWidthCb->isChecked()) {
    if (minWidthRadio->isChecked()) {
      espec.size.minW = widthSb->value();
    }
    if (fixedWidthRadio->isChecked()) {
      espec.size.fixedW = widthSb->value();
    }
  }

  printf("saved %s\n",(const char *)(widget.toAscii()));

  config->setElementSpec(widget,espec);
}

void ThemeBuilderUI::slot_open()
{
  QString s = QFileDialog::getOpenFileName(NULL,"Load Theme","","*.cfg");
  if (!s.isNull()) {
    slot_loadTheme(s);
  }
}

void ThemeBuilderUI::slot_new()
{
  QString s = QFileDialog::getSaveFileName(NULL,"New Theme","","*.cfg");
  if (!s.isNull()) {
    //QFile::copy(":default.cfg",s);
    QFile f(s);
    f.open(QIODevice::WriteOnly | QIODevice::Truncate);
    f.close();

    slot_loadTheme(s);
  }
}
