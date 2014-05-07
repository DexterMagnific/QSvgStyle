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
#ifndef THEMECONFIG_H
#define THEMECONFIG_H

#include "specs.h"

class QString;
class QVariant;
class QSettings;
class QStringList;

/**
 * Class that loads, saves (and in the future caches) theme settings
 */
class ThemeConfig {
  public:
    ThemeConfig();
    /** Builds and loads the theme @ref theme (which is filename) which has
     * an optional parent @ref parent.
     */
    ThemeConfig(const QString &theme, ThemeConfig *parent = NULL);
    ~ThemeConfig();

    /**
     * Loads a configuration from the given theme filename
     */
    void load(const QString &theme);

    /**
     * Set this theme config's parent configuration
     */
    void setParent(ThemeConfig *parent) { m_parent = parent; };
    ThemeConfig *parent() const { return m_parent; };

    /**
     * Returns whether this theme configuration is valid
     */
    bool isValid() const { return m_valid; };

    /** Returns the frame spec of the given widget from the theme config file
     * If expand is true and some keys are not found, the inherited frame then
     * the inherited element then this theme config's parent theme are searched
     * otherwise the frame spec in returned exactly as read from the config file
     * without inheritence processing. Note that if expand is false and the key
     * is not found NULL_INT will be stored in ints, and NULL_STR will be stored
     * in strings
     * */
    frame_spec_t getFrameSpec(const QString &elementName, bool expand=true) const;
    /** Returns the interior spec of the given widget from the theme config file */
    interior_spec_t getInteriorSpec(const QString &elementName, bool expand=true) const;
    /** Returns the indicator spec of the given widget from the theme config file */
    indicator_spec_t getIndicatorSpec(const QString &elementName, bool expand=true) const;
    /** Returns the label (text+icon) spec of the given widget from the theme config file */
    label_spec_t getLabelSpec(const QString &elementName, bool expand=true) const;
    /** Returns the size spec of the given widget from the theme config file */
    size_spec_t getSizeSpec(const QString &elementName, bool expand=true) const;
    /** Returns the element spec of the given widget from the theme config file */
    element_spec_t getElementSpec(const QString &elementName, bool expand=true) const;
    /** Returns the theme spec of this theme */
    theme_spec_t getThemeSpec() const;

    void setFrameSpec(const QString &elementName, const frame_spec_t &fspec) const;
    void setInteriorSpec(const QString &elementName, const interior_spec_t &ispec) const;
    void setIndicatorSpec(const QString &elementName, const indicator_spec_t &dspec) const;
    void setLabelSpec(const QString &elementName, const label_spec_t &lspec) const;
    void setSizeSpec(const QString &elementName, const size_spec_t &espec) const;
    void setElementSpec(const QString &elementName, const element_spec_t &espec) const;
    void setThemeSpec(const theme_spec_t &tspec) const;

    /** returns the list of supported elements for which settings are recognized */
    static QStringList getManagedElements();

  private:
    /**
     * returns the value of the given key from the given group from the theme config file
     * If the key is not found in the config file, it is searched in the groups
     * contained in the @a inherits list. If it is not found, it is searched in
     * this theme config's parent theme config. It it is not found, the @a ok is
     * set to false.
     * Works only if ok is not NULL
     */
    QVariant getValue(const QString &group, const QString& key, bool* ok, const QStringList &inherits, const ThemeConfig *parent) const;
    /**
     * returns the value of the given key from the given group from the theme config file
     */
    QVariant getValue(const QString &group, const QString& key, bool* ok) const;
    /**
     * sets the value of the given key from the given group in the theme config file
     * If the key is not present it is removed from the file
     */
    template<typename T> void setValue(const QString &group, const QString &key,const value_t<T> &v) const;
    /**
     * sets the value of the given key from the given group in the theme config file
     */
    void setValue(const QString &group, const QString &key, const QString &v) const;
    /**
     * removes the given key from the given group in the theme config file
     */
    void removeKey(const QString &group, const QString &key) const;

    bool m_valid;
    QSettings *settings;
    ThemeConfig *m_parent;
};

#endif // THEMECONFIG_H
