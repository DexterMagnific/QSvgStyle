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
#include <QElapsedTimer>

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

static void applyTransform(QPainter *painter, const QRect &bounds, RenderTransforms transform)
{
  if (transform == NoTransform)
    return;

  QPoint center = bounds.center();
  painter->translate(center);

  if (transform & Rotate90)
    painter->rotate(90);
  else if (transform & Rotate180)
    painter->rotate(180);
  else if (transform & Rotate270)
    painter->rotate(270);

  if (transform & FlipHorizontal)
    painter->scale(-1, 1);
  if (transform & FlipVertical)
    painter->scale(1, -1);

  painter->translate(-center);
}

static void restoreTransform(QPainter *painter, const QRect &bounds, RenderTransforms transform)
{
  if (transform == NoTransform)
    return;

  QPoint center = bounds.center();
  painter->translate(center);

  if (transform & FlipVertical)
    painter->scale(1, -1);
  if (transform & FlipHorizontal)
    painter->scale(-1, 1);

  if (transform & Rotate270)
    painter->rotate(-270);
  else if (transform & Rotate180)
    painter->rotate(-180);
  else if (transform & Rotate90)
    painter->rotate(-90);

  painter->translate(-center);
}

void QSvgCachedRenderer::render(QPainter *painter, const QString &elementId, const QRect &bounds,
                                RenderTransforms transform)
{
  if ( (bounds.width() > 250) || (bounds.height() > 250) ) {
    // direct render, don't cache big items
    applyTransform(painter, bounds, transform);
    renderer->render(painter, elementId, bounds);
    restoreTransform(painter, bounds, transform);
    return;
  }

  // key = elementId @ width x height (no transform in key - cache stores neutral)
  const QString e = QString("%1@%2x%3")
      .arg(elementId)
      .arg(bounds.width())
      .arg(bounds.height());

  QElapsedTimer t;
  int elapsed = 0;

  if ( svgCache.contains(e) ) {
    totalCacheHits++;

    svgCacheEntry &entry = svgCache[e];
    entry.hits++;

    // element found in cache - apply transform, draw ONCE
    applyTransform(painter, bounds, transform);
    t.restart();
    painter->drawPixmap(bounds, entry.pixmap);
    elapsed = t.elapsed();
    restoreTransform(painter, bounds, transform);

    entry.cachedRenderTime += elapsed;
    totalCachedRenderTime += elapsed;
  } else {
    totalCacheMisses++;

    // not found, add to cache
    svgCacheEntry entry;
    entry.hits = entry.svgRenderTime = entry.cachedRenderTime = 0;

    // Render to pixmap with NEUTRAL painter (no transform)
    entry.pixmap = QPixmap(bounds.size());
    entry.pixmap.fill(Qt::transparent);
    QPainter p(&entry.pixmap);

    t.restart();
    renderer->render(&p, elementId, QRect(QPoint(0,0), bounds.size()));
    entry.mask = entry.pixmap.mask();
    elapsed = t.elapsed();

    // now render the pixmap using the original painter WITH transform
    applyTransform(painter, bounds, transform);
    painter->drawPixmap(bounds, entry.pixmap);
    restoreTransform(painter, bounds, transform);

    entry.svgRenderTime += elapsed;
    totalSvgRenderTime += elapsed;

    svgCache.insert(e, entry);
  }
}

QRegion QSvgCachedRenderer::elementRegion(const QString &elementId, const QRect &bounds)
{
  // key = elementId @ width x height
  const QString e = QString("%1@%2x%3")
      .arg(elementId)
      .arg(bounds.width())
      .arg(bounds.height());

  if ( svgCache.contains(e) ) {
    svgCacheEntry &entry = svgCache[e];
    return QRegion(entry.mask).translated(bounds.x(),bounds.y());
  } else {
    return QRegion(bounds);
  }
}

void QSvgCachedRenderer::dumpStats()
{
  qWarning() << "[QSvgCacheRenderer] Stats:";
  qWarning() << "Hits:" << totalCacheHits << "Misses:" << totalCacheMisses
             << "Ratio:" << totalCacheHits*100.0/(totalCacheHits+totalCacheMisses);
  qWarning() << "Cache render time:" << totalCachedRenderTime
             << "SVG render time:" << totalSvgRenderTime
             << "Cache speedup:" << totalSvgRenderTime*1.0/totalCachedRenderTime;
}
