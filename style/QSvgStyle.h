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
     * Default constructor. Loads the QSvgStyle configuration file and
     * sets the default, application and user set themes
     */
    QSvgStyle();
    ~QSvgStyle();

    /**
     * Set the name of the user specific theme.
     * This overrides this user's specific theme read from the QSvgStyle
     * global configuration file
     * @note if @a themename is @ref QString::null, then this style will bypass
     * this setting and fallback to default theme
     */
    void setUserTheme(const QString &themename);
    /**
     * Set the name of the application specific theme.
     * This overrides this application's specific theme which
     * is guessed from @ref QApplication::applicationName()
     * @note if @a themename is @ref QString::null, then this style will bypass
     * this setting and fallback to user then to default theme
     */
    void setApplicationTheme(const QString &themename);
    /**
     * Set the name of the default theme. The default theme is normally
     * built-in in the QSvgStyle engine. This allows you to set an external
     * default theme.
     * @note if @a theme is @ref QString::null, then this style will bypass
     * this setting.
     * To restore the default theme into the built-in theme, use @ref restoreBuiltinDefaultTheme()
     */
    void setDefaultTheme(const QString &themename);
    /**
     * Restore the default theme to the built-in theme
     */
    void restoreBuiltinDefaultTheme();

    void polish(QWidget *widget);
    void unpolish(QWidget *widget);

    /**
     * This is an event filter which is installed on animated
     * widgets. It receives event widgets and performs appropriate
     * animation steps
     */
    virtual bool eventFilter(QObject * o, QEvent * e);

    /** Reimplemented functions from QStyle */
    virtual int pixelMetric ( PixelMetric metric, const QStyleOption * option = 0, const QWidget * widget = 0 ) const;
    virtual QRect subElementRect ( SubElement element, const QStyleOption * option, const QWidget * widget = 0 ) const;
    virtual QRect subControlRect ( ComplexControl control, const QStyleOptionComplex * option, SubControl subControl, const QWidget * widget = 0 ) const;
    QSize sizeFromContents ( ContentsType type, const QStyleOption * option, const QSize & contentsSize, const QWidget * widget = 0 ) const;

    virtual void drawPrimitive(PrimitiveElement element, const QStyleOption * option, QPainter * painter, const QWidget * widget = 0) const;
    virtual void drawControl ( ControlElement element, const QStyleOption * option, QPainter * painter, const QWidget * widget = 0 ) const;
    virtual void drawComplexControl ( ComplexControl control, const QStyleOptionComplex * option, QPainter * painter, const QWidget * widget = 0 ) const;
    virtual int styleHint(StyleHint hint, const QStyleOption * option = 0, const QWidget * widget = 0, QStyleHintReturn * returnData = 0 ) const;
    virtual SubControl hitTestComplexControl ( ComplexControl control, const QStyleOptionComplex * option, const QPoint & position, const QWidget * widget = 0 ) const;

  protected slots:
    QIcon standardIconImplementation ( StandardPixmap standardIcon, const QStyleOption * option = 0, const QWidget * widget = 0 ) const;

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
    /** Returns whether the given is a container widget (e.g. Frame, Tab, ...)
     */
    bool isContainerWidget(const QWidget * widget) const;
    /** Returns whether QSvgStyle is able to anilmate the given widget
     * */
    bool isAnimatableWidget(const QWidget * widget) const;

    /** Render the @ref element from the SVG file into the given \ref bounds.
     * If \ref pattern is true, the element is taken as a pattern to repeat
     * either horizontally, vertically or both, depending of \ref h and \ref v
     * The \ref frameno parameter indicates the frame number in an animated
     * sequence. If it is -1, animation is disabled
     * if \ref orientation is Qt::Vertical, the element is drawn rotated by an angle
     * of -90 degrees.
     * @warning if orientation is Vertical, rendering is three times
     * slower than Horizontal.
     */
    void renderElement(QPainter *painter,const QString &element, const QRect &bounds, int hsize = 0, int vsize = 0, Qt::Orientation orientation = Qt::Horizontal, int frameno = -1) const;

    /** Returns the frame spec of the given widget from the theme config file */
    inline frame_spec_t getFrameSpec(const QString &widgetName) const;
    /** Returns the interior spec of the given widget from the theme config file */
    inline interior_spec_t getInteriorSpec(const QString &widgetName) const;
    /** Returns the indicator spec of the given widget from the theme config file */
    inline indicator_spec_t getIndicatorSpec(const QString &widgetName) const;
    /** Returns the label (text+icon) spec of the given widget from the theme config file */
    inline label_spec_t getLabelSpec(const QString &widgetName) const;
    /** Returns the size spec of the given widget from the theme config file */
    inline size_spec_t getSizeSpec(const QString &widgetName) const;

    /** Computes the position of the given widget in a possible capsule */
    void capsulePosition(const QWidget *widget, bool &capsule, int &h, int &v) const;

    /** Generic method that draws a frame */
    void renderFrame(QPainter *painter,
                    /* frame bounds */ const QRect &bounds,
                    /* frame spec */ const frame_spec_t &fspec,
                    /* frame SVG element (basename) */ const QString &element,
                    /* orientation */ Qt::Orientation orientation = Qt::Horizontal) const;
    /** Generic method that draws a frame interior */
    void renderInterior(QPainter *painter,
                       /* frame bounds */ const QRect &bounds,
                       /* frame spec */ const frame_spec_t &fspec,
                       /* interior spec */ const interior_spec_t &ispec,
                       /* interior SVG element */ const QString &element,
                        /* orientation */ Qt::Orientation orientation = Qt::Horizontal) const;
    /** Generic method that draws an indicator (e.g. arrows) */
    void renderIndicator(QPainter *painter,
                       /* frame bounds */ const QRect &bounds,
                       /* frame spec */ const frame_spec_t &fspec,
                       /* interior spec */ const interior_spec_t &ispec,
                       /* indicator spec */ const indicator_spec_t &dspec,
                       /* indicator SVG element */ const QString &element,
                       Qt::Alignment alignment = Qt::AlignVCenter | Qt::AlignCenter) const;

    /** Generic method that draws a label (text and/or icon) */
    void renderLabel(QPainter *painter,
                     /* frame bounds */ const QRect &bounds,
                     /* frame spec */ const frame_spec_t &fspec,
                     /* interior spec */ const interior_spec_t &ispec,
                     /* label spec */ const label_spec_t &lspec,
                     /* text alignment */ int talign,
                     /* text */ const QString &text,
                     /* disabled text ? */ bool disabled = false,
                     /* icon */ const QPixmap &icon = 0,
                     /* text-icon alignment */ const Qt::ToolButtonStyle tialign = Qt::ToolButtonTextBesideIcon) const;

    /** Generic method to compute the ideal (in QSvgStyle : minimal and strictly sufficient) size of a widget */
    QSize sizeFromContents(/* font to determine width/height */ const QFont &font,
                     /* frame spec */ const frame_spec_t &fspec,
                     /* interior spec */ const interior_spec_t &ispec,
                     /* label spec */ const label_spec_t &lspec,
                     /* size spec */ const size_spec_t &sspec,
                     /* text */ const QString &text,
                     /* icon */ const QPixmap &icon,
                     /* text-icon alignment */ const Qt::ToolButtonStyle tialign = Qt::ToolButtonTextBesideIcon) const;

    /** Returns a normalized rect, i.e. a square */
    QRect squaredRect(const QRect &r) const;

    /** Returns a QRect for the given frame spec */
    QRect frameRect(const QRect &bounds, frame_spec_t f) const {
      Q_UNUSED(f);
      return bounds;
    }
    /** Returns a QRect for the given frame and interior specs */
    QRect interiorRect(const QRect &bounds, frame_spec_t f,interior_spec_t i) const {
      Q_UNUSED(i);
      QRect r = frameRect(bounds,f).adjusted(f.left,f.top,-f.right,-f.bottom);
      if ( r.width() < 0 )
        r.setWidth(0);
      if ( r.height() < 0 )
        r.setHeight(0);
      return r;
    }
    /** Returns a QRect for the given frame, interior and text specs */
    QRect labelRect(const QRect &bounds, frame_spec_t f,interior_spec_t i,label_spec_t t) const {
      QRect r = interiorRect(bounds,f,i).adjusted(t.left,t.top,-t.right,-t.bottom);
      if ( r.width() < 0 )
        r.setWidth(0);
      if ( r.height() < 0 )
        r.setHeight(0);
      return r;
    }

    /** draw a rectangle. The painter->drawRect draws a rect that is 1 pixel too large */
    void drawRealRect(QPainter *p, const QRect &r) const;

    /** Helper functions that convert various enums to string */
    const QString PE_str(PrimitiveElement element) const;
    const QString CE_str(ControlElement element) const;
    const QString CC_str(ComplexControl element) const;
    const QString SE_str(SubElement element) const;
    const QString SC_str(ComplexControl control, SubControl subControl) const;
    const QString CT_str(ContentsType type) const;

    /** Helper functions that determine the appriopriate QSvgStyle
     * configuration group for a given element
     * TODO implement
     */
    const QString PE_group(PrimitiveElement element);
    const QString CE_group(ControlElement element);
    const QString CC_group(ComplexControl element);

  private slots:
    /** Slot called on timer timeout to advance in animations */
    void advance();
    /** Slot called on timer timeout to advance busy progress bars */
    void advanceProgresses();

  private:
    QSvgRenderer *defaultRndr, *themeRndr, *appRndr;
    ThemeConfig *defaultSettings, *themeSettings, *appSettings, *settings;
    QSettings *globalSettings;

    QString xdg_config_home;

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

    /* To be called to cumpute theme dependencies */
    void setupThemeDeps();

    static void SWAP(int &x, int &y) {
      int tmp = x;
      x = y;
      y = tmp;
    }
};

#endif
