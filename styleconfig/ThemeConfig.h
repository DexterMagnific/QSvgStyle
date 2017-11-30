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
#include "QSvgCachedSettings.h"

class QString;
class QVariant;
class QSettings;
class QStringList;

/**
 * Class that loads, saves (and in the future caches) theme settings
 */
class ThemeConfig : public QSvgCachedSettings {
  public:
    ThemeConfig();
    ThemeConfig(const QString &theme);

    /* Get post-processed frame spec after 'inherits' resolution */
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
    void setFrameSpec(const QString &group, const frame_spec_t &fs);
    void setInteriorSpec(const QString& group, const interior_spec_t& is);
    void setIndicatorSpec(const QString &group, const indicator_spec_t &ds);
    void setLabelSpec(const QString &group, const label_spec_t &ls);
    void setElementSpec(const QString &group, const element_spec_t &es);
    void setThemeSpec(const theme_spec_t &ts);

    friend class ThemeBuilderUIBase;
};

#endif // THEMECONFIG_H
