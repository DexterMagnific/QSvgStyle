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
#include "ThemeBuilderUI.h"

#include <stdlib.h> // mkstemp
#include <unistd.h> // close

#include <QDebug>
#include <QStandardItemModel>
#include <QMetaObject>

// UI
#include <QFile>
#include <QListWidget>
#include <QFileDialog>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QFileInfo>
#include <QStyleFactory>
#include <QMessageBox>
#include <QTimer>

// Includes for preview
#include <QPushButton>
#include <QToolButton>
#include <QMenu>
#include <QRadioButton>
#include <QCheckBox>
#include <QScrollBar>
#include <QProgressBar>
#include <QGroupBox>
#include <QLayout>
#include <QDockWidget>
#include <QToolBar>
#include <QMenuBar>
#include <QToolTip>
#include <QMenu>
#include <QListView>

#include "NewThemeUI.h"
#include "../style/ThemeConfig.h"
#include "../style/QSvgStyle.h"
#include "../common/groups.h"

ThemeBuilderUI::ThemeBuilderUI(QWidget* parent)
 : QMainWindow(parent), config(0), style(0), previewWidget(0),
   currentWidget(0),
   currentDrawStackItem(0), currentDrawMode(0), currentPreviewVariant(0),
   cfgModified(0), previewUpdateEnabled(false),
   timer(0), timer2(0), newThemeDlg(0),
   svgWatcher(this)
{
  // Setup using auto-generated UIC code
  setupUi(this);

  // populate widget tree views
  QListWidgetItem *i;

  // Use xx_group() functions from QSvgStyle class to set group role
  QIcon icon1;
  icon1.addFile(QString::fromUtf8(":/icon/pixmaps/pushbutton.png"), QSize(), QIcon::Normal, QIcon::Off);
  i = new QListWidgetItem(buttonList);
  i->setIcon(icon1);
  i->setText("Push button");
  i->setData(GroupRole,CE_group(QStyle::CE_PushButton));

  QIcon icon2;
  icon2.addFile(QString::fromUtf8(":/icon/pixmaps/toolbutton.png"), QSize(), QIcon::Normal, QIcon::Off);
  i = new QListWidgetItem(buttonList);
  i->setIcon(icon2);
  i->setText("Tool button");
  i->setData(GroupRole,CC_group(QStyle::CC_ToolButton));

  QIcon icon3;
  icon3.addFile(QString::fromUtf8(":/icon/pixmaps/radiobutton.png"), QSize(), QIcon::Normal, QIcon::Off);
  i = new QListWidgetItem(buttonList);
  i->setIcon(icon3);
  i->setText("Radio button");
  i->setData(GroupRole,CE_group(QStyle::CE_RadioButton));

  QIcon icon4;
  icon4.addFile(QString::fromUtf8(":/icon/pixmaps/checkbox.png"), QSize(), QIcon::Normal, QIcon::Off);
  i = new QListWidgetItem(buttonList);
  i->setIcon(icon4);
  i->setText("Check box");
  i->setData(GroupRole,CE_group(QStyle::CE_CheckBox));

  QIcon icon5;
  icon5.addFile(QString::fromUtf8(":/icon/pixmaps/lineedit.png"), QSize(), QIcon::Normal, QIcon::Off);
  i = new QListWidgetItem(inputList);
  i->setIcon(icon5);
  i->setText("Line edit");
  i->setData(GroupRole,PE_group(QStyle::PE_FrameLineEdit));

  QIcon icon6;
  icon6.addFile(QString::fromUtf8(":/icon/pixmaps/spinbox.png"), QSize(), QIcon::Normal, QIcon::Off);
  i = new QListWidgetItem(inputList);
  i->setIcon(icon6);
  i->setText("Spin box");
  i->setData(GroupRole,CC_group(QStyle::CC_SpinBox));

  QIcon icon7;
  icon7.addFile(QString::fromUtf8(":/icon/pixmaps/vscrollbar.png"), QSize(), QIcon::Normal, QIcon::Off);
  i = new QListWidgetItem(inputList);
  i->setIcon(icon7);
  i->setText("Scroll bar");
  i->setData(GroupRole,CC_group(QStyle::CC_ScrollBar));

  QIcon icon8;
  icon8.addFile(QString::fromUtf8(":/icon/pixmaps/hslider.png"), QSize(), QIcon::Normal, QIcon::Off);
  i = new QListWidgetItem(inputList);
  i->setIcon(icon8);
  i->setText("Slider");
  i->setData(GroupRole,CC_group(QStyle::CC_Slider));

//   QIcon icon9;
//   icon9.addFile(QString::fromUtf8(":/icon/pixmaps/dial.png"), QSize(), QIcon::Normal, QIcon::Off);
//   i = new QListWidgetItem(inputList);
//   i->setIcon(icon9);
//   i->setText("Dial");
//   i->setData(GroupRole,CC_group(QStyle::CC_Dial));

  QIcon icon10;
  icon10.addFile(QString::fromUtf8(":/icon/pixmaps/progress.png"), QSize(), QIcon::Normal, QIcon::Off);
  i = new QListWidgetItem(displayList);
  i->setIcon(icon10);
  i->setText("Progress bar");
  i->setData(GroupRole,CE_group(QStyle::CE_ProgressBar));

  QIcon icon11;
  icon11.addFile(QString::fromUtf8(":/icon/pixmaps/edithlayout.png"), QSize(), QIcon::Normal, QIcon::Off);
  i = new QListWidgetItem(displayList);
  i->setIcon(icon11);
  i->setText("Splitter");
  i->setData(GroupRole,CE_group(QStyle::CE_Splitter));

  QIcon icon111;
  //icon111.addFile(QString::fromUtf8(":/icon/pixmaps/edithlayout.png"), QSize(), QIcon::Normal, QIcon::Off);
  i = new QListWidgetItem(displayList);
  i->setIcon(icon111);
  i->setText("Tooltip");
  i->setData(GroupRole,PE_group(QStyle::PE_PanelTipLabel));

  QIcon icon112;
  //icon112.addFile(QString::fromUtf8(":/icon/pixmaps/edithlayout.png"), QSize(), QIcon::Normal, QIcon::Off);
  i = new QListWidgetItem(displayList);
  i->setIcon(icon112);
  i->setText("Header");
  i->setData(GroupRole,CE_group(QStyle::CE_Header));

  QIcon icon113;
  //icon112.addFile(QString::fromUtf8(":/icon/pixmaps/edithlayout.png"), QSize(), QIcon::Normal, QIcon::Off);
  i = new QListWidgetItem(displayList);
  i->setIcon(icon113);
  i->setText("View item");
  i->setData(GroupRole,PE_group(QStyle::PE_PanelItemViewItem));

  QIcon icon12;
  icon12.addFile(QString::fromUtf8(":/icon/pixmaps/groupbox.png"), QSize(), QIcon::Normal, QIcon::Off);
  i = new QListWidgetItem(containerList);
  i->setIcon(icon12);
  i->setText("Group box");
  i->setData(GroupRole,CC_group(QStyle::CC_GroupBox));

  QIcon icon13;
  icon13.addFile(QString::fromUtf8(":/icon/pixmaps/toolbox.png"), QSize(), QIcon::Normal, QIcon::Off);
  i = new QListWidgetItem(containerList);
  i->setIcon(icon13);
  i->setText("Tool box");
  i->setData(GroupRole,CE_group(QStyle::CE_ToolBoxTab));

  QIcon icon14;
  icon14.addFile(QString::fromUtf8(":/icon/pixmaps/tabwidget.png"), QSize(), QIcon::Normal, QIcon::Off);
  i = new QListWidgetItem(containerList);
  i->setIcon(icon14);
  i->setText("Tab widget");
  i->setData(GroupRole,CE_group(QStyle::CE_TabBarTab));

  QIcon icon15;
  icon15.addFile(QString::fromUtf8(":/icon/pixmaps/frame.png"), QSize(), QIcon::Normal, QIcon::Off);
  i = new QListWidgetItem(containerList);
  i->setIcon(icon15);
  i->setText("Frame");
  i->setData(GroupRole,PE_group(QStyle::PE_Frame));

  QIcon icon16;
  icon16.addFile(QString::fromUtf8(":/icon/pixmaps/dockwidget.png"), QSize(), QIcon::Normal, QIcon::Off);
  i = new QListWidgetItem(containerList);
  i->setIcon(icon16);
  i->setText("Dock widget");
  i->setData(GroupRole,CE_group(QStyle::CE_DockWidgetTitle));

  QIcon icon17;
  icon17.addFile(QString::fromUtf8(":/icon/pixmaps/toolbox.png"), QSize(), QIcon::Normal, QIcon::Off);
  i = new QListWidgetItem(containerList);
  //i->setIcon(icon17);
  i->setText("Tool bar");
  i->setData(GroupRole,CE_group(QStyle::CE_ToolBar));

  QIcon icon18;
  icon18.addFile(QString::fromUtf8(":/icon/pixmaps/menubar.png"), QSize(), QIcon::Normal, QIcon::Off);
  i = new QListWidgetItem(containerList);
  i->setIcon(icon18);
  i->setText("Menu bar item");
  i->setData(GroupRole,CE_group(QStyle::CE_MenuBarItem));

  QIcon icon19;
  icon19.addFile(QString::fromUtf8(":/icon/pixmaps/toolbox.png"), QSize(), QIcon::Normal, QIcon::Off);
  i = new QListWidgetItem(containerList);
  //i->setIcon(icon19);
  i->setText("Menu item");
  i->setData(GroupRole,CE_group(QStyle::CE_MenuItem));

  QIcon icon20;
  icon20.addFile(QString::fromUtf8(":/icon/pixmaps/righttoleft.png"), QSize(), QIcon::Normal, QIcon::Off);
  i = new QListWidgetItem(miscList);
  i->setIcon(icon20);
  i->setText("Indicators");
  i->setData(GroupRole,PE_group(QStyle::PE_IndicatorArrowDown));

  QIcon icon21;
  icon21.addFile(QString::fromUtf8(":/icon/pixmaps/adjustsize.png"), QSize(), QIcon::Normal, QIcon::Off);
  i = new QListWidgetItem(miscList);
  i->setIcon(icon21);
  i->setText("Size grip");
  i->setData(GroupRole,CE_group(QStyle::CE_SizeGrip));

//   QIcon icon22;
//   icon22.addFile(QString::fromUtf8(":/icon/pixmaps/optimize.png"), QSize(), QIcon::Normal, QIcon::Off);
//   i = new QListWidgetItem(miscList);
//   i->setIcon(icon21);
//   i->setText("Metrics");
  //i->setData(GroupRole,"ToolBox");

  // Menu for recent files
  recentFiles = new QMenu("Recent files", recentBtn);
  recentBtn->setMenu(recentFiles);

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

  // also populate inherit combo box
  foreach(QListWidgetItem *i, buttonList->findItems("*",Qt::MatchWildcard)) {
    inheritCombo->addItem(i->icon(),i->text(),i->data(GroupRole));
  }
  foreach(QListWidgetItem *i, inputList->findItems("*",Qt::MatchWildcard)) {
    inheritCombo->addItem(i->icon(),i->text(),i->data(GroupRole));
  }
  foreach(QListWidgetItem *i, displayList->findItems("*",Qt::MatchWildcard)) {
    inheritCombo->addItem(i->icon(),i->text(),i->data(GroupRole));
  }
  foreach(QListWidgetItem *i, containerList->findItems("*",Qt::MatchWildcard)) {
    inheritCombo->addItem(i->icon(),i->text(),i->data(GroupRole));
  }

  // add tree widgets after window creation to get minimal window size
  drawStackTree = new QTreeWidget(previewTab);
  drawStackTree->setObjectName(QString::fromUtf8("drawStackTree"));
  drawStackTree->setAlternatingRowColors(true);
  drawStackTree->headerItem()->setText(0,"Function");
  drawStackTree->headerItem()->setText(1,"Args");
  previewLayout->addWidget(drawStackTree, 3, 0, 1, 1);

//   resolvedValuesTree = new QTreeWidget(previewTab);
//   resolvedValuesTree->setObjectName(QString::fromUtf8("resolvedValuesTree"));
//   resolvedValuesTree->headerItem()->setText(0,"Name");
//   resolvedValuesTree->headerItem()->setText(1,"File value");
//   resolvedValuesTree->headerItem()->setText(2,"Resolved value");
//   previewLayout->addWidget(resolvedValuesTree, 5, 0, 1, 1);

  drawStackTree->header()->setResizeMode(0,QHeaderView::ResizeToContents);
//   resolvedValuesTree->header()->setResizeMode(QHeaderView::ResizeToContents);

  // insert appropriate widget into status bar
  statusbarLbl1 = new QLabel(this);
  statusbarLbl2 = new QLabel(this);
  statusBar()->insertWidget(0,statusbarLbl1);
  statusBar()->insertWidget(1,statusbarLbl2);

  // setup font size spin
  fontSizeSpin->setValue(font().pointSize());

  // Get an instance of QSvgStyle
  if ( !style ) {
    style = (QSvgStyle *) QStyleFactory::create("QSvgStyle");
    if ( !style ) {
      qWarning() << "[QSvgThemeBuilder]" << "Could not load QSvgStyle style, preview will not be available !";
      QMessageBox::warning(this,"QSvgStyle style not found",
                           "QSvgStyle style library could not be loaded. Maybe it is"
                           " not installed in the right directory.\n"
                           " Preview will not be available.");
    }
    // Check that the style version is the same as the one we were compiled against
    if ( style->Version /* dynamic load */ != QSvgStyle::Version /* compiled .h */) {
      qWarning() << "[QSvgThemeBuilder]" << "QSvgStyle version mismatch !";
      qWarning() << "[QSvgThemeBuilder]" << "QStyleFactory reported version" << style->Version;
      qWarning() << "[QSvgThemeBuilder]" << "ThemeBuilder was built with version" << QSvgStyle::Version;
      QMessageBox::warning(this,"QSvgStyle version mismatch",
                           "The version of the installed QSvgStyle style is not"
                           " the same as the one known by QSvgThemeBuilder.\n"
                           " You may experience crashes when previewing.\n");
    }
  }

  // install event filter on previewArea so that we can react to resize and
  // close events and save its geometry
  previewArea->installEventFilter(this);

  // connections
  // main buttons
  connect(quitBtn,SIGNAL(clicked()),this,SLOT(slot_quit()));
  connect(newBtn,SIGNAL(clicked()), this,SLOT(slot_newTheme()));
  connect(openBtn,SIGNAL(clicked()), this,SLOT(slot_openTheme()));
  connect(saveBtn,SIGNAL(clicked()), this,SLOT(slot_saveTheme()));
  connect(saveAsBtn,SIGNAL(clicked()), this,SLOT(slot_saveAsTheme()));

  // Change of selected widget to edit
  connect(buttonList,SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)),
          this,SLOT(slot_widgetChanged(QListWidgetItem*,QListWidgetItem*)));
  connect(inputList,SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)),
          this,SLOT(slot_widgetChanged(QListWidgetItem*,QListWidgetItem*)));
  connect(displayList,SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)),
          this,SLOT(slot_widgetChanged(QListWidgetItem*,QListWidgetItem*)));
  connect(containerList,SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)),
          this,SLOT(slot_widgetChanged(QListWidgetItem*,QListWidgetItem*)));
  connect(miscList,SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)),
          this,SLOT(slot_widgetChanged(QListWidgetItem*,QListWidgetItem*)));
  connect(toolBox,SIGNAL(currentChanged(int)),
          this,SLOT(slot_toolboxTabChanged(int)));

  // Changes inside properties tab
  connect(authorEdit,SIGNAL(textEdited(QString)),
          this,SLOT(slot_authorEditChanged(QString)));
  connect(descrEdit,SIGNAL(textEdited(QString)),
          this,SLOT(slot_descrEditChanged(QString)));
  connect(themeNameEdit,SIGNAL(textEdited(QString)),
          this,SLOT(slot_themeNameEditChanged(QString)));

  // Changes inside the common tab
  connect(inheritCb,SIGNAL(stateChanged(int)),
          this,SLOT(slot_inheritCbChanged(int)));
  connect(inheritCombo,SIGNAL(currentIndexChanged(int)),
          this,SLOT(slot_inheritComboChanged(int)));

  connect(frameCb,SIGNAL(stateChanged(int)),
          this,SLOT(slot_frameCbChanged(int)));
  connect(frameIdCb,SIGNAL(stateChanged(int)),
          this,SLOT(slot_frameIdCbChanged(int)));
  connect(frameIdCombo,SIGNAL(editTextChanged(QString)),
          this,SLOT(slot_frameIdComboChanged(QString)));
  connect(frameWidthCb,SIGNAL(stateChanged(int)),
          this,SLOT(slot_frameWidthCbChanged(int)));
  connect(frameWidthSpin,SIGNAL(valueChanged(int)),
          this,SLOT(slot_frameWidthSpinChanged(int)));

  connect(interiorCb,SIGNAL(stateChanged(int)),
          this,SLOT(slot_interiorCbChanged(int)));
  connect(interiorIdCb,SIGNAL(stateChanged(int)),
          this,SLOT(slot_interiorIdCbChanged(int)));
  connect(interiorIdCombo,SIGNAL(editTextChanged(QString)),
          this,SLOT(slot_interiorIdComboChanged(QString)));
  connect(interiorRepeatCb,SIGNAL(stateChanged(int)),
          this,SLOT(slot_interiorRepeatCbChanged(int)));
  connect(interiorRepeatXSpin,SIGNAL(valueChanged(int)),
          this,SLOT(slot_interiorRepeatXSpinChanged(int)));
  connect(interiorRepeatYSpin,SIGNAL(valueChanged(int)),
          this,SLOT(slot_interiorRepeatYSpinChanged(int)));

  connect(indicatorIdCb,SIGNAL(stateChanged(int)),
          this,SLOT(slot_indicatorIdCbChanged(int)));
  connect(indicatorIdCombo,SIGNAL(editTextChanged(QString)),
          this,SLOT(slot_indicatorIdComboChanged(QString)));

  connect(labelSpacingCb,SIGNAL(stateChanged(int)),
          this,SLOT(slot_labelSpacingCbChanged(int)));
  connect(labelSpacingSpin,SIGNAL(valueChanged(int)),
          this,SLOT(slot_labelSpacingSpinChanged(int)));
  connect(labelMarginCb,SIGNAL(stateChanged(int)),
          this,SLOT(slot_labelMarginCbChanged(int)));
  connect(labelMarginHSpin,SIGNAL(valueChanged(int)),
          this,SLOT(slot_labelMarginHSpinChanged(int)));
  connect(labelMarginVSpin,SIGNAL(valueChanged(int)),
          this,SLOT(slot_labelMarginVSpinChanged(int)));

  // Preview tab
  connect(repaintBtn,SIGNAL(clicked(bool)),
          this,SLOT(slot_repaintBtnClicked(bool)));
  connect(rtlBtn,SIGNAL(clicked(bool)),
          this,SLOT(slot_rtlBtnClicked(bool)));
  connect(drawModeBtn,SIGNAL(clicked(bool)),
          this,SLOT(slot_drawModeBtnClicked(bool)));
  connect(detachBtn,SIGNAL(clicked(bool)),
          this,SLOT(slot_detachBtnClicked(bool)));
  connect(fontSizeSpin,SIGNAL(valueChanged(int)),
          this,SLOT(slot_fontSizeChanged(int)));
  connect(enableBtn,SIGNAL(clicked(bool)),
          this,SLOT(slot_enableBtnClicked(bool)));
  connect(previewVariantBtn,SIGNAL(clicked(bool)),
          this,SLOT(slot_previewVariantBtnClicked(bool)));

  // style callbacks
  if ( style ) {
    // NOTE connect/disconnect both begin and end
    connect(style,SIGNAL(sig_drawPrimitive_begin(QString)),
            this,SLOT(slot_drawPrimitive_begin(QString)));
    connect(style,SIGNAL(sig_drawPrimitive_end(QString)),
            this,SLOT(slot_drawPrimitive_end(QString)));

    connect(style,SIGNAL(sig_drawControl_begin(QString)),
            this,SLOT(slot_drawControl_begin(QString)));
    connect(style,SIGNAL(sig_drawControl_end(QString)),
            this,SLOT(slot_drawControl_end(QString)));

    connect(style,SIGNAL(sig_drawComplexControl_begin(QString)),
            this,SLOT(slot_drawComplexControl_begin(QString)));
    connect(style,SIGNAL(sig_drawComplexControl_end(QString)),
            this,SLOT(slot_drawComplexControl_end(QString)));

    connect(style,SIGNAL(sig_renderFrame_begin(QString)),
            this,SLOT(slot_renderFrame_begin(QString)));
    connect(style,SIGNAL(sig_renderFrame_end(QString)),
            this,SLOT(slot_renderFrame_end(QString)));

    connect(style,SIGNAL(sig_renderInterior_begin(QString)),
            this,SLOT(slot_renderInterior_begin(QString)));
    connect(style,SIGNAL(sig_renderInterior_end(QString)),
            this,SLOT(slot_renderInterior_end(QString)));

    connect(style,SIGNAL(sig_renderLabel_begin(QString)),
            this,SLOT(slot_renderLabel_begin(QString)));
    connect(style,SIGNAL(sig_renderLabel_end(QString)),
            this,SLOT(slot_renderLabel_end(QString)));

    connect(style,SIGNAL(sig_renderIndicator_begin(QString)),
            this,SLOT(slot_renderIndicator_begin(QString)));
    connect(style,SIGNAL(sig_renderIndicator_end(QString)),
            this,SLOT(slot_renderIndicator_end(QString)));

    connect(style,SIGNAL(sig_renderElement_begin(QString)),
            this,SLOT(slot_renderElement_begin(QString)));
    connect(style,SIGNAL(sig_renderElement_end(QString)),
            this,SLOT(slot_renderElement_end(QString)));

    connect(style,SIGNAL(sig_sizeFromContents_begin(QString)),
            this,SLOT(slot_sizeFromContents_begin(QString)));
    connect(style,SIGNAL(sig_sizeFromContents_end(QString)),
            this,SLOT(slot_sizeFromContents_end(QString)));
  }

  // Timer for previewed widget repaints
  timer = new QTimer(this);
  connect(timer,SIGNAL(timeout()), this,SLOT(slot_uiSettingsChanged()));
  // Timer for SVG file reload
  timer2 = new QTimer(this);
  connect(timer2,SIGNAL(timeout()), this,SLOT(slot_reloadSvgFile()));

  // Watch SVG file changes
  connect(&svgWatcher,SIGNAL(fileChanged(QString)),
          this,SLOT(slot_svgFileChanged(QString)));

  // Reset UI
  resetUi();

  // TESTING remove me in release
  //aboutFrame->hide();
  //toolBox->setEnabled(true);
  // TESTING end

  // set minimal and sufficient window size
  resize(minimumSizeHint());
}

ThemeBuilderUI::~ThemeBuilderUI()
{
  if ( config )
    delete config;

  if ( style )
    delete style;
}

bool ThemeBuilderUI::eventFilter(QObject* o, QEvent* e)
{
  // previewArea
  if ( (o == previewArea) && previewWidget && (e->type() == QEvent::Resize) ) {
    // save detached preview area geometry
    if ( previewArea->isTopLevel() ) {
      detachedPeviewGeometry = previewArea->geometry();
    }
  }

  if ( (o == previewArea) && (e->type() == QEvent::Close) ) {
    // detached preview area closed -> re-attach to main window
    slot_detachBtnClicked(false);
  }

  if ( (o == previewWidget) && previewWidget &&
       ( (e->type() == QEvent::Paint) || (e->type() == QEvent::Destroy)) ) {
    // previewWidget is about to be repainted or destroyed -> clear drawStackTree table
    clearDrawStackTree();
  }

  // always process the event
  return QObject::eventFilter(o, e);
}

void ThemeBuilderUI::clearDrawStackTree()
{
  foreach(QTreeWidgetItem *i, drawStackTree->findItems("*",Qt::MatchWildcard)) {
    delete i;
  }
  currentDrawStackItem = 0;
}

void ThemeBuilderUI::setStyleForWidgetAndChildren(QStyle* style, QWidget* w)
{
  if ( !style || !w )
    return;

  w->setStyle(style);

  // iterate through children
  foreach(QObject *o, w->children()) {
    // lookup for immediate children
    QWidget *c = qobject_cast< QWidget* >(o);
    if ( c ) {
      c->setStyle(style);
    }

    // layout of level 1: iterate through layout items and set style
    // NOTE we don't go beyond level 1 layouts
    QLayout *l = qobject_cast< QLayout* >(o);
    if ( l ) {
      for (int i=0; i<l->count(); ++i) {
        QWidget *lc = l->itemAt(i)->widget(); // child widget inside layout
        if ( lc )
          lc->setStyle(style);
      }
    }
  }
}

void ThemeBuilderUI::noteStyleOperation_begin(const QString& op, const QString& args)
{
  QTreeWidgetItem *i = new QTreeWidgetItem(currentDrawStackItem);
  i->setText(0,op);
  i->setText(1,args);
  if ( !currentDrawStackItem )
    drawStackTree->addTopLevelItem(i);
  else
    currentDrawStackItem->setExpanded(true);

  currentDrawStackItem = i;
}

void ThemeBuilderUI::noteStyleOperation_end(const QString& op, const QString& args)
{
  Q_UNUSED(op);
  Q_UNUSED(args);

  if ( currentDrawStackItem )
    currentDrawStackItem = currentDrawStackItem->parent();
}

void ThemeBuilderUI::resetUi()
{
  // Enable/disable some widgets
  toolBox->setEnabled(false);
  tabWidget->setTabEnabled(1,false);
  tabWidget2->setTabEnabled(0,false);
  tabWidget2->setTabEnabled(1,false);
  tabWidget2->setCurrentIndex(2);
  saveBtn->setEnabled(false);
  saveAsBtn->setEnabled(false);

  // clear theme file name
  themeNameLbl->clear();
  themeNameLbl->setToolTip(QString());

  // clear common settings
  inheritCb->setCheckState(Qt::Unchecked);
  inheritCombo->setCurrentIndex(-1);
  frameCb->setCheckState(Qt::PartiallyChecked);
  frameIdCb->setCheckState(Qt::PartiallyChecked);
  frameWidthCb->setCheckState(Qt::PartiallyChecked);
  interiorCb->setCheckState(Qt::PartiallyChecked);
  interiorIdCb->setCheckState(Qt::PartiallyChecked);
  interiorRepeatCb->setCheckState(Qt::PartiallyChecked);
  labelSpacingCb->setCheckState(Qt::PartiallyChecked);
  labelMarginCb->setCheckState(Qt::PartiallyChecked);

  currentToolboxTab = toolBox->currentIndex();

  // Reset preview tab stuff
  clearDrawStackTree();
  currentDrawMode = 0;
  currentPreviewVariant = 0;
  if ( previewWidget )
    delete previewWidget;
  previewWidget = 0;
  currentWidget = 0;
  previewUpdateEnabled = false;

  // Reset state
  if ( config ) {
    delete config;
    config = 0;
  }
  QStringList l = svgWatcher.files();
  if ( !l.empty() )
    svgWatcher.removePaths(l);
  cfgModified = false;
  cfgFile.clear();
  tempCfgFile.clear();
  svgFile.clear();
}

bool ThemeBuilderUI::ensureSettingsSaved()
{
  if ( !cfgModified || !config )
    return true;

  QMessageBox::StandardButton b = QMessageBox::warning(this,"Save current theme ?",
                                         "Current theme has been modified. Do you"
                                         " want to save it ?",
                                         QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);

  if ( (b == QMessageBox::Cancel) || (b == QMessageBox::Escape) )
    return false;

  if ( b == QMessageBox::Yes )
    slot_saveTheme();

  return true;
}

void ThemeBuilderUI::schedulePreviewUpdate()
{
  timer->stop();

  if ( !previewUpdateEnabled )
    return;

  timer->start(500);
}

void ThemeBuilderUI::openTheme(const QString& filename)
{
  char tmp[256] = "qsvgthemebuilder_XXXXXX";

  resetUi();

  // Get a unique temp filename
  int fd;
  if ( (fd = mkstemp(tmp)) == -1 ) {
    qWarning() << "[QSvgThemeBuilder]" << "Could not generate a temporary file name";
    return;
  }

  ::close(fd);
  unlink(tmp);
  tempCfgFile = QDir::tempPath()+"/"+tmp;

  if ( !QFile::copy(filename,tempCfgFile) ) {
    qWarning() << "[QSvgThemeBuilder]" << "Could not create temporary file";
    return;
  } else {
    qDebug() << "[QSvgThemeBuilder]" << "Temporary file" << tempCfgFile << "created";
    // BUG QFile::copy does not obey umask. Add write permission for self
    QFile::setPermissions(tempCfgFile,QFile::permissions(tempCfgFile) | QFile::WriteOwner);
  }

  cfgFile = filename;

  config = new ThemeConfig(tempCfgFile);

  svgFile = QFileInfo(cfgFile).absoluteDir().path()+"/"+QFileInfo(cfgFile).baseName()+".svg";

  if ( QFile::exists(svgFile) ) {
    qDebug() << "[QSvgThemeBuilder]" << "Matching SVG file" << svgFile << "found";
  } else
    svgFile.clear();

  if ( style && !svgFile.isEmpty() ) {
    // NOTE Thanks to Qt, ThemeBuilder can modify this file safely while the style
    // uses it. Changes made by ThemeBuilder will be seen by the style immediately
    // even if the style and ThemeBuilder use different QSettings objects for
    // the same file location

    // NOTE use invokeMethod, this allows us to not link against libqsvgstyle.so
    //style->loadCustomThemeConfig(tempCfgFile);
    QStyle::staticMetaObject.invokeMethod(style,"loadCustomThemeConfig",
                                          Qt::DirectConnection,
                                          Q_ARG(QString,tempCfgFile));
    //style->loadCustomSVG(svgFile);
    QStyle::staticMetaObject.invokeMethod(style,"loadCustomSVG",
                                          Qt::DirectConnection,
                                          Q_ARG(QString,svgFile));

    svgWatcher.addPath(svgFile);
  }

  if ( svgFile.isEmpty() )
    QMessageBox::warning(this, "Preview feature",
                         "The matching SVG file for this config"
                         " is missing in this directory. Preview will not be available");

    recentFiles->addAction(cfgFile);
  recentBtn->setEnabled(true);

  toolBox->setEnabled(true);
  tabWidget2->setTabEnabled(0,true);
  tabWidget2->setTabEnabled(1,true);
  tabWidget2->setCurrentIndex(0);
  themeNameLbl->setText(QFileInfo(cfgFile).fileName());
  themeNameLbl->setToolTip(cfgFile);

  // fill in properties form
  theme_spec_t ts = config->getThemeSpec();

  // properties tab
  themeNameEdit->setText(ts.name);
  authorEdit->setText(ts.author);
  descrEdit->setText(ts.descr);

  slot_toolboxTabChanged(toolBox->currentIndex());
}

void ThemeBuilderUI::slot_newTheme()
{
  int res = QDialog::Rejected;
  QString dirname,basename;

  if ( !newThemeDlg ) {
    newThemeDlg = new NewThemeUI(NULL);
  }

restart:
  res = newThemeDlg->exec();

  if ( res == QDialog::Rejected )
    return;

  dirname = newThemeDlg->directory()+"/"+newThemeDlg->themeBaseFilename();
  basename = dirname+"/"+newThemeDlg->themeBaseFilename();

  if ( QFile::exists(basename+".cfg") || QFile::exists(basename+".svg") ) {
    QMessageBox::warning(this,"Theme exists",
                         "This theme already exists. Please choose another name",
                         QMessageBox::Ok);
    goto restart;
  }

  if ( !QDir().mkpath(dirname) ) {
    QMessageBox::critical(this, "Error", "Could not create directory\n"+dirname,
                          QMessageBox::Ok);
    goto restart;
  }

  qDebug() << "[QSvgThemeBuilder]" << "creating" << basename+".cfg" << "based on default theme";

  if ( !QFile::copy(":default.cfg",basename+".cfg") ) {
    QMessageBox::critical(this, "Error", "Could not create file\n"+basename+".cfg",
                          QMessageBox::Ok);
    return;
  }

  qDebug() << "[QSvgThemeBuilder]" << "creating" << basename+".svg" << "based on default theme";

  if ( !QFile::copy(":default.svg",basename+".svg") ) {
    QMessageBox::critical(this, "Error", "Could not create file\n"+basename+".svg",
                          QMessageBox::Ok);
    return;
  }

  // BUG set owner write permissions
  QFile::setPermissions(basename+".cfg",QFile::permissions(basename+".cfg") | QFile::WriteOwner);
  QFile::setPermissions(basename+".svg",QFile::permissions(basename+".svg") | QFile::WriteOwner);

  openTheme(basename+".cfg");
}

void ThemeBuilderUI::slot_openTheme()
{
  QString s = QFileDialog::getOpenFileName(NULL,"Load Theme","",
                                           "QSvgStyle configuration files(*.cfg);;All files(*)");
  if ( s.isNull() )
    return;

  if ( !ensureSettingsSaved() )
    return;

  openTheme(s);
}

void ThemeBuilderUI::slot_saveAsTheme()
{
}

void ThemeBuilderUI::slot_saveTheme()
{
  if ( !config || !cfgModified )
    return;

  // We normally don't need this
  saveSettingsFromUi(currentWidget);

  config->sync();

  if ( !QFile::remove(cfgFile) ) {
    qWarning() << "[QSvgThemeBuilder]" << "Could not remove" + cfgFile;
    return;
  }

  if ( !QFile::copy(tempCfgFile,cfgFile) ) {
    qWarning() << "[QSvgThemeBuilder]" << "Could not save" + cfgFile;
    return;
  }

  qDebug() << "[QSvgThemeBuilder]" << cfgFile << "saved";

  cfgModified = false;
  saveBtn->setEnabled(false);
}

void ThemeBuilderUI::slot_quit()
{
  if ( !ensureSettingsSaved() )
    return;

  QApplication::closeAllWindows();
}

void ThemeBuilderUI::slot_svgFileChanged(const QString& filename)
{
  Q_UNUSED(filename);

  timer2->stop();
  timer2->start(500);
}

void ThemeBuilderUI::slot_reloadSvgFile()
{
  timer2->stop();

  if ( style && !svgFile.isEmpty() ) {
    qDebug() << "[QSvgThemeBuilder]" << "SVG file changed, reloading it";
    //style->loadCustomSVG(svgFile);
    QStyle::staticMetaObject.invokeMethod(style,"loadCustomSVG",
                                          Qt::DirectConnection,
                                          QGenericArgument(),
                                          Q_ARG(QString,svgFile));
    setupPreviewForWidget(currentWidget);
  }
}

void ThemeBuilderUI::setupUiForWidget(const QListWidgetItem* current)
{
  if ( !config )
    return;

  previewUpdateEnabled = false;

  if ( current ) {
    QString group = current->data(GroupRole).toString();

    tabWidget->setEnabled(true);

    // get spec as exactly set in the config file, without inheritance resolution
    raw_es = config->getRawElementSpec(group);
    // now get the resolved spec
    es = config->getElementSpec(group);

    // common tab
    // inherit cb and combo
    if ( raw_es.inherits.present ) {
      inheritCb->setCheckState(Qt::Checked);
    } else {
      inheritCb->setCheckState(Qt::Unchecked);
    }

    // frame stuff
    if ( raw_es.frame.hasFrame.present ) {
      frameCb->setChecked(raw_es.frame.hasFrame);
    } else {
      frameCb->setCheckState(Qt::PartiallyChecked);
    }

    if ( raw_es.frame.element.present ) {
      if ( QString(raw_es.frame.element).isEmpty() ) {
        frameIdCb->setCheckState(Qt::Unchecked);
      } else {
        frameIdCb->setCheckState(Qt::Checked);
      }
    } else {
      frameIdCb->setCheckState(Qt::PartiallyChecked);
    }

    if ( raw_es.frame.width.present ) {
      if ( QString("%1").arg(raw_es.frame.width).isEmpty() ) {
        frameWidthCb->setCheckState(Qt::Unchecked);
      } else {
        frameWidthCb->setCheckState(Qt::Checked);
      }
    } else {
      frameWidthCb->setCheckState(Qt::PartiallyChecked);
    }

    // interior stuff
    if ( raw_es.interior.hasInterior.present ) {
      interiorCb->setChecked(raw_es.interior.hasInterior);
    } else {
      interiorCb->setCheckState(Qt::PartiallyChecked);
    }

    if ( raw_es.interior.element.present ) {
      if ( QString(raw_es.interior.element).isEmpty() ) {
        interiorIdCb->setCheckState(Qt::Unchecked);
      } else {
        interiorIdCb->setCheckState(Qt::Checked);
      }
    } else {
      interiorIdCb->setCheckState(Qt::PartiallyChecked);
    }

    if ( raw_es.interior.px.present || raw_es.interior.py.present ) {
      interiorRepeatCb->setCheckState(Qt::Unchecked);
    } else {
      interiorRepeatCb->setCheckState(Qt::PartiallyChecked);
    }

    // label stuff
    if ( raw_es.label.tispace.present ) {
      labelSpacingCb->setCheckState(Qt::Checked);
    } else {
      labelSpacingCb->setCheckState(Qt::PartiallyChecked);
    }

    if ( raw_es.label.hmargin.present || raw_es.label.vmargin.present ) {
      labelMarginCb->setCheckState(Qt::Checked);
    } else {
      labelMarginCb->setCheckState(Qt::PartiallyChecked);
    }

    // indicator stuff
    if ( raw_es.indicator.element.present ) {
      if ( QString(raw_es.indicator.element).isEmpty() ) {
        indicatorIdCb->setCheckState(Qt::Unchecked);
      } else {
        indicatorIdCb->setCheckState(Qt::Checked);
      }
    } else {
      indicatorIdCb->setCheckState(Qt::PartiallyChecked);
    }

    // These are needed to force associated widget value updates even
    // if the check state has not changed when switching widgets
    slot_inheritCbChanged(inheritCb->checkState());
    slot_frameCbChanged(frameCb->checkState());
    slot_frameIdCbChanged(frameIdCb->checkState());
    slot_frameWidthCbChanged(frameWidthCb->checkState());
    slot_interiorCbChanged(interiorCb->checkState());
    slot_interiorIdCbChanged(interiorIdCb->checkState());
    slot_interiorRepeatCbChanged(interiorRepeatCb->checkState());
    slot_labelSpacingCbChanged(labelSpacingCb->checkState());
    slot_labelMarginCbChanged(labelMarginCb->checkState());
    slot_indicatorIdCbChanged(indicatorIdCb->checkState());
  } else {
    tabWidget->setEnabled(false);
  }

  previewUpdateEnabled = true;
}

void ThemeBuilderUI::setupPreviewForWidget(const QListWidgetItem *current)
{
  if ( previewWidget ) {
    delete previewWidget;
    previewWidget = 0;
    clearDrawStackTree();
  }

  // max variants that can be shown for this widget
  int variants = 1;
  // size policy for shown widget. By default, we do not occupy more room than we
  // should, thus the sizeHint() returned by the widget is a maximum
  QSizePolicy qsz = QSizePolicy(QSizePolicy::Maximum,QSizePolicy::Maximum);

  QIcon icon;
  QString group;

  if ( !current )
    goto end;

  if ( svgFile.isEmpty() )
    goto end;

  // prepare icon
  icon.addFile(QString::fromUtf8(":/icon/pixmaps/insertimage.png"), QSize(), QIcon::Normal, QIcon::Off);

  // get group
  group = current->data(GroupRole).toString();

  // NOTE strictly use the same XX_group(XXX) calls as the ones
  // used to setup the UI in the constructor

  // Push button
  if ( group == CE_group(QStyle::CE_PushButton) ) {
    variants = 4;

    QPushButton *widget = new QPushButton();

    switch (currentPreviewVariant % variants ) {
      case 0:
        widget->setText("This is a push button");
        widget->setIcon(icon);
        break;
      case 1:
        widget->setText("This is a push button");
        break;
      case 2:
        widget->setIcon(icon);
        break;
      case 3:
        break;
    }

    previewWidget = widget;
  }

  // Tool button
  if ( group == CC_group(QStyle::CC_ToolButton) ) {
    variants = 20;

    QToolButton *widget = new QToolButton();

    switch (currentPreviewVariant % variants ) {
      // Normal
      case 0:
        widget->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        widget->setText("text+icon");
        widget->setIcon(icon);
        break;
      case 1:
        widget->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        widget->setText("text under icon");
        widget->setIcon(icon);
        break;
      case 2:
        widget->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        widget->setText("text only");
        break;
      case 3:
        widget->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        widget->setIcon(icon);
        break;
      case 4:
        break;

      // with arrow
      case 5:
        widget->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        widget->setText("text+icon");
        widget->setIcon(icon);
        widget->setArrowType(Qt::UpArrow);
        break;
      case 6:
        widget->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        widget->setText("text under icon");
        widget->setIcon(icon);
        widget->setArrowType(Qt::UpArrow);
        break;
      case 7:
        widget->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        widget->setText("text only");
        widget->setArrowType(Qt::UpArrow);
        break;
      case 8:
        widget->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        widget->setIcon(icon);
        widget->setArrowType(Qt::UpArrow);
        break;
      case 9:
        widget->setArrowType(Qt::UpArrow);
        break;

      // with instant popup
      case 10: {
        widget->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        widget->setPopupMode(QToolButton::InstantPopup);
        widget->setText("text+icon");
        widget->setIcon(icon);
        QMenu *m = new QMenu("popup menu",widget);
        m->addAction("menu item");
        widget->setMenu(m);
      }
        break;
      case 11: {
        widget->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        widget->setPopupMode(QToolButton::InstantPopup);
        widget->setText("text under icon");
        widget->setIcon(icon);
        QMenu *m = new QMenu("popup menu",widget);
        m->addAction("menu item");
        widget->setMenu(m);
      }
        break;
      case 12: {
        widget->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        widget->setPopupMode(QToolButton::InstantPopup);
        widget->setText("text only");
        QMenu *m = new QMenu("popup menu",widget);
        m->addAction("menu item");
        widget->setMenu(m);
      }
        break;
      case 13: {
        widget->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        widget->setPopupMode(QToolButton::InstantPopup);
        widget->setIcon(icon);
        QMenu *m = new QMenu("popup menu",widget);
        m->addAction("menu item");
        widget->setMenu(m);
      }
        break;
      case 14: {
        widget->setPopupMode(QToolButton::InstantPopup);
        QMenu *m = new QMenu("popup menu",widget);
        m->addAction("menu item");
        widget->setMenu(m);
      }
        break;

      // with drop down menu
      case 15: {
        widget->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        widget->setPopupMode(QToolButton::MenuButtonPopup);
        widget->setText("text+icon");
        widget->setIcon(icon);
        QMenu *m = new QMenu("popup menu",widget);
        m->addAction("menu item");
        widget->setMenu(m);
      }
      break;
      case 16: {
        widget->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        widget->setPopupMode(QToolButton::MenuButtonPopup);
        widget->setText("text under icon");
        widget->setIcon(icon);
        QMenu *m = new QMenu("popup menu",widget);
        m->addAction("menu item");
        widget->setMenu(m);
      }
      break;
      case 17: {
        widget->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        widget->setPopupMode(QToolButton::MenuButtonPopup);
        widget->setText("text only");
        QMenu *m = new QMenu("popup menu",widget);
        m->addAction("menu item");
        widget->setMenu(m);
      }
      break;
      case 18: {
        widget->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        widget->setPopupMode(QToolButton::MenuButtonPopup);
        widget->setIcon(icon);
        QMenu *m = new QMenu("popup menu",widget);
        m->addAction("menu item");
        widget->setMenu(m);
      }
      break;
      case 19: {
        widget->setPopupMode(QToolButton::MenuButtonPopup);
        QMenu *m = new QMenu("popup menu",widget);
        m->addAction("menu item");
        widget->setMenu(m);
      }
      break;
    }

    previewWidget = widget;
  }

  if ( group == CE_group(QStyle::CE_RadioButton) ) {
    variants = 3;

    QRadioButton *widget = new QRadioButton();

    switch ( currentPreviewVariant % variants ) {
      case 0:
        widget->setText("This is a radio button");
        widget->setIcon(icon);
        break;
      case 1:
        widget->setText("This is a radio button");
        break;
      case 2:
        break;
    }

    previewWidget = widget;
  }

  if ( group == CE_group(QStyle::CE_CheckBox) ) {
    variants = 3;

    QCheckBox *widget = new QCheckBox();
    widget->setTristate(true);

    switch ( currentPreviewVariant % variants ) {
      case 0:
        widget->setText("This is a check box");
        widget->setIcon(icon);
        break;
      case 1:
        widget->setText("This is a check box");
        break;
      case 2:
        break;
    }

    previewWidget = widget;
  }

  if ( group == PE_group(QStyle::PE_FrameLineEdit) ) {
    variants = 1;

    QLineEdit *widget = new QLineEdit();
    widget->setText("This is a line edit");
    widget->setPlaceholderText("type some text here");

    previewWidget = widget;
  }

  if ( group == CC_group(QStyle::CC_SpinBox) ) {
    variants = 1;

    QSpinBox *widget = new QSpinBox();
    widget->setSuffix(" suffix");
    widget->setPrefix("prefix ");

    previewWidget = widget;
  }

  if ( group == CC_group(QStyle::CC_ScrollBar) ) {
    variants = 2;

    QScrollBar *widget = new QScrollBar();

    switch (currentPreviewVariant % variants) {
      case 0:
        widget->setOrientation(Qt::Horizontal);
        break;
      case 1:
        break;
    }

    previewWidget = widget;
    qsz = widget->sizePolicy();
  }

  if ( group == CC_group(QStyle::CC_Slider) ) {
    variants = 2;

    QSlider *widget = new QSlider();

    switch (currentPreviewVariant % variants) {
      case 0:
        widget->setOrientation(Qt::Horizontal);
        break;
      case 1:
        break;
    }

    previewWidget = widget;
    qsz = widget->sizePolicy();
  }

  if ( group == CE_group(QStyle::CE_ProgressBar) ) {
    variants = 4;

    QProgressBar *widget = new QProgressBar();
    widget->setTextVisible(true);

    switch (currentPreviewVariant % variants) {
      case 0:
        break;
      case 1:
        widget->setOrientation(Qt::Vertical);
        break;
      case 2:
        widget->setRange(0,100);
        widget->setValue(25);
        break;
      case 3:
        widget->setOrientation(Qt::Vertical);
        widget->setRange(0,100);
        widget->setValue(25);
        break;
    }

    previewWidget = widget;
    qsz = widget->sizePolicy();
  }

  if ( group == CC_group(QStyle::CC_GroupBox) ) {
    variants = 6;

    QGroupBox *widget = new QGroupBox();
    widget->setTitle("This is a group box");

    switch (currentPreviewVariant % variants) {
      case 0:
        widget->setAlignment(Qt::AlignCenter);
        break;
      case 1:
        widget->setAlignment(Qt::AlignLeft);
        break;
      case 2:
        widget->setAlignment(Qt::AlignRight);
        break;
      case 3:
        widget->setAlignment(Qt::AlignCenter);
        widget->setCheckable(true);
        break;
      case 4:
        widget->setAlignment(Qt::AlignLeft);
        widget->setCheckable(true);
        break;
      case 5:
        widget->setAlignment(Qt::AlignRight);
        widget->setCheckable(true);
        break;
    }

    previewWidget = widget;
  }

  if ( group == CE_group(QStyle::CE_ToolBoxTab) ) {
    variants = 1;

    QToolBox *widget = new QToolBox();
    widget->setFrameShape(QFrame::QFrame::StyledPanel);
    widget->setFrameShadow(QFrame::QFrame::Sunken);

    widget->addItem(new QLabel("content 1"),icon,"Page 1");
    widget->addItem(new QLabel("content 2"),"Page 2");

    previewWidget = widget;
  }

  if ( group == CE_group(QStyle::CE_TabBarTab) ) {
    variants = 4;

    QTabWidget *widget = new QTabWidget();

    widget->addTab(new QLabel("content 1"),icon,"Tab 1");
    widget->addTab(new QLabel("content 2"),"Tab 2");

    switch (currentPreviewVariant % variants) {
      case 0:
        break;
      case 1:
        widget->setTabPosition(QTabWidget::South);
        break;
      case 2:
        widget->setTabPosition(QTabWidget::East);
        break;
      case 3:
        widget->setTabPosition(QTabWidget::West);
        break;
    }

    previewWidget = widget;
    qsz = widget->sizePolicy();
  }

  if ( group == PE_group(QStyle::PE_Frame) ) {
    variants = 2;

    QFrame *widget = new QFrame();
    widget->setFrameShape(QFrame::StyledPanel);

    switch (currentPreviewVariant % variants) {
      case 0:
        widget->setFrameShadow(QFrame::Raised);
        break;
      case 1:
        widget->setFrameShadow(QFrame::Sunken);
        break;
    }

    previewWidget = widget;
    qsz = widget->sizePolicy();
  }

  if ( group == CE_group(QStyle::CE_DockWidgetTitle) ) {
    variants = 1;

    QDockWidget *widget = new QDockWidget();
    widget->setWidget(new QLabel("contents", widget));
    widget->setWindowTitle("This is a dock widget");

    previewWidget = widget;
    qsz = widget->sizePolicy();
  }

  if ( group == CE_group(QStyle::CE_ToolBar) ) {
    variants = 1;

    QToolBar *widget = new QToolBar(this);
    widget->setMovable(true);
    //widget->setFloatable(true);
    widget->addAction(icon,"action1");
    widget->addAction(icon,"action2");
    widget->addSeparator();
    widget->addAction(icon,"action3");

    addToolBar(widget);
    previewWidget = widget;
  }

  if ( group == CE_group(QStyle::CE_MenuBarItem) ) {
    variants = 1;

    QMenuBar *widget = new QMenuBar();
    widget->addAction("Menu1");
    widget->addAction("Menu2");

    previewWidget = widget;
    qsz = QSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);
  }

  if ( group == CE_group(QStyle::CE_MenuItem) ) {
    variants = 1;

    QMenu *widget = new QMenu("this is a menu");
    widget->addAction("Menu item 1");
    widget->addAction(icon,"Menu item 2");
    widget->addSeparator();
    widget->addAction(icon,"Menu item 3", NULL,NULL,QKeySequence("ALT+F1"));
    widget->actions()[3]->setCheckable(true);
    widget->addAction("Menu item 4");
    widget->actions()[4]->setEnabled(false);

    QMenu *submenu = new QMenu("a sub menu");
    submenu->addAction("sub menu 1");
    submenu->addAction("sub menu 2");
    widget->addMenu(submenu);
    widget->actions()[5]->setCheckable(true);
    widget->actions()[5]->setShortcut(QKeySequence("CTRL+Q"));

    previewWidget = widget;
  }

  if ( group == CE_group(QStyle::CE_Header) ) {
    variants = 1;

    QHeaderView *widget = new QHeaderView(Qt::Horizontal);
    QStandardItemModel *model = new QStandardItemModel(1,3, widget);
    widget->setModel(model);
    widget->model()->setHeaderData(0,Qt::Horizontal,"Section 1",Qt::DisplayRole);
    widget->model()->setHeaderData(0,Qt::Horizontal,icon,Qt::DecorationRole);
    widget->model()->setHeaderData(1,Qt::Horizontal,"Section 2",Qt::DisplayRole);
    widget->model()->setHeaderData(2,Qt::Horizontal,"Section 3",Qt::DisplayRole);
    widget->setSortIndicator(0,Qt::AscendingOrder);
    widget->setSortIndicator(1,Qt::DescendingOrder);

    // FIXME size !
    previewWidget = widget;
    widget->resize(widget->minimumSizeHint());
    qsz = QSizePolicy(widget->sizePolicy().horizontalPolicy(),QSizePolicy::Maximum);;
  }

  if ( group == PE_group(QStyle::PE_PanelItemViewItem) ) {
    variants = 1;

    QListWidget *widget = new QListWidget();
    widget->addItem("item 1");
    widget->addItem("item 2");
    widget->addItem("item 3");
    widget->addItem("item 4");
    widget->addItem("item 5");
    widget->addItem("item 6");

    widget->item(0)->setCheckState(Qt::Checked);
    widget->item(1)->setIcon(icon);
    widget->item(3)->setCheckState(Qt::Unchecked);
    widget->item(3)->setIcon(icon);

    widget->setAlternatingRowColors(true);

    previewWidget = widget;
  }

end:
  if ( previewWidget ) {
    repaintBtn->setEnabled(true);
    rtlBtn->setEnabled(true);
    drawModeBtn->setEnabled(true);
    fontSizeSpin->setEnabled(true);
    enableBtn->setEnabled(true);
    drawStackTree->setEnabled(true);
//     resolvedValuesTree->setEnabled(true);
    previewArea->setEnabled(true);
    previewVariantBtn->setEnabled(true);

    currentPreviewVariant = currentPreviewVariant % variants;
    previewVariantBtn->setText(QString("%1/%2").arg(currentPreviewVariant+1).arg(variants));

    previewWidget->installEventFilter(this);
    setStyleForWidgetAndChildren(style, previewWidget);

    if ( !qobject_cast< const QToolBar* >(previewWidget) ) {
      previewWidget->setSizePolicy(qsz);
      previewArea->setWidget(previewWidget);
    }

    slot_enableBtnClicked(enableBtn->isChecked());
    slot_rtlBtnClicked(rtlBtn->isChecked());
    slot_fontSizeChanged(fontSizeSpin->value());
  } else {
    repaintBtn->setEnabled(false);
    rtlBtn->setEnabled(false);
    enableBtn->setEnabled(false);
    drawModeBtn->setEnabled(false);
    fontSizeSpin->setEnabled(false);
    drawStackTree->setEnabled(false);
//     resolvedValuesTree->setEnabled(false);
    previewVariantBtn->setEnabled(false);

    if ( !svgFile.isEmpty() )
      previewWidget = new QLabel("There is no preview available for this element");
    else
      previewWidget = new QLabel("Matching SVG file not found. No preview available");

    previewWidget->setSizePolicy(qsz);
    previewArea->setWidget(previewWidget);
  }
}

void ThemeBuilderUI::saveSettingsFromUi(const QListWidgetItem *current)
{
  if ( !config )
    return;

  theme_spec_t _ts;
  _ts.author = authorEdit->text();
  _ts.name = themeNameEdit->text();
  _ts.descr = descrEdit->text();

  config->setThemeSpec(_ts);

  if ( !current )
    return;

  QString group = current->data(GroupRole).toString();

  element_spec_t _es;
  if ( inheritCb->isChecked() )
    _es.inherits = inheritCombo->itemData(inheritCombo->currentIndex());

  if ( frameCb->checkState() == Qt::Checked )
    _es.frame.hasFrame = true;
  if ( frameCb->checkState() == Qt::Unchecked )
    _es.frame.hasFrame = false;
  if ( frameCb->isChecked() ) {
    if ( frameIdCb->checkState() == Qt::Checked )
      _es.frame.element = frameIdCombo->currentText();
    if ( frameIdCb->checkState() == Qt::Unchecked )
      _es.frame.element = QString();
    if ( frameWidthCb->checkState() == Qt::Checked )
      _es.frame.width = frameWidthSpin->value();
    if ( frameWidthCb->checkState() == Qt::Unchecked )
      _es.frame.width = 0;
  }

  if ( interiorCb->checkState() == Qt::Checked )
    _es.interior.hasInterior = true;
  if ( interiorCb->checkState() == Qt::Unchecked )
    _es.interior.hasInterior = false;
  if ( interiorCb->isChecked() ) {
    if ( interiorIdCb->checkState() == Qt::Checked )
      _es.interior.element = interiorIdCombo->currentText();
    if ( interiorIdCb->checkState() == Qt::Unchecked )
      _es.interior.element = QString();
    if ( interiorRepeatCb->checkState() == Qt::Checked ) {
      _es.interior.px = interiorRepeatXSpin->value();
      _es.interior.py = interiorRepeatYSpin->value();
    }
    if ( interiorRepeatCb->checkState() == Qt::Unchecked ) {
      _es.interior.px = 0;
      _es.interior.py = 0;
    }
  }

  if ( indicatorIdCb->checkState() == Qt::Checked )
    _es.indicator.element = indicatorIdCombo->currentText();
  if ( indicatorIdCb->checkState() == Qt::Unchecked )
    _es.indicator.element = QString();

  if ( labelSpacingCb->checkState() == Qt::Checked )
    _es.label.tispace = labelSpacingSpin->value();
  if ( labelSpacingCb->checkState() == Qt::Unchecked )
    _es.label.tispace = 0;

  if ( labelMarginCb->checkState() == Qt::Checked ) {
    _es.label.hmargin = labelMarginHSpin->value();
    _es.label.vmargin = labelMarginVSpin->value();
  }
  if ( labelMarginCb->checkState() == Qt::Unchecked ) {
    _es.label.hmargin = 0;
    _es.label.vmargin = 0;
  }

  config->setElementSpec(group, _es);
}

void ThemeBuilderUI::slot_uiSettingsChanged()
{
  timer->stop();

  cfgModified = true;
  saveBtn->setEnabled(true);

  saveSettingsFromUi(currentWidget);
  setupPreviewForWidget(currentWidget);
}

void ThemeBuilderUI::slot_repaintBtnClicked(bool checked)
{
  Q_UNUSED(checked);

  if ( previewWidget )
    previewWidget->repaint();
}

void ThemeBuilderUI::slot_rtlBtnClicked(bool checked)
{
  if ( previewWidget ) {
    if ( checked )
      previewWidget->setLayoutDirection(Qt::RightToLeft);
    else
      previewWidget->setLayoutDirection(Qt::LeftToRight);
  }
}

void ThemeBuilderUI::slot_drawModeBtnClicked(bool checked)
{
  Q_UNUSED(checked);

  currentDrawMode = (currentDrawMode+1)%3;

  if ( style)
    style->dbgOverdraw = style->dbgWireframe = 0;

  switch (currentDrawMode) {
    case 0:
      drawModeBtn->setText("N");
      break;
    case 1:
      drawModeBtn->setText("W");
      if ( style )
        style->dbgWireframe = 1;
      break;
    case 2:
      drawModeBtn->setText("O");
      if ( style )
        style->dbgOverdraw = 1;
      break;
  }

  if ( previewWidget )
    previewWidget->repaint();
}

void ThemeBuilderUI::slot_detachBtnClicked(bool checked)
{
  Q_UNUSED(checked);

  QIcon icon;

  if ( !previewArea->isTopLevel() ) {
    icon.addFile(QString::fromUtf8(":/icon/pixmaps/dockwidget.png"), QSize(), QIcon::Normal, QIcon::Off);
    detachBtn->setIcon(icon);
    detachBtn->setText("Attach");
    previewArea->setParent(NULL);
    previewArea->show();
    if ( detachedPeviewGeometry.isValid() )
      previewArea->setGeometry(detachedPeviewGeometry);
    previewArea->window()->setWindowTitle("QSvgThemeBuilder preview");
  } else {
    icon.addFile(QString::fromUtf8(":/icon/pixmaps/widget.png"), QSize(), QIcon::Normal, QIcon::Off);
    detachBtn->setIcon(icon);
    detachBtn->setText("Detach");
    detachedPeviewGeometry = previewArea->geometry();
    // HACK if the preview tab is not visible, the preview area does not appear
    int i = tabWidget2->currentIndex();
    tabWidget2->setCurrentIndex(1);
    previewLayout->addWidget(previewArea, 1, 0, 1, 1);
    tabWidget2->setCurrentIndex(i);
    // END
  }
}

void ThemeBuilderUI::slot_fontSizeChanged(int val)
{
  if ( previewWidget ) {
    QFont f = previewWidget->font();
    f.setPointSize(val);
    previewWidget->setFont(f);
  }
}

void ThemeBuilderUI::slot_enableBtnClicked(bool checked)
{
  if ( previewWidget )
    previewWidget->setEnabled(checked);
}

void ThemeBuilderUI::slot_previewVariantBtnClicked(bool checked)
{
  Q_UNUSED(checked);

  currentPreviewVariant++;

  setupPreviewForWidget(currentWidget);
}

void ThemeBuilderUI::slot_authorEditChanged(const QString& text)
{
  Q_UNUSED(text);

  schedulePreviewUpdate();
}

void ThemeBuilderUI::slot_themeNameEditChanged(const QString& text)
{
  Q_UNUSED(text);

  schedulePreviewUpdate();
}

void ThemeBuilderUI::slot_descrEditChanged(const QString& text)
{
  Q_UNUSED(text);

  schedulePreviewUpdate();
}

void ThemeBuilderUI::slot_inheritCbChanged(int state)
{
  inheritCombo->removeItem(inheritCombo->findText("<invalid>"));

  if ( state == Qt::Checked ) {
    inheritCombo->setEnabled(true);
    if ( raw_es.inherits.present ) {
      int idx = inheritCombo->findData(raw_es.inherits);
      if ( idx == -1 ) {
        inheritCombo->addItem("<invalid>");
        inheritCombo->setCurrentIndex(inheritCombo->findText("<invalid>"));
      } else {
        inheritCombo->setCurrentIndex(idx);
      }
    }
  } else {
    inheritCombo->setEnabled(false);
    inheritCombo->setCurrentIndex(-1);
  }

  schedulePreviewUpdate();
}

void ThemeBuilderUI::slot_inheritComboChanged(int idx)
{
  Q_UNUSED(idx);

  schedulePreviewUpdate();
}

void ThemeBuilderUI::slot_frameCbChanged(int state)
{
  if ( (state == Qt::Checked) || (state == Qt::PartiallyChecked) ) {
    frameIdCb->setEnabled(true);
    frameWidthCb->setEnabled(true);
  } else {
    frameIdCb->setEnabled(false);
    frameWidthCb->setEnabled(false);
  }

  if ( state == Qt::Checked ) {
    frameEdit->setEnabled(true);
    frameEdit->setText("<yes>");
  }
  if ( state == Qt::PartiallyChecked ) {
    frameEdit->setEnabled(false);
    frameEdit->setText("<inherit>");
    //frameEdit->setText(QString("inherit=<%1>").arg(es.frame.hasFrame ? "yes" : "no"));
  }
  if ( state == Qt::Unchecked ) {
    frameEdit->setEnabled(false);
    frameEdit->setText("<no>");
  }

  slot_frameIdCbChanged(frameIdCb->checkState());
  slot_frameWidthCbChanged(frameWidthCb->checkState());

  schedulePreviewUpdate();
}

void ThemeBuilderUI::slot_frameIdCbChanged(int state)
{
  if ( state == Qt::Checked ) {
    frameIdCombo->setEnabled(frameCb->isChecked());
  } else {
    frameIdCombo->setEnabled(false);
  }
  if ( state == Qt::Checked ) {
    if ( raw_es.frame.element.present ) {
      int idx = frameIdCombo->findText(raw_es.frame.element);
      if ( idx == -1 )
        frameIdCombo->addItem(raw_es.frame.element);
      idx = frameIdCombo->findText(raw_es.frame.element);
      frameIdCombo->setCurrentIndex(idx);
      frameIdCombo->setEditText(raw_es.frame.element);
    }
  }
  if ( state == Qt::PartiallyChecked ) {
    frameIdCombo->setEditText("<inherit>");
  }
  if ( state == Qt::Unchecked ) {
    frameIdCombo->setEditText("<none>");
  }

  schedulePreviewUpdate();
}

void ThemeBuilderUI::slot_frameIdComboChanged(const QString& text)
{
  Q_UNUSED(text);

  schedulePreviewUpdate();
}

void ThemeBuilderUI::slot_frameWidthCbChanged(int state)
{
  if ( state == Qt::Checked ) {
    frameWidthSpin->setEnabled(frameCb->isChecked());
    frameWidthSpin->setSpecialValueText(QString());
    frameWidthSpin->setMinimum(1);
    frameWidthSpin->setValue(raw_es.frame.width);
  } else {
    frameWidthSpin->setEnabled(false);
    frameWidthSpin->setMinimum(-1);
    frameWidthSpin->setValue(frameWidthSpin->minimum());
  }
  if ( state == Qt::PartiallyChecked ) {
    frameWidthSpin->setSpecialValueText("<inherit>");
  }
  if ( state == Qt::Unchecked ) {
    frameWidthSpin->setSpecialValueText("<none>");
  }

  schedulePreviewUpdate();
}

void ThemeBuilderUI::slot_frameWidthSpinChanged(int val)
{
  Q_UNUSED(val);

  schedulePreviewUpdate();
}

void ThemeBuilderUI::slot_interiorCbChanged(int state)
{
  if ( (state == Qt::Checked) || (state == Qt::PartiallyChecked) ) {
    interiorIdCb->setEnabled(true);
    interiorRepeatCb->setEnabled(true);
  } else {
    interiorIdCb->setEnabled(false);
    interiorRepeatCb->setEnabled(false);
  }

  if ( state == Qt::Checked ) {
    interiorEdit->setEnabled(true);
    interiorEdit->setText("<yes>");
  }
  if ( state == Qt::PartiallyChecked ) {
    interiorEdit->setEnabled(false);
    interiorEdit->setText("<inherit>");
  }
  if ( state == Qt::Unchecked ) {
    interiorEdit->setEnabled(false);
    interiorEdit->setText("<no>");
  }

  slot_interiorIdCbChanged(interiorIdCb->checkState());
  slot_interiorRepeatCbChanged(interiorRepeatCb->checkState());

  schedulePreviewUpdate();
}

void ThemeBuilderUI::slot_interiorIdCbChanged(int state)
{
  if ( state == Qt::Checked ) {
    interiorIdCombo->setEnabled(interiorIdCb->isChecked());
  } else {
    interiorIdCombo->setEnabled(false);
  }
  if ( state == Qt::Checked ) {
    if ( raw_es.interior.element.present ) {
      int idx = interiorIdCombo->findText(raw_es.interior.element);
      if ( idx == -1 )
        interiorIdCombo->addItem(raw_es.interior.element);
      idx = interiorIdCombo->findText(raw_es.interior.element);
      interiorIdCombo->setCurrentIndex(idx);
      interiorIdCombo->setEditText(raw_es.interior.element);
    }
  }
  if ( state == Qt::PartiallyChecked ) {
    interiorIdCombo->setEditText("<inherit>");
  }
  if ( state == Qt::Unchecked ) {
    interiorIdCombo->setEditText("<none>");
  }

  schedulePreviewUpdate();
}

void ThemeBuilderUI::slot_interiorIdComboChanged(const QString &text)
{
  Q_UNUSED(text);

  schedulePreviewUpdate();
}

void ThemeBuilderUI::slot_interiorRepeatCbChanged(int state)
{
  if ( state == Qt::Checked ) {
    interiorRepeatXSpin->setEnabled(interiorRepeatCb->isChecked());
    interiorRepeatXSpin->setSpecialValueText(QString());
    interiorRepeatXSpin->setMinimum(1);
    interiorRepeatXSpin->setValue(qMax((int)raw_es.interior.px,interiorRepeatXSpin->minimum()));

    interiorRepeatYSpin->setEnabled(interiorRepeatCb->isChecked());
    interiorRepeatYSpin->setSpecialValueText(QString());
    interiorRepeatYSpin->setMinimum(1);
    interiorRepeatYSpin->setValue(qMax((int)raw_es.interior.py,interiorRepeatYSpin->minimum()));
  } else {
    interiorRepeatXSpin->setEnabled(false);
    interiorRepeatXSpin->setMinimum(-1);
    interiorRepeatXSpin->setValue(interiorRepeatXSpin->minimum());

    interiorRepeatYSpin->setEnabled(false);
    interiorRepeatYSpin->setMinimum(-1);
    interiorRepeatYSpin->setValue(interiorRepeatXSpin->minimum());
  }
  if ( state == Qt::PartiallyChecked ) {
    interiorRepeatXSpin->setSpecialValueText("<inherit>");
    interiorRepeatYSpin->setSpecialValueText("<inherit>");
  }
  if ( state == Qt::Unchecked ) {
    interiorRepeatXSpin->setSpecialValueText("<none>");
    interiorRepeatYSpin->setSpecialValueText("<none>");
  }

  schedulePreviewUpdate();
}

void ThemeBuilderUI::slot_interiorRepeatXSpinChanged(int val)
{
  Q_UNUSED(val);

  schedulePreviewUpdate();
}

void ThemeBuilderUI::slot_interiorRepeatYSpinChanged(int val)
{
  Q_UNUSED(val)

  schedulePreviewUpdate();
}

void ThemeBuilderUI::slot_labelSpacingCbChanged(int state)
{
  if ( state == Qt::Checked ) {
    labelSpacingSpin->setEnabled(true);
    labelSpacingSpin->setSpecialValueText(QString());
    labelSpacingSpin->setMinimum(1);
    labelSpacingSpin->setValue(qMax((int)raw_es.label.tispace,labelSpacingSpin->minimum()));
  } else {
    labelSpacingSpin->setEnabled(false);
    labelSpacingSpin->setMinimum(-1);
    labelSpacingSpin->setValue(labelSpacingSpin->minimum());
  }
  if ( state == Qt::PartiallyChecked ) {
    labelSpacingSpin->setSpecialValueText("<inherit>");
  }
  if ( state == Qt::Unchecked ) {
    labelSpacingSpin->setSpecialValueText("<none>");
  }

  schedulePreviewUpdate();
}

void ThemeBuilderUI::slot_labelSpacingSpinChanged(int val)
{
  Q_UNUSED(val);

  schedulePreviewUpdate();
}

void ThemeBuilderUI::slot_labelMarginCbChanged(int state)
{
  if ( state == Qt::Checked ) {
    labelMarginHSpin->setEnabled(labelMarginCb->isChecked());
    labelMarginHSpin->setSpecialValueText(QString());
    labelMarginHSpin->setMinimum(0);
    labelMarginHSpin->setValue(qMax((int)raw_es.label.hmargin,labelMarginHSpin->minimum()));

    labelMarginVSpin->setEnabled(labelMarginCb->isChecked());
    labelMarginVSpin->setSpecialValueText(QString());
    labelMarginVSpin->setMinimum(0);
    labelMarginVSpin->setValue(qMax((int)raw_es.label.vmargin,labelMarginVSpin->minimum()));
  } else {
    labelMarginHSpin->setEnabled(false);
    labelMarginHSpin->setMinimum(-1);
    labelMarginHSpin->setValue(labelMarginHSpin->minimum());

    labelMarginVSpin->setEnabled(false);
    labelMarginVSpin->setMinimum(-1);
    labelMarginVSpin->setValue(labelMarginHSpin->minimum());
  }
  if ( state == Qt::PartiallyChecked ) {
    labelMarginHSpin->setSpecialValueText("<inherit>");
    labelMarginVSpin->setSpecialValueText("<inherit>");
  }
  if ( state == Qt::Unchecked ) {
    labelMarginHSpin->setSpecialValueText("<none>");
    labelMarginVSpin->setSpecialValueText("<none>");
  }

  schedulePreviewUpdate();
}

void ThemeBuilderUI::slot_labelMarginHSpinChanged(int val)
{
  Q_UNUSED(val);

  schedulePreviewUpdate();
}

void ThemeBuilderUI::slot_labelMarginVSpinChanged(int val)
{
  Q_UNUSED(val);

  schedulePreviewUpdate();
}

void ThemeBuilderUI::slot_indicatorIdCbChanged(int state)
{
  if ( state == Qt::Checked ) {
    indicatorIdCombo->setEnabled(indicatorIdCb->isChecked());
  } else {
    indicatorIdCombo->setEnabled(false);
  }
  if ( state == Qt::Checked ) {
    if ( raw_es.indicator.element.present ) {
      int idx = indicatorIdCombo->findText(raw_es.indicator.element);
      if ( idx == -1 )
        indicatorIdCombo->addItem(raw_es.indicator.element);
      idx = indicatorIdCombo->findText(raw_es.indicator.element);
      indicatorIdCombo->setCurrentIndex(idx);
      indicatorIdCombo->setEditText(raw_es.indicator.element);
    }
  }
  if ( state == Qt::PartiallyChecked ) {
    indicatorIdCombo->setEditText("<inherit>");
  }
  if ( state == Qt::Unchecked ) {
    indicatorIdCombo->setEditText("<none>");
  }

  schedulePreviewUpdate();
}

void ThemeBuilderUI::slot_indicatorIdComboChanged(const QString& text)
{
  Q_UNUSED(text);

  schedulePreviewUpdate();
}


void ThemeBuilderUI::slot_toolboxTabChanged(int index)
{
  QListWidgetItem *previous = 0, *current = 0;

  switch ( currentToolboxTab ) {
    case 0:
      previous = buttonList->currentItem();
      break;
    case 1:
      previous = inputList->currentItem();
      break;
    case 2:
      previous = displayList->currentItem();
      break;
    case 3:
      previous = containerList->currentItem();
      break;
    case 4:
      previous = miscList->currentItem();
      break;
    default:
      break;
  }

  currentToolboxTab = index;

  switch ( currentToolboxTab ) {
    case 0:
      current = buttonList->currentItem();
      break;
    case 1:
      current = inputList->currentItem();
      break;
    case 2:
      current = displayList->currentItem();
      break;
    case 3:
      current = containerList->currentItem();
      break;
    case 4:
      current = miscList->currentItem();
      break;
    default:
      break;
  }

//   if ( current )
//     current->listWidget()->setFocus(Qt::ActiveWindowFocusReason);

  slot_widgetChanged(current,previous);
}

void ThemeBuilderUI::slot_widgetChanged(QListWidgetItem *current, QListWidgetItem *previous)
{
  saveSettingsFromUi(currentWidget);

  if ( current ) {
    if ( !previous )
      tabWidget2->setCurrentIndex(1);
    currentPreviewVariant = 0;
  }

  currentWidget = current;

  setupUiForWidget(currentWidget);
  setupPreviewForWidget(currentWidget);
}

void ThemeBuilderUI::slot_drawPrimitive_begin(const QString& s)
{
  noteStyleOperation_begin("drawPrimitive",s);
}

void ThemeBuilderUI::slot_drawPrimitive_end(const QString& s)
{
  noteStyleOperation_end("drawPrimitive",s);
}

void ThemeBuilderUI::slot_drawComplexControl_begin(const QString &s)
{
  noteStyleOperation_begin("drawComplexControl",s);
}
void ThemeBuilderUI::slot_drawComplexControl_end(const QString &s)
{
  noteStyleOperation_end("drawComplexControl",s);
}
void ThemeBuilderUI::slot_drawControl_begin(const QString &s)
{
  noteStyleOperation_begin("drawControl",s);
}
void ThemeBuilderUI::slot_drawControl_end(const QString &s)
{
  noteStyleOperation_end("drawControl",s);
}
void ThemeBuilderUI::slot_renderElement_begin(const QString &s)
{
  noteStyleOperation_begin("renderElement",s);
}
void ThemeBuilderUI::slot_renderElement_end(const QString &s)
{
  noteStyleOperation_end("renderElement",s);
}
void ThemeBuilderUI::slot_renderFrame_begin(const QString &s)
{
  noteStyleOperation_begin("renderFrame",s);
}
void ThemeBuilderUI::slot_renderFrame_end(const QString &s)
{
  noteStyleOperation_end("renderFrame",s);
}
void ThemeBuilderUI::slot_renderIndicator_begin(const QString &s)
{
  noteStyleOperation_begin("renderIndicator",s);
}
void ThemeBuilderUI::slot_renderIndicator_end(const QString &s)
{
  noteStyleOperation_end("renderIndicator",s);
}
void ThemeBuilderUI::slot_renderInterior_begin(const QString &s)
{
  noteStyleOperation_begin("renderInterior",s);
}
void ThemeBuilderUI::slot_renderInterior_end(const QString &s)
{
  noteStyleOperation_end("renderInterior",s);
}
void ThemeBuilderUI::slot_renderLabel_begin(const QString &s)
{
  noteStyleOperation_begin("renderLabel",s);
}
void ThemeBuilderUI::slot_renderLabel_end(const QString &s)
{
  noteStyleOperation_end("renderLabel",s);
}
void ThemeBuilderUI::slot_sizeFromContents_begin(const QString &s)
{
  noteStyleOperation_begin("sizeFromContents",s);
}
void ThemeBuilderUI::slot_sizeFromContents_end(const QString &s)
{
  noteStyleOperation_end("sizeFromContents",s);
}
