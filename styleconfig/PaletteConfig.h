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
#include "QSvgCachedSettings.h"

class QString;
class QVariant;
class QSettings;
class QStringList;

/**
 * Class that loads, saves palette settings
 */
class PaletteConfig : public QSvgCachedSettings {
  public:
    PaletteConfig();
    PaletteConfig(const QString &palette);
    virtual ~PaletteConfig();

    /* Get post-processed color spec after 'inherits' resolution */
    color_spec_t getColorSpec(const QString &group) const;
    palette_spec_t getPaletteSpec() const;

    /* Get frame spec exactly as read from the configuration file */
    color_spec_t getRawColorSpec(const QString &group) const;

    void setColorSpec(const QString &group, const color_spec_t &cs);

  private:
};

#endif // PALETTECONFIG_H
