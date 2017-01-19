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
#ifndef THEMEMANAGERUI_H
#define THEMEMANAGERUI_H

#include <QPalette>
#include <QDir>
#include <QStyledItemDelegate>

#include "ui_ThemeManagerUIBase.h"
#include <specs.h>

class QSvgThemableStyle;
class StyleConfig;
class QTimer;

class ThemeManagerUI : public QMainWindow, private Ui::ThemeManagerUIBase {
  Q_OBJECT

  public:
    ThemeManagerUI(QWidget* parent = 0);
    virtual ~ThemeManagerUI();

  private slots:
    void slot_currentColorGroupChanged();
    void slot_themeChanged(int idx);
    void slot_paletteChanged(int idx);
    void slot_specificChanged(QTreeWidgetItem *item, int column);
    void slot_uiSettingsChanged();
    void slot_cancelBtnClicked();
    void slot_okBtnClicked();
    void slot_applyBtnClicked();
    void slot_resetBtnClicked();

   protected:
    virtual void closeEvent(QCloseEvent *e);

  private:
    // enum that allow us to attach arbitrary data to QTreeViewItem items
    // using @ref QTreeViewItem::setData()
    enum {
      GroupRole = Qt::UserRole + 10, // the theme configuration group for the
                                     // selected widget
      SettingRole = Qt::UserRole + 11, // the specific setting name
    };

    // Sets the given style for the given widget and all its children
    void setStyleForWidgetAndChildren(QStyle* style, QWidget* w);
    // schedules a preview update
    void schedulePreviewUpdate();
    // saves UI settings to tempCfgFile
    void saveSettingsFromUi();
    // sets up UI from tempCfgFile
    void setupUiFromCfg();

    // propagates configuration change to other Qt apps
    void notifyConfigurationChange();
    // saves configuration into user config file
    void commitConfiguration();
    // resets the configuration to origCfgFile
    void resetConfiguration();

    QSvgThemableStyle *style;
    QPalette::ColorGroup previewColorGroup;
    // Style settings
    StyleConfig *config;
    // cfg file modified
    bool cfgModified;
    // Temporary working file, a copy of qsvgstyle.cfg
    QString tempCfgFile;
    // Temporary original file, a copy of qsvgstyle.cfg
    QString origCfgFile;

    // Timer for repainting the widget when settings change
    QTimer *timer;
};

// Spin Box item delegate
class SpinBoxDelegate : public QStyledItemDelegate {
  Q_OBJECT

  public:
    SpinBoxDelegate(QObject *parent = 0);

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                          const QModelIndex &index) const;

    void setEditorData(QWidget *editor, const QModelIndex &index) const;
    void setModelData(QWidget *editor, QAbstractItemModel *model,
                      const QModelIndex &index) const;

    void updateEditorGeometry(QWidget *editor,
                              const QStyleOptionViewItem &option,
                              const QModelIndex &index) const;

  private slots:
    void slot_valueChanged(int);
};

#endif // THEMEMANAGERRUI_H
