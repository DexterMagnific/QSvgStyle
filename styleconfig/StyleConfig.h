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
#ifndef STYLECONFIG_H
#define STYLECONFIG_H

#include "specs.h"
#include "QSvgCachedSettings.h"
#include "ThemeConfig.h"

class QVariant;
class QSettings;
class QDir;
template<typename T> class QList;

/**
 * Class that loads, saves style settings
 */
class StyleConfig : public QSvgCachedSettings {
  public:
    StyleConfig();
    StyleConfig(const QString &style);
    ~StyleConfig();


    style_spec_t getStyleSpec() const;

    void setStyleSpec(const style_spec_t &cs);

    QVariant getStyleTweak(const QString &key) const {
        return getRawValue("Tweaks",key);
    }

    void setStyleTweak(const QString &key, const QVariant &v) {
        setValue("Tweaks",key,v);
    }

    /**
     * Returns the list of themes. List contains user themes first
     */
    static QList<theme_spec_t> getThemeList();

    /**
     * Returns the system config dir
     */
    static QDir getSystemConfigDir();

    /**
     * Returns the user config dir
     */
    static QDir getUserConfigDir();

    /**
     * Returns the user config file
     */
    static QString getUserConfigFile();

  private:
};

#endif // STYLECONFIG_H
