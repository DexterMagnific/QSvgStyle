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

  // close path
  res.append("Z");

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
  p.setAttribute("transform", QString("translate(%1,%2)")
                 .arg(x())
                 .arg(y()));

  res.appendChild(p);

  return res;
}

SvgGenSubFrame::SvgGenSubFrame()
  : QObject(), QGraphicsItemGroup(NULL), color1(Qt::blue),
    color2(Qt::black), fill(FillTypeFlat)
{
  m_width = m_height = 100;
  roundness = 0;
  sbwidth = 10;
  split = false;
  round = true;
  hastop = hasbottom = hasleft = hasright = true;

  top = new QGraphicsPathItem();
  bottom = new QGraphicsPathItem();
  left = new QGraphicsPathItem();
  right = new QGraphicsPathItem();
  topleft = new QGraphicsPathItem();
  topright = new QGraphicsPathItem();
  bottomleft = new QGraphicsPathItem();
  bottomright = new QGraphicsPathItem();

  QPen p(Qt::NoPen);
  QBrush b(color1);

  top->setPen(p);
  bottom->setPen(p);
  left->setPen(p);
  right->setPen(p);
  topleft->setPen(p);
  topright->setPen(p);
  bottomleft->setPen(p);
  bottomright->setPen(p);

  top->setBrush(b);
  bottom->setBrush(b);
  left->setBrush(b);
  right->setBrush(b);
  topleft->setBrush(b);
  topright->setBrush(b);
  bottomleft->setBrush(b);
  bottomright->setBrush(b);

  addToGroup(top);
  addToGroup(bottom);
  addToGroup(left);
  addToGroup(right);
  addToGroup(topleft);
  addToGroup(topright);
  addToGroup(bottomleft);
  addToGroup(bottomright);

  calcSubFrame();
}

SvgGenSubFrame::SvgGenSubFrame(const SvgGenSubFrame* prev)
  : SvgGenSubFrame()
{
  m_width = prev->width();
  m_height =  prev->height();
  roundness = prev->outerCornerRoundness();
  sbwidth = prev->subFrameWidth();
  round = prev->hasRoundCorners();
  split = prev->isSplit();
  hastop = prev->hasTop();
  hasbottom = prev->hasBottom();
  hasleft = prev->hasLeft();
  hasright = prev->hasRight();

  QPointF p = prev->pos()-QPointF(sbwidth,sbwidth);
  setPos(p);
  //setZValue(prev->zValue()-1);

  calcSubFrame();
}

SvgGenSubFrame::SvgGenSubFrame(const QSizeF &sz, qreal sbwidth, qreal roundness, bool round, bool split)
  : SvgGenSubFrame()
{
  m_width = sz.width();
  m_height = sz.height();
  this->sbwidth = sbwidth;
  this->roundness = roundness;
  this->round = round;
  this->split = split;
  hastop = hasbottom = hasleft = hasright = true;

  calcSubFrame();
}

SvgGenSubFrame::~SvgGenSubFrame()
{
}

void SvgGenSubFrame::setSize(const QSizeF& sz)
{
  m_width = sz.width();
  m_height = sz.height();

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
  round = hasIt;

  calcSubFrame();
}

void SvgGenSubFrame::setSplitMode(bool val)
{
  split = val;

  calcSubFrame();
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

void SvgGenSubFrame::setHasTop(bool hasIt)
{
  hastop = hasIt;

  calcFill();
}

void SvgGenSubFrame::setHasBottom(bool hasIt)
{
  hasbottom = hasIt;

  calcFill();
}

void SvgGenSubFrame::setHasLeft(bool hasIt)
{
  hasleft = hasIt;

  calcFill();
}

void SvgGenSubFrame::setHasRight(bool hasIt)
{
  hasright = hasIt;

  calcFill();
}

void SvgGenSubFrame::setFillType(FillType type)
{
  fill = type;
  calcFill();
}

QRectF SvgGenSubFrame::boundingRect() const
{
  return top->mapRectToParent(top->boundingRect()).united(
        bottom->mapRectToParent(bottom->boundingRect())).united(
        left->mapRectToParent(left->boundingRect())).united(
        right->mapRectToParent(right->boundingRect())).united(
        topleft->mapRectToParent(topleft->boundingRect())).united(
        topright->mapRectToParent(topright->boundingRect())).united(
        bottomleft->mapRectToParent(bottomleft->boundingRect())).united(
        bottomright->mapRectToParent(bottomright->boundingRect()));
}

void SvgGenSubFrame::calcSubFrame()
{
  // NOTE all parts will be built at 0,0 and then moved to their final location
  QPainterPath tl,tr,bl,br;
  QPainterPath t,b,l,r;
  QPainterPath lj,rj;

  // reminder: thickness of subframe is sbwidth
  // width,height is the size of the sides (without corners)

  // outer corner size
  qreal oR = roundness+sbwidth;
  // inner corner size
  qreal iR = roundness;

  // sides
  t.addRect(0,0,m_width,sbwidth);
  top->setPath(t);
  top->setPos(0,-oR);

  b.addRect(0,0,m_width,sbwidth);
  bottom->setPath(b);
  bottom->setPos(0,m_height+iR);

  l.addRect(0,0,sbwidth,m_height);
  left->setPath(l);
  left->setPos(-oR,0);

  r.addRect(0,0,sbwidth,m_height);
  right->setPath(r);
  right->setPos(m_width+iR,0);

  // corners
  if ( round ) {
    tl.moveTo(0,oR);
    tl.arcTo(0,0,2*oR,2*oR, 180/*start angle*/, -90/*length*/);
    tl.lineTo(oR,sbwidth);
    tl.arcTo(sbwidth,sbwidth,2*iR,2*iR, 90, 90);
  } else {
    tl.moveTo(0,0);
    tl.lineTo(oR,0);
    tl.lineTo(oR,sbwidth);
    tl.lineTo(sbwidth,sbwidth);
    tl.lineTo(sbwidth,oR);
    tl.lineTo(0,oR);
  }
  topleft->setPath(tl);
  topleft->setPos(-oR,-oR);

  if ( round ) {
    // I cannot explain why this is not necessary
    //tr.moveTo(0,0);
    tr.arcTo(-oR,0,2*oR,2*oR, 90, -90);
    tr.lineTo(iR,oR);
    tr.arcTo(-iR,sbwidth,2*iR,2*iR, 0, 90);
  } else {
    tr.moveTo(0,0);
    tr.lineTo(oR,0);
    tr.lineTo(oR,oR);
    tr.lineTo(iR,oR);
    tr.lineTo(iR,sbwidth);
    tr.lineTo(0,sbwidth);
  }
  topright->setPath(tr);
  topright->setPos(m_width,-oR);

  if ( round ) {
    //bl.moveTo(0,0);
    bl.arcTo(0,-oR,2*oR,2*oR, 180, 90);
    bl.lineTo(oR,iR);
    bl.arcTo(sbwidth,-iR,2*iR,2*iR, -90, -90);
  } else {
    bl.moveTo(0,0);
    bl.lineTo(0,oR);
    bl.lineTo(oR,oR);
    bl.lineTo(oR,iR);
    bl.lineTo(sbwidth,iR);
    bl.lineTo(sbwidth,0);
  }
  bottomleft->setPath(bl);
  bottomleft->setPos(-oR,m_height);

  if ( round ) {
    // I cannot explain why this gives wrong result. it should
    // be necessary
    //br.moveTo(0,oR);
    // FIXME there is still an unecessary node at 0,0
    br.arcTo(-oR,-oR,2*oR,2*oR, 0, -90);
    br.lineTo(0,iR);
    br.arcTo(-iR,-iR,2*iR,2*iR, -90, 90);
  } else {
    br.moveTo(oR,0);
    br.lineTo(oR,oR);
    br.lineTo(0,oR);
    br.lineTo(0,iR);
    br.lineTo(iR,iR);
    br.lineTo(iR,0);
  }
  bottomright->setPath(br);
  bottomright->setPos(m_width,m_height);

  if ( split ) {
    const qreal d = 10.0;
    topleft->moveBy(-d,-d);
    topright->moveBy(d,-d);
    bottomleft->moveBy(-d,d);
    bottomright->moveBy(d,d);
    left->moveBy(-d,0);
    right->moveBy(d,0);
    top->moveBy(0,-d);
    bottom->moveBy(0,d);
  }

//  qWarning() << "bounding rects\n"
//             << "\ntop" << top->mapRectToParent(top->boundingRect())
//             << "\nbottom" << bottom->boundingRect().translated(bottom->pos())
//             << "\nleft" << left->boundingRect().translated(left->pos())
//             << "\nright" << right->boundingRect().translated(right->pos())
//             << "\ntopleft" << topleft->boundingRect().translated(topleft->pos())
//             << "\nbottomleft" << bottomleft->boundingRect().translated(topright->pos())
//             << "\ntopright" << topright->boundingRect().translated(bottomleft->pos())
//             << "\nbottomright" << bottomright->boundingRect().translated(bottomright->pos())
//             << "\n*****bounding" << boundingRect()
//             << "\niR" << iR << "oR" << oR;

  calcFill();
}

void SvgGenSubFrame::calcFill()
{
  // set fill
  switch (fill) {
    case FillTypeFlat: {
      top->setBrush(hastop ? color1 : Qt::transparent);
      bottom->setBrush(hasbottom ? color1 : Qt::transparent);
      left->setBrush(hasleft ? color1 : Qt::transparent);
      right->setBrush(hasright ? color1 : Qt::transparent);
      topleft->setBrush(hastop && hasleft ? color1 : Qt::transparent);
      topright->setBrush(hastop && hasright ? color1 : Qt::transparent);
      bottomleft->setBrush(hasbottom && hasleft ? color1 : Qt::transparent);
      bottomright->setBrush(hasbottom && hasright ? color1 : Qt::transparent);
      break;
    }
    case FillTypeGradient : {
      QLinearGradient fill;
      QRectF r = boundingRect();

      fill.setColorAt(0, color1);
      fill.setColorAt(1, color2);
      fill.setStart(r.center().x(),r.top());
      fill.setFinalStop(r.center().x(),r.bottom());

      QBrush b(fill);

      top->setBrush(hastop ? b : Qt::NoBrush);
      bottom->setBrush(hasbottom ? b : Qt::NoBrush);
      left->setBrush(hasleft ? b : Qt::NoBrush);
      right->setBrush(hasright ? b : Qt::NoBrush);
      topleft->setBrush(hastop && hasleft ? b : Qt::NoBrush);
      topright->setBrush(hastop && hasright ? b : Qt::NoBrush);
      bottomleft->setBrush(hasbottom && hasleft ? b : Qt::NoBrush);
      bottomright->setBrush(hasbottom && hasright ? b : Qt::NoBrush);
      break;
    }
    case FillTypeInvertedGradient : {
      QLinearGradient fill;
      QRectF r = boundingRect();

      fill.setColorAt(1, color1);
      fill.setColorAt(0, color2);
      fill.setStart(r.center().x(),r.top());
      fill.setFinalStop(r.center().x(),r.bottom());

      QBrush b(fill);

      top->setBrush(hastop ? b : Qt::NoBrush);
      bottom->setBrush(hasbottom ? b : Qt::NoBrush);
      left->setBrush(hasleft ? b : Qt::NoBrush);
      right->setBrush(hasright ? b : Qt::NoBrush);
      topleft->setBrush(hastop && hasleft ? b : Qt::NoBrush);
      topright->setBrush(hastop && hasright ? b : Qt::NoBrush);
      bottomleft->setBrush(hasbottom && hasleft ? b : Qt::NoBrush);
      bottomright->setBrush(hasbottom && hasright ? b : Qt::NoBrush);
      break;
    }
  }
}

QDomDocumentFragment SvgGenSubFrame::toSvg(QDomDocument doc, const QString &part)
{
  QDomDocumentFragment res = doc.createDocumentFragment();

  QGraphicsPathItem *item = 0;
  bool hasIt = true;

  if ( part == "top" ) {
    item = top;
    hasIt = hastop;
  } else if ( part == "bottom" ) {
    item = bottom;
    hasIt = hasbottom;
  } else if ( part == "left" ) {
    item = left;
    hasIt = hasleft;
  } else if ( part == "right" ) {
    item = right;
    hasIt = hasright;
  } else if ( part == "topleft" ) {
    item = topleft;
    hasIt = hastop && hasleft;
  } else if ( part == "topright" ) {
    item = topright;
    hasIt = hastop && hasright;
  } else if ( part == "bottomleft" ) {
    item = bottomleft;
    hasIt = hasbottom && hasleft;
  } else if ( part == "bottomright" ) {
    item = bottomright;
    hasIt = hasbottom && hasright;
  }

  QDomElement p = doc.createElement("path");

  p.setAttribute("d", painterPathToSvg_d(item->path()));
  p.setAttribute("style",QString("stroke:none;fill:%1;fill-opacity:%3")
                 .arg(hasIt ? color1.name(QColor::HexRgb) : "none")
                 .arg(hasIt ? color1.alphaF() : 0));
  p.setAttribute("transform", QString("translate(%1,%2)")
                 .arg(item->x()+x())
                 .arg(item->y()+y()));

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
  m_hasTopFrame(true),
  m_hasBottomFrame(true),
  m_hasLeftFrame(true),
  m_hasRightFrame(true),
  m_hasTopShadow(true),
  m_hasBottomShadow(true),
  m_hasLeftShadow(true),
  m_hasRightShadow(true),
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
    if ( interior && !m_hasFrame )
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
        // shadow for interior
        shadow->setCornerRoundness(interiorRoundness());
      }
      shadow->setHasTop(m_hasTopFrame && m_hasTopShadow);
      shadow->setHasBottom(m_hasBottomFrame && m_hasBottomShadow);
      shadow->setHasLeft(m_hasLeftFrame && m_hasLeftShadow);
      shadow->setHasRight(m_hasRightFrame && m_hasRightShadow);
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

void SvgGen::setHasTopFrame(bool hasIt) {
  m_hasTopFrame = hasIt;

  Q_FOREACH(SvgGenSubFrame *sf, subFrames) {
    sf->setHasTop(hasIt);
  }

  if ( shadow ) {
    shadow->setHasTop(m_hasTopShadow && hasIt);
  }
}

void SvgGen::setHasBottomFrame(bool hasIt) {
  m_hasBottomFrame = hasIt;

  Q_FOREACH(SvgGenSubFrame *sf, subFrames) {
    sf->setHasBottom(hasIt);
  }

  if ( shadow ) {
    shadow->setHasBottom(m_hasBottomShadow && hasIt);
  }
}

void SvgGen::setHasLeftFrame(bool hasIt) {
  m_hasLeftFrame = hasIt;

  Q_FOREACH(SvgGenSubFrame *sf, subFrames) {
    sf->setHasLeft(hasIt);
  }

  if ( shadow ) {
    shadow->setHasLeft(m_hasLeftShadow && hasIt);
  }
}

void SvgGen::setHasRightFrame(bool hasIt) {
  m_hasRightFrame = hasIt;

  Q_FOREACH(SvgGenSubFrame *sf, subFrames) {
    sf->setHasRight(hasIt);
  }

  if ( shadow ) {
    shadow->setHasRight(m_hasRightShadow && hasIt);
  }
}

void SvgGen::setHasTopShadow(bool hasIt) {
  m_hasTopShadow = hasIt;

  if ( shadow ) {
    shadow->setHasTop(m_hasTopFrame && hasIt);
  }
}

void SvgGen::setHasBottomShadow(bool hasIt) {
  m_hasBottomShadow = hasIt;

  if ( shadow ) {
    shadow->setHasBottom(m_hasBottomFrame && hasIt);
  }
}

void SvgGen::setHasLeftShadow(bool hasIt) {
  m_hasLeftShadow = hasIt;

  if ( shadow ) {
    shadow->setHasLeft(m_hasLeftFrame && hasIt);
  }
}

void SvgGen::setHasRightShadow(bool hasIt) {
  m_hasRightShadow = hasIt;

  if ( shadow ) {
    shadow->setHasRight(m_hasRightFrame && hasIt);
  }
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
      sf->setHasTop(m_hasTopFrame);
      sf->setHasBottom(m_hasBottomFrame);
      sf->setHasLeft(m_hasLeftFrame);
      sf->setHasRight(m_hasRightFrame);
    } else {
      // no frame -> this is the shadow -> no split + adjust width/height
      // width/height of subframe is for sides only, excluding corner size
      QSizeF sz(width,height);
      if ( m_roundMode )
        sz = QSizeF(qMax(0.0,width-2*interiorRoundness()),
                    qMax(0.0,height-2*interiorRoundness()));
      sf = new SvgGenSubFrame(sz,5,0,m_roundMode,false);
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

  Q_FOREACH(SvgGenSubFrame *sf, subFrames) {
    sf->setSize(sz);
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
  qreal rnd = subFrames[idx]->outerCornerRoundness();
  for (int i=idx+1; i<subFrames.count(); i++) {
    subFrames[i]->setCornerRoundness(rnd);

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
