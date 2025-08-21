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

#include "ThemeConfig.h"
#include "StyleConfig.h"
#include "../style/QSvgThemableStyle.h"

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

  // add themes
  QList<theme_spec_t> tlist = StyleConfig::getThemeList();

  // add builtin theme
  QTreeWidgetItem *i = new QTreeWidgetItem(themeList);
  i->setText(0, "<builtin>");
  themeList->addTopLevelItem(i);

  // add them
  Q_FOREACH(theme_spec_t t, tlist) {
    QTreeWidgetItem *i = new QTreeWidgetItem(themeList);
    i->setText(0, t.name);
    i->setData(0, ThemePathRole, t.path);

    themeList->addTopLevelItem(i);
    //themeList->addItem(t.name, t.path);
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

  QString theme = item->text(0);
  QString themePath = item->data(0, ThemePathRole).toString();

  if ( theme == "<builtin>" ) {
    /* Default theme */
    QStyle::staticMetaObject.invokeMethod(style,"loadBuiltinTheme",
                                          Qt::DirectConnection);

    //themeLbl->setText("Uses the QSvgStyle default theme");
  } else {
    ThemeConfig t(themePath);
    theme_spec_t ts = t.getThemeSpec();

    QStyle::staticMetaObject.invokeMethod(style,"loadTheme",
                                          Qt::DirectConnection,
                                          Q_ARG(QString,theme));

    //themeLbl->setText(QString("%1 by %2").arg(ts.descr).arg(ts.author));
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
#if HAVE_X11
  using namespace QNativeInterface;
  QX11Application *native = qApp->nativeInterface<QX11Application>();

  qDebug() << "FIXME notifyConfigurationChange()";

  QByteArray stamp;
  QDataStream s(&stamp, QIODevice::WriteOnly);
  s << QDateTime::currentDateTime();

  QByteArray settings_atom_name("_QT_SETTINGS_TIMESTAMP_");
  settings_atom_name += XDisplayString(native->display());

  xcb_connection_t *xcb_conn = native->connection();
  xcb_intern_atom_cookie_t cookie = xcb_intern_atom(xcb_conn, false, settings_atom_name.size(), settings_atom_name.constData());
  xcb_intern_atom_reply_t *reply = xcb_intern_atom_reply(xcb_conn, cookie, 0);
  xcb_atom_t atom = reply->atom;
  free(reply);

  xcb_change_property(xcb_conn, XCB_PROP_MODE_REPLACE, qApp->topLevelWindows().first()->winId(), atom, XCB_ATOM_ATOM,
                      8, stamp.size(), (const void *)stamp.constData());

  //XChangeProperty(QX11Info::display(), QX11Info::appRootWindow(0),
                  //ATOM(_QT_SETTINGS_TIMESTAMP), ATOM(_QT_SETTINGS_TIMESTAMP), 8,
                  //PropModeReplace, (unsigned char *)stamp.data(), stamp.size());
#endif
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
  ss.theme = themeList->currentItem()->text(0);

  config->setStyleSpec(ss);
}

void ThemeManagerUI::setupUiFromCfg()
{
  delete config;
  config = NULL;

  config = new StyleConfig(tempCfgFile);
  config->setUseCache(false);

  // get theme at startup
  QString startupTheme = config->getStyleSpec().theme;

  const QList<QTreeWidgetItem*> l = themeList->findItems(startupTheme, Qt::MatchExactly);
  if ( l.size() > 0 )
    themeList->setCurrentItem(l.at(0));
  else
    themeList->setCurrentItem(NULL);

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
