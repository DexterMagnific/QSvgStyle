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
#ifndef THEMEBUILDERUI_H
#define THEMEBUILDERUI_H

#include <QStringList>
#include <QShowEvent>

#include "ui_ThemeBuilderUIBase.h"

class ThemeConfig;
class QTreeWidget;
class QTreeWidgetItem;


class ThemeBuilderUI : public QMainWindow, private Ui::ThemeBuilderUIBase {
  Q_OBJECT

  public:
    ThemeBuilderUI(QWidget *parent = 0);
    ~ThemeBuilderUI();

  public slots:
    void slot_loadTheme(const QString &theme);
    void slot_open();
    void slot_save(const QString &widget);
    void slot_new();
    void slot_quit();
    void slot_ElementChanged(const QString &);

  private:
    enum {
      GroupRole = Qt::UserRole + 10,
    };

    QTreeWidget *drawStackTree;
    QTreeWidget *resolvedValuesTree;
    QLabel *statusbarLbl1;
    QLabel *statusbarLbl2;

    ThemeConfig *config;
    ThemeConfig *defaultConfig;
    QString filename;
    QStringList svgElements;
    QString lastElement;
};

#endif // THEMEBUILDERUI_H
