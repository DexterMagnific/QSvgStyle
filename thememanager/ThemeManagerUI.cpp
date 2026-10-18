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
#include <QWindow>

#if HAVE_X11
#include <X11/Xlib.h>
#include <xcb/xcb.h>
#endif

#include "StyleConfig.h"
#include "../style/QSvgThemableStyle.h"
#include "../styleconfig/specs.h"

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

  //qWarning() << "User theme dir" << StyleConfig::getUserConfigDir();
  //qWarning() << "System theme dir" << StyleConfig::getSystemConfigDir();

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
                           "Preview will not be available.").arg(QLibraryInfo::path(QLibraryInfo::PluginsPath)));
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

  // add themes using new discovery API (priority order: Environment -> User -> System -> Builtin)
  QList<theme_spec_t> tlist = StyleConfig::discoverThemes();

  // add them
  Q_FOREACH(theme_spec_t t, tlist) {
    QTreeWidgetItem *i = new QTreeWidgetItem(themeList);
    
    // Show theme name with origin indicator
    QString originStr;
    switch (t.origin) {
      case ThemeOrigin_Environment: originStr = "[Environment] "; break;
      case ThemeOrigin_User: originStr = "[User] "; break;
      case ThemeOrigin_System: originStr = "[System] "; break;
      case ThemeOrigin_Builtin: originStr = "[Built-in] "; break;
    }
    i->setText(0, originStr + t.name);
    i->setData(0, ThemePathRole, t.path);
    // Store the canonical name and origin, the display text is decorated
    i->setData(0, ThemeNameRole, t.name);
    i->setData(0, ThemeOriginRole, static_cast<int>(t.origin));

    themeList->addTopLevelItem(i);
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

  connect(themeList,SIGNAL(currentItemChanged(QTreeWidgetItem *, QTreeWidgetItem *)),
          this,SLOT(slot_themeChanged(QTreeWidgetItem *, QTreeWidgetItem *)));
  //connect(paletteCombo,SIGNAL(currentIndexChanged(int)),
  //        this,SLOT(slot_paletteChanged(int)));

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

  resize(minimumSizeHint());
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

void ThemeManagerUI::slot_themeChanged(QTreeWidgetItem *item, QTreeWidgetItem *prev)
{
  Q_UNUSED(prev);

  QString theme = item->data(0, ThemeNameRole).toString();

  if ( theme.isEmpty() || (theme == "<builtin>") ) {
    /* Default theme */
    QStyle::staticMetaObject.invokeMethod(style,"loadBuiltinTheme",
                                          Qt::DirectConnection);

    //themeLbl->setText("Uses the QSvgStyle default theme");
  } else {
    QStyle::staticMetaObject.invokeMethod(style,"loadTheme",
                                          Qt::DirectConnection,
                                          Q_ARG(QString,theme));
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

void ThemeManagerUI::notifyConfigurationChange()
{
	// TODO
}

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
  // Flush pending settings changes to the temporary file before copying it
  // over the user configuration file
  if ( config )
    config->commitWriteCache();

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
  QTreeWidgetItem *current = themeList->currentItem();
  if ( !current )
    return;

  style_spec_t ss;
  ss.theme = current->data(0, ThemeNameRole).toString();

  config->setStyleSpec(ss);
}

void ThemeManagerUI::setupUiFromCfg()
{
  delete config;
  config = NULL;

  config = new StyleConfig(tempCfgFile);
  config->setUseCache(false);

  // Load the theme through the engine first: this resolves the "<builtin>"
  // placeholder, themes that are no longer installed and the
  // QSVGSTYLE_THEME override exactly like at runtime.
  QStyle::staticMetaObject.invokeMethod(style,"loadCustomStyleConfig",
                                      Qt::DirectConnection,
                                      Q_ARG(QString,tempCfgFile));

  QString effectiveTheme;
  if ( style )
    QStyle::staticMetaObject.invokeMethod(style, "currentTheme",
                                          Qt::DirectConnection,
                                          Q_RETURN_ARG(QString, effectiveTheme));

  // Match the canonical theme name, the item text is decorated with its origin
  QTreeWidgetItem *match = NULL;
  QTreeWidgetItem *firstBuiltin = NULL;

  for (int i = 0; i < themeList->topLevelItemCount(); ++i) {
    QTreeWidgetItem *it = themeList->topLevelItem(i);
    const int origin = it->data(0, ThemeOriginRole).toInt();

    if ( !firstBuiltin && (origin == ThemeOrigin_Builtin) )
      firstBuiltin = it;

    if ( (it->data(0, ThemeNameRole).toString() == effectiveTheme) &&
         (!match || (origin != ThemeOrigin_Builtin)) )
      match = it;
  }

  // Fall back to a built-in theme if the effective theme is not listed
  themeList->setCurrentItem(match ? match : firstBuiltin);
}

void ThemeManagerUI::schedulePreviewUpdate()
{
  timer->stop();

  cfgModified = true;
  timer->start(500);
}
