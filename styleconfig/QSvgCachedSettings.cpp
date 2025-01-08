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

#include "QSvgCachedSettings.h"

#include <QDebug>
#include <QString>
#include <QVariant>
#include <QSettings>
#include <QFile>
#include <QStringList>

QSvgCachedSettings::QSvgCachedSettings()
  : usecache(true),
    settings(NULL)
{
}

QSvgCachedSettings::QSvgCachedSettings(const QString &file)
  : QSvgCachedSettings()
{
  load(file);
}

QSvgCachedSettings::~QSvgCachedSettings()
{
  if ( settings ) {
    commitWriteCache();
    delete settings;
  }
}

void QSvgCachedSettings::load(const QString &filename)
{
  if (settings) {
    commitWriteCache();
    delete settings;
  }

  settings = NULL;
  invalidateCache();

  if (!QFile::exists(filename))
    return;

  settings = new QSettings(filename,QSettings::IniFormat);
  file = filename;
}

void QSvgCachedSettings::invalidateCache()
{
  readCache.clear();
  writeCache.clear();
}

void QSvgCachedSettings::setUseCache(bool enabled)
{
  if ( !enabled ) {
    commitWriteCache();
  }

  usecache = enabled;
}

QVariant QSvgCachedSettings::getRawValue(const QString &group, const QString &key) const
{
  const QString k = group+"/"+key;

  if ( !settings )
    return QVariant();

  // read from cache
  if ( usecache && readCache.contains(k) )
    return readCache.value(k);

  // read from file and cache it
  if ( !settings )
    return QVariant();

  QVariant v = settings->value(k);

  // even if not using cache, cache the value to anticipate a future
  // use of cache
  readCache.insert(k,v);

  return v;
}

QVariant QSvgCachedSettings::getValue(const QString& group, const QString& key, int depth) const
{
  QVariant r;
  QVariant inherit;

  if ( !settings )
    return QVariant();

  // get value
  r = getRawValue(group, key);

  // search inherited element if not found
  if ( r.isNull() && (depth <= 2) ) {
    inherit = getRawValue(group,"element.inherits");
    r = getValue(inherit.toString(),key,depth+1);
  }

  return r;
}

void QSvgCachedSettings::setValue(const QString& group, const QString& key, const QVariant& v)
{
  const QString k = group+"/"+key;

  if ( !settings )
    return;

  if ( usecache ) {
    writeCache.insert(k,v);
    // also store in read cache for fast retrieval
    readCache.insert(k, v);
  } else {
    if ( v.isNull() ) {
      settings->remove(k);
    } else {
      settings->setValue(k,v);
    }
  }
}

void QSvgCachedSettings::removeAllWithPrefix(const QString &group, const QString &prefix)
{
  if ( !settings )
    return;

  settings->beginGroup(group);
  QStringList keys = settings->childKeys();
  settings->endGroup();

  foreach (QString k, keys) {
    if ( k.startsWith(prefix) ) {
      setValue(group, k, QVariant());
    }
  }
}

void QSvgCachedSettings::commitWriteCache()
{
  QHash<QString,QVariant>::const_iterator it;

  if ( !settings )
    return;

  if ( usecache ) {
    for (it = writeCache.constBegin(); it != writeCache.constEnd(); ++it) {
      const QString k = it.key();
      const QVariant v = it.value();

      if ( v.isNull() ) {
        // Null value -> this means we want to inherit it
        // so delete key in file
        settings->remove(k);
        qWarning() << "clearing" << k;
      } else {
        qWarning() << "setting" << k << "to" << v;
        settings->setValue(k,v);
      }
    }

    // everything has been written, so clear write cache
    writeCache.clear();
  }

  // sync into filesystem
  settings->sync();
}
