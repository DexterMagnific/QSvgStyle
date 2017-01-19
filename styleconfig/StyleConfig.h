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
#ifndef STYLEONFIG_H
#define STYLECONFIG_H

#include "specs.h"

#include <QString>

#include "ThemeConfig.h"
#include "PaletteConfig.h"

class QVariant;
class QSettings;
class QDir;
template<typename T> class QList;

/**
 * Class that loads, saves style settings
 */
class StyleConfig {
  public:
    StyleConfig();
    StyleConfig(const QString &style);
    ~StyleConfig();

    /**
     * Loads a configuration from the given style filename
     */
    void load(const QString &style);

    /**
     * Forces an immediate disk write of the current settings
     */
    void sync();

    style_spec_t getStyleSpec() const;

    void setStyleSpec(const style_spec_t &cs) const;

    QVariant getSpecificValue(const QString &key) const {
        return getRawValue("Tweaks",key);
    }

    void setSpecificValue(const QString &key, const QVariant &v) const {
        setValue("Tweaks",key,v);
    }

    /**
     * Returns the list of themes. List contains user themes first
     */
    static QList<theme_spec_t> getThemeList();

    /**
     * Returns the list of palettes. List contains user palettes first
     */
    static QList<palette_spec_t> getPaletteList();

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
    //friend class StyleBuilderUIBase;

    /**
     * Returns the value of the @ref key key in the group @ref group
     * If the value is not found, a null QVariant is returned
     * (i.e QVariant::isNull() is true)
     */
    QVariant getRawValue(const QString &group,
                         const QString& key) const;

    /**
     * Returns the value of the @ref key key in the group @ref group
     * If the key is not found in the group, it is searched in the group
     * set by the entry "inherits" if present.
     */
    QVariant getValue(const QString &group,
                      const QString& key,
                      int depth = 0) const;

    /**
     * sets the value of the given key from the given group in the style config file
     * If the key has a null value (i.e. QVariant::isNull() is true),
     * the key is removed from the configuration file
     */
    void setValue(const QString &group, const QString &key,const QVariant &v) const;

    QSettings *settings;
};

#endif // STYLECONFIG_H
