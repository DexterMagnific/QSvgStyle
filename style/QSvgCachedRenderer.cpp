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

#include "QSvgCachedRenderer.h"

#include <QDebug>
#include <QPainter>
#include <QTime>

QSvgCachedRenderer::QSvgCachedRenderer()
  : renderer(NULL),
    totalCacheHits(0), totalCacheMisses(0),
    totalSvgRenderTime(0), totalCachedRenderTime(0)
{
}

QSvgCachedRenderer::QSvgCachedRenderer(const QString &file)
  : QSvgCachedRenderer()
{
  load(file);
}

QSvgCachedRenderer::~QSvgCachedRenderer()
{
  delete renderer;

  // If you want to dumpStats(), please uncomment the relevant
  // code in renderElement()

  //dumpStats();
}

bool QSvgCachedRenderer::load(const QString &file)
{
  if ( renderer )
    delete renderer;

  svgCache.clear();
  totalCacheHits = totalCacheMisses = 0;
  totalSvgRenderTime = totalCachedRenderTime = 0;

  renderer = new QSvgRenderer();

  return renderer->load(file);
}

void QSvgCachedRenderer::render(QPainter *painter, const QString &elementId, const QRect &bounds)
{
  // key = elementId @ width x height
  const QString e = QString("%1@%2x%3")
      .arg(elementId)
      .arg(bounds.width())
      .arg(bounds.height());

  QTime t;
  int elapsed = 0;

  if ( svgCache.contains(e) ) {
    totalCacheHits++;

    // dont use value() as this will return a copy
    svgCacheEntry &entry = svgCache[e];
    entry.hits++;

    // element found in cache
    t.restart();
    painter->drawPixmap(bounds,entry.pixmap);
    elapsed = t.elapsed();

    entry.cachedRenderTime += elapsed;
    totalCachedRenderTime += elapsed;

    // TESTING remove me !
    // in order to compare, we will also render from SVG
    // please enable this if you want to dumpStats()
//    QPixmap px(bounds.size());
//    QPainter p(&px);

//    t.restart();
//    renderer->render(&p,elementId,QRect(QPoint(0,0),bounds.size()));
//    elapsed = t.elapsed();

//    entry.svgRenderTime += elapsed;
//    totalSvgRenderTime += elapsed;
    // END TESTING
  } else {
    totalCacheMisses++;

    // not found, add to cache
    svgCacheEntry entry;
    entry.hits = entry.svgRenderTime = entry.cachedRenderTime = 0;

    // Penalty: because the original painter can contain all sorts of
    // transformations, we must first render to a pixmap using a new
    // painter in order to get an unaltered element to reuse later
    // But this happens only on a miss (i.e. once per element@size)
    entry.pixmap = QPixmap(bounds.size());
    entry.pixmap.fill(Qt::transparent);
    // warning: the pixmap must be drawn with a neutral painter
    QPainter p(&entry.pixmap);

    t.restart();
    renderer->render(&p,elementId,QRect(QPoint(0,0),bounds.size()));
    elapsed = t.elapsed();

    // now render the pixmap using the original painter
    painter->drawPixmap(bounds,entry.pixmap);

    entry.svgRenderTime += elapsed;
    totalSvgRenderTime += elapsed;

    svgCache.insert(e,entry);
  }
}

void QSvgCachedRenderer::dumpStats()
{
  QHash<QString,svgCacheEntry>::const_iterator it;

  qWarning() << "[QSvgCacheRenderer] Stats:";
  qWarning() << "Hits:" << totalCacheHits << "Misses:" << totalCacheMisses
             << "Ratio:" << totalCacheHits*100.0/(totalCacheHits+totalCacheMisses);
  qWarning() << "Cache render time:" << totalCachedRenderTime
             << "SVG render time:" << totalSvgRenderTime
             << "Cache speedup:" << totalSvgRenderTime*1.0/totalCachedRenderTime;
}
