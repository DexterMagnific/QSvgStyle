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
#ifndef QSVGCACHEDSETTINGS_H
#define QSVGCACHEDSETTINGS_H

#include <QHash>

class QString;
class QVariant;
class QSettings;

/**
 * @brief Wrapper around QSettings class with read/write caching capabilities
 */
class QSvgCachedSettings
{
  public:
    QSvgCachedSettings();
    QSvgCachedSettings(const QString &file);
    virtual ~QSvgCachedSettings();

    /**
     * Loads the given configuration file. If a file is already loaded,
     * its cache is committed (if used) and it is closed
     */
    void load(const QString &file);

    /**
     * Returns the loaded filename
     */
    QString filename() const { return file; }

    /**
     * Writes any cached values to the configuration file
     */
    void commitWriteCache();

    /**
     * Invalidates the current cache.
     * This forces values to be read from configuration file again
     * and discards any unwritten values
     */
    void invalidateCache();

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
     * set by the entry "element.inherits" if present.
     */
    QVariant getValue(const QString &group,
                      const QString& key,
                      int depth = 0) const;

    /**
     * sets the value of the given key from the given group
     * If the key has a null value (i.e. QVariant::isNull() is true),
     * the key is removed from the configuration file
     */
    void setValue(const QString &group, const QString &key,const QVariant &v);

    /**
      * Removes all entries in the given group starting with the given prefix
      */
    void removeAllWithPrefix(const QString &group, const QString &prefix);

    /**
     * Enables or disables the use of caching.
     * If enabled is false and the cache contains any
     * unwritten values, these are written immediately to the file
     */
    void setUseCache(bool enabled);

  private:
    bool usecache;
    QString file;
    QSettings *settings;
    mutable QHash<QString,QVariant> readCache;
    QHash<QString,QVariant> writeCache;
};

#endif
