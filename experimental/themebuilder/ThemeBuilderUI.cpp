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
#include <QTreeWidget>
#include <QTreeWidgetItem>

#include "../style/ThemeConfig.h"
#include <QLineEdit>
#include <QScrollBar>

ThemeBuilderUI::ThemeBuilderUI(QWidget* parent)
 : QMainWindow(parent), config(NULL)
{
  // Setup using auto-generated UIC code
  setupUi(this);


  // populate widget tree views
  QListWidgetItem *i;

  // FIXME use xx_group() functions from QSvgStyle class to set group role
  QIcon icon1;
  icon1.addFile(QString::fromUtf8(":/icon/pixmaps/pushbutton.png"), QSize(), QIcon::Normal, QIcon::Off);
  i = new QListWidgetItem(buttonList);
  i->setIcon(icon1);
  i->setText("Push button");
  i->setData(GroupRole,"PushButton");

  QIcon icon2;
  icon2.addFile(QString::fromUtf8(":/icon/pixmaps/toolbutton.png"), QSize(), QIcon::Normal, QIcon::Off);
  i = new QListWidgetItem(buttonList);
  i->setIcon(icon2);
  i->setText("Tool button");
  i->setData(GroupRole,"ToolButton");

  QIcon icon3;
  icon3.addFile(QString::fromUtf8(":/icon/pixmaps/radiobutton.png"), QSize(), QIcon::Normal, QIcon::Off);
  i = new QListWidgetItem(buttonList);
  i->setIcon(icon3);
  i->setText("Radio button");
  i->setData(GroupRole,"RadioButton");

  QIcon icon4;
  icon4.addFile(QString::fromUtf8(":/icon/pixmaps/checkbox.png"), QSize(), QIcon::Normal, QIcon::Off);
  i = new QListWidgetItem(buttonList);
  i->setIcon(icon4);
  i->setText("Check box");
  i->setData(GroupRole,"CheckBox");

  QIcon icon5;
  icon5.addFile(QString::fromUtf8(":/icon/pixmaps/lineedit.png"), QSize(), QIcon::Normal, QIcon::Off);
  i = new QListWidgetItem(inputList);
  i->setIcon(icon5);
  i->setText("Line edit");
  i->setData(GroupRole,"LineEdit");

  QIcon icon6;
  icon6.addFile(QString::fromUtf8(":/icon/pixmaps/spinbox.png"), QSize(), QIcon::Normal, QIcon::Off);
  i = new QListWidgetItem(inputList);
  i->setIcon(icon6);
  i->setText("Spin box");
  i->setData(GroupRole,"SpinBox");

  QIcon icon7;
  icon7.addFile(QString::fromUtf8(":/icon/pixmaps/vscrollbar.png"), QSize(), QIcon::Normal, QIcon::Off);
  i = new QListWidgetItem(inputList);
  i->setIcon(icon7);
  i->setText("Scroll bar");
  i->setData(GroupRole,"ScrollBar");

  QIcon icon8;
  icon8.addFile(QString::fromUtf8(":/icon/pixmaps/hslider.png"), QSize(), QIcon::Normal, QIcon::Off);
  i = new QListWidgetItem(inputList);
  i->setIcon(icon8);
  i->setText("Slider");
  i->setData(GroupRole,"Slider");

  QIcon icon9;
  icon9.addFile(QString::fromUtf8(":/icon/pixmaps/dial.png"), QSize(), QIcon::Normal, QIcon::Off);
  i = new QListWidgetItem(inputList);
  i->setIcon(icon9);
  i->setText("Dial");
  i->setData(GroupRole,"Dial");

  QIcon icon10;
  icon10.addFile(QString::fromUtf8(":/icon/pixmaps/progress.png"), QSize(), QIcon::Normal, QIcon::Off);
  i = new QListWidgetItem(displayList);
  i->setIcon(icon10);
  i->setText("Progress bar");
  i->setData(GroupRole,"ProgressBar");

  QIcon icon11;
  icon11.addFile(QString::fromUtf8(":/icon/pixmaps/edithlayout.png"), QSize(), QIcon::Normal, QIcon::Off);
  i = new QListWidgetItem(displayList);
  i->setIcon(icon11);
  i->setText("Splitter");
  i->setData(GroupRole,"Splitter");

  QIcon icon12;
  icon12.addFile(QString::fromUtf8(":/icon/pixmaps/groupbox.png"), QSize(), QIcon::Normal, QIcon::Off);
  i = new QListWidgetItem(containerList);
  i->setIcon(icon12);
  i->setText("Group box");
  i->setData(GroupRole,"GroupBox");

  QIcon icon13;
  icon13.addFile(QString::fromUtf8(":/icon/pixmaps/toolbox.png"), QSize(), QIcon::Normal, QIcon::Off);
  i = new QListWidgetItem(containerList);
  i->setIcon(icon13);
  i->setText("Tool box");
  i->setData(GroupRole,"ToolBox");

  QIcon icon14;
  icon14.addFile(QString::fromUtf8(":/icon/pixmaps/tabwidget.png"), QSize(), QIcon::Normal, QIcon::Off);
  i = new QListWidgetItem(containerList);
  i->setIcon(icon14);
  i->setText("Tab widget");
  i->setData(GroupRole,"TabWidget");

  QIcon icon15;
  icon15.addFile(QString::fromUtf8(":/icon/pixmaps/frame.png"), QSize(), QIcon::Normal, QIcon::Off);
  i = new QListWidgetItem(containerList);
  i->setIcon(icon15);
  i->setText("Frame");
  i->setData(GroupRole,"Frame");

  QIcon icon16;
  icon16.addFile(QString::fromUtf8(":/icon/pixmaps/dockwidget.png"), QSize(), QIcon::Normal, QIcon::Off);
  i = new QListWidgetItem(containerList);
  i->setIcon(icon16);
  i->setText("Dock widget");
  i->setData(GroupRole,"DockWidget");

  QIcon icon17;
  icon17.addFile(QString::fromUtf8(":/icon/pixmaps/toolbox.png"), QSize(), QIcon::Normal, QIcon::Off);
  i = new QListWidgetItem(containerList);
  //i->setIcon(icon17);
  i->setText("Tool bar");
  i->setData(GroupRole,"ToolBar");

  QIcon icon18;
  icon18.addFile(QString::fromUtf8(":/icon/pixmaps/menubar.png"), QSize(), QIcon::Normal, QIcon::Off);
  i = new QListWidgetItem(containerList);
  i->setIcon(icon18);
  i->setText("Menu bar item");
  i->setData(GroupRole,"MenuBarItem");

  QIcon icon19;
  icon19.addFile(QString::fromUtf8(":/icon/pixmaps/toolbox.png"), QSize(), QIcon::Normal, QIcon::Off);
  i = new QListWidgetItem(containerList);
  //i->setIcon(icon19);
  i->setText("Menu item");
  i->setData(GroupRole,"MenuItem");

  QIcon icon20;
  icon20.addFile(QString::fromUtf8(":/icon/pixmaps/toolbox.png"), QSize(), QIcon::Normal, QIcon::Off);
  i = new QListWidgetItem(miscList);
  //i->setIcon(icon20);
  i->setText("Metrics");
  //i->setData(GroupRole,"ToolBox");

  // Adjust toolBox size
  // Shitty QListWidget size policies do not work
  int maxW = buttonList->sizeHintForColumn(0);
  maxW = qMax(maxW,inputList->sizeHintForColumn(0));
  maxW = qMax(maxW,displayList->sizeHintForColumn(0));
  maxW = qMax(maxW,containerList->sizeHintForColumn(0));
  maxW = qMax(maxW,miscList->sizeHintForColumn(0));
  maxW += 30; // be comfortable

  buttonList->setFixedWidth(maxW);
  inputList->setFixedWidth(maxW);
  displayList->setFixedWidth(maxW);
  containerList->setFixedWidth(maxW);
  miscList->setFixedWidth(maxW);

  toolBox->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Expanding);

  // add tree widgets after window creation to get minimal window size
  drawStackTree = new QTreeWidget(dbgTab);
  drawStackTree->setObjectName(QString::fromUtf8("drawStackTree"));
  drawStackTree->setAlternatingRowColors(true);
  drawStackTree->headerItem()->setText(0,"Function");
  drawStackTree->headerItem()->setText(1,"Args");
  dbgLayout->addWidget(drawStackTree, 3, 0, 1, 1);

  resolvedValuesTree = new QTreeWidget(dbgTab);
  resolvedValuesTree->setObjectName(QString::fromUtf8("resolvedValuesTree"));
  resolvedValuesTree->headerItem()->setText(0,"Name");
  resolvedValuesTree->headerItem()->setText(1,"File value");
  resolvedValuesTree->headerItem()->setText(2,"Resolved value");
  dbgLayout->addWidget(resolvedValuesTree, 5, 0, 1, 1);

  drawStackTree->header()->setResizeMode(0,QHeaderView::ResizeToContents);
  resolvedValuesTree->header()->setResizeMode(QHeaderView::ResizeToContents);

  // insert appropriate widget into status bar
  statusbarLbl1 = new QLabel(this);
  statusbarLbl2 = new QLabel(this);
  statusBar()->insertWidget(0,statusbarLbl1);
  statusBar()->insertWidget(1,statusbarLbl2);

  // Enable/disable some widgets
  tabWidget->setTabEnabled(1,false);
  tabWidget2->setTabEnabled(0,false);
  tabWidget2->setTabEnabled(1,false);
  tabWidget2->setTabEnabled(2,false);

  // connections
  connect(quitBtn,SIGNAL(clicked()),this,SLOT(close()));

  // set minimal and sufficient window size
  resize(minimumSizeHint());
}

ThemeBuilderUI::~ThemeBuilderUI()
{
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
