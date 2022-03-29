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

#include "NewThemeUI.h"

#include <QDir>
#include <QFileDialog>
#include <QCompleter>
#include <QFileSystemModel>

#include "StyleConfig.h"

NewThemeUI::NewThemeUI(QWidget* parent)
  : QDialog(parent)
{
  setupUi(this);

  QCompleter *c = new QCompleter(this);
  c->setModel(new QFileSystemModel(c));
  dirEdit->setCompleter(c);

  slot_dirResetBtnClicked(false);

  connect(dirResetBtn,SIGNAL(clicked(bool)),
          this,SLOT(slot_dirResetBtnClicked(bool)));
  connect(dirChooseBtn,SIGNAL(clicked(bool)),
          this,SLOT(slot_dirChooseBtnClicked(bool)));
}

NewThemeUI::~NewThemeUI()
{
}

void NewThemeUI::slot_dirResetBtnClicked(bool checked)
{
  Q_UNUSED(checked);

  dirEdit->setText(StyleConfig::getUserConfigDir().absolutePath());
}

void NewThemeUI::slot_dirChooseBtnClicked(bool checked)
{
  Q_UNUSED(checked);

  QString d = QFileDialog::getExistingDirectory(NULL, "Choose directory", dirEdit->text());

  if ( !d.isEmpty() ) {
    dirEdit->setText(d);
  }
}

void NewThemeUI::accept()
{
  mThemeBaseFilename = themeNameEdit->text();
  mDirectory = dirEdit->text();

  QDialog::accept();
}
