/***************************************************************************
 *   Copyright (C) 2009 by Saïd LANKRI                                     *
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
#ifndef QSVGSTYLE_H
#define QSVGSTYLE_H

#include <QCommonStyle>
#include <QString>

#include "../themeconfig/specs.h"

/** Define this to allow style instrumentation */
//#define QS_INSTRUMENTATION

class QWidget;
class QSvgRenderer;
class QSettings;
class QVariant;
class QFont;
class QTimer;
template<typename T> class QList;
template<typename T1, typename T2> class QMap;

class ThemeConfig;

class QSvgStyle : public QCommonStyle {
  Q_OBJECT

  public:
    /**
     * QSvgStyle constructor. The only SVG themeable style for Qt around
     * the world
     */
    QSvgStyle();
    ~QSvgStyle();

    /**
     * Reimplemented from QStyle
     * Called when this style is a applied to a widget
     */
    virtual void polish(QWidget *widget);
    /**
     * Reimplemented from QStyle
     * Called when this style is unapplied to a widget
     */
    virtual void unpolish(QWidget *widget);
    /**
     * This is an event filter which is installed on animated
     * widgets. It receives event widgets such as show and hide and performs
     * appropriate animations
     */
    virtual bool eventFilter(QObject * o, QEvent * e);

    /**
     * Reimplemented functions from QStyle
     * See QStyle documentation
     */
    virtual int pixelMetric ( PixelMetric metric, const QStyleOption * option = 0, const QWidget * widget = 0 ) const;
    virtual QRect subElementRect ( SubElement element, const QStyleOption * option, const QWidget * widget = 0 ) const;
    virtual QRect subControlRect ( ComplexControl control, const QStyleOptionComplex * option, SubControl subControl, const QWidget * widget = 0 ) const;
    QSize sizeFromContents ( ContentsType type, const QStyleOption * option, const QSize & contentsSize, const QWidget * widget = 0 ) const;

    virtual void drawPrimitive(QStyle::PrimitiveElement e, const QStyleOption* option, QPainter* p, const QWidget* widget = 0) const;
    virtual void drawControl(ControlElement element, const QStyleOption * option, QPainter * painter, const QWidget * widget = 0 ) const;
    virtual void drawComplexControl(ComplexControl control, const QStyleOptionComplex * option, QPainter * painter, const QWidget * widget = 0 ) const;
    virtual int styleHint(StyleHint hint, const QStyleOption * option = 0, const QWidget * widget = 0, QStyleHintReturn * returnData = 0 ) const;
    virtual SubControl hitTestComplexControl (ComplexControl control, const QStyleOptionComplex * option, const QPoint & position, const QWidget * widget = 0 ) const;

  protected slots:
    QIcon standardIconImplementation ( StandardPixmap standardIcon, const QStyleOption * option = 0, const QWidget * widget = 0 ) const;

  protected:
    /**
     * Loads and sets the given theme
     * Theme is searched for in $XDG_CONFIG_HOME/QSvgStyle/theme directory
     */
    void setTheme(const QString &theme);
    /**
     * Wrapper method around @ref loadTheme that reads
     * $XDG_CONFIG_HOME/QSvgStyle/qsvgstyle.cfg configuration file and loads
     * the theme set with the variable theme=
     */
    void setUserTheme();
    /**
     * Loads and sets the built-in default theme
     */
    void setBuiltinTheme();

  signals:
    /**
     * These signals are emitted on various QSvgStyle painting events
     * They are mainly used by the QSvgThemeViewer application to monitor
     * style actions
     * FIXME signals/slots are a slow mechanism. Maybe replace with direct
     * callback invocation
     */
    void sig_drawPrimitive_begin(const QString &) const;
    void sig_drawPrimitive_end(const QString &) const;

    void sig_drawControl_begin(const QString &) const;
    void sig_drawControl_end(const QString &) const;

    void sig_drawComplexControl_begin(const QString &) const;
    void sig_drawComplexControl_end(const QString &) const;

    void sig_renderFrame_begin(const QString &) const;
    void sig_renderFrame_end(const QString &) const;

    void sig_renderInterior_begin(const QString &) const;
    void sig_renderInterior_end(const QString &) const;

    void sig_renderIndicator_begin(const QString &) const;
    void sig_renderIndicator_end(const QString &) const;

    void sig_renderLabel_begin(const QString &) const;
    void sig_renderLabel_end(const QString &) const;

    void sig_renderElement_begin(const QString &) const;
    void sig_renderElement_end(const QString &) const;

    void sig_sizeFromContents_begin(const QString &) const;
    void sig_sizeFromContents_end(const QString &) const;

  private:
    /**
     * Returns whether the given widget is a container widget
     * (e.g. Frame, Tab, ...)
     * QSvgStyle does not apply hovered, pressed and toggled renderings
     * to those widgets. It simply draws them either in the normal, disabled
     * or selected state
     */
    bool isContainerWidget(const QWidget * widget) const;
    /**
     * Returns whether QSvgStyle is able to anilmate the given widget
     */
    bool isAnimatableWidget(const QWidget * widget) const;

    /**
     * Core QSvgStyle drawing routine
     *
     * Render the @ref element from the SVG file into the given \ref bounds
     * (the element is stretched/collapsed if necessary)
     * If \ref pattern is true, the element is taken as a pattern to repeat
     * either horizontally, vertically or both, depending of \ref h and \ref v
     * The \ref frameno parameter indicates the frame number in an animated
     * sequence. If it is -1, animation is disabled
     * if \ref orientation is Qt::Vertical, the element is drawn rotated by an angle
     * of -90 degrees.
     * FIXME @warning if orientation is Vertical, rendering is three times
     * slower than Horizontal.
     */
    void renderElement(QPainter *painter,
                       const QString &element,
                       const QRect &bounds,
                       int hsize = 0,
                       int vsize = 0,
                       Qt::Orientation orientation = Qt::Horizontal,
                       int frameno = -1) const;

    /**
     * Returns the frame spec of the given group
     */
    inline frame_spec_t getFrameSpec(const QString &group) const;
    /**
     * Returns the interior spec of the given group
     */
    inline interior_spec_t getInteriorSpec(const QString &group) const;
    /**
     * Returns the indicator spec of the given group
     */
    inline indicator_spec_t getIndicatorSpec(const QString &group) const;
    /**
     * Returns the label (text+icon) spec of the given group
     */
    inline label_spec_t getLabelSpec(const QString &group) const;
    /**
     * Returns the size spec of the given group
     */
    inline size_spec_t getSizeSpec(const QString &group) const;

    /**
     * QSvgStyle support for capsule grouping
     * When multiple widgets of the same type are grouped inside a
     * vertical, a horizontal or a grid layout, QSvgStyle is able to
     * to merge these widgets' borders according to the widgets positions
     * inside the layout, resulting in a "capsule" appearence.
     *
     * This function determines the position of the given widget in a
     * possible capsule and if it determines that the conditions for a capsule
     * grouping are satisfied, sets @capsule to true and adjusts the
     * @ref h and @ref v output arguments for the horizontal and vertical
     * capsule positions respectively.
     *
     * @ref h and @ref v are set to the following: -1 for left/top capsule
     * positions, 0 for middle, 1 for right/bottom position and 2 for both
     * left/top and right/bottom position
     *
     * Capsule grouping has the following restrictions:
     *   - all widgets inside the layout where @ref widget is must be of the
     *     same type
     *   - the layout must be eiter a QHBoxLayout, QVBoxLayout or QGridLayout
     *   - the layout spacing must be zero
     *
     * @note although this function works for all types of widgets, QSvgStyle
     * uses it mostly on button widgets (push buttons and tool buttons)
     */
    void capsulePosition(const QWidget *widget, bool &capsule, int &h, int &v) const;

    /**
     * Generic method that draws a frame
     */
    void renderFrame(QPainter *painter,
                    /* frame bounds */ const QRect &bounds,
                    /* frame spec */ const frame_spec_t &fspec,
                    /* frame SVG element (basename) */ const QString &element,
                    /* orientation */ Qt::Orientation orientation = Qt::Horizontal) const;
    /**
     * Generic method that draws a frame interior
     */
    void renderInterior(QPainter *painter,
                       /* frame bounds */ const QRect &bounds,
                       /* frame spec */ const frame_spec_t &fspec,
                       /* interior spec */ const interior_spec_t &ispec,
                       /* interior SVG element */ const QString &element,
                        /* orientation */ Qt::Orientation orientation = Qt::Horizontal) const;
    /**
     * Generic method that draws an indicator (e.g. drop down arrows)
     */
    void renderIndicator(QPainter *painter,
                       /* frame bounds */ const QRect &bounds,
                       /* frame spec */ const frame_spec_t &fspec,
                       /* interior spec */ const interior_spec_t &ispec,
                       /* indicator spec */ const indicator_spec_t &dspec,
                       /* indicator SVG element */ const QString &element,
                       Qt::Alignment alignment = Qt::AlignVCenter | Qt::AlignCenter) const;
    /**
     * Generic method that draws a label (text and/or icon)
     */
    void renderLabel(QPainter *painter,
                     /* text direction */ Qt::LayoutDirection direction,
                     /* frame bounds */ const QRect &bounds,
                     /* frame spec */ const frame_spec_t &fspec,
                     /* interior spec */ const interior_spec_t &ispec,
                     /* label spec */ const label_spec_t &lspec,
                     /* text alignment */ int talign,
                     /* text */ const QString &text,
                     /* disabled text ? */ bool disabled = false,
                     /* icon */ const QPixmap &icon = 0,
                     /* text-icon alignment */ const Qt::ToolButtonStyle tialign = Qt::ToolButtonTextBesideIcon) const;
    /**
     * Generic method to compute the ideal
     * (in QSvgStyle : minimal and strictly sufficient) size of a widget
     */
    QSize sizeFromContents(/* font metrics to determine width/height */ const QFontMetrics &fm,
                     /* frame spec */ const frame_spec_t &fspec,
                     /* interior spec */ const interior_spec_t &ispec,
                     /* label spec */ const label_spec_t &lspec,
                     /* size spec */ const size_spec_t &sspec,
                     /* text */ const QString &text,
                     /* icon */ const QPixmap &icon,
                     /* text-icon alignment */ const Qt::ToolButtonStyle tialign = Qt::ToolButtonTextBesideIcon) const;
    /**
     * Returns the squared rect that can fit inside the given rect
     * The topleft of the result is the same as the topleft of @ref r
     */
    QRect squaredRect(const QRect &r) const;
    /**
     * Returns a QRect for drawing the frame inside the given @ref bounds
     * QSvgStyle draws frames inside @ref bounds with no margins,
     * so this function returns @ref bounds
     */
    QRect frameRect(const QRect &bounds,
                    frame_spec_t f) const {
      Q_UNUSED(f);
      return bounds;
    }
    /**
     * Returns a QRect for drawing the interior inside the given @ref bounds
     * with regards to the frame spec @ref f
     * QSvgStyle draws interiors immediately after the frame with no margins
     */
    QRect interiorRect(const QRect &bounds,
                       frame_spec_t f,
                       interior_spec_t i) const {
      Q_UNUSED(i);
      QRect r;
      if (f.hasFrame)
        r = frameRect(bounds,f).adjusted(f.left,f.top,-f.right,-f.bottom);
      else
        r = frameRect(bounds,f);
      if ( r.width() < 0 )
        r.setWidth(0);
      if ( r.height() < 0 )
        r.setHeight(0);
      return r;
    }
    /**
     * Returns a QRect for drawing labels (text and/or icons) inside the given
     * @ref bounds, with regards th the frame spec @f, interior spec @ref i
     * and label spec @ref t
     * QSvgStyle draws labels inside the interior returned by @ref interiorRect
     * after applying the label margins set in @ref t
     */
    QRect labelRect(const QRect &bounds,
                    frame_spec_t f,
                    interior_spec_t i,
                    label_spec_t t) const {
      QRect r = interiorRect(bounds,f,i).adjusted(t.left,t.top,-t.right,-t.bottom);
      if ( r.width() < 0 )
        r.setWidth(0);
      if ( r.height() < 0 )
        r.setHeight(0);
      return r;
    }

    /**
     * Used when debugging.
     * draw a rectangle that is 1 pixel shorter on right and bottom
     */
    void drawRealRect(QPainter *p, const QRect &r) const;

    /** Helper functions that convert various QStyle enums to strings */
    QString PE_str(PrimitiveElement element) const;
    QString CE_str(ControlElement element) const;
    QString CC_str(ComplexControl element) const;
    QString SE_str(SubElement element) const;
    QString SC_str(ComplexControl control, SubControl subControl) const;
    QString CT_str(ContentsType type) const;

    /**
     * Helper functions that determine the appriopriate QSvgStyle
     * configuration group to draw a given element
     */
    QString PE_group(PrimitiveElement element) const;
    QString SE_group(SubElement element) const;
    QString CE_group(ControlElement element) const;
    QString CC_group(ComplexControl element) const;
    QString CT_group(ContentsType type) const;

    /**
     * Helper function that converts a QStyle::State value to a string
     */
    QString state_str(State st, const QWidget *w) const;
    /**
     * Helper function that converts a QStyle::State to a QIcon::Mode
     */
    QIcon::Mode state_iconmode(State st) const;
    /**
     * Helper function that converts a QStyle::State to a QIcon::State
     */
    QIcon::State state_iconstate(State st) const;

  private slots:
    /**
     * Slot called on timer timeout to animate widgets
     */
    void slot_animate();
    /**
     * Slot called on timer timeout to animate busy progress bars
     */
    void slot_animateProgressBars();

  private:
    QString cls;
    QSvgRenderer *defaultRndr, *themeRndr;
    ThemeConfig *defaultSettings, *themeSettings, *settings;

    /* timer used for animated themes */
    QTimer *timer;
    QTimer *progresstimer;

    /* List of registered widgets for a animations */
    QList<QWidget *> animatedWidgets;

    /* List of busy progress bars */
    QMap<QWidget *,int> progressbars;

    /* Animation status */
    bool animationEnabled;

    /* Animation frame counter */
    int animationcount;

    /* QSvgStyle debugging capabilities */
    bool dbgWireframe, dbgOverdraw;

    static void SWAP(int &x, int &y) {
      int tmp = x;
      x = y;
      y = tmp;
    }
};

#endif
