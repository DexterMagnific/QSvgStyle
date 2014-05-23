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
#ifndef THEMECONFIG_H
#define THEMECONFIG_H

#include "specs.h"

class QString;
class QVariant;
class QSettings;
class QStringList;
template <class Key, class T> class QMap;

/**
 * Class that loads, saves (and in the future caches) theme settings
 */
class ThemeConfig {
  public:
    ThemeConfig();
    ThemeConfig(const QString &theme);
    ~ThemeConfig();

    /**
     * Loads a configuration from the given theme filename
     */
    void load(const QString &theme);

    frame_spec_t getFrameSpec(const QString &group) const;
    interior_spec_t getInteriorSpec(const QString &group) const;
    indicator_spec_t getIndicatorSpec(const QString &group) const;
    label_spec_t getLabelSpec(const QString &group) const;
    element_spec_t getElementSpec(const QString &group) const;
    theme_spec_t getThemeSpec() const;

    /* Get frame spec exactly as read from the configuration file */
    frame_spec_t getRawFrameSpec(const QString &group) const;
    interior_spec_t getRawInteriorSpec(const QString &group) const;
    indicator_spec_t getRawIndicatorSpec(const QString &group) const;
    label_spec_t getRawLabelSpec(const QString &group) const;
    element_spec_t getRawElementSpec(const QString &group) const;

    /* Get the frame as read from the configuration file and recursively
     * following inheritance if some values are unset within the specified
     * group.
     * At the end, if some values remain unset, they are filled with default
     * values so they can be safely read, but are left with the unset status
     */
    void setFrameSpec(const QString &group, const frame_spec_t &fs) const;
    void setInteriorSpec(const QString& group, const interior_spec_t& is) const;
    void setIndicatorSpec(const QString &group, const indicator_spec_t &ds) const;
    void setLabelSpec(const QString &group, const label_spec_t &ls) const;
    void setElementSpec(const QString &group, const element_spec_t &es) const;
    void setThemeSpec(const theme_spec_t &ts) const;

  private:
    friend class ThemeBuilderUIBase;

    /**
     * Returns the value of the @ref key key in the group @ref group
     * If the value is not found, a null QVariant is returned
     * (i.e QVariant::isNull() is true)
     * The key is searched first in the theme configuration cache then
     * read from the configuration file
     */
    QVariant getRawValue(const QString &group,
                         const QString& key) const;

    /**
     * Returns the value of the @ref key key in the group @ref group
     * If the key is not found in the group, it is searched in the group
     * set by the entry "inherits" if present.
     * The key is searched first in the theme configuration cache then
     * read from the configuration file
     */
    QVariant getValue(const QString &group,
                      const QString& key,
                      int depth = 0) const;

    /**
     * sets the value of the given key from the given group in the theme config file
     * If the key has a null value (i.e. QVariant::isNull() is true),
     * the key is removed from the configuration file
     */
    void setValue(const QString &group, const QString &key,const QVariant &v) const;

    QMap<QString,QVariant> values;
    QMap<QString,QVariant> rawValues;

    QSettings *settings;
};

#endif // THEMECONFIG_H
