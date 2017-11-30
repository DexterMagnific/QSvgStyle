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
#include "ThemeManagerUI.h"

#include <QDebug>
#include <QSettings>
#include <QMetaObject>
#include <QDir>
#include <QLibraryInfo>
#include <QStyleFactory>
#include <QMessageBox>
#include <QTemporaryFile>
#include <QTimer>
#include <QCloseEvent>

#if HAVE_X11
#include <X11/Xlib.h>
#include <QtX11Extras/QX11Info>
#include <xcb/xcb.h>
#endif

#include "ThemeConfig.h"
#include "PaletteConfig.h"
#include "StyleConfig.h"
#include "../style/QSvgThemableStyle.h"

SpinBoxDelegate::SpinBoxDelegate(QObject *parent)
  : QStyledItemDelegate(parent)
{
}

QWidget *SpinBoxDelegate::createEditor(QWidget *parent,
                                       const QStyleOptionViewItem &/* option */,
                                       const QModelIndex &index) const
{
  if (index.column() != 0) {
    QSpinBox *editor = new QSpinBox(parent);
    editor->setMinimum(0);
    editor->setMaximum(99);

    // update model without waiting the close of the editor
    connect(editor,SIGNAL(valueChanged(int)),
            this,SLOT(slot_valueChanged(int)));

    return editor;
  } else
    return NULL;
}

void SpinBoxDelegate::setEditorData(QWidget *editor,
                                    const QModelIndex &index) const
{
  int value = index.model()->data(index, Qt::EditRole).toInt();

  QSpinBox *spinBox = static_cast<QSpinBox*>(editor);
  spinBox->setValue(value);
}

void SpinBoxDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                                   const QModelIndex &index) const
{
  QSpinBox *spinBox = static_cast<QSpinBox*>(editor);
  spinBox->interpretText();
  int value = spinBox->value();

  model->setData(index, value, Qt::EditRole);
}

void SpinBoxDelegate::updateEditorGeometry(QWidget *editor,
                                           const QStyleOptionViewItem &option,
                                           const QModelIndex &/* index */) const
{
  editor->setGeometry(option.rect);
}

void SpinBoxDelegate::slot_valueChanged(int val)
{
  Q_UNUSED(val);
  emit commitData(static_cast<QSpinBox *>(sender()));
}

ThemeManagerUI::ThemeManagerUI(QWidget* parent)
  : QMainWindow(parent),
    style(NULL),
    previewColorGroup(QPalette::Active),
    config(NULL),
    cfgModified(false),
    timer(NULL)
{
  // Setup using auto-generated UIC code
  setupUi(this);

  // Further setup for specific tree: move item text to item data
  QTreeWidgetItemIterator it(specificTree);
  while (*it) {
    if ( !(*it)->childCount() ) { // leaf
      (*it)->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEditable|Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
      // move .ui text to item data
      (*it)->setData(1, SettingRole, (*it)->text(1));
    }
    ++it;
  }

#if QT_VERSION < 0x050000
  specificTree->header()->setResizeMode(0,QHeaderView::ResizeToContents);
#else
  specificTree->header()->setSectionResizeMode(0,QHeaderView::ResizeToContents);
#endif

  // Setup spin box item delegate for Specific tree
  SpinBoxDelegate *spinDelegate = new SpinBoxDelegate(0);
  specificTree->setItemDelegate(spinDelegate);

  // Get an instance of QSvgStyle
  if ( !style ) {
    style = static_cast<QSvgThemableStyle *>(QStyleFactory::create("QSvgStyle"));
    if ( !style ) {
      qWarning() << "[QSvgThemeManager]" << "Could not load QSvgStyle style, preview will not be available !";
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

  setStyleForWidgetAndChildren(style, previewWidget);

  // add themes and palettes
  QList<theme_spec_t> tlist = StyleConfig::getThemeList();
  QList<palette_spec_t> plist = StyleConfig::getPaletteList();

  themeCombo->addItem("<builtin>");
  // add them
  Q_FOREACH(theme_spec_t t, tlist) {
    themeCombo->addItem(t.name, t.path);
  }
  paletteCombo->addItem("<none>");
  paletteCombo->addItem("<system>");
  Q_FOREACH(palette_spec_t p, plist) {
    paletteCombo->addItem(p.name, p.path);
  }

  // Create temp cfg file from current qsvgstyle.cfg
  QTemporaryFile f(QDir::tempPath()+"/qsvgthememanager_XXXXXX");
  f.setAutoRemove(false);

  QString filename = StyleConfig::getUserConfigFile();
  QFile in(filename);
  in.open(QIODevice::ReadOnly);
  QByteArray data = in.readAll();
  in.close();

  f.open();
  tempCfgFile = f.fileName();
  f.write(data);
  f.close();

  // Create orig cfg file from current qsvgstyle.cfg
  QTemporaryFile f2(QDir::tempPath()+"/qsvgthememanager_orig_XXXXXX");
  f2.setAutoRemove(false);

  f2.open();
  origCfgFile = f2.fileName();
  f2.write(data);
  f2.close();

  // fill on UI with cfg values
  setupUiFromCfg();

  // UI automation
  connect(activeRadio,SIGNAL(toggled(bool)),
          this,SLOT(slot_currentColorGroupChanged()));
  connect(inactiveRadio,SIGNAL(toggled(bool)),
          this,SLOT(slot_currentColorGroupChanged()));
  connect(disabledRadio,SIGNAL(toggled(bool)),
          this,SLOT(slot_currentColorGroupChanged()));

  connect(themeCombo,SIGNAL(currentIndexChanged(int)),
          this,SLOT(slot_themeChanged(int)));
  connect(paletteCombo,SIGNAL(currentIndexChanged(int)),
          this,SLOT(slot_paletteChanged(int)));

  connect(specificTree,SIGNAL(itemChanged(QTreeWidgetItem *, int)),
          this,SLOT(slot_specificChanged(QTreeWidgetItem *,int)));

  connect(okBtn,SIGNAL(clicked()),
          this,SLOT(slot_okBtnClicked()));
  connect(cancelBtn,SIGNAL(clicked()),
          this,SLOT(slot_cancelBtnClicked()));
  connect(applyBtn,SIGNAL(clicked()),
          this,SLOT(slot_applyBtnClicked()));
  connect(resetBtn,SIGNAL(clicked()),
          this,SLOT(slot_resetBtnClicked()));

  // Timer for previewed widget repaints
  timer = new QTimer(this);
  connect(timer,SIGNAL(timeout()), this,SLOT(slot_uiSettingsChanged()));

  resize(sizeHint());
}

ThemeManagerUI::~ThemeManagerUI()
{
  delete config;

  QFile::remove(tempCfgFile);
  QFile::remove(origCfgFile);
}

void ThemeManagerUI::closeEvent(QCloseEvent* e)
{
  QApplication::closeAllWindows();

  QWidget::closeEvent(e);
}

void ThemeManagerUI::slot_currentColorGroupChanged()
{
  if ( activeRadio->isChecked() )
    previewColorGroup = QPalette::Active;
  else if ( inactiveRadio->isChecked() )
    previewColorGroup = QPalette::Inactive;
  else
    previewColorGroup = QPalette::Disabled;

  previewWidget->setDisabled(disabledRadio->isChecked());
}

void ThemeManagerUI::slot_themeChanged(int idx)
{
  QString theme = themeCombo->itemText(idx);
  QString themePath = themeCombo->itemData(idx).toString();

  if ( theme == "<builtin>" ) {
    /* Default theme */
    QStyle::staticMetaObject.invokeMethod(style,"loadBuiltinTheme",
                                          Qt::DirectConnection);

    themeLbl->setText("Uses the QSvgStyle default theme");
  } else {
    ThemeConfig t(themePath);
    theme_spec_t ts = t.getThemeSpec();

    QStyle::staticMetaObject.invokeMethod(style,"loadTheme",
                                          Qt::DirectConnection,
                                          Q_ARG(QString,theme));

    themeLbl->setText(QString("%1 by %2").arg(ts.descr).arg(ts.author));
  }

  schedulePreviewUpdate();
}

void ThemeManagerUI::slot_paletteChanged(int idx)
{
  QString palette = paletteCombo->itemText(idx);
  QString palettePath = paletteCombo->itemData(idx).toString();

  if ( palette == "<none>" ) {
    /* No palette */
    QStyle::staticMetaObject.invokeMethod(style,"unloadPalette",
                                          Qt::DirectConnection);

    paletteLbl->setText("No palette applied");
  } else if ( palette == "<system>" ) {
    /* System palette */
    QStyle::staticMetaObject.invokeMethod(style,"loadSystemPalette",
                                          Qt::DirectConnection);

    paletteLbl->setText("Uses the desktop environment colors");
  } else {
    PaletteConfig p(palettePath);
    palette_spec_t ps = p.getPaletteSpec();

    QStyle::staticMetaObject.invokeMethod(style,"loadPalette",
                                          Qt::DirectConnection,
                                          Q_ARG(QString,palette));

    paletteLbl->setText(QString("%1 by %2").arg(ps.descr).arg(ps.author));
  }

  schedulePreviewUpdate();
}

void ThemeManagerUI::slot_uiSettingsChanged()
{
  timer->stop();

  resetBtn->setEnabled(true);
  applyBtn->setEnabled(true);

  saveSettingsFromUi();

  // HACK force geometry recalculation
  QFont f = previewWidget->font();
  int val = f.pointSize();
  f.setPointSize(val-1);
  previewWidget->setFont(f);
  f.setPointSize(val);
  previewWidget->setFont(f);
}

void ThemeManagerUI::slot_specificChanged(QTreeWidgetItem* item, int column)
{
  Q_UNUSED(column);
  Q_UNUSED(item);

  schedulePreviewUpdate();
}

void ThemeManagerUI::slot_applyBtnClicked()
{
  // Apply = save cfg + notify
  commitConfiguration();
  notifyConfigurationChange();
}

void ThemeManagerUI::slot_cancelBtnClicked()
{
  // Cancel = reset cfg to saved + notify + quit
  resetConfiguration();
  notifyConfigurationChange();
  QApplication::closeAllWindows();
}

void ThemeManagerUI::slot_okBtnClicked()
{
  // Ok = apply + quit
  commitConfiguration();
  notifyConfigurationChange();
  QApplication::closeAllWindows();
}


void ThemeManagerUI::slot_resetBtnClicked()
{
  // reset = replace temp and user by orig + notify
  resetConfiguration();
  notifyConfigurationChange();
  setupUiFromCfg();
}

#if HAVE_X11
void ThemeManagerUI::notifyConfigurationChange()
{
  qDebug() << "FIXME notifyConfigurationChange()";

  QByteArray stamp;
  QDataStream s(&stamp, QIODevice::WriteOnly);
  s << QDateTime::currentDateTime();

  QByteArray settings_atom_name("_QT_SETTINGS_TIMESTAMP_");
  settings_atom_name += XDisplayString(QX11Info::display());

  xcb_connection_t *xcb_conn = QX11Info::connection();
  xcb_intern_atom_cookie_t cookie = xcb_intern_atom(xcb_conn, false, settings_atom_name.size(), settings_atom_name.constData());
  xcb_intern_atom_reply_t *reply = xcb_intern_atom_reply(xcb_conn, cookie, 0);
  xcb_atom_t atom = reply->atom;
  free(reply);

  xcb_change_property(xcb_conn, XCB_PROP_MODE_REPLACE, QX11Info::appRootWindow(), atom, XCB_ATOM_ATOM,
                      8, stamp.size(), (const void *)stamp.constData());

  //XChangeProperty(QX11Info::display(), QX11Info::appRootWindow(0),
                  //ATOM(_QT_SETTINGS_TIMESTAMP), ATOM(_QT_SETTINGS_TIMESTAMP), 8,
                  //PropModeReplace, (unsigned char *)stamp.data(), stamp.size());
}
#endif

void ThemeManagerUI::resetConfiguration()
{
  delete config;
  config = NULL;

  // copy origCfgFile to user config file and temp config file
  QFile src(origCfgFile);
  QFile dst1(StyleConfig::getUserConfigFile());
  QFile dst2(tempCfgFile);

  src.open(QIODevice::ReadOnly);
  QByteArray data = src.readAll();
  src.close();

  dst1.open(QIODevice::WriteOnly);
  dst1.write(data);
  dst1.close();

  dst2.open(QIODevice::WriteOnly);
  dst2.write(data);
  dst2.close();

  applyBtn->setEnabled(false);
  resetBtn->setEnabled(false);
}

void ThemeManagerUI::commitConfiguration()
{
  // copy tempCfgFile to user config file
  QFile src(tempCfgFile);
  QFile dst(StyleConfig::getUserConfigFile());

  src.open(QIODevice::ReadOnly);
  QByteArray data = src.readAll();
  src.close();

  dst.open(QIODevice::WriteOnly);
  dst.write(data);
  dst.close();

  applyBtn->setEnabled(false);
}

void ThemeManagerUI::setStyleForWidgetAndChildren(QStyle* style, QWidget* w)
{
  if ( !style || !w )
    return;

   w->setStyle(style);

  // iterate through children
  foreach(QObject *o, w->children()) {
    // lookup for immediate children
    QWidget *c = qobject_cast< QWidget* >(o);
    if ( c ) {
      setStyleForWidgetAndChildren(style, c);
    }

    // layout of level 1: iterate through layout items and set style
    // NOTE we don't go beyond level 1 layouts
    QLayout *l = qobject_cast< QLayout* >(o);
    if ( l ) {
      for (int i=0; i<l->count(); ++i) {
        QWidget *lc = l->itemAt(i)->widget(); // child widget inside layout
        if ( lc )
          setStyleForWidgetAndChildren(style, lc);
      }
    }
  }
}

void ThemeManagerUI::saveSettingsFromUi()
{
  style_spec_t ss;
  ss.theme = themeCombo->currentText();
  ss.palette = paletteCombo->currentText();

  config->setStyleSpec(ss);

  QTreeWidgetItemIterator it(specificTree, QTreeWidgetItemIterator::Editable);
  while (*it) {
    QTreeWidgetItem *item = (*it);
    if ( !item->data(1,SettingRole).isNull() ) {
      config->setSpecificValue(item->data(1,SettingRole).toString(),item->text(1));
    }
    ++it;
  }
}

void ThemeManagerUI::setupUiFromCfg()
{
  delete config;
  config = NULL;

  config = new StyleConfig(tempCfgFile);
  config->setUseCache(false);

  // get theme at startup
  QString startupTheme = config->getStyleSpec().theme;

  int idx = themeCombo->findText(startupTheme);
  if ( idx > -1 )
    themeCombo->setCurrentIndex(idx);
  else
    themeCombo->setCurrentIndex(0);

  // get palette at startup
  QString startupPalette = config->getStyleSpec().palette;

  idx = paletteCombo->findText(startupPalette);
  if ( idx > -1 )
    paletteCombo->setCurrentIndex(idx);
  else
    paletteCombo->setCurrentIndex(0);

  // Fill in tweaks tree values
  QTreeWidgetItemIterator it(specificTree, QTreeWidgetItemIterator::Editable);
  while (*it) {
    if ( !(*it)->childCount() ) {
      // set value from config file
      value_t<int> v = config->getSpecificValue((*it)->data(1,SettingRole).toString());
      (*it)->setText(1, QString("%1").arg(v));
    }
    ++it;
  }

  QStyle::staticMetaObject.invokeMethod(style,"loadCustomStyleConfig",
                                      Qt::DirectConnection,
                                      Q_ARG(QString,tempCfgFile));
}

void ThemeManagerUI::schedulePreviewUpdate()
{
  timer->stop();

  cfgModified = true;
  timer->start(500);
}
