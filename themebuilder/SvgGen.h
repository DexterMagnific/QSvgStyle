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

#ifndef SVGGEN_H
#define SVGGEN_H

#include <QSizeF>
#include <QPainterPath>
#include <QGraphicsPathItem>
#include <QSize>
#include <QVector>
#include <QDomElement>
#include <QString>
#include <QTimer>

class QGraphicsScene;
class QGraphicsSceneMouseEvent;

class SvgGenInterior : public QGraphicsPathItem {
  public:
    SvgGenInterior(QGraphicsItem *parent = Q_NULLPTR);
    ~SvgGenInterior();

    void setRoundInterior(bool hasIt);
    void setInteriorRoundness(qreal val);
    void setSize(const QSizeF &sz);

    void setBaseName(const QString &val) { basename = val; }
    void setVariant(const QString &val) { variant = val; }
    void setStatus(const QString &val) { status = val; }

    QDomElement *toSvg();

  private:
    void calcInterior();

    qreal width, height, roundness;
    bool roundInterior;
    QString basename, variant, status;
};

/**
 * @brief QGraphicsPathItem that represents a rounded subframe
 *
 * This class draws a rounded subframe that has a specific width and corner
 * roundness.
 * A subframe is a union of two subpaths: a inner subframe and an outer
 * subframe.
 * The size set with the @ref setSize() method, the position set by setX(), setY()
 * and the like functions, as well as the corner roundness set by @ref setCornerRoundness()
 * methods is applicable to the inner subframe. The outer subframe will
 * be computed by adding to it a margin of @ref subFrameWidth()
 * When the corner roundness is set to 0, the inner subframe will be a rectangle
 * The outer subframe roundness is @ref outerCornerRoundess()
 * and can be used as an initial roundness for the subsequent (above) subframes.
 * The outer subframe bounding rect can be obtained using @ref outerSubFrameRect(),
 * which returns a QRectF in item ccordinates as usual.
 */
class SvgGenSubFrame : public QObject, public QGraphicsPathItem {
  Q_OBJECT

  public:
    SvgGenSubFrame(QGraphicsItem *parent = Q_NULLPTR);
    SvgGenSubFrame(const SvgGenSubFrame *prev);
    SvgGenSubFrame(const QSizeF &sz, qreal sbwidth, qreal roundness = 0, bool round = true, bool split = true);
    virtual ~SvgGenSubFrame();

    void setSubFrameWidth(qreal val);
    void setCornerRoundness(qreal val);
    void setRoundCorners(bool val);
    void setSplitMode(bool val);
    /* The size given to this method is the inner subframe size */
    void setSize(const QSizeF &sz);

    /* corner roundness is for the inner subframe */
    qreal cornerRoundess() const { return roundness; }
    bool hasRoundCorners() const { return roundCorners; }
    qreal subFrameWidth() const { return sbwidth; }
    bool isSplit() const { return split; }
    /** Returns the outer subframe rect. The resulting rect is to be
     * taken as if the subframe was in non split mode */
    QRectF outerSubFrameRect() const;
    qreal outerCornerRoundness() const { return roundness+sbwidth; }

    void setBaseName(const QString &val) { basename = val; }
    void setVariant(const QString &val) { variant = val; }
    void setStatus(const QString &val) { status = val; }

    /* We have to reimplement this as the default implementation calls
     * shape()->contains().
     * The problem is that shape() is a QPainterPathStroker of path(),
     * which is roughly an outline of the whole shape.
     * As SubFrames have a hole in the middle, this won't work
     */
    bool contains(const QPointF &point) const;
    //QPainterPath shape() const;

    enum subFramePart {
      TopPart = 0,
      BottomPart,
      LeftPart,
      RightPart,
      TopLeftPart,
      TopRightPart,
      BottomLeftPart,
      BottomRightPart,
    };

    QDomElement *toSvg(subFramePart part);

  protected:
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    virtual void hoverEnterEvent(QGraphicsSceneHoverEvent *event);
    virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent *event);
    virtual void hoverMoveEvent(QGraphicsSceneHoverEvent *event);

  private slots:
    void hoverTimeout();

  private:
    void calcSubFrame();

    qreal width, height, roundness, sbwidth, roundCorners, split;
    QPainterPath top,bottom,left,right,topleft,topright,bottomleft,bottomright;
    QString basename, variant, status;
    QTimer hoverTimer;
};

class SvgGen : public QObject {
  Q_OBJECT

  public:
    /** Constructor. Takes ownership of the scene */
    SvgGen(QGraphicsScene *scene, QObject *parent = Q_NULLPTR);
    ~SvgGen();

    void setHasInterior(bool hasIt);
    void setHasFrame(bool hasIt);
    void setHasShadow(bool hasIt);
    void setRoundMode(bool hasIt);
    void setSplitMode(bool hasIt);

    void setCenter(const QPointF &p);

    /* The size given to this method is the interior size */
    void setSize(const QSizeF &sz);

    void setFrameWidth(int width);
    void setSubFrameWidth(int idx, qreal width);
    void setShadowWidth(qreal val);
    /* roundness of iterior when object does not have a frame */
    void setRoundness(qreal val);

    void setBaseName(const QString &val);
    void setVariant(const QString &val);
    void setStatus(const QString &val);

    qreal subFrameWidth(int n) const {
      if ( subFrames.count()-1 >= n )
        return subFrames[n]->subFrameWidth();
      else
        return -1;
    }
    int frameWidth() const { return framewidth; }

    QDomElement *toSvg();

  private:
    void pushSubFrame();
    void popSubFrame();
    void calc();

    qreal width,height,shadowwidth;
    int framewidth;
    bool hasShadow, hasInterior, hasFrame, roundMode, splitMode;
    QPointF center;
    QGraphicsScene *scene;
    QVector<SvgGenSubFrame *> subFrames;
    SvgGenInterior *interior;
    QGraphicsPathItem *cross;

    QWidget *frameUI;
};

#endif
