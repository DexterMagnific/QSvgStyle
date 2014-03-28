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

#include "../themeconfig/ThemeConfig.h"
#include <QLineEdit>

ThemeBuilderUI::ThemeBuilderUI(QWidget* parent)
 : QMainWindow(parent), config(NULL)
{
  setupUi(this);
  
  // Debug menu
  QAction *showDrawingStack = new QAction(tr("Drawing call stack"),NULL);
  showDrawingStack->setCheckable(true);
  showDrawingStack->setData(QVariant(0));
  
  QAction *layoutMode = new QAction(tr("Layout mode"), NULL);
  layoutMode->setCheckable(true);
  layoutMode->setData(QVariant(1));
  
  QAction *resolvedValues = new QAction(tr("Resolved values"), NULL);
  resolvedValues->setCheckable(true);
  resolvedValues->setData(QVariant(2));
  
  QAction *widgetInternals = new QAction(tr("Widget internals"), NULL);
  widgetInternals->setCheckable(true);;
  widgetInternals->setData(QVariant(3));
  
  debugMenu = new QMenu(NULL);
  debugMenu->addAction(showDrawingStack);
  debugMenu->addAction(layoutMode);
  debugMenu->addAction(resolvedValues);
  debugMenu->addAction(widgetInternals);
  debugBtn->setMenu(debugMenu);
  
  connect(previewBtn,SIGNAL(toggled(bool)),previewTab,SLOT(setVisible(bool)));
  connect(propertiesBtn,SIGNAL(toggled(bool)),propertiesTab,SLOT(setVisible(bool)));
  connect(quitBtn,SIGNAL(clicked()),this,SLOT(close()));
  
  connect(debugMenu,SIGNAL(triggered(QAction*)),this,SLOT(slot_debugMenuTriggered(QAction*)));
  
  previewTab->setVisible(previewBtn->isChecked());
  propertiesTab->setVisible(propertiesBtn->isChecked());

  callStackTree->header()->setResizeMode(0,QHeaderView::ResizeToContents);
  resolvedValuesTree->header()->setResizeMode(QHeaderView::ResizeToContents);
  
  slot_debugMenuTriggered(showDrawingStack);
  slot_debugMenuTriggered(resolvedValues);
}

ThemeBuilderUI::~ThemeBuilderUI()
{
}

void ThemeBuilderUI::slot_debugMenuTriggered ( QAction* action )
{
  if (action->data() == 0) {
    callStackTree->setVisible(action->isChecked());
    label_4->setVisible(action->isChecked());
  }
  if (action->data() == 2) {
    resolvedValuesTree->setVisible(action->isChecked());
  }
}

void ThemeBuilderUI::slot_quit()
{
}

void ThemeBuilderUI::slot_loadTheme(const QString& theme)
{
}

void ThemeBuilderUI::slot_ElementChanged(const QString& element)
{
}

void ThemeBuilderUI::slot_save(const QString& widget)
{
}

void ThemeBuilderUI::slot_open()
{
}

void ThemeBuilderUI::slot_new()
{
}
