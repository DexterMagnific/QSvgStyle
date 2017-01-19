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
#ifndef PALETTECONFIG_H
#define PALETTECONFIG_H

#include "specs.h"

class QString;
class QVariant;
class QSettings;
class QStringList;

/**
 * Class that loads, saves palette settings
 */
class PaletteConfig {
  public:
    PaletteConfig();
    PaletteConfig(const QString &palette);
    ~PaletteConfig();

    /**
     * Loads a configuration from the given palette filename
     */
    void load(const QString &palette);

    /**
     * Forces an immediate disk write of the current settings
     */
    void sync();

    /* Get post-processed color spec after 'inherits' resolution */
    color_spec_t getColorSpec(const QString &group) const;
    palette_spec_t getPaletteSpec() const;

    /* Get frame spec exactly as read from the configuration file */
    color_spec_t getRawColorSpec(const QString &group) const;

    void setColorSpec(const QString &group, const color_spec_t &cs) const;

  private:
    //friend class PaletteBuilderUIBase;

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
     * sets the value of the given key from the given group in the palette config file
     * If the key has a null value (i.e. QVariant::isNull() is true),
     * the key is removed from the configuration file
     */
    void setValue(const QString &group, const QString &key,const QVariant &v) const;

    QSettings *settings;
};

#endif // THEMECONFIG_H
