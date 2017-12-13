/***************************************************************************
 *   Copyright (C) 2017 by Saïd LANKRI                                     *
 *   said.lankri@gmail.com                                                 *
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

#include "SvgGen.h"

#include <QDebug>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QLinearGradient>

/*
 * Converts a QPainterPath to an SVG d="..." attribute of a <path> element
 *
 * Note that it is too bad QPainterPath translates arcTo() directives
 * into a series of cubicTo() calls because arcTo() has a direct mapping
 * into a 'd' attribute.
 */
static QString painterPathToSvg_d(const QPainterPath &p)
{
  QString res;

  for (int i=0; i<p.elementCount(); i++) {
    QPainterPath::Element e = p.elementAt(i);

    switch (e.type) {
      case QPainterPath::MoveToElement:
        res.append(QString("M %1 %2 ").arg(e.x).arg(e.y));
        break;
      case QPainterPath::LineToElement:
        res.append(QString("L %1 %2 ").arg(e.x).arg(e.y));
        break;
      case QPainterPath::CurveToElement: {
        QPainterPath::Element e2 = p.elementAt(i+1);
        QPainterPath::Element e3 = p.elementAt(i+2);
        res.append(QString("C %1 %2 %3 %4 %5 %6 ").arg(e.x).arg(e.y).arg(e2.x)
            .arg(e2.y).arg(e3.x).arg(e3.y));
        break;
      }
      case QPainterPath::CurveToDataElement:
        /* nothing, handled within CurveToElement */
        break;
    }
  }

  return res;
}

SvgGenInterior::SvgGenInterior(QGraphicsItem* parent)
  : QGraphicsPathItem(parent), roundness(10), roundInterior(false),
    color1(Qt::green), color2(Qt::black), fill(FillTypeFlat)
{
  setPen(QPen(Qt::NoBrush,0));
  setSize(QSize(100,100));
}

SvgGenInterior::~SvgGenInterior()
{
}

void SvgGenInterior::setRoundInterior(bool hasIt)
{
  roundInterior = hasIt;

  calcInterior();
}

void SvgGenInterior::setInteriorRoundness(qreal val)
{
  roundness = val;

  calcInterior();
}

void SvgGenInterior::setSize(const QSizeF& sz)
{
  width = sz.width();
  height = sz.height();

  calcInterior();
}

void SvgGenInterior::setFillType(FillType type)
{
  fill = type;
  calcFill();
}

void SvgGenInterior::setFirstColor(const QColor &c)
{
  color1 = c;
  calcFill();
}

void SvgGenInterior::setSecondColor(const QColor &c)
{
  color2 = c;
  calcFill();
}

void SvgGenInterior::calcFill()
{
  // set fill
  switch (fill) {
    case FillTypeFlat: {
      setBrush(color1);
      break;
    }
    case FillTypeGradient : {
      QLinearGradient fill;
      QRectF r = boundingRect();

      fill.setColorAt(0, color1);
      fill.setColorAt(1, color2);
      fill.setStart(r.center().x(),r.top());
      fill.setFinalStop(r.center().x(),r.bottom());

      setBrush(QBrush(fill));
      break;
    }
    case FillTypeInvertedGradient : {
      QLinearGradient fill;
      QRectF r = boundingRect();

      fill.setColorAt(1, color1);
      fill.setColorAt(0, color2);
      fill.setStart(r.center().x(),r.top());
      fill.setFinalStop(r.center().x(),r.bottom());

      setBrush(QBrush(fill));
      break;
    }
  }
}

void SvgGenInterior::calcInterior()
{
  QPainterPath p;
  if ( !roundInterior ) {
    p.addRect(0,0,width,height);
  } else {
    p.addRoundedRect(0,0,width,height,roundness,roundness,Qt::AbsoluteSize);
  }

  setPath(p);
  calcFill();
}

QDomDocumentFragment SvgGenInterior::toSvg(QDomDocument &doc)
{
  QDomDocumentFragment res = doc.createDocumentFragment();
  QDomElement p = doc.createElement("path");

  p.setAttribute("d", painterPathToSvg_d(path()));
  p.setAttribute("style",QString("stroke:none;fill:%1;fill-opacity:%3")
                 .arg(color1.name(QColor::HexRgb))
                 .arg(color1.alphaF()));
  p.setAttribute("transform", QString("translate(%1,%2)").arg(pos().x()).arg(pos().y()));

  res.appendChild(p);

  return res;
}

SvgGenSubFrame::SvgGenSubFrame(QGraphicsItem* parent)
  : QObject(), QGraphicsPathItem(parent), color1(Qt::green),
    color2(Qt::black), fill(FillTypeFlat)
{
  setPen(QPen(Qt::NoBrush,0));

  width = height = 100;
  roundness = 0;
  sbwidth = 10;
  split = false;

  calcSubFrame();
}

SvgGenSubFrame::SvgGenSubFrame(const SvgGenSubFrame* prev)
  : QObject(), QGraphicsPathItem(NULL), color1(Qt::green),
    color2(Qt::black), fill(FillTypeFlat)
{
  setPen(QPen(Qt::NoBrush,0));

  width = prev->outerSubFrameRect().width();
  height =  prev->outerSubFrameRect().height();
  roundness = prev->outerCornerRoundness();
  sbwidth = prev->subFrameWidth();
  roundCorners = prev->hasRoundCorners();
  split = prev->isSplit();

  QPointF p = prev->pos()-QPointF(sbwidth,sbwidth);
  setPos(p);
  //setZValue(prev->zValue()-1);

  calcSubFrame();
}

SvgGenSubFrame::SvgGenSubFrame(const QSizeF &sz, qreal sbwidth, qreal roundness, bool round, bool split)
  : QObject(), QGraphicsPathItem(NULL), color1(Qt::green),
    color2(Qt::black), fill(FillTypeFlat)
{
  setPen(QPen(Qt::NoBrush,0));
  width = sz.width();
  height = sz.height();
  this->sbwidth = sbwidth;
  this->roundness = roundness;
  roundCorners = round;
  this->split = split;

  calcSubFrame();
}

SvgGenSubFrame::~SvgGenSubFrame()
{
}

void SvgGenSubFrame::setSize(const QSizeF& sz)
{
  width = sz.width();
  height = sz.height();

  calcSubFrame();
}

void SvgGenSubFrame::setCornerRoundness(qreal val)
{
  roundness = val;

  calcSubFrame();
}

void SvgGenSubFrame::setSubFrameWidth(qreal val)
{
  sbwidth = val;

  calcSubFrame();
}

void SvgGenSubFrame::setRoundCorners(bool hasIt)
{
  roundCorners = hasIt;

  calcSubFrame();
}

void SvgGenSubFrame::setSplitMode(bool val)
{
  split = val;

  calcSubFrame();
}

QRectF SvgGenSubFrame::outerSubFrameRect() const
{
  qreal penw = pen().widthF();
  QRectF r = boundingRect().adjusted(penw/2,penw/2,-penw/2,-penw/2);
  if ( split ) {
    const qreal d = 10.0;
    r.adjust(d,d,-d,-d);
  }
  return r;
}

void SvgGenSubFrame::setFirstColor(const QColor &c)
{
  color1 = c;
  calcFill();
}

void SvgGenSubFrame::setSecondColor(const QColor &c)
{
  color2 = c;
  calcFill();
}

void SvgGenSubFrame::setFillType(FillType type)
{
  fill = type;
  calcFill();
}

void SvgGenSubFrame::calcSubFrame()
{
  // in this function, prefix i=inner, o=outer
  QRectF isfRect = QRectF(0,0,width,height);
  QRectF osfRect = isfRect.marginsAdded(QMargins(sbwidth,sbwidth,sbwidth,sbwidth));

  // Adapted from QPainterPath.cpp::addRoundedRect()
  qreal w,h,x,y;
  qreal xr, yr; // X Radius, Y Radius
  qreal rxx2, ryy2;

  QPainterPath itop,ibottom,ileft,iright,itopleft,itopright,ibottomleft,ibottomright;
  QPainterPath otop,obottom,oleft,oright,otopleft,otopright,obottomleft,obottomright;

  // Inner subframe
  isfRect.getRect(&x,&y,&w,&h);

  // first subframe has always zero roundness (it is always square)
  // as it it not possible to add points to paths (only lines),
  // cheat here
  // FIXME causes graphic artifact
  bool zeroRoundness = (roundness == 0);
  if ( !roundness )
    roundness = qreal(0.00000001);

  if ( !roundCorners ) {
    itopleft.moveTo(x,y+roundness);
    itopleft.lineTo(x,y);
    itopleft.lineTo(x+roundness,y);

    itopright.moveTo(x+w-roundness,y);
    itopright.lineTo(x+w,y);
    itopright.lineTo(x+w,y+roundness);

    ibottomright.moveTo(x+w,y+h-roundness);
    ibottomright.lineTo(x+w,y+h);
    ibottomright.lineTo(x+w-roundness,y+h);

    ibottomleft.moveTo(x+roundness,y+h);
    ibottomleft.lineTo(x,y+h);
    ibottomleft.lineTo(x,y+h-roundness);
  } else {
    xr = yr = roundness;
    // translate absolute roundness into relative one to w/2 or h/2
    if (w == 0) {
      xr = 0;
    } else {
      xr = 100 * qMin(xr, w/2) / (w/2);
    }
    if (h == 0) {
      yr = 0;
    } else {
      yr = 100 * qMin(yr, h/2) / (h/2);
    }

    rxx2 = w*xr/100;
    ryy2 = h*yr/100;

    // draw corners
    itopleft.arcMoveTo(x, y, rxx2, ryy2, 180);
    itopleft.arcTo(x, y, rxx2, ryy2, 180, -90); // topleft

    itopright.moveTo(x+w-rxx2/2,y);
    itopright.arcTo(x+w-rxx2, y, rxx2, ryy2, 90, -90); // topright

    ibottomright.moveTo(x+w,y+h-ryy2/2);
    ibottomright.arcTo(x+w-rxx2, y+h-ryy2, rxx2, ryy2, 0, -90); // bottomright

    ibottomleft.moveTo(x+rxx2/2,y+h);
    ibottomleft.arcTo(x, y+h-ryy2, rxx2, ryy2, 270, -90); // bottomleft
    // END
  }

  // join corners
  itop.moveTo(itopleft.currentPosition()); // top
  itop.lineTo(itopright.elementAt(0));

  ileft.moveTo(itopleft.elementAt(0)); // left
  ileft.lineTo(ibottomleft.currentPosition());

  ibottom.moveTo(ibottomleft.elementAt(0)); // bottom
  ibottom.lineTo(ibottomright.currentPosition());

  iright.moveTo(itopright.currentPosition()); // right
  iright.lineTo(ibottomright.elementAt(0));

  // Outer subframe
  osfRect.getRect(&x,&y,&w,&h);

  if ( zeroRoundness )
    roundness = 0;

  if ( !roundCorners ) {
    otopleft.moveTo(x,y+roundness+sbwidth);
    otopleft.lineTo(x,y);
    otopleft.lineTo(x+roundness+sbwidth,y);

    otopright.moveTo(x+w-roundness-sbwidth,y);
    otopright.lineTo(x+w,y);
    otopright.lineTo(x+w,y+roundness+sbwidth);

    obottomright.moveTo(x+w,y+h-roundness-sbwidth);
    obottomright.lineTo(x+w,y+h);
    obottomright.lineTo(x+w-roundness-sbwidth,y+h);

    obottomleft.moveTo(x+roundness+sbwidth,y+h);
    obottomleft.lineTo(x,y+h);
    obottomleft.lineTo(x,y+h-roundness-sbwidth);
  } else {
    xr = yr = roundness+sbwidth;
    // translate absolute roundness into relative one to w/2 or h/2
    if (w == 0) {
      xr = 0;
    } else {
      xr = 100 * qMin(xr, w/2) / (w/2);
    }
    if (h == 0) {
      yr = 0;
    } else {
      yr = 100 * qMin(yr, h/2) / (h/2);
    }

    rxx2 = w*xr/100;
    ryy2 = h*yr/100;

    // draw corners first
    otopleft.arcMoveTo(x, y, rxx2, ryy2, 180);
    otopleft.arcTo(x, y, rxx2, ryy2, 180, -90); // topleft

    otopright.moveTo(x+w-rxx2/2,y);
    otopright.arcTo(x+w-rxx2, y, rxx2, ryy2, 90, -90); // topright

    obottomright.moveTo(x+w,y+h-ryy2/2);
    obottomright.arcTo(x+w-rxx2, y+h-ryy2, rxx2, ryy2, 0, -90); // bottomright

    obottomleft.moveTo(x+rxx2/2,y+h);
    obottomleft.arcTo(x, y+h-ryy2, rxx2, ryy2, 270, -90); // bottomleft
  }

  otop.moveTo(otopleft.currentPosition()); // top
  otop.lineTo(otopright.elementAt(0));

  oleft.moveTo(otopleft.elementAt(0)); // left
  oleft.lineTo(obottomleft.currentPosition());

  obottom.moveTo(obottomleft.elementAt(0)); // bottom
  obottom.lineTo(obottomright.currentPosition());

  oright.moveTo(otopright.currentPosition()); // right
  oright.lineTo(obottomright.elementAt(0));

  // Unite i* and o* elements
  top = itop;
  top.connectPath(otop.toReversed());
  top.closeSubpath();

  bottom = ibottom;
  bottom.connectPath(obottom.toReversed());
  bottom.closeSubpath();

  left = ileft;
  left.connectPath(oleft.toReversed());
  left.closeSubpath();

  right = iright;
  right.connectPath(oright.toReversed());
  right.closeSubpath();

  topleft = itopleft;
  topleft.connectPath(otopleft.toReversed());
  topleft.closeSubpath();

  bottomleft = ibottomleft;
  bottomleft.connectPath(obottomleft.toReversed());
  bottomleft.closeSubpath();

  topright = itopright;
  topright.connectPath(otopright.toReversed());
  topright.closeSubpath();

  bottomright = ibottomright;
  bottomright.connectPath(obottomright.toReversed());
  bottomright.closeSubpath();

  if ( !split ) {
    // Non split mode: unite corners into inner and outer subframes
    QPainterPath isf, osf;

    isf.addPath(itopleft);
    isf.connectPath(itopright);
    isf.connectPath(ibottomright);
    isf.connectPath(ibottomleft);
    isf.closeSubpath();

    osf.addPath(otopleft);
    osf.connectPath(otopright);
    osf.connectPath(obottomright);
    osf.connectPath(obottomleft);
    osf.closeSubpath();

    // Finally unite inner and outer subframes
    osf.addPath(isf);
    setPath(osf);
  } else {
    // split mode: add each part individually

    const qreal d = 10.0;
    top.translate(0,-d);
    bottom.translate(0,d);

    topleft.translate(-d,-d);
    bottomleft.translate(-d,d);

    topright.translate(d,-d);
    bottomright.translate(d,d);

    left.translate(-d,0);
    right.translate(d,0);

    QPainterPath p;
    p.addPath(topleft);
    p.addPath(top);
    p.addPath(topright);
    p.addPath(right);
    p.addPath(bottomright);
    p.addPath(bottom);
    p.addPath(bottomleft);
    p.addPath(left);

    setPath(p);
  }

  calcFill();
//   qDebug() << osfRect.center()
//            << osf.contains(osfRect.center())
//            << contains(osfRect.center())
//            << shape().contains(osfRect.center())
//            << path().contains(osfRect.center());
}

void SvgGenSubFrame::calcFill()
{
  // set fill
  switch (fill) {
    case FillTypeFlat: {
      setBrush(color1);
      break;
    }
    case FillTypeGradient : {
      QLinearGradient fill;
      QRectF r = boundingRect();

      fill.setColorAt(0, color1);
      fill.setColorAt(1, color2);
      fill.setStart(r.center().x(),r.top());
      fill.setFinalStop(r.center().x(),r.bottom());

      setBrush(QBrush(fill));
      break;
    }
    case FillTypeInvertedGradient : {
      QLinearGradient fill;
      QRectF r = boundingRect();

      fill.setColorAt(1, color1);
      fill.setColorAt(0, color2);
      fill.setStart(r.center().x(),r.top());
      fill.setFinalStop(r.center().x(),r.bottom());

      setBrush(QBrush(fill));
      break;
    }
  }
}

bool SvgGenSubFrame::contains(const QPointF& point) const
{
  //return QGraphicsPathItem::contains(point);
  //qDebug() << "contains" << path().contains(point);
  return path().contains(point);
}

QDomDocumentFragment SvgGenSubFrame::toSvg(QDomDocument doc, const QString &part)
{
  QDomDocumentFragment res = doc.createDocumentFragment();

  QPainterPath path;
  if ( part == "top" )
    path = top;
  else if ( part == "bottom" )
    path = bottom;
  else if ( part == "left" )
    path = left;
  else if ( part == "right" )
    path = right;
  else if ( part == "topleft" )
    path = topleft;
  else if ( part == "topright" )
    path = topright;
  else if ( part == "bottomleft" )
    path = bottomleft;
  else if ( part == "bottomright" )
    path = bottomright;

  QDomElement p = doc.createElement("path");

  p.setAttribute("d", painterPathToSvg_d(path));
  p.setAttribute("style",QString("stroke:none;fill:%1;fill-opacity:%3")
                 .arg(color1.name(QColor::HexRgb))
                 .arg(color1.alphaF()));
  p.setAttribute("transform", QString("translate(%1,%2)").arg(pos().x()).arg(pos().y()));

  res.appendChild(p);

  return res;
}

// QPainterPath SvgGenSubFrame::shape() const
// {
//   return path();
// }

SvgGen::SvgGen(QGraphicsScene *scene, QObject *parent)
  : QObject(parent),
  width(100),
  height(100),
  shadowwidth(5),
  framewidth(1),
  m_hasShadow(false),
  m_hasInterior(true),
  m_hasFrame(false),
  m_roundMode(true),
  m_splitMode(true),
  m_basename("basename"),
  m_variant("variant"),
  m_status("status"),
  center(QPointF(0,0)),
  scene(scene),
  interior(0),
  shadow(0),
  cross(0)
{
  scene->setParent(this);

  setHasFrame(m_hasFrame);
  setHasInterior(m_hasInterior);
  setHasShadow(m_hasShadow);

  QPen crossPen(Qt::blue);
//   crossPen.setStyle(Qt::DashLine);

  QPainterPath crossPath;
  crossPath.moveTo(center.x(),center.y()-10);
  crossPath.lineTo(center.x(),center.y()+10);
  crossPath.moveTo(center.x()-10,center.y());
  crossPath.lineTo(center.x()+10,center.y());
  cross = new QGraphicsPathItem(crossPath);
  cross->setPen(crossPen);
  cross->setZValue(9999);
  scene->addItem(cross);

  //qDebug() << scene->items(interior->pos()+interior->boundingRect().center());
}

SvgGen::~SvgGen()
{
}

void SvgGen::setHasInterior(bool hasIt)
{
  m_hasInterior = hasIt;

  if ( hasIt ) {
    if ( !interior ) {
      interior = new SvgGenInterior(NULL);
      interior->setSize(QSize(width,height));
      QPointF oldCenter = interior->pos()+interior->boundingRect().center();
      QPointF delta = center-oldCenter;
      interior->moveBy(delta.x(),delta.y());

      if ( !m_hasFrame ) {
        interior->setRoundInterior(m_roundMode);
      } else {
        interior->setRoundInterior(false);
      }

      scene->addItem(interior);
    }
  } else {
    if ( interior ) {
      scene->removeItem(interior);
      delete interior;
      interior = NULL;
    }
  }

  // rebuild shadow
  rebuildShadow();
}

void SvgGen::setHasShadow(bool hasIt)
{
  m_hasShadow = hasIt;

  if ( !hasIt ) {
    if ( shadow ) {
      scene->removeItem(shadow);
      delete shadow;
      shadow = NULL;
    }
    if ( interior )
      interior->setRoundInterior(m_roundMode);
  } else {
    if ( !shadow ) {
      // add a subframe ...
      pushSubFrame();
      // then take it as a shadow
      shadow = subFrames.takeLast();
      shadow->setSubFrameWidth(shadowwidth);
      shadow->setFirstColor(Qt::lightGray);
      shadow->setSecondColor(Qt::gray);
      if ( m_roundMode && !m_hasFrame && m_hasInterior ) {
        shadow->setCornerRoundness(interior->interiorRoundness());
      }
    }
  }
}

void SvgGen::setHasFrame(bool hasIt)
{
  m_hasFrame = hasIt;

  if ( !hasIt ) {
    for (int i=subFrames.count(); i>0; i--)
      popSubFrame();
  } else {
    for (int i=subFrames.count(); i<framewidth; i++)
      pushSubFrame();
  }

  // rebuild shadow
  rebuildShadow();
}

void SvgGen::setRoundMode(bool hasIt)
{
  m_roundMode = hasIt;

  if ( hasIt ) {
    if ( m_hasFrame ) {
      Q_FOREACH(SvgGenSubFrame *sf, subFrames) {
        sf->setRoundCorners(true);
      }
      if ( interior )
        interior->setRoundInterior(false);
    } else {
      if ( interior )
        interior->setRoundInterior(true);
    }
  } else {
    Q_FOREACH(SvgGenSubFrame *sf, subFrames) {
      sf->setRoundCorners(false);
    }
    if ( interior )
      interior->setRoundInterior(false);
  }

  rebuildShadow();
}

void SvgGen::setInteriorRoundness(qreal val)
{
  if ( interior )
    interior->setInteriorRoundness(val);

  rebuildShadow();
}

void SvgGen::setShadowWidth(qreal val)
{
  shadowwidth = val;

  if ( shadow )
    shadow->setSubFrameWidth(val);
}

void SvgGen::setSplitMode(bool hasIt)
{
  m_splitMode = hasIt;

  Q_FOREACH(SvgGenSubFrame *sf, subFrames) {
    sf->setSplitMode(hasIt);
  }

  rebuildShadow();
}

void SvgGen::pushSubFrame()
{
  int nb = subFrames.count();
  SvgGenSubFrame *sf = NULL;
  if ( nb > 0 ) {
    sf = new SvgGenSubFrame(subFrames[nb-1]);
  } else {
    if ( m_hasFrame ) {
      sf = new SvgGenSubFrame(QSizeF(width,height),5,0,m_roundMode,m_splitMode);
    } else {
      // no frame -> this is the shadow -> no split
      sf = new SvgGenSubFrame(QSizeF(width,height),5,0,m_roundMode,false);
    }
  }

  //qDebug() << "push subframe" << nb << "shape" << sf->shape();

  QPointF oldCenter = sf->pos()+sf->boundingRect().center();
  QPointF delta = center-oldCenter;
  sf->moveBy(delta.x(),delta.y());

  if ( interior && m_hasFrame ) {
    interior->setRoundInterior(false);
  }

  subFrames.append(sf);
  scene->addItem(sf);
}

void SvgGen::popSubFrame()
{
  if ( !subFrames.count() )
    return;

  SvgGenSubFrame *sf = subFrames.takeLast();
  scene->removeItem(sf);
  delete sf;

  if ( !subFrames.count() ) {
    if ( interior )
      interior->setRoundInterior(m_roundMode);
  }
  // rebuild shadow
  rebuildShadow();
}

void SvgGen::rebuildShadow()
{
  if ( m_hasShadow ) {
    QColor c1 = shadow->firstColor();
    QColor c2 = shadow->secondColor();
    SvgGenSubFrame::FillType ft = shadow->fillType();

    setHasShadow(false);
    setHasShadow(true);
    shadow->setFirstColor(c1);
    shadow->setSecondColor(c2);
    shadow->setFillType(ft);
  }
}

void SvgGen::setFrameWidth(int width)
{
  framewidth = width;
  int nb = subFrames.count();
  if ( m_hasFrame ) {
    if ( width > nb ) {
      while (nb++ < width)
        pushSubFrame();
    } else {
      while (nb-- > width)
        popSubFrame();
    }
  }
  // rebuild shadow
  rebuildShadow();
}

void SvgGen::setSize(const QSizeF& sz)
{
  width = sz.width();
  height = sz.height();

  QSizeF sz2 = sz;
  Q_FOREACH(SvgGenSubFrame *sf, subFrames) {
    sf->setSize(sz2);
    sz2 = sf->outerSubFrameRect().size();
  }

  if ( interior )
    interior->setSize(sz);

  rebuildShadow();

  setCenter(center);
}

void SvgGen::setSubFrameWidth(int idx, qreal width)
{
  if ( idx > subFrames.count()-1 )
    return;

  subFrames[idx]->setSubFrameWidth(width);

  /* adjust all subframes outwards */
  QSizeF sz = subFrames[idx]->outerSubFrameRect().size();
  qreal rnd = subFrames[idx]->outerCornerRoundness();
  for (int i=idx+1; i<subFrames.count(); i++) {
    subFrames[i]->setCornerRoundness(rnd);
    subFrames[i]->setSize(sz);

    sz = subFrames[i]->outerSubFrameRect().size();
    rnd = subFrames[i]->outerCornerRoundness();
  }

  // rebuild shadow
  rebuildShadow();

  setCenter(center);
}

void SvgGen::setCenter(const QPointF &p)
{
  center = p;
  QPointF oldCenter, delta;

  Q_FOREACH(SvgGenSubFrame *sf, subFrames) {
    oldCenter = sf->pos()+sf->boundingRect().center();
    delta = p-oldCenter;
    sf->moveBy(delta.x(),delta.y());
  }

  if ( interior ) {
    oldCenter = interior->pos()+interior->boundingRect().center();
    delta = p-oldCenter;
    interior->moveBy(delta.x(),delta.y());
  }

  if ( shadow ) {
    oldCenter = shadow->pos()+shadow->boundingRect().center();
    delta = p-oldCenter;
    shadow->moveBy(delta.x(),delta.y());
  }

  oldCenter = cross->pos()+cross->boundingRect().center();
  delta = p-oldCenter;
  cross->moveBy(delta.x(),delta.y());
}

void SvgGen::setSubFrameFillType(int idx, SvgGenSubFrame::FillType type)
{
  if ( idx > subFrames.count()-1 )
    return;

  subFrames[idx]->setFillType(type);
}

void SvgGen::setSubFrameFirstColor(int idx, const QColor &c)
{
  if ( idx > subFrames.count()-1 )
    return;

  subFrames[idx]->setFirstColor(c);
}

void SvgGen::setSubFrameSecondColor(int idx, const QColor &c)
{
  if ( idx > subFrames.count()-1 )
    return;

  subFrames[idx]->setSecondColor(c);
}

void SvgGen::setInteriorFillType(SvgGenInterior::FillType type)
{
  if ( interior )
    interior->setFillType(type);
}

void SvgGen::setInteriorFirstColor(const QColor &c)
{
  if ( interior )
    interior->setFirstColor(c);
}

void SvgGen::setInteriorSecondColor(const QColor &c)
{
  if ( interior )
    interior->setSecondColor(c);
}

void SvgGen::setShadowFillType(SvgGenSubFrame::FillType type)
{
  if ( shadow )
    shadow->setFillType(type);
}

void SvgGen::setShadowFirstColor(const QColor &c)
{
  if ( shadow )
    shadow->setFirstColor(c);
}

void SvgGen::setShadowSecondColor(const QColor &c)
{
  if ( shadow )
    shadow->setSecondColor(c);
}

QDomDocument SvgGen::toSvg()
{
  QDomDocument doc;

  QDomProcessingInstruction xml =
    doc.createProcessingInstruction("xml",
      "version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"");

  QDomElement svg = doc.createElement("svg");
  svg.setAttributeNS("xmlns","svg","http://www.w3.org/2000/svg");
  svg.setAttribute("xmlns","http://www.w3.org/2000/svg");
  svg.setAttribute("version","1.1");

  QDomElement g = doc.createElement("g");
  g.setAttribute("id",QString("qsvggen-%1-%2-%3").arg(m_basename).arg(m_variant).arg(m_status));

  QDomElement qsvg_global = doc.createElement("qsvgstyle");
  qsvg_global.setAttribute("type","gen");
  qsvg_global.setAttribute("interior", m_hasInterior ? "true" : "false");
  qsvg_global.setAttribute("frame", m_hasFrame ? "true" : "false");
  qsvg_global.setAttribute("shadow", m_hasShadow ? "true" : "false");
  qsvg_global.setAttribute("split", m_splitMode ? "true" : "false");
  qsvg_global.setAttribute("square", (width == height) ? "true" : "false");
  qsvg_global.setAttribute("round", m_roundMode ? "true" : "false");
  qsvg_global.setAttribute("basename", m_basename);
  qsvg_global.setAttribute("variant", m_variant);
  qsvg_global.setAttribute("status", m_status);

  QDomElement _interior = doc.createElement("g");
  if ( m_variant.isEmpty() )
    _interior.setAttribute("id", QString("%1-%2").arg(m_basename).arg(m_status));
  else
    _interior.setAttribute("id", QString("%1-%2-%3").arg(m_basename).arg(m_variant).arg(m_status));
  QDomElement qsvg_interior = doc.createElement("qsvgstyle");
  if ( m_hasInterior ) {
    qsvg_interior.setAttribute("type", "interior");
    qsvg_interior.setAttribute("roundness", interior->interiorRoundness());
  }

  QDomElement frame = doc.createElement("g");
  if ( m_variant.isEmpty() )
    frame.setAttribute("id", QString("%1-frame-%2").arg(m_basename).arg(m_status));
  else
    frame.setAttribute("id", QString("%1-frame-%2-%3").arg(m_basename).arg(m_variant).arg(m_status));
  QDomElement qsvg_frame = doc.createElement("qsvgstyle");
  if ( m_hasFrame ) {
    qsvg_frame.setAttribute("type", "frame");
    qsvg_frame.setAttribute("framewidth", framewidth);
    QString subwidths;
    Q_FOREACH(SvgGenSubFrame *sf, subFrames)
      subwidths.append(QString("%1,").arg(sf->subFrameWidth()));
    qsvg_frame.setAttribute("subframewidths", subwidths);
    qsvg_frame.setAttribute("shadowwitdh",shadowwidth);
  }

  // Organize nodes inside SVG+XML document
  doc.appendChild(xml);
  doc.appendChild(svg);

  if ( m_hasInterior || m_hasFrame ) {
    svg.appendChild(g);
    g.appendChild(qsvg_global);
  }

  if ( m_hasInterior ) {
    g.appendChild(_interior);
    _interior.appendChild(qsvg_interior);
    _interior.appendChild(interior->toSvg(doc));
    if ( !m_hasFrame && m_hasShadow ) {
      QStringList parts;
      parts << "top" << "bottom" << "left" << "right" << "topleft" << "topright"
            << "bottomleft" << "bottomright";
      Q_FOREACH(QString part, parts) {
        _interior.appendChild(shadow->toSvg(doc, part));
      }
    }
  }

  if ( m_hasFrame ) {
    g.appendChild(frame);
    frame.appendChild(qsvg_frame);

    QStringList parts;
    parts << "top" << "bottom" << "left" << "right" << "topleft" << "topright"
          << "bottomleft" << "bottomright";
    Q_FOREACH(QString part, parts) {
      QDomElement gpart = doc.createElement("g");
      if ( m_variant.isEmpty() )
        gpart.setAttribute("id", QString("%1-%2-%3").arg(m_basename).arg(m_status).arg(part));
      else
        gpart.setAttribute("id", QString("%1-%2-%3-%4").arg(m_basename).arg(m_variant).arg(m_status).arg(part));
      Q_FOREACH(SvgGenSubFrame *sf, subFrames) {
        gpart.appendChild(sf->toSvg(doc, part));
        if ( m_hasShadow )
          gpart.appendChild(shadow->toSvg(doc, part));
      }
      frame.appendChild(gpart);
    }
  }

  return doc;
}
