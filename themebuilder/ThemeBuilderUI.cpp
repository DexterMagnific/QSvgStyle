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

#include <QDebug>
#include <QSettings>
#include <QMetaObject>
#include <QFile>
#include <QDir>
#include <QMimeData>
#include <QClipboard>

// UI
#include <QFileDialog>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QHeaderView>
#include <QFileInfo>
#include <QStyleFactory>
#include <QMessageBox>
#include <QTimer>
#include <QStandardItemModel>
#include <QDesktopServices>
#include <QUrl>
#include <QTemporaryFile>
#include <QLibraryInfo>
#include <QEvent>
#include <QTreeWidgetItemIterator>
#include <QColorDialog>
#include <QPainter>
#include <QPen>
#include <QPixmap>
#include <QGraphicsScene>
#include <QFontDatabase>

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
#include <QSpinBox>
#include <QDial>
#include <QStatusBar>
#include <QMdiArea>
#include <QMdiSubWindow>

#include "NewThemeUI.h"
#include "ThemeConfig.h"
#include "PaletteConfig.h"
#include "StyleConfig.h"
#include "SvgGen.h"
#include "../style/QSvgThemableStyle.h"
#include "groups.h"

// optimizer
#include "keys.h"
#include "basecleaner.h"
#include "remover.h"
#include "replacer.h"

// Specific tree Item Delegate
// name is stored in SpecificSettingName
// display value is stored in Qt::DisplayRole
// internal value is stored in Qt::EditRole
// setting type is stored in SpecificSettingType
// setting range is stored in SpecificSetting Range
SpecificTreeDelegate::SpecificTreeDelegate(QObject *parent)
  : QStyledItemDelegate(parent)
{
}

QWidget *SpecificTreeDelegate::createEditor(QWidget *parent,
                                       const QStyleOptionViewItem &/* option */,
                                       const QModelIndex &index) const
{
  QWidget *res = NULL;

  // Delegate is only for column 2
  if ( index.column() != 1 )
    return NULL;

  const QString type = index.data(SpecificSettingType).toString();
  const QString range_str = index.data(SpecificSettingRange).toString();

  if ( type == "int" ) {
    QSpinBox *widget = new QSpinBox(parent);
    QStringList min_max = range_str.split('-');
    if ( min_max.size() == 2 ) {
      bool ok = false;
      int min = min_max.at(0).toInt(&ok);
      if ( ok )
        widget->setMinimum(min);
      int max = min_max.at(1).toInt(&ok);
      if ( ok )
        widget->setMaximum(max);
    }

    connect(widget,SIGNAL(valueChanged(int)), this,SLOT(slot_valueChanged(int)));

    res = widget;
  }

  if ( type == "bool" ) {
    QCheckBox *widget = new QCheckBox(parent);

    connect(widget,SIGNAL(toggled(bool)), this,SLOT(slot_boolChanged(bool)));

    res = widget;
  }

  if ( type == "enum" || type == "int_enum" ) {
    QComboBox *widget = new QComboBox(parent);
    QStringList items = range_str.split(',');
    widget->addItems(items);

    connect(widget,SIGNAL(currentIndexChanged(int)), this,SLOT(slot_currentIndexChanged(int)));
    res = widget;
  }

  connect(res,SIGNAL(destroyed(QObject*)),
          this,SLOT(slot_editorDestroyed(QObject*)));

  return res;
}

void SpecificTreeDelegate::slot_editorDestroyed(QObject *o)
{
  Q_UNUSED(o);
}

void SpecificTreeDelegate::setEditorData(QWidget *editor,
                                         const QModelIndex &index) const
{
  const QString type = index.data(SpecificSettingType).toString();
  const QString range_str = index.data(SpecificSettingRange).toString();
  const QVariant value_data = index.data(Qt::EditRole);

  if ( type == "int" ) {
    QSpinBox *widget = qobject_cast<QSpinBox *>(editor);
    widget->blockSignals(true);
    if ( widget ) {
      bool ok = false;
      int value = value_data.toInt(&ok);
      if ( ok )
        widget->setValue(value);
      else
        widget->setValue(0);
    }
    widget->blockSignals(false);
  }

  if ( type == "bool" ) {
    //qWarning() << "setEditorData" << value_data;
    QCheckBox *widget = qobject_cast<QCheckBox *>(editor);
    widget->blockSignals(true);
    const QStringList true_false = range_str.split(',');
    QString true_str = "true";
    QString false_str = "false";
    if ( true_false.size() >= 1 )
      true_str = true_false.at(0);
    if ( true_false.size() >= 2 )
      false_str = true_false.at(1);
    if ( widget ) {
      if ( value_data.toBool() ) {
        widget->setChecked(true);
        widget->setText(true_str);
      } else {
        widget->setChecked(false);
        widget->setText(false_str);
      }
    }
    widget->blockSignals(false);
  }

  if ( type == "enum" || type == "int_enum" ) {
    //qWarning() << "setEditorData" << value_data;
    QComboBox *widget = qobject_cast<QComboBox *>(editor);
    widget->blockSignals(true);
    if ( type == "enum" ) {
      // enums stored as strings
      int idx = widget->findText(value_data.toString());
      widget->setCurrentIndex(idx);
    } else {
      // enums stored as ints
      widget->setCurrentIndex(value_data.toInt());
    }
    widget->blockSignals(false);
  }
}

void SpecificTreeDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                                        const QModelIndex &index) const
{
  const QString type = index.data(SpecificSettingType).toString();
  const QString range_str = index.data(SpecificSettingRange).toString();

  //model->blockSignals(true); // avoid ping pong

  // Qt Bug : The model installed on QTreeWidget is a QTreeModel.
  // This has the particularity of considering that Qt::DisplayRole = Qt::EditRole
  // see qtreewidget.cpp::QTreeModem::setData() which calls
  // QTreeWidget::setData() for which DisplayRole=EditRole

  if ( type == "int" ) {
    QSpinBox *widget = qobject_cast<QSpinBox *>(editor);
    if ( widget ) {
      widget->interpretText();
      model->setData(index, widget->value(), Qt::EditRole);
      //model->setData(index, widget->value(), Qt::DisplayRole);
    }
  }

  if ( type == "bool" ) {
    QCheckBox *widget = qobject_cast<QCheckBox *>(editor);
    const QStringList true_false = range_str.split(',');
    QString true_str = "true";
    QString false_str = "false";
    if ( true_false.size() >= 1 )
      true_str = true_false.at(0);
    if ( true_false.size() >= 2 )
      false_str = true_false.at(1);

    if ( widget ) {
      //qWarning() << "setModelData" << widget->isChecked();

      model->setData(index, widget->isChecked(), Qt::EditRole);
      //model->setData(index, widget->isChecked() ? true_str : false_str, Qt::DisplayRole);
    }

    // HACK in addition to setting the model data, we have to change
    // the checkbox text because no repaint is trigerred when setting
    // the model data
    widget->setText(widget->isChecked() ? true_str : false_str);
  }

  if ( type == "enum" || type == "int_enum" ) {
    QComboBox *widget = qobject_cast<QComboBox *>(editor);
    if ( widget ) {
      if ( type == "int_enum" ) {
        // Store index
        //qWarning() << "setModelData" << widget->currentIndex();
        model->setData(index, widget->currentIndex(), Qt::EditRole);
      } else {
        // Store string
        //qWarning() << "setModelData" << widget->currentText();
        model->setData(index, widget->currentText(), Qt::EditRole);
      }
      //model->setData(index, widget->currentText(), Qt::DisplayRole);
    }
  }

  model->blockSignals(false);
}

void SpecificTreeDelegate::updateEditorGeometry(QWidget *editor,
                                           const QStyleOptionViewItem &option,
                                           const QModelIndex &/* index */) const
{
  editor->setGeometry(option.rect);
}

void SpecificTreeDelegate::paint(QPainter *painter,
                            const QStyleOptionViewItem &option,
                            const QModelIndex &index) const
{
  if ( index.column() != 1) {
    QStyledItemDelegate::paint(painter,option,index);
    return;
  }

  const QString type = index.data(SpecificSettingType).toString();
  const QString range_str = index.data(SpecificSettingRange).toString();
  const QVariant value_data = index.data(Qt::EditRole);

  const QStyle *s = option.widget ? option.widget->style() :  0;

  if ( !s )
    return;

  // Checkboxes: we need to draw the checkbox and the text
  if ( type == "bool" ) {
    //qWarning() << "paint" << value_data;
    QStyleOptionButton o;
    o.initFrom(option.widget);
    o.rect = option.rect;

    const QStringList true_false = range_str.split(',');
    QString true_str = "true";
    QString false_str = "false";
    if ( true_false.size() >= 1 )
      true_str = true_false.at(0);
    if ( true_false.size() >= 2 )
      false_str = true_false.at(1);

    o.state &= ~(QStyle::State_NoChange | QStyle::State_On);

    if ( value_data.toBool() ) {
      o.state |= QStyle::State_On;
      o.text = true_str;
    } else {
      o.state &= ~QStyle::State_On;
      o.text = false_str;
    }

    s->drawControl(QStyle::CE_CheckBox, &o, painter, 0);
    return;
  }

  if ( type == "enum" || type == "int_enum" ) {
    QStyleOptionViewItem o(option);
    initStyleOption(&o, index);

    const QStringList items = range_str.split(',');
    if ( items.size()-1 >= value_data.toInt() )
      o.text = items.at(value_data.toInt());

    s->drawControl(QStyle::CE_ItemViewItem, &o, painter, 0);
    return;
  }

  // Others: showing text is fine, so we can rely on default implementation
  QStyledItemDelegate::paint(painter,option,index);
}

void SpecificTreeDelegate::slot_currentIndexChanged(int idx)
{
  Q_UNUSED(idx);

  emit commitData(qobject_cast<QComboBox *>(sender()));
}

void SpecificTreeDelegate::slot_valueChanged(int val)
{
  Q_UNUSED(val);

  emit commitData(qobject_cast<QSpinBox *>(sender()));
}

void SpecificTreeDelegate::slot_boolChanged(bool checked)
{
  Q_UNUSED(checked);

  emit commitData(qobject_cast<QCheckBox *>(sender()));
}

ThemeBuilderUI::ThemeBuilderUI(QWidget* parent)
 : QMainWindow(parent), config(0), style(0), previewWidget(0),
   currentWidget(0),
   currentDrawStackItem(0), currentDrawMode(0), currentPreviewVariant(0),
   cfgModified(0), previewUpdateEnabled(false),
   timer(0), timer2(0), newThemeDlg(0),
   svgWatcher(this), svgGen(NULL)
{
  qDebug() << "Current style:" << QApplication::style()->metaObject()->className();

  // Setup using auto-generated UIC code
  setupUi(this);
  setWindowTitle(windowTitle()+"[*]");

  // Menu for recent files
  recentFiles = new QMenu("Recent files", recentBtn);
  recentBtn->setMenu(recentFiles);

  // Columns contain internal data that are not intended to be shown
  // Column 0: display name
  // Column 1: Flags: F=Frame, I=Interior, D=Indicator, L=Label support
  // Column 2: QStyle Enum used to draw the preview
  // Column 3: QSvgStyle config group
  buttonList->setColumnHidden(1, true);
  buttonList->setColumnHidden(2, true);
  buttonList->setColumnHidden(3, true);

  inputList->setColumnHidden(1, true);
  inputList->setColumnHidden(2, true);
  inputList->setColumnHidden(3, true);

  displayList->setColumnHidden(1, true);
  displayList->setColumnHidden(2, true);
  displayList->setColumnHidden(3, true);

  containerList->setColumnHidden(1, true);
  containerList->setColumnHidden(2, true);
  containerList->setColumnHidden(3, true);

  buttonList->header()->setSectionResizeMode(0,QHeaderView::ResizeToContents);
  inputList->header()->setSectionResizeMode(0,QHeaderView::ResizeToContents);
  displayList->header()->setSectionResizeMode(0,QHeaderView::ResizeToContents);
  containerList->header()->setSectionResizeMode(0,QHeaderView::ResizeToContents);

  int maxW = 0;
  maxW = qMax(maxW,buttonList->header()->sectionSize(0));
  maxW = qMax(maxW,inputList->header()->sectionSize(0));
  maxW = qMax(maxW,displayList->header()->sectionSize(0));
  maxW = qMax(maxW,containerList->header()->sectionSize(0));
  maxW += 30; // be comfortable

  buttonList->setFixedWidth(maxW);
  inputList->setFixedWidth(maxW);
  displayList->setFixedWidth(maxW);
  containerList->setFixedWidth(maxW);

  // Adjust gen tool box size
  // Shitty QToolBox in fact renders its pages inside a QScrollArea
  // whose sizeHint is clipped to 36*h,24*h, h=fontMetrics().height()
  maxW = 0;
  int j;
  for (j=0; j<genToolbox->count(); j++) {
    maxW = qMax(maxW,genToolbox->widget(j)->sizeHint().width());
  }
  maxW += 10;

  genToolbox->setFixedWidth(maxW);

  // also populate inherit combo box
  foreach(QTreeWidgetItem *i, buttonList->findItems("*",Qt::MatchWildcard)) {
    if ( !i->text(2).isEmpty() )
      inheritCombo->addItem(i->icon(0),i->text(0),i->text(3));
    else {
      for (int j=0; j<i->childCount(); j++) {
        QTreeWidgetItem *c = i->child(j);
        if ( !c->text(3).isEmpty() )
          inheritCombo->addItem(i->icon(0),
                                QString("%1::%2").arg(i->text(0)).arg(c->text(0)),
                                c->text(3));
      }
    }
  }
  inheritCombo->insertSeparator(999);
  foreach(QTreeWidgetItem *i, inputList->findItems("*",Qt::MatchWildcard)) {
    if ( !i->text(3).isEmpty() )
      inheritCombo->addItem(i->icon(0),i->text(0),i->text(3));
    else {
      for (int j=0; j<i->childCount(); j++) {
        QTreeWidgetItem *c = i->child(j);
        if ( !c->text(3).isEmpty() )
          inheritCombo->addItem(i->icon(0),
                                QString("%1::%2").arg(i->text(0)).arg(c->text(0)),
                                c->text(3));
      }
    }
  }
  inheritCombo->insertSeparator(999);
  foreach(QTreeWidgetItem *i, displayList->findItems("*",Qt::MatchWildcard)) {
    if ( !i->text(3).isEmpty() )
      inheritCombo->addItem(i->icon(0),i->text(0),i->text(3));
    else {
      for (int j=0; j<i->childCount(); j++) {
        QTreeWidgetItem *c = i->child(j);
        if ( !c->text(3).isEmpty() )
          inheritCombo->addItem(i->icon(0),
                                QString("%1::%2").arg(i->text(0)).arg(c->text(0)),
                                c->text(3));
      }
    }
  }
  inheritCombo->insertSeparator(999);
  foreach(QTreeWidgetItem *i, containerList->findItems("*",Qt::MatchWildcard)) {
    if ( !i->text(3).isEmpty() )
      inheritCombo->addItem(i->icon(0),i->text(0),i->text(3));
    else {
      for (int j=0; j<i->childCount(); j++) {
        QTreeWidgetItem *c = i->child(j);
        if ( !c->text(3).isEmpty() )
          inheritCombo->addItem(i->icon(0),
                                QString("%1::%2").arg(i->text(0)).arg(c->text(0)),
                                c->text(3));
      }
    }
  }

  // Further setup for specific tree: move item text to item data
  // Column 0: display name
  // Column 1: value
  // Column 2: setting name
  // Column 3: type
  // Column 4: range
  specificTree->setColumnHidden(2, true);
  specificTree->setColumnHidden(3, true);
  specificTree->setColumnHidden(4, true);

  // Setup Delegates for specific tree
  SpecificTreeDelegate *d = new SpecificTreeDelegate();
  specificTree->setItemDelegate(d);

  QTreeWidgetItemIterator it(specificTree);
  while (*it) {
    if ( !(*it)->childCount() ) { // leaf
      (*it)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEditable|Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
      // move .ui texts to item data
      (*it)->setData(1, SpecificSettingName, (*it)->text(2));
      (*it)->setData(1, SpecificSettingType, (*it)->text(3));
      (*it)->setData(1, SpecificSettingRange, (*it)->text(4));
    }
    ++it;
  }
  specificTree->header()->setSectionResizeMode(0,QHeaderView::ResizeToContents);

  // add tree widgets after window creation to get minimal window size
  drawStackTree = new QTreeWidget(previewTab);
  drawStackTree->setObjectName(QString::fromUtf8("drawStackTree"));
  drawStackTree->setAlternatingRowColors(true);
  drawStackTree->headerItem()->setText(0,"Function");
  drawStackTree->headerItem()->setText(1,"Args");
  debugLayout->addWidget(drawStackTree, 2, 0, 1, 1);

  drawStackTree->header()->setSectionResizeMode(0,QHeaderView::ResizeToContents);

  // insert appropriate widget into status bar
  statusbarLbl1 = new QLabel(this);
  statusbarLbl2 = new QLabel(this);
  statusBar()->insertWidget(0,statusbarLbl1);
  statusBar()->insertWidget(1,statusbarLbl2);

  // setup font size spin
  fontSizeSpin->setValue(font().pointSize());

  // Get an instance of QSvgStyle
  if ( !style ) {
    style = static_cast<QSvgThemableStyle *>(QStyleFactory::create("QSvgStyle"));
    if ( !style ) {
      qWarning() << "[QSvgThemeBuilder]" << "Could not load QSvgStyle style, preview will not be available !";
      QMessageBox::warning(this,"QSvgStyle style not found",
                           QString("QSvgStyle style library could not be loaded."
                           "Maybe it is not installed in the right directory.\n"
                           "The Qt plugin library is\n"
                           "%1\n"
                           "Preview will not be available.").arg(QLibraryInfo::location(QLibraryInfo::PluginsPath)));
    }
    // Check that the style version is the same as the one we were compiled against
    if ( style->Version /* dynamic load */ != QSvgThemableStyle::Version /* compiled .h */) {
      qWarning() << "[QSvgThemeBuilder]" << "QSvgStyle version mismatch !";
      qWarning() << "[QSvgThemeBuilder]" << "QStyleFactory reported version" << style->Version;
      qWarning() << "[QSvgThemeBuilder]" << "ThemeBuilder was built with version" << QSvgThemableStyle::Version;
      QMessageBox::warning(this,"QSvgStyle version mismatch",
                           "The version of the installed QSvgStyle style is not"
                           " the same as the one known by QSvgThemeBuilder.\n"
                           " You may experience crashes when previewing.\n");
    }
  }

  // Disable config caching, we want to see live changes
  QStyle::staticMetaObject.invokeMethod(style,"setUseConfigCache",
                                        Qt::DirectConnection,
                                        Q_ARG(bool, false));

  // Populate palette combo
  paletteCombo->addItem("<none>");
  paletteCombo->addItem("<system>");
  Q_FOREACH(palette_spec_t p, StyleConfig::getPaletteList()) {
    paletteCombo->addItem(p.name);
  }

  // current palette
  QString startupPalette;
  QStyle::staticMetaObject.invokeMethod(style,"currentPalette",
                                        Qt::DirectConnection,
                                        Q_RETURN_ARG(QString,startupPalette));

  int idx = paletteCombo->findText(startupPalette);
  if ( idx > -1 )
    paletteCombo->setCurrentIndex(idx);
  else
    paletteCombo->setCurrentIndex(0);

  // install event filter on previewArea so that we can react to resize and
  // close events and save its geometry
  tabWidget3->installEventFilter(this);

  // SVG Quick generator
  QGraphicsScene *scene = new QGraphicsScene();
  genSvgView->setScene(scene);
  //genSvgView->setRenderHints(QPainter::Antialiasing);
  //genSvgView->scale(2,2);
  svgGen = new SvgGen(scene, this);

  // connections
  // main buttons
  connect(quitBtn,SIGNAL(clicked()),this,SLOT(slot_quit()));
  connect(newBtn,SIGNAL(clicked()), this,SLOT(slot_newTheme()));
  connect(openBtn,SIGNAL(clicked()), this,SLOT(slot_openTheme()));
  connect(saveBtn,SIGNAL(clicked()), this,SLOT(slot_saveTheme()));
  connect(optimizeSvgBtn,SIGNAL(clicked()), this,SLOT(slot_optimizeSvg()));
  connect(editSvgBtn,SIGNAL(clicked()), this,SLOT(slot_editSvg()));

  // Change of selected widget to edit
  connect(buttonList,SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)),
          this,SLOT(slot_widgetChanged(QTreeWidgetItem*,QTreeWidgetItem*)));
  connect(inputList,SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)),
          this,SLOT(slot_widgetChanged(QTreeWidgetItem*,QTreeWidgetItem*)));
  connect(displayList,SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)),
          this,SLOT(slot_widgetChanged(QTreeWidgetItem*,QTreeWidgetItem*)));
  connect(containerList,SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)),
          this,SLOT(slot_widgetChanged(QTreeWidgetItem*,QTreeWidgetItem*)));
  connect(toolBox,SIGNAL(currentChanged(int)),
          this,SLOT(slot_toolboxTabChanged(int)));

  // Changes inside properties tab
  connect(authorEdit,SIGNAL(textEdited(QString)),
          this,SLOT(slot_authorEditChanged(QString)));
  connect(variantEdit,SIGNAL(textEdited(QString)),
          this,SLOT(slot_variantEditChanged(QString)));
  connect(descrEdit,SIGNAL(textEdited(QString)),
          this,SLOT(slot_descrEditChanged(QString)));
  connect(themeNameEdit,SIGNAL(textEdited(QString)),
          this,SLOT(slot_themeNameEditChanged(QString)));
  connect(keywordsEdit,SIGNAL(textEdited(QString)),
          this,SLOT(slot_keywordsEditChanged(QString)));

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
  connect(indicatorSizeCb,SIGNAL(stateChanged(int)),
          this,SLOT(slot_indicatorSizeCbChanged(int)));
  connect(indicatorSizeSpin,SIGNAL(valueChanged(int)),
          this,SLOT(slot_indicatorSizeSpinChanged(int)));

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
  connect(paletteCombo,SIGNAL(currentIndexChanged(int)),
          this,SLOT(slot_paletteChanged(int)));

  // SVG quick gen tab
  connect(genFrameBtn,SIGNAL(clicked(bool)),
          this,SLOT(slot_genFrameBtnClicked(bool)));
  connect(genInteriorBtn,SIGNAL(clicked(bool)),
          this,SLOT(slot_genInteriorBtnClicked(bool)));
  connect(genShadowBtn,SIGNAL(clicked(bool)),
          this,SLOT(slot_genShadowBtnClicked(bool)));
  connect(genRoundBtn,SIGNAL(clicked(bool)),
          this,SLOT(slot_genRoundBtnClicked(bool)));
  connect(genSplitBtn,SIGNAL(clicked(bool)),
          this,SLOT(slot_genSplitBtnClicked(bool)));
  connect(genFrameWidthSpin,SIGNAL(valueChanged(int)),
          this,SLOT(slot_genFrameWidthChanged(int)));
  connect(genFrameTopBtn,SIGNAL(clicked(bool)),
          this,SLOT(slot_genFrameTopBtnClicked(bool)));
  connect(genFrameBottomBtn,SIGNAL(clicked(bool)),
          this,SLOT(slot_genFrameBottomBtnClicked(bool)));
  connect(genFrameLeftBtn,SIGNAL(clicked(bool)),
          this,SLOT(slot_genFrameLeftBtnClicked(bool)));
  connect(genFrameRightBtn,SIGNAL(clicked(bool)),
          this,SLOT(slot_genFrameRightBtnClicked(bool)));
  connect(genInteriorRoundnessSpin,SIGNAL(valueChanged(qreal)),
          this,SLOT(slot_genInteriorRoundnessChanged(qreal)));
  connect(genSquareBtn,SIGNAL(clicked(bool)),
          this,SLOT(slot_genSquareBtnClicked(bool)));
  connect(genCopyBtn,SIGNAL(clicked(bool)),
          this,SLOT(slot_genCopyBtnClicked(bool)));
  connect(genBasenameEdit,SIGNAL(textChanged(QString)),
          this,SLOT(slot_genBasenameChanged(QString)));
  connect(genVariantEdit,SIGNAL(textChanged(QString)),
          this,SLOT(slot_genVariantChanged(QString)));
  connect(genStatusCombo,SIGNAL(editTextChanged(QString)),
          this,SLOT(slot_genStatusChanged(QString)));
  connect(interiorFillTypeBtn,SIGNAL(clicked(bool)),
          this,SLOT(slot_genInteriorFillTypeBtnClicked(bool)));
  connect(interiorColor1Btn,SIGNAL(clicked(bool)),
          this,SLOT(slot_genInteriorColor1BtnClicked(bool)));
  connect(interiorColor2Btn,SIGNAL(clicked(bool)),
          this,SLOT(slot_genInteriorColor2BtnClicked(bool)));
  connect(shadowFillTypeBtn,SIGNAL(clicked(bool)),
          this,SLOT(slot_genShadowFillTypeBtnClicked(bool)));
  connect(shadowColor1Btn,SIGNAL(clicked(bool)),
          this,SLOT(slot_genShadowColor1BtnClicked(bool)));
  connect(shadowColor2Btn,SIGNAL(clicked(bool)),
          this,SLOT(slot_genShadowColor2BtnClicked(bool)));
  connect(genShadowWidthSpin,SIGNAL(valueChanged(qreal)),
          this,SLOT(slot_genShadowWidthChanged(qreal)));
  connect(genShadowTopBtn,SIGNAL(clicked(bool)),
          this,SLOT(slot_genShadowTopBtnClicked(bool)));
  connect(genShadowBottomBtn,SIGNAL(clicked(bool)),
          this,SLOT(slot_genShadowBottomBtnClicked(bool)));
  connect(genShadowLeftBtn,SIGNAL(clicked(bool)),
          this,SLOT(slot_genShadowLeftBtnClicked(bool)));
  connect(genShadowRightBtn,SIGNAL(clicked(bool)),
          this,SLOT(slot_genShadowRightBtnClicked(bool)));


  // Tweaks tree
  connect(specificTree,SIGNAL(itemChanged(QTreeWidgetItem *, int)),
          this,SLOT(slot_specificChanged(QTreeWidgetItem *,int)));

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

    connect(style,SIGNAL(sig_missingElement(QString)),
            this,SLOT(slot_missingElement(QString)));
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
  loadRecentFiles();
  resetUi();

  // Setup optimizer arguments
  QStringList options = QStringList()
    << "--remove-comments"
    << "--remove-unused-defs"
    << "--remove-nonsvg-elts"
    << "--remove-sodipodi-elts"
    << "--remove-ai-elts"
    << "--remove-corel-elts"
    << "--remove-msvisio-elts"
    << "--remove-sketch-elts"
    // NOTE do not set, invisible elements are used to give larger bounding boxes
    // to elements than the contents of drawings (e.g. margins)
    ///<< "--remove-invisible-elts"
    << "--remove-empty-containers"
    << "--remove-duplicated-defs"
    << "--remove-gaussian-blur=0"
    << "--remove-notappl-atts"
    << "--remove-default-atts"
    << "--remove-inkscape-atts"
    << "--remove-sodipodi-atts"
    << "--remove-ai-atts"
    << "--remove-corel-atts"
    << "--remove-msvisio-atts"
    << "--remove-sketch-atts"
    << "--remove-stroke-props"
    << "--remove-fill-props"
    << "--remove-unused-xlinks"
    << "--simplify-transform-matrix"
    << "--apply-transforms-to-shapes"
    << "--join-style-atts"
    << "--remove-unneeded-symbols"
    << "--apply-transforms-to-paths"
    << "--colors-to-rrggbb"
    << "--transform-precision=8"
    << "--coordinates-precision=6"
    << "--attributes-precision=6";

  Keys.parseOptions(options);

  // TESTING remove me in release
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

void ThemeBuilderUI::closeEvent(QCloseEvent* e)
{
  if ( !ensureSettingsSaved() ) {
    e->ignore();
    return;
  }

  saveRecentFiles();
  resetUi();

  QApplication::closeAllWindows();

  QWidget::closeEvent(e);
}

bool ThemeBuilderUI::eventFilter(QObject* o, QEvent* e)
{
  // previewArea
  if ( (o == tabWidget3) && previewWidget && (e->type() == QEvent::Resize) ) {
    // save detached preview area geometry
    if ( tabWidget3->isTopLevel() ) {
      detachedPeviewGeometry = tabWidget3->geometry();
    }
  }

  if ( (o == tabWidget3) && (e->type() == QEvent::Close) ) {
    // detached preview area closed -> re-attach to main window
    slot_detachBtnClicked(false);
  }

  if ( (o == previewWidget) && previewWidget &&
       (e->type() == QEvent::Destroy) ) {
    // previewWidget is about to be destroyed -> clear drawStackTree table
    clearDrawStackTree();
  }

  if ( (o == previewWidget) && previewWidget &&
       (e->type() == QEvent::Paint) ) {
    // previewWidget is about to be repainted -> clear drawStackTree table
    clearDrawStackTree();
    // Grab the time at the beginning of the paint event
    paintTimer.restart();
    // Post a custom "finish paint" event so we can diff time

    sizeLbl->setText(QString("Size: %1x%2").arg(previewWidget->width()).arg(previewWidget->height()));

    QApplication::postEvent(previewWidget, new QEvent(static_cast<QEvent::Type>(QEvent::User+1)));
  }

  if ( (o == previewWidget) && previewWidget &&
       (e->type() == static_cast<QEvent::Type>(QEvent::User+1)) ) {
    // Grab the time at the end of the paint event
    qint64 t = paintTimer.elapsed();
    timeLbl->setText(QString("Time: %1ms").arg(t));
  }

  // always process the event
  return QObject::eventFilter(o, e);
}

void ThemeBuilderUI::clearDrawStackTree()
{
  // foreach(QTreeWidgetItem *i, drawStackTree->findItems("*",Qt::MatchWildcard)) {
  //   delete i;
  // }
  drawStackTree->clear();
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

void ThemeBuilderUI::noteStyleOperation_missingElement(const QString &s)
{
  QTreeWidgetItem *i = new QTreeWidgetItem(currentDrawStackItem);
  i->setText(0,"renderElement");
  i->setText(1,s);
  i->setIcon(0,QIcon(":/icon/pixmaps/debug.png"));
  if ( !currentDrawStackItem )
    drawStackTree->addTopLevelItem(i);
  else
    currentDrawStackItem->setExpanded(true);
}

void ThemeBuilderUI::resetUi()
{
  // Enable/disable some widgets
  toolBox->setEnabled(false);
  tabWidget->setEnabled(false);
  tabWidget2->setTabEnabled(0,false);
  tabWidget2->setTabEnabled(1,false);
  tabWidget3->setEnabled(false);
  saveBtn->setEnabled(false);
  optimizeSvgBtn->setEnabled(false);
  editSvgBtn->setEnabled(false);

  blockUISignals(true);
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
  blockUISignals(false);

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

  // Reset SVG Gen stuff
  setupSvgGenUI();

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
  if ( !tempCfgFile.isEmpty() )
    QFile::remove(tempCfgFile);
  tempCfgFile.clear();
  svgFile.clear();

  setWindowModified(false);
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

void ThemeBuilderUI::schedulePreviewUpdate(bool modified)
{
  timer->stop();

  if ( !previewUpdateEnabled )
    return;

  cfgModified = modified;

  timer->start(500);
}

void ThemeBuilderUI::blockUISignals(bool blocked)
{
  inheritCb->blockSignals(blocked);
  inheritCombo->blockSignals(blocked);

  frameCb->blockSignals(blocked);
  frameIdCb->blockSignals(blocked);
  frameIdCombo->blockSignals(blocked);
  frameWidthCb->blockSignals(blocked);
  frameWidthSpin->blockSignals(blocked);

  interiorCb->blockSignals(blocked);
  interiorIdCb->blockSignals(blocked);
  interiorIdCombo->blockSignals(blocked);
  interiorRepeatCb->blockSignals(blocked);
  interiorRepeatXSpin->blockSignals(blocked);
  interiorRepeatYSpin->blockSignals(blocked);

  indicatorIdCb->blockSignals(blocked);
  indicatorIdCombo->blockSignals(blocked);
  indicatorSizeCb->blockSignals(blocked);
  indicatorSizeSpin->blockSignals(blocked);

  labelMarginCb->blockSignals(blocked);
  labelMarginHSpin->blockSignals(blocked);
  labelMarginVSpin->blockSignals(blocked);
  labelSpacingCb->blockSignals(blocked);
  labelSpacingSpin->blockSignals(blocked);
}

bool ThemeBuilderUI::openTheme(const QString& filename)
{
  resetUi();

  if ( !QFile::exists(filename) ) {
    return false;
  }

  // Get a unique temp filename
  QTemporaryFile f(QDir::tempPath()+"/qsvgthemebuilder_XXXXXX");
  if ( !f.open() ) {
    qWarning() << "[QSvgThemeBuilder]" << "Could not create temporary file";
    return false;
  } else {
    qDebug() << "[QSvgThemeBuilder]" << "Temporary file" << f.fileName() << "created";
  }

  // do not destroy temp file on object destruction
  f.setAutoRemove(false);

  tempCfgFile = f.fileName();

  QFile in(filename);
  in.open(QIODevice::ReadOnly);
  QByteArray data = in.readAll();
  in.close();

  f.write(data);
  f.close();

  cfgFile = filename;

  config = new ThemeConfig(tempCfgFile);
  config->setUseCache(false);

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

  QAction *a = new QAction(recentFiles);
  a->setText(cfgFile);
  a->setData(cfgFile);
  recentFiles->addAction(a);
  while ( recentFiles->actions().count() > 5 ) {
    QAction *a = recentFiles->actions().at(0);
    recentFiles->removeAction(a);
    delete a;
  }
  connect(a,SIGNAL(triggered(bool)), this,SLOT(slot_openRecentTheme()));

  recentBtn->setEnabled(true);

  toolBox->setEnabled(true);
  tabWidget2->setTabEnabled(0,true);
  tabWidget2->setTabEnabled(1,true);
  tabWidget2->setCurrentIndex(0);
  tabWidget3->setEnabled(true);
  themeNameLbl->setText(QFileInfo(cfgFile).fileName());
  themeNameLbl->setToolTip(cfgFile);

  if ( !svgFile.isEmpty() ) {
    editSvgBtn->setEnabled(true);
    optimizeSvgBtn->setEnabled(true);
  }

  // fill in properties form
  theme_spec_t ts = config->getThemeSpec();

  // properties tab
  themeNameEdit->setText(ts.name);
  variantEdit->setText(ts.variant);
  authorEdit->setText(ts.author);
  descrEdit->setText(ts.descr);
  keywordsEdit->setText(ts.keywords);

  // Fill in tweaks tree values
  QTreeWidgetItemIterator it(specificTree, QTreeWidgetItemIterator::Editable);
  while (*it) {
    if ( !(*it)->childCount() ) {
      // set value from config file
      const QVariant v = config->getThemeTweak((*it)->data(1,SpecificSettingName).toString());
      (*it)->setData(1, Qt::EditRole, v);
    }
    ++it;
  }

  // preview tab

  slot_toolboxTabChanged(toolBox->currentIndex());

  return true;
}

void ThemeBuilderUI::saveRecentFiles()
{
  QString dir = QDir::homePath().append("/.config");
  QString filename = QString("%1/QSvgThemeBuilder/qsvgthemebuilder.cfg").arg(dir);

  QSettings f(filename,QSettings::NativeFormat);

  f.beginGroup("General");

  QString s;
  Q_FOREACH(QAction *a, recentFiles->actions()) {
    s.append(a->data().toString()).append(":");
  }

  f.setValue("recentFiles",s);

  f.endGroup();
}

void ThemeBuilderUI::loadRecentFiles()
{
  QString dir = QStandardPaths::locate(QStandardPaths::ConfigLocation,"",QStandardPaths::LocateDirectory);
  QString filename = QString("%1/QSvgThemeBuilder/qsvgthemebuilder.cfg").arg(dir);

  if ( !QFile::exists( filename ) )
    return;

  QSettings f(filename,QSettings::NativeFormat);

  f.beginGroup("General");

  QStringList sl = f.value("recentFiles").toString().split(":",QString::SkipEmptyParts);

  Q_FOREACH(QString s, sl) {
    QAction *a = new QAction(recentFiles);
    a->setText(s);
    a->setData(s);
    recentFiles->addAction(a);
    connect(a,SIGNAL(triggered(bool)), this,SLOT(slot_openRecentTheme()));
  }

  if ( !sl.isEmpty() )
    recentBtn->setEnabled(true);
  else
    recentBtn->setEnabled(false);
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

  qWarning() << "[QSvgThemeBuilder]" << "creating" << basename+".cfg" << "based on default theme";

  if ( !QFile::copy(":default.cfg",basename+".cfg") ) {
    QMessageBox::critical(this, "Error", "Could not create file\n"+basename+".cfg",
                          QMessageBox::Ok);
    return;
  }

  qWarning() << "[QSvgThemeBuilder]" << "creating" << basename+".svg" << "based on default theme";

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

void ThemeBuilderUI::slot_openRecentTheme()
{
  QAction *a = qobject_cast<QAction *> (QObject::sender());
  if ( !a )
    return; /* ??? */

  if ( !ensureSettingsSaved() )
    return;

  QString filename = a->data().toString();
  recentFiles->removeAction(a);
  a->deleteLater();
  if ( recentFiles->actions().isEmpty() )
    recentBtn->setEnabled(false);

  if ( !openTheme(filename) ) {
    QMessageBox::warning(this,"Could not open theme",
                         QString("Could not open theme %1")
                         .arg(filename));
  }
}

void ThemeBuilderUI::slot_saveTheme()
{
  if ( !config || !cfgModified )
    return;

  // We normally don't need this
  saveSettingsFromUi(currentWidget);

  config->commitWriteCache();

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
  setWindowModified(false);
}

void ThemeBuilderUI::slot_editSvg()
{
  if ( svgFile.isEmpty() )
    return;

  QDesktopServices::openUrl(QUrl(svgFile));
}

void ThemeBuilderUI::slot_optimizeSvg()
{
  if ( svgFile.isEmpty() )
    return;

  qint64 oldsize,newsize;

  oldsize = QFile(svgFile).size();

  // get temporary file name
  QTemporaryFile outFile(QDir::tempPath()+"/svg_XXXXXX");
  outFile.open();
  outFile.close();

  qDebug() << outFile.fileName();

  // optimize
  optimizeSvg(svgFile,outFile.fileName());

  newsize = QFile(outFile.fileName()).size();

  // Replace svg file by temp file contents
  svgWatcher.removePath(svgFile);

  QFile in(outFile.fileName());
  in.open(QIODevice::ReadOnly);
  QByteArray data = in.readAll();
  in.close();

  QFile out(svgFile);
  out.open(QIODevice::WriteOnly | QIODevice::Truncate);
  out.write(data);
  out.close();

  svgWatcher.addPath(svgFile);

  statusbar->showMessage(QString("Optimized svg file: %1 -> %2 bytes").arg(oldsize).arg(newsize),
                         10000);

  qDebug() << "[QSvgThemeBuilder]" << "Optimized" << svgFile
    << QString("(%1 -> %2 bytes)").arg(oldsize).arg(newsize);

  slot_svgFileChanged(svgFile);
}

/****************************************************************************
 **
 ** SVG Cleaner is batch, tunable, crossplatform SVG cleaning program.
 ** Copyright (C) 2012-2014 Evgeniy Reizner
 **
 ** This program is free software; you can redistribute it and/or modify
 ** it under the terms of the GNU General Public License as published by
 ** the Free Software Foundation; either version 2 of the License, or
 ** (at your option) any later version.
 **
 ** This program is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 ** GNU General Public License for more details.
 **
 ** You should have received a copy of the GNU General Public License along
 ** with this program; if not, write to the Free Software Foundation, Inc.,
 ** 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 **
 ****************************************************************************/
void ThemeBuilderUI::optimizeSvg(const QString& inPath, const QString& outPath)
{
  XMLDocument doc;
  doc.LoadFile(inPath.toUtf8().constData());
  if (BaseCleaner::svgElement(&doc).isNull()) {
    QFile inputFile(inPath);
    if (!inputFile.open(QIODevice::ReadOnly | QIODevice::Text))
      qFatal("Error: cannot open input file");
    QTextStream inputStream(&inputFile);
    QString svgText = inputStream.readAll();
    svgText.replace("<svg:", "<");
    svgText.replace("</svg:", "</");
    doc.Clear();
    doc.Parse(svgText.toUtf8().constData());
    if (BaseCleaner::svgElement(&doc).isNull())
      qFatal("Error: invalid svg file");
  }

  Replacer replacer(&doc);
  Remover remover(&doc);

//  replacer.calcElemAttrCount("initial");

  // mandatory fixes used to simplify subsequent functions
  replacer.splitStyleAttr();
  // TODO: add key
  replacer.convertCDATAStyle();
  replacer.convertUnits();
  replacer.prepareDefs();
  replacer.fixWrongAttr();
  replacer.markUsedElements();

  remover.cleanSvgElementAttribute();
  if (Keys.flag(Key::CreateViewbox))
    replacer.convertSizeToViewbox();
  if (Keys.flag(Key::RemoveUnusedDefs))
    remover.removeUnusedDefs();
  if (Keys.flag(Key::RemoveDuplicatedDefs))
    remover.removeDuplicatedDefs();
  if (Keys.flag(Key::MergeGradients)) {
    replacer.mergeGradients();
    replacer.mergeGradientsWithEqualStopElem();
  }
  remover.removeElements();
  remover.removeAttributes();
  remover.removeElementsFinal();
  if (Keys.flag(Key::RemoveUnreferencedIds))
    remover.removeUnreferencedIds();
  if (Keys.flag(Key::RemoveUnusedXLinks))
    remover.removeUnusedXLinks();
  remover.cleanPresentationAttributes();
  if (Keys.flag(Key::ApplyTransformsToShapes))
    replacer.applyTransformToShapes();
  if (Keys.flag(Key::RemoveOutsideElements))
    replacer.calcElementsBoundingBox();
  if (Keys.flag(Key::ConvertBasicShapes))
    replacer.convertBasicShapes();
  if (Keys.flag(Key::UngroupContainers)) {
    remover.ungroupAElement();
    remover.removeGroups();
  }
  replacer.processPaths();
  if (Keys.flag(Key::ReplaceEqualEltsByUse))
    replacer.replaceEqualElementsByUse();
  if (Keys.flag(Key::RemoveNotAppliedAttributes))
    replacer.moveStyleFromUsedElemToUse();
  if (Keys.flag(Key::RemoveOutsideElements))
    remover.removeElementsOutsideTheViewbox();
  if (Keys.flag(Key::GroupElemByStyle))
    replacer.groupElementsByStyles();
  if (Keys.flag(Key::ApplyTransformsToDefs))
    replacer.applyTransformToDefs();
  if (Keys.flag(Key::TrimIds))
    replacer.trimIds();
  replacer.roundNumericAttributes();
  // TODO: check only for xmlns:xlink
  remover.cleanSvgElementAttribute();
  if (Keys.flag(Key::SortDefs))
    replacer.sortDefs();
  replacer.finalFixes();
  if (Keys.flag(Key::JoinStyleAttributes))
    replacer.joinStyleAttr();

  QFile outFile(outPath);
  if (!outFile.open(QIODevice::WriteOnly | QIODevice::Text))
    qFatal("Error: could not write output file");

  SVGPrinter printer(0, Keys.flag(Key::CompactOutput));
  doc.Print(&printer);
  outFile.write(printer.CStr());
  outFile.close();
}
/* END */

void ThemeBuilderUI::slot_quit()
{
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
                                          Q_ARG(QString,svgFile));
    setupPreviewForWidget(currentWidget);
  }
}

void ThemeBuilderUI::setupUiForWidget(const QTreeWidgetItem *current)
{
  if ( !config )
    return;

  previewUpdateEnabled = false;
  QString group = current ? current->text(3) : QString();

  if ( current && !group.isEmpty() ) {
    blockUISignals(true);

    tabWidget->setEnabled(true);

    // get spec as exactly set in the config file, without inheritance resolution
    raw_es = config->getRawElementSpec(group);
    // now get the inherited spec
    inherit_es = config->getElementSpec(raw_es.inherits);

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

    if ( raw_es.indicator.size.present ) {
      indicatorSizeCb->setCheckState(Qt::Checked);
    } else {
      indicatorSizeCb->setCheckState(Qt::PartiallyChecked);
    }

    blockUISignals(false);

    // These are needed to force associated widget value updates even
    // if the check state has not changed when switching widgets
    slot_inheritCbChanged(inheritCb->checkState());
    slot_frameCbChanged(frameCb->checkState());
    //slot_frameIdCbChanged(frameIdCb->checkState());
    //slot_frameWidthCbChanged(frameWidthCb->checkState());
    slot_interiorCbChanged(interiorCb->checkState());
    //slot_interiorIdCbChanged(interiorIdCb->checkState());
    //slot_interiorRepeatCbChanged(interiorRepeatCb->checkState());
    slot_labelSpacingCbChanged(labelSpacingCb->checkState());
    slot_labelMarginCbChanged(labelMarginCb->checkState());
    slot_indicatorIdCbChanged(indicatorIdCb->checkState());
    slot_indicatorSizeCbChanged(indicatorSizeCb->checkState());
  } else {
    tabWidget->setEnabled(false);
  }

  previewUpdateEnabled = true;
}

void ThemeBuilderUI::setupPreviewForWidget(const QTreeWidgetItem *current)
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
  QString widget_str;

  if ( !current )
    goto end;

  if ( svgFile.isEmpty() )
    goto end;

  // prepare icon
  icon.addFile(QString::fromUtf8(":/icon/pixmaps/insertimage.png"), QSize(), QIcon::Normal, QIcon::Off);

  // get widget
  widget_str = current->text(2);
  // widget string is empty, likely a sub control of a global one
  if ( widget_str.isEmpty() && current->parent() )
    widget_str = current->parent()->text(2);

  // Push button
  if ( widget_str == CE_str(QStyle::CE_PushButton) ) {
    variants = 5;

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
        widget->setText("This is a flat push button");
        widget->setFlat(true);
        break;
      case 4:
        widget->setText("This one has a menu");
        QMenu *m = new QMenu("popup menu",widget);
        m->addAction("menu item");
        widget->setMenu(m);
        break;
    }

    previewWidget = widget;
  }

  // Tool button
  if ( widget_str == CC_str(QStyle::CC_ToolButton) ) {
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

  if ( widget_str == CE_str(QStyle::CE_RadioButton) ) {
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

  if ( widget_str == CE_str(QStyle::CE_CheckBox) ) {
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

  if ( widget_str == PE_str(QStyle::PE_FrameLineEdit) ) {
    variants = 5;

    QLineEdit *widget = new QLineEdit();
    //widget->setText("This is a line edit");
    widget->setPlaceholderText("type some text here");

    switch ( currentPreviewVariant % variants ) {
      case 0:
        widget->setFrame(true);
        break;
      case 1:
        widget->setFrame(false);
        break;
      case 2:
        widget->setClearButtonEnabled(true);
        break;
      case 3:
        widget->setText("Read Only");
        widget->setReadOnly(true);
        break;
      case 4:
        widget->setEchoMode(QLineEdit::PasswordEchoOnEdit);
        break;
    }

    previewWidget = widget;
  }

  if ( widget_str == CC_str(QStyle::CC_SpinBox) ) {
    variants = 4;

    QSpinBox *widget = new QSpinBox();
    widget->setSuffix(" suffix");
    widget->setPrefix("prefix ");
    widget->setSpecialValueText("Special minimum value");

    switch ( currentPreviewVariant % variants ) {
      case 0:
        widget->setButtonSymbols(QAbstractSpinBox::UpDownArrows);
        widget->setFrame(true);
        break;
      case 1:
        widget->setButtonSymbols(QAbstractSpinBox::PlusMinus);
        widget->setFrame(true);
        break;
      case 2:
        widget->setButtonSymbols(QAbstractSpinBox::NoButtons);
        widget->setFrame(true);
        break;
      case 3:
        widget->setButtonSymbols(QAbstractSpinBox::UpDownArrows);
        widget->setFrame(false);
        break;
    }

    previewWidget = widget;
  }

  if ( widget_str == CC_str(QStyle::CC_ScrollBar) ) {
    variants = 2;

    QScrollBar *widget = new QScrollBar();
    widget->setFocusPolicy(Qt::StrongFocus);

    switch (currentPreviewVariant % variants) {
      case 0:
        widget->setOrientation(Qt::Horizontal);
        break;
      case 1:
        widget->setOrientation(Qt::Vertical);
        break;
    }

    previewWidget = widget;
    qsz = widget->sizePolicy();
  }

  if ( widget_str == CC_str(QStyle::CC_Slider) ) {
    variants = 4;

    QSlider *widget = new QSlider();

    switch (currentPreviewVariant % variants) {
      case 0:
        widget->setOrientation(Qt::Horizontal);
        break;
      case 1:
        widget->setOrientation(Qt::Vertical);
        break;
      case 2:
        widget->setOrientation(Qt::Horizontal);
        widget->setTickPosition(QSlider::TicksBothSides);
        break;
      case 3:
        widget->setOrientation(Qt::Vertical);
        widget->setTickPosition(QSlider::TicksBothSides);
        break;
    }

    previewWidget = widget;
    qsz = widget->sizePolicy();
  }

  if ( widget_str == CC_str(QStyle::CC_Dial) ) {
    variants = 2;

    QDial *widget = new QDial();

    switch (currentPreviewVariant % variants) {
      case 0:
        break;
      case 1:
        widget->setNotchesVisible(true);
        break;
    }

    previewWidget = widget;
    qsz = widget->sizePolicy();
  }

  if ( widget_str == CE_str(QStyle::CE_ProgressBar) ) {
    variants = 4;

    QProgressBar *widget = new QProgressBar();
    widget->setTextVisible(false);

    switch (currentPreviewVariant % variants) {
      case 0:
        break;
      case 1:
        widget->setOrientation(Qt::Vertical);
        break;
      case 2:
        widget->setRange(0,100);
        widget->setValue(25);
        widget->setTextVisible(true);
        break;
      case 3:
        widget->setOrientation(Qt::Vertical);
        widget->setRange(0,100);
        widget->setValue(25);
        widget->setTextVisible(true);
        break;
    }

    previewWidget = widget;
    qsz = widget->sizePolicy();
  }

  if ( widget_str == CC_str(QStyle::CC_GroupBox) ) {
    variants = 6;

    QGroupBox *widget = new QGroupBox();
    widget->setTitle("This is a group box");

    widget->setSizePolicy(QSizePolicy::Expanding, widget->sizePolicy().verticalPolicy());

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

  if ( widget_str == CE_str(QStyle::CE_ToolBoxTab) ) {
    variants = 1;

    QToolBox *widget = new QToolBox();
    widget->setFrameShape(QFrame::QFrame::StyledPanel);
    widget->setFrameShadow(QFrame::QFrame::Sunken);

    widget->addItem(new QLabel("content 1"),icon,"Page 1");
    widget->addItem(new QLabel("content 2"),"Page 2");

    previewWidget = widget;
  }

  if ( widget_str == CE_str(QStyle::CE_TabBarTab) ) {
    variants = 5;

    QTabWidget *widget = new QTabWidget();

    widget->addTab(new QLabel("content 1"),icon,"1st");
    widget->addTab(new QLabel("content 2"),icon,"Middle Tab");
    widget->addTab(new QLabel("content 3"),"Last Tab");
    widget->setTabsClosable(true);

    QToolButton *cornerWidget = new QToolButton();
    cornerWidget->setText("Corner widget");

    widget->setCornerWidget(cornerWidget);

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
      case 4:
        widget->setDocumentMode(true);
        break;
    }

    previewWidget = widget;
    qsz = widget->sizePolicy();
  }

  if ( widget_str == PE_str(QStyle::PE_Frame) ) {
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

  if ( widget_str == CE_str(QStyle::CE_DockWidgetTitle) ) {
    variants = 2;

    QDockWidget *widget = new QDockWidget();
    widget->setWidget(new QLabel("contents", widget));
    widget->setWindowTitle("This is a dock widget");
    widget->setFeatures(widget->features()
      | QDockWidget::DockWidgetClosable
      | QDockWidget::DockWidgetMovable
      | QDockWidget::DockWidgetFloatable);

    switch (currentPreviewVariant % variants) {
      case 0:
        break;
      case 1:
        widget->setFeatures(widget->features() | QDockWidget::DockWidgetVerticalTitleBar);
        break;
    }

    previewWidget = widget;
    qsz = widget->sizePolicy();
  }

  if ( widget_str == CE_str(QStyle::CE_ToolBar) ) {
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

  if ( widget_str == CE_str(QStyle::CE_MenuBarItem) || widget_str == PE_str(QStyle::PE_PanelMenuBar) ) {
    variants = 1;

    QMenuBar *widget = new QMenuBar();
    widget->addAction("Menu1");
    widget->addAction("Menu2");

    previewWidget = widget;
    qsz = QSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);
  }

  if ( widget_str == CE_str(QStyle::CE_MenuItem) || widget_str == PE_str(QStyle::PE_PanelMenu) ) {
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

  if ( widget_str == CE_str(QStyle::CE_Header) ) {
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
    widget->setSortIndicatorShown(true);

    // FIXME size !
    previewWidget = widget;
    widget->resize(widget->minimumSizeHint());
    qsz = QSizePolicy(widget->sizePolicy().horizontalPolicy(),QSizePolicy::Maximum);;
  }

  if ( widget_str == CE_str(QStyle::CE_ItemViewItem) ) {
    variants = 1;

    QTreeWidget *widget = new QTreeWidget();

    QTreeWidgetItem *item1 = new QTreeWidgetItem(widget);
    item1->setText(0,"item1");
    item1->setCheckState(0,Qt::Checked);
    item1->setIcon(0,icon);

    QTreeWidgetItem *subitem1 = new QTreeWidgetItem(item1);
    subitem1->setText(0,"subitem 1");
    QTreeWidgetItem *subitem2 = new QTreeWidgetItem(item1);
    subitem2->setText(0,"subitem 2");
    QTreeWidgetItem *subitem3 = new QTreeWidgetItem(item1);
    subitem3->setText(0,"subitem 3");

    QTreeWidgetItem *item2 = new QTreeWidgetItem(widget);
    item2->setText(0,"item2");
    item2->setIcon(0,icon);

    QTreeWidgetItem *item3 = new QTreeWidgetItem(widget);
    item3->setText(0,"item3");
    item3->setCheckState(0,Qt::Unchecked);

    widget->setAlternatingRowColors(true);
    widget->setHeaderHidden(true);

    previewWidget = widget;
  }

  if ( widget_str == PE_str(QStyle::PE_PanelStatusBar) ) {
    variants = 1;

    QStatusBar *widget = new QStatusBar();
    QLabel *label1 = new QLabel("label1", widget);
    QLabel *label2 = new QLabel("label2", widget);
    QLabel *label3 = new QLabel("label3", widget);

    widget->addWidget(label1);
    widget->addWidget(label2);
    widget->addWidget(label3);

    setStatusBar(widget);
    previewWidget = widget;
  }

  if ( widget_str == PE_str(QStyle::PE_FrameWindow) ) {
    variants = 1;

    QMdiArea *widget = new QMdiArea();

    QFrame *f1 = new QFrame();
    f1->setMinimumHeight(100);
    f1->setWindowIcon(QIcon(":/icon/pixmaps/hint.png"));
    f1->setWindowTitle("Window 1");

    QFrame *f2 = new QFrame();
    f2->setMinimumHeight(100);
    f2->setWindowIcon(QIcon(":/icon/pixmaps/hint.png"));
    f2->setWindowTitle("Window 2");

    widget->addSubWindow(f1,
                         Qt::WindowCloseButtonHint |
                         Qt::WindowShadeButtonHint |
                         Qt::WindowContextHelpButtonHint |
                         Qt::WindowMinMaxButtonsHint |
                         Qt::WindowSystemMenuHint);

    widget->addSubWindow(f2,
                         Qt::WindowCloseButtonHint |
                         Qt::WindowShadeButtonHint |
                         Qt::WindowContextHelpButtonHint |
                         Qt::WindowMinMaxButtonsHint |
                         Qt::WindowSystemMenuHint);

    //QList<QMdiSubWindow *> l = widget->subWindowList();



    previewWidget = widget;
  }

end:
  if ( previewWidget ) {
    previewWidget->setBackgroundRole(previewArea->backgroundRole());
    repaintBtn->setEnabled(true);
    rtlBtn->setEnabled(true);
    drawModeBtn->setEnabled(true);
    fontSizeSpin->setEnabled(true);
    enableBtn->setEnabled(true);
    drawStackTree->setEnabled(true);
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

    if ( !svgFile.isEmpty() ) {
      if ( currentWidget )
        previewWidget = new QLabel("There is no preview available for this element");
      else
        previewWidget = new QLabel("Select an element to see a preview");
    } else {
      previewWidget = new QLabel("Matching SVG file not found. No preview available");
    }

    previewWidget->setSizePolicy(qsz);
    previewArea->setWidget(previewWidget);
    timeLbl->setText("Time: -");
    sizeLbl->setText("Size: -");
  }
}

void ThemeBuilderUI::saveSettingsFromUi(const QTreeWidgetItem *current)
{
  if ( !config )
    return;

  // General options
  theme_spec_t _ts;
  _ts.author = authorEdit->text();
  _ts.variant = variantEdit->text();
  _ts.name = themeNameEdit->text();
  _ts.descr = descrEdit->text();
  _ts.keywords = keywordsEdit->text();

  config->setThemeSpec(_ts);

  // Tweak options
  QTreeWidgetItemIterator it(specificTree, QTreeWidgetItemIterator::Editable);
  while (*it) {
    QTreeWidgetItem *item = (*it);
    if ( !item->data(1,SpecificSettingName).isNull() ) {
      config->setThemeTweak(item->data(1,SpecificSettingName).toString(),
                            item->data(1,Qt::EditRole).toString());
    }
    ++it;
  }

  // Group options
  if ( !current )
    return;

  QString group = current->text(3);

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
  if ( indicatorSizeCb->checkState() == Qt::Checked )
    _es.indicator.size = indicatorSizeSpin->value();
  if ( indicatorSizeCb->checkState() == Qt::Unchecked )
    _es.indicator.size = 7;

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

  // re-read specs and setup UI again when inheritCombo changes
  if ( _es.inherits != raw_es.inherits ) {
    setupUiForWidget(current);
  }
}

void ThemeBuilderUI::slot_uiSettingsChanged()
{
  timer->stop();

  if ( cfgModified ) {
    saveBtn->setEnabled(true);
    setWindowModified(true);

    saveSettingsFromUi(currentWidget);
  }

  if ( previewWidget ) {
    // HACK: this will force geometry recalculation
    slot_fontSizeChanged(previewWidget->font().pointSize()-1);
    slot_fontSizeChanged(previewWidget->font().pointSize()+1);
  }
}

void ThemeBuilderUI::slot_repaintBtnClicked(bool checked)
{
  Q_UNUSED(checked);

  if ( previewWidget )
    previewWidget->update();
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
    previewWidget->update();
}

void ThemeBuilderUI::slot_detachBtnClicked(bool checked)
{
  Q_UNUSED(checked);

  QIcon icon;

  if ( !tabWidget3->isTopLevel() ) {
    icon.addFile(QString::fromUtf8(":/icon/pixmaps/dockwidget.png"), QSize(), QIcon::Normal, QIcon::Off);
    detachBtn->setIcon(icon);
    detachBtn->setText("Attach");
    //previewArea->setParent(NULL);
    tabWidget3->setParent(NULL);
    tabWidget3->show();
    if ( detachedPeviewGeometry.isValid() )
      tabWidget3->setGeometry(detachedPeviewGeometry);
    tabWidget3->window()->setWindowTitle("QSvgThemeBuilder preview");
  } else {
    icon.addFile(QString::fromUtf8(":/icon/pixmaps/widget.png"), QSize(), QIcon::Normal, QIcon::Off);
    detachBtn->setIcon(icon);
    detachBtn->setText("Detach");
    detachedPeviewGeometry = tabWidget3->geometry();
    gridLayout->addWidget(tabWidget3, 2, 2, 1, 1);
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

void ThemeBuilderUI::slot_paletteChanged(int val)
{
  QStyle::staticMetaObject.invokeMethod(style,"loadPalette",
                                        Qt::DirectConnection,
                                        Q_ARG(QString,paletteCombo->itemText(val)));

  schedulePreviewUpdate(0);
}

void ThemeBuilderUI::slot_authorEditChanged(const QString& text)
{
  Q_UNUSED(text);

  schedulePreviewUpdate();
}

void ThemeBuilderUI::slot_variantEditChanged(const QString& text)
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

void ThemeBuilderUI::slot_keywordsEditChanged(const QString& text)
{
  Q_UNUSED(text);

  schedulePreviewUpdate();
}

void ThemeBuilderUI::slot_specificChanged(QTreeWidgetItem *item, int column)
{
  Q_UNUSED(item);
  Q_UNUSED(column);

  schedulePreviewUpdate();
}

void ThemeBuilderUI::slot_genFrameBtnClicked(bool checked)
{
  svgGen->setHasFrame(checked);
  genFramePage->setEnabled(checked);
  genToolbox->repaint();
  genInteriorRoundnessSpin->setEnabled(!checked);

  Q_FOREACH(GenSubFramePropUI *w, genSubFrameProps)
    setupSubFramePropsUI(w);
}

void ThemeBuilderUI::slot_genInteriorBtnClicked(bool checked)
{
  svgGen->setHasInterior(checked);
  svgGen->setInteriorRoundness(genInteriorRoundnessSpin->value());
  genInteriorPage->setEnabled(checked);
  genToolbox->repaint();
}

void ThemeBuilderUI::slot_genShadowBtnClicked(bool checked)
{
  svgGen->setHasShadow(checked);
  genShadowPage->setEnabled(checked);
  genToolbox->repaint();
}

void ThemeBuilderUI::slot_genRoundBtnClicked(bool checked)
{
  svgGen->setRoundMode(checked);
}

void ThemeBuilderUI::slot_genSplitBtnClicked(bool checked)
{
  svgGen->setSplitMode(checked);
}

void ThemeBuilderUI::slot_genFrameTopBtnClicked(bool checked)
{
  svgGen->setHasTopFrame(checked);
  genShadowTopBtn->setEnabled(checked);
}

void ThemeBuilderUI::slot_genFrameBottomBtnClicked(bool checked)
{
  svgGen->setHasBottomFrame(checked);
  genShadowBottomBtn->setEnabled(checked);
}

void ThemeBuilderUI::slot_genFrameLeftBtnClicked(bool checked)
{
  svgGen->setHasLeftFrame(checked);
  genShadowLeftBtn->setEnabled(checked);
}

void ThemeBuilderUI::slot_genFrameRightBtnClicked(bool checked)
{
  svgGen->setHasRightFrame(checked);
  genShadowRightBtn->setEnabled(checked);
}

void ThemeBuilderUI::slot_genFrameWidthChanged(int val)
{
  svgGen->setFrameWidth(val);

  int n = genSubFrameProps.count();

  if ( val > n ) {
    QGridLayout *l = qobject_cast<QGridLayout *> (genSubFrameWidthsScrollContents->layout());
    if ( !l )
      return;

    while ( n++ < val ) {
      GenSubFramePropUI *w = new GenSubFramePropUI(genSubFrameWidthsScrollContents);
      l->addWidget(w,n-1,0);

      // add
      genSubFrameProps.append(w);

      // setup
      setupSubFramePropsUI(w);

      w->show();

      genFrameWidthSpin->setFixedWidth(w->subFrameWidthSpin->width());

      connect(w->subFrameWidthSpin,SIGNAL(valueChanged(qreal)),
              this,SLOT(slot_genSubFrameWidthChanged(qreal)));
      connect(w->subFrameFillTypeBtn,SIGNAL(clicked(bool)),
              this,SLOT(slot_genSubFrameFillTypeBtnClicked(bool)));
      connect(w->subFrameColor1Btn,SIGNAL(clicked(bool)),
              this,SLOT(slot_genSubFrameColor1BtnClicked(bool)));
      connect(w->subFrameColor2Btn,SIGNAL(clicked(bool)),
              this,SLOT(slot_genSubFrameColor2BtnClicked(bool)));
    }
  } else {
    while ( n-- > val ) {
      GenSubFramePropUI *w = genSubFrameProps.takeLast();
      delete w;
    }
  }

  // determine max width for subframe prop UIs
  int maxW = 0;
  Q_FOREACH (QObject *o, genSubFrameWidthsScrollContents->children()) {
    QWidget *w = qobject_cast<QWidget *>(o);
    if (w) {
      maxW = qMax(maxW,w->sizeHint().width());
    }
  }

  // add always visible V scroll ber
  maxW += genToolbox->style()->pixelMetric(QStyle::PM_ScrollBarExtent,0,0);

  genSubFrameWidthsScroll->setFixedWidth(maxW); // OK
  // align for beauty
  genFrameSpacer->changeSize(
        genToolbox->style()->pixelMetric(QStyle::PM_ScrollBarExtent,0,0),
        0,
        QSizePolicy::Fixed);

  // set size of toolbox
  genToolbox->setFixedWidth(maxW+genFramePage->layout()->contentsMargins().left()
                            +genFramePage->layout()->contentsMargins().right()
                            +genToolbox->contentsMargins().left()
                            +genToolbox->contentsMargins().right()); // OK
}

void ThemeBuilderUI::slot_genSubFrameWidthChanged(qreal val)
{
  QDoubleSpinBox *s = qobject_cast<QDoubleSpinBox *>(sender());
  GenSubFramePropUI *w = qobject_cast<GenSubFramePropUI *>(s->parentWidget());
  int n = genSubFrameProps.indexOf(w);
  if ( n < 0 )
    return;
  svgGen->setSubFrameWidth(n, val);
}

void ThemeBuilderUI::slot_genCopyBtnClicked(bool checked)
{
  Q_UNUSED(checked);

  QByteArray data(svgGen->toSvg().toString().toLatin1());

  QMimeData *m = new QMimeData();
  m->setData("image/svg+xml", data);

  qApp->clipboard()->setMimeData(m, QClipboard::Clipboard);

  statusbar->showMessage("SVG copied to clipboard !", 10000);
}

void ThemeBuilderUI::slot_genBasenameChanged(const QString &text)
{
  svgGen->setBaseName(text);
}

void ThemeBuilderUI::slot_genVariantChanged(const QString &text)
{
  svgGen->setVariant(text);
}

void ThemeBuilderUI::slot_genStatusChanged(const QString &text)
{
  svgGen->setStatus(text);
}

void ThemeBuilderUI::slot_genInteriorRoundnessChanged(qreal val)
{
  svgGen->setInteriorRoundness(val);
}

void ThemeBuilderUI::slot_genSquareBtnClicked(bool checked)
{
  if ( checked )
    svgGen->setSize(QSizeF(100,100));
  else
    svgGen->setSize(QSizeF(200,100));
}

void ThemeBuilderUI::slot_genSubFrameFillTypeBtnClicked(bool checked)
{
  Q_UNUSED(checked);

  QToolButton *b = qobject_cast<QToolButton *>(sender());
  GenSubFramePropUI *w = qobject_cast<GenSubFramePropUI *>(b->parentWidget());
  int n = genSubFrameProps.indexOf(w);
  if ( n < 0 )
    return;

  SvgGenSubFrame::FillType type = svgGen->subFrameFillType(n);

  type = static_cast<SvgGenSubFrame::FillType> (static_cast<int>(type)+1);
  if ( type > SvgGenSubFrame::FillTypeMax )
    type = SvgGenSubFrame::FillTypeFirst;

  svgGen->setSubFrameFillType(n, type);

  switch (type) {
    case SvgGenSubFrame::FillTypeFlat:
      b->setText("F");
      break;
    case SvgGenSubFrame::FillTypeGradient:
      b->setText("G");
      break;
    case SvgGenSubFrame::FillTypeInvertedGradient:
      b->setText("IG");
      break;
  }

  w->subFrameColor2Btn->setEnabled(type != SvgGenSubFrame::FillTypeFlat);
}

void ThemeBuilderUI::slot_genSubFrameColor1BtnClicked(bool checked)
{
  Q_UNUSED(checked);

  QToolButton *b = qobject_cast<QToolButton *>(sender());
  GenSubFramePropUI *w = qobject_cast<GenSubFramePropUI *>(b->parentWidget());
  int n = genSubFrameProps.indexOf(w);
  if ( n < 0 )
    return;

  QColor c = QColorDialog::getColor(svgGen->subFrameFirstColor(n),
                                    NULL,
                                    QString("Select color"),
                                    QColorDialog::ShowAlphaChannel);

  if ( !c.isValid() )
    c = svgGen->subFrameFirstColor(n);
  svgGen->setSubFrameFirstColor(n, c);
  QPalette p = b->palette();
  QPalette::ColorRole role = b->backgroundRole();
  p.setColor(role, c);
  b->setPalette(p);
  b->setToolTip(QString("Color1: %1").arg(c.name(QColor::HexArgb)));
}

void ThemeBuilderUI::slot_genSubFrameColor2BtnClicked(bool checked)
{
  Q_UNUSED(checked);

  QToolButton *b = qobject_cast<QToolButton *>(sender());
  GenSubFramePropUI *w = qobject_cast<GenSubFramePropUI *>(b->parentWidget());
  int n = genSubFrameProps.indexOf(w);
  if ( n < 0 )
    return;

  QColor c = QColorDialog::getColor(svgGen->subFrameSecondColor(n),
                                    NULL,
                                    QString("Select color"),
                                    QColorDialog::ShowAlphaChannel);

  if ( !c.isValid() )
    c = svgGen->subFrameSecondColor(n);
  svgGen->setSubFrameSecondColor(n, c);
  QPalette p = b->palette();
  QPalette::ColorRole role = b->backgroundRole();
  p.setColor(role, c);
  b->setPalette(p);
  b->setToolTip(QString("Color1: %1").arg(c.name(QColor::HexArgb)));
}

void ThemeBuilderUI::slot_genInteriorFillTypeBtnClicked(bool checked)
{
  Q_UNUSED(checked);

  QToolButton *b = qobject_cast<QToolButton *>(sender());

  SvgGenInterior::FillType type = svgGen->interiorFillType();

  type = static_cast<SvgGenInterior::FillType> (static_cast<int>(type)+1);
  if ( type > SvgGenInterior::FillTypeMax )
    type = SvgGenInterior::FillTypeFirst;

  svgGen->setInteriorFillType(type);

  switch (type) {
    case SvgGenInterior::FillTypeFlat:
      b->setText("F");
      break;
    case SvgGenInterior::FillTypeGradient:
      b->setText("G");
      break;
    case SvgGenInterior::FillTypeInvertedGradient:
      b->setText("IG");
      break;
  }

  interiorColor2Btn->setEnabled(type != SvgGenInterior::FillTypeFlat);
}

void ThemeBuilderUI::slot_genInteriorColor1BtnClicked(bool checked)
{
  Q_UNUSED(checked);

  QToolButton *b = qobject_cast<QToolButton *>(sender());

  QColor c = QColorDialog::getColor(svgGen->interiorFirstColor(),
                                    NULL,
                                    QString("Select color"),
                                    QColorDialog::ShowAlphaChannel);

  if ( !c.isValid() )
    c = svgGen->interiorFirstColor();
  svgGen->setInteriorFirstColor(c);
  QPalette p = b->palette();
  QPalette::ColorRole role = b->backgroundRole();
  p.setColor(role, c);
  b->setPalette(p);
  b->setToolTip(QString("Color1: %1").arg(c.name(QColor::HexArgb)));
}

void ThemeBuilderUI::slot_genInteriorColor2BtnClicked(bool checked)
{
  Q_UNUSED(checked);

  QToolButton *b = qobject_cast<QToolButton *>(sender());

  QColor c = QColorDialog::getColor(svgGen->interiorSecondColor(),
                                    NULL,
                                    QString("Select color"),
                                    QColorDialog::ShowAlphaChannel);

  if ( !c.isValid() )
    c = svgGen->interiorSecondColor();
  svgGen->setInteriorSecondColor(c);
  QPalette p = b->palette();
  QPalette::ColorRole role = b->backgroundRole();
  p.setColor(role, c);
  b->setPalette(p);
  b->setToolTip(QString("Color1: %1").arg(c.name(QColor::HexArgb)));
}

void ThemeBuilderUI::slot_genShadowFillTypeBtnClicked(bool checked)
{
  Q_UNUSED(checked);

  QToolButton *b = qobject_cast<QToolButton *>(sender());

  SvgGenSubFrame::FillType type = svgGen->shadowFillType();

  type = static_cast<SvgGenSubFrame::FillType> (static_cast<int>(type)+1);
  if ( type > SvgGenSubFrame::FillTypeMax )
    type = SvgGenSubFrame::FillTypeFirst;

  svgGen->setShadowFillType(type);

  switch (type) {
    case SvgGenSubFrame::FillTypeFlat:
      b->setText("F");
      break;
    case SvgGenSubFrame::FillTypeGradient:
      b->setText("G");
      break;
    case SvgGenSubFrame::FillTypeInvertedGradient:
      b->setText("IG");
      break;
  }

  shadowColor2Btn->setEnabled(type != SvgGenSubFrame::FillTypeFlat);
}

void ThemeBuilderUI::slot_genShadowColor1BtnClicked(bool checked)
{
  Q_UNUSED(checked);

  QToolButton *b = qobject_cast<QToolButton *>(sender());

  QColor c = QColorDialog::getColor(svgGen->shadowFirstColor(),
                                    NULL,
                                    QString("Select color"),
                                    QColorDialog::ShowAlphaChannel);

  if ( !c.isValid() )
    c = svgGen->shadowFirstColor();
  svgGen->setShadowFirstColor(c);
  QPalette p = b->palette();
  QPalette::ColorRole role = b->backgroundRole();
  p.setColor(role, c);
  b->setPalette(p);
  b->setToolTip(QString("Color1: %1").arg(c.name(QColor::HexArgb)));
}

void ThemeBuilderUI::slot_genShadowColor2BtnClicked(bool checked)
{
  Q_UNUSED(checked);

  QToolButton *b = qobject_cast<QToolButton *>(sender());

  QColor c = QColorDialog::getColor(svgGen->shadowSecondColor(),
                                    NULL,
                                    QString("Select color"),
                                    QColorDialog::ShowAlphaChannel);

  if ( !c.isValid() )
    c = svgGen->shadowSecondColor();
  svgGen->setShadowSecondColor(c);
  QPalette p = b->palette();
  QPalette::ColorRole role = b->backgroundRole();
  p.setColor(role, c);
  b->setPalette(p);
  b->setToolTip(QString("Color1: %1").arg(c.name(QColor::HexArgb)));
}

void ThemeBuilderUI::slot_genShadowWidthChanged(qreal val)
{
  svgGen->setShadowWidth(val);
}

void ThemeBuilderUI::slot_genShadowTopBtnClicked(bool checked)
{
  svgGen->setHasTopShadow(checked);
}

void ThemeBuilderUI::slot_genShadowBottomBtnClicked(bool checked)
{
  svgGen->setHasBottomShadow(checked);
}

void ThemeBuilderUI::slot_genShadowLeftBtnClicked(bool checked)
{
  svgGen->setHasLeftShadow(checked);
}

void ThemeBuilderUI::slot_genShadowRightBtnClicked(bool checked)
{
  svgGen->setHasRightShadow(checked);
}

void ThemeBuilderUI::setupSvgGenUI()
{
  genFrameBtn->setChecked(svgGen->hasFrame());
  genInteriorBtn->setChecked(svgGen->hasInterior());
  genInteriorRoundnessSpin->setEnabled(!svgGen->hasFrame());
  genShadowBtn->setChecked(svgGen->hasShadow());

  slot_genFrameBtnClicked(svgGen->hasFrame());
  slot_genInteriorBtnClicked(svgGen->hasInterior());
  slot_genShadowBtnClicked(svgGen->hasShadow());

  genSquareBtn->setChecked(svgGen->isSquare());
  genRoundBtn->setChecked(svgGen->roundMode());
  genSplitBtn->setChecked(svgGen->splitMode());

  genBasenameEdit->setText(svgGen->basename());
  genVariantEdit->setText(svgGen->variant());
  genStatusCombo->setCurrentText(svgGen->status());

  genFrameWidthSpin->setValue(svgGen->frameWidth());
  slot_genFrameWidthChanged(svgGen->frameWidth());

  genFrameTopBtn->setChecked(svgGen->hasTopFrame());
  genFrameBottomBtn->setChecked(svgGen->hasBottomFrame());
  genFrameLeftBtn->setChecked(svgGen->hasLeftFrame());
  genFrameRightBtn->setChecked(svgGen->hasRightFrame());

  setupInteriorPropsUI();
  setupShadowPropsUI();
}

void ThemeBuilderUI::setupSubFramePropsUI(GenSubFramePropUI *w)
{
  int idx = genSubFrameProps.indexOf(w);
  if ( idx < 0 )
    return;

  w->subFrameNoLbl->setText(QString("%1").arg(idx+1));
  w->subFrameWidthSpin->setValue(svgGen->subFrameWidth(idx));
  w->subFrameNoLbl->setText(QString("%1").arg(idx));
  w->subFrameNoLbl->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
  QPalette p = w->subFrameColor1Btn->palette();
  QPalette::ColorRole role = w->subFrameColor1Btn->backgroundRole();
  QColor c = svgGen->subFrameFirstColor(idx);
  p.setColor(role, c);
  w->subFrameColor1Btn->setPalette(p);
  w->subFrameColor1Btn->setToolTip(QString("Color1: %1")
                                   .arg(c.name(QColor::HexArgb)));
  p = w->subFrameColor2Btn->palette();
  role = w->subFrameColor2Btn->backgroundRole();
  c = svgGen->subFrameSecondColor(idx);
  p.setColor(role, c);
  w->subFrameColor2Btn->setToolTip(QString("Color2: %1")
                                   .arg(c.name(QColor::HexArgb)));
  w->subFrameColor2Btn->setPalette(p);
  SvgGenSubFrame::FillType ft = svgGen->subFrameFillType(idx);
  switch (ft) {
    case SvgGenSubFrame::FillTypeFlat:
      w->subFrameFillTypeBtn->setText("F");
      break;
    case SvgGenSubFrame::FillTypeGradient:
      w->subFrameFillTypeBtn->setText("G");
      break;
    case SvgGenSubFrame::FillTypeInvertedGradient:
      w->subFrameFillTypeBtn->setText("IG");
      break;
  }

  w->subFrameColor2Btn->setEnabled(
        svgGen->subFrameFillType(idx) != SvgGenSubFrame::FillTypeFlat);
}

void ThemeBuilderUI::setupInteriorPropsUI()
{
  QPalette p = interiorColor1Btn->palette();
  QPalette::ColorRole role = interiorColor1Btn->backgroundRole();
  QColor c = svgGen->interiorFirstColor();
  p.setColor(role, c);
  interiorColor1Btn->setPalette(p);
  interiorColor1Btn->setToolTip(QString("Color1: %1")
                                   .arg(c.name(QColor::HexArgb)));
  p = interiorColor2Btn->palette();
  role = interiorColor2Btn->backgroundRole();
  c = svgGen->interiorSecondColor();
  p.setColor(role, c);
  interiorColor2Btn->setToolTip(QString("Color2: %1")
                                   .arg(c.name(QColor::HexArgb)));
  interiorColor2Btn->setPalette(p);
  SvgGenInterior::FillType ft = svgGen->interiorFillType();
  switch (ft) {
    case SvgGenInterior::FillTypeFlat:
      interiorFillTypeBtn->setText("F");
      break;
    case SvgGenInterior::FillTypeGradient:
      interiorFillTypeBtn->setText("G");
      break;
    case SvgGenInterior::FillTypeInvertedGradient:
      interiorFillTypeBtn->setText("IG");
      break;
  }

  interiorColor2Btn->setEnabled(
        svgGen->interiorFillType() != SvgGenInterior::FillTypeFlat);

  genInteriorRoundnessSpin->setValue(svgGen->interiorRoundness());
}

void ThemeBuilderUI::setupShadowPropsUI()
{
  genShadowTopBtn->setEnabled(svgGen->hasTopFrame());
  genShadowTopBtn->setChecked(svgGen->hasTopShadow());
  genShadowBottomBtn->setEnabled(svgGen->hasBottomFrame());
  genShadowBottomBtn->setChecked(svgGen->hasBottomShadow());
  genShadowLeftBtn->setEnabled(svgGen->hasLeftFrame());
  genShadowLeftBtn->setChecked(svgGen->hasLeftShadow());
  genShadowRightBtn->setEnabled(svgGen->hasRightFrame());
  genShadowRightBtn->setChecked(svgGen->hasRightShadow());

  QPalette p = shadowColor1Btn->palette();
  QPalette::ColorRole role = shadowColor1Btn->backgroundRole();
  QColor c = svgGen->shadowFirstColor();
  p.setColor(role, c);
  shadowColor1Btn->setPalette(p);
  shadowColor1Btn->setToolTip(QString("Color1: %1")
                                   .arg(c.name(QColor::HexArgb)));
  p = shadowColor2Btn->palette();
  role = shadowColor2Btn->backgroundRole();
  c = svgGen->shadowSecondColor();
  p.setColor(role, c);
  shadowColor2Btn->setToolTip(QString("Color2: %1")
                                   .arg(c.name(QColor::HexArgb)));
  shadowColor2Btn->setPalette(p);
  SvgGenSubFrame::FillType ft = svgGen->shadowFillType();
  switch (ft) {
    case SvgGenSubFrame::FillTypeFlat:
      shadowFillTypeBtn->setText("F");
      break;
    case SvgGenSubFrame::FillTypeGradient:
      shadowFillTypeBtn->setText("G");
      break;
    case SvgGenSubFrame::FillTypeInvertedGradient:
      shadowFillTypeBtn->setText("IG");
      break;
  }

  shadowColor2Btn->setEnabled(
        svgGen->shadowFillType() != SvgGenSubFrame::FillTypeFlat);

  genShadowWidthSpin->setValue(svgGen->shadowWidth());
}

void ThemeBuilderUI::slot_inheritCbChanged(int state)
{
  blockUISignals(true);

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

  blockUISignals(false);
  schedulePreviewUpdate();
}

void ThemeBuilderUI::slot_inheritComboChanged(int idx)
{
  Q_UNUSED(idx);

  schedulePreviewUpdate();
}

void ThemeBuilderUI::slot_frameCbChanged(int state)
{
  blockSignals(true);

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
    frameEdit->setText(QString("=%1").arg(inherit_es.frame.hasFrame ? "yes" : "no"));
  }
  if ( state == Qt::Unchecked ) {
    frameEdit->setEnabled(false);
    frameEdit->setText("<no>");
  }

  blockSignals(false);

  slot_frameIdCbChanged(frameIdCb->checkState());
  slot_frameWidthCbChanged(frameWidthCb->checkState());

  schedulePreviewUpdate();
}

void ThemeBuilderUI::slot_frameIdCbChanged(int state)
{
  blockSignals(true);

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
    frameIdCombo->setEditText(QString("=%1").arg(inherit_es.frame.element));
  }
  if ( state == Qt::Unchecked ) {
    frameIdCombo->setEditText("<none>");
  }

  blockSignals(false);
  schedulePreviewUpdate();
}

void ThemeBuilderUI::slot_frameIdComboChanged(const QString& text)
{
  Q_UNUSED(text);

  schedulePreviewUpdate();
}

void ThemeBuilderUI::slot_frameWidthCbChanged(int state)
{
  blockUISignals(true);

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
    frameWidthSpin->setSpecialValueText(QString("=%1").arg(inherit_es.frame.width));
  }
  if ( state == Qt::Unchecked ) {
    frameWidthSpin->setSpecialValueText("<none>");
  }

  blockUISignals(false);
  schedulePreviewUpdate();
}

void ThemeBuilderUI::slot_frameWidthSpinChanged(int val)
{
  Q_UNUSED(val);

  schedulePreviewUpdate();
}

void ThemeBuilderUI::slot_interiorCbChanged(int state)
{
  blockUISignals(true);

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
    interiorEdit->setText(QString("=%1").arg(inherit_es.interior.hasInterior ? "yes" : "no"));
  }
  if ( state == Qt::Unchecked ) {
    interiorEdit->setEnabled(false);
    interiorEdit->setText("<no>");
  }

  blockUISignals(false);

  slot_interiorIdCbChanged(interiorIdCb->checkState());
  slot_interiorRepeatCbChanged(interiorRepeatCb->checkState());

  schedulePreviewUpdate();
}

void ThemeBuilderUI::slot_interiorIdCbChanged(int state)
{
  blockSignals(true);

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
    interiorIdCombo->setEditText(QString("=%1").arg(inherit_es.interior.element));
  }
  if ( state == Qt::Unchecked ) {
    interiorIdCombo->setEditText("<none>");
  }

  blockUISignals(false);
  schedulePreviewUpdate();
}

void ThemeBuilderUI::slot_interiorIdComboChanged(const QString &text)
{
  Q_UNUSED(text);

  schedulePreviewUpdate();
}

void ThemeBuilderUI::slot_interiorRepeatCbChanged(int state)
{
  blockUISignals(true);

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
    if ( inherit_es.interior.px > 0 )
      interiorRepeatXSpin->setSpecialValueText(QString("=%1").arg(inherit_es.interior.px));
    else
      interiorRepeatXSpin->setSpecialValueText("=none");

    if ( inherit_es.interior.py > 0 )
      interiorRepeatYSpin->setSpecialValueText(QString("=%1").arg(inherit_es.interior.py));
    else
      interiorRepeatYSpin->setSpecialValueText("=none");
  }
  if ( state == Qt::Unchecked ) {
    interiorRepeatXSpin->setSpecialValueText("<none>");
    interiorRepeatYSpin->setSpecialValueText("<none>");
  }

  blockUISignals(false);
  schedulePreviewUpdate();
}

void ThemeBuilderUI::slot_interiorRepeatXSpinChanged(int val)
{
  Q_UNUSED(val);

  schedulePreviewUpdate();
}

void ThemeBuilderUI::slot_interiorRepeatYSpinChanged(int val)
{
  Q_UNUSED(val);

  schedulePreviewUpdate();
}

void ThemeBuilderUI::slot_labelSpacingCbChanged(int state)
{
  blockUISignals(true);

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
    labelSpacingSpin->setSpecialValueText(QString("=%1").arg(inherit_es.label.tispace));
  }
  if ( state == Qt::Unchecked ) {
    labelSpacingSpin->setSpecialValueText("<none>");
  }

  blockUISignals(false);
  schedulePreviewUpdate();
}

void ThemeBuilderUI::slot_labelSpacingSpinChanged(int val)
{
  Q_UNUSED(val);

  schedulePreviewUpdate();
}

void ThemeBuilderUI::slot_labelMarginCbChanged(int state)
{
  blockUISignals(true);

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
    labelMarginHSpin->setSpecialValueText(QString("=%1").arg(inherit_es.label.hmargin));
    labelMarginVSpin->setSpecialValueText(QString("=%1").arg(inherit_es.label.vmargin));
  }
  if ( state == Qt::Unchecked ) {
    labelMarginHSpin->setSpecialValueText("<none>");
    labelMarginVSpin->setSpecialValueText("<none>");
  }

  blockUISignals(false);
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
  blockUISignals(true);

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
    indicatorIdCombo->setEditText(QString("=%1").arg(inherit_es.indicator.element));
  }
  if ( state == Qt::Unchecked ) {
    indicatorIdCombo->setEditText("<none>");
  }

  blockUISignals(false);
  schedulePreviewUpdate();
}

void ThemeBuilderUI::slot_indicatorIdComboChanged(const QString& text)
{
  Q_UNUSED(text);

  schedulePreviewUpdate();
}

void ThemeBuilderUI::slot_indicatorSizeCbChanged(int state)
{
  blockUISignals(true);

  if ( state == Qt::Checked ) {
    indicatorSizeSpin->setEnabled(indicatorSizeCb->isChecked());
    indicatorSizeSpin->setSpecialValueText(QString());
    indicatorSizeSpin->setMinimum(1);
    indicatorSizeSpin->setValue(raw_es.indicator.size);
  } else {
    indicatorSizeSpin->setEnabled(false);
    indicatorSizeSpin->setMinimum(-1);
    indicatorSizeSpin->setValue(indicatorSizeSpin->minimum());
  }
  if ( state == Qt::PartiallyChecked ) {
    indicatorSizeSpin->setSpecialValueText(QString("=%1").arg(inherit_es.indicator.size));
  }
  if ( state == Qt::Unchecked ) {
    indicatorSizeSpin->setSpecialValueText("<none>");
  }

  blockUISignals(false);
  schedulePreviewUpdate();
}

void ThemeBuilderUI::slot_indicatorSizeSpinChanged(int val)
{
  Q_UNUSED(val);

  schedulePreviewUpdate();
}

void ThemeBuilderUI::slot_toolboxTabChanged(int index)
{
  QTreeWidgetItem *previous = 0, *current = 0;

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
    default:
      break;
  }

//   if ( current )
//     current->listWidget()->setFocus(Qt::ActiveWindowFocusReason);

  slot_widgetChanged(current,previous);
}

void ThemeBuilderUI::slot_widgetChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous)
{
  saveSettingsFromUi(currentWidget);

  currentWidget = current;

  setupUiForWidget(currentWidget);

  if ( previous &&
       current &&
       (
         (current->parent() == previous)  || // selected child of previous
         (current == previous->parent()) || // selected parent of previous
         ( (current->parent() == previous->parent()) && // selected sibling
           current->parent() // except if root children
         )
       )
     )
    ; // nothing -> same widget
  else {
    currentPreviewVariant = 0;
    setupPreviewForWidget(currentWidget);
  }
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

void ThemeBuilderUI::slot_missingElement(const QString &s)
{
  noteStyleOperation_missingElement(s);
}
