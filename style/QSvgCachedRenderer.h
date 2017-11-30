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

#ifndef QSVGCACHEDRENDERER_H
#define QSVGCACHEDRENDERER_H

#include <QHash>
#include <QPixmap>
#include <QSvgRenderer>

class QPainter;
class QRectF;
class QString;

/**
 * @brief Wrapper around QSvgRenderer class with rendering caching capabilities
 */
class QSvgCachedRenderer
{
  public:
    QSvgCachedRenderer();
    QSvgCachedRenderer(const QString &file);
    virtual ~QSvgCachedRenderer();

    /**
     * Loads the given SVG file
     */
    bool load(const QString &file);

    /**
      * Returns if the loaded file is valid
      */
    bool isValid() const { return renderer ? renderer->isValid() : false; }

    /**
      * Renders the given element id inside the given rect using the given painter
      */
    void render(QPainter *painter, const QString &elementId, const QRect &bounds = QRect());

    /**
      * Returns whether the given element id exists in SVG file and is renderable
      */
    bool elementExists(const QString &id) const {
      return renderer ? renderer->elementExists(id) : false;
    }

  private:
    typedef struct svgCacheEntry {
        quint32 hits;
        qreal svgRenderTime;
        quint64 cachedRenderTime;

        QPixmap pixmap;
    } svgCacheEntry;

    void dumpStats();

    // the SVG renderer
    QSvgRenderer *renderer;

    // the in-memory SVG cache
    QHash<QString,svgCacheEntry> svgCache;

    quint32 totalCacheHits, totalCacheMisses;
    quint64 totalSvgRenderTime, totalCachedRenderTime;
};

#endif // QSVGCACHEDRENDERER_H
