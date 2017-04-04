/***************************************************************************
 *   Copyright (C) 2014 by Saïd LANKRI                                     *
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

#include "specs.h"

class QWidget;
class QSvgRenderer;
class QSettings;
class QVariant;
class QFont;
class QTimer;
class QLayout;
template<typename T> class QList;
template<typename T1, typename T2> class QMap;

class ThemeConfig;
class PaletteConfig;
class StyleConfig;

class QSvgThemableStyle : public QCommonStyle {
  Q_OBJECT

  public:
    enum QSvgStyleVersion { Version = 2 };

    /**
     * QSvgStyle constructor
     */
    QSvgThemableStyle();
    virtual ~QSvgThemableStyle();

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
#if QT_VERSION >= 0x050000
    virtual QIcon standardIcon(StandardPixmap standardIcon, const QStyleOption * option = 0, const QWidget * widget = 0) const;
#else
  protected slots:
    QIcon standardIconImplementation ( StandardPixmap standardIcon, const QStyleOption * option = 0, const QWidget * widget = 0 ) const;
#endif

  private:
    /* Used internally by QSvgThemeBuilder */
    Q_INVOKABLE void loadCustomThemeConfig(const QString &filename);
    Q_INVOKABLE void loadCustomSVG(const QString &filename);
    /* Used internally by QSvgPaletteBuilder */
    Q_INVOKABLE void loadCustomPaletteConfig(const QString &filename);
    /* User internally by QSvgThemeManager */
    Q_INVOKABLE void loadCustomStyleConfig(const QString &filename);

    /* Loads user config in ~/.config/QSvgStyle/qsvgstyle.cfg */
    void loadUserConfig();

    /**
     * Loads and sets the built-in default theme
     */
    Q_INVOKABLE void loadBuiltinTheme();

    /**
     * Loads and sets the given theme
     * Theme is searched for in ~/.config/QSvgStyle/theme directory
     */
    Q_INVOKABLE void loadTheme(const QString &theme);
    Q_INVOKABLE QString currentTheme() const { return curTheme; };

    /**
     * Wrapper method around @ref loadTheme that reads
     * ~/.config/QSvgStyle/qsvgstyle.cfg configuration file and loads
     * the theme set with the variable theme=
     */
    void loadUserTheme();

    /**
     * Loads and sets the given palette
     * Palette is searched for in ~/.config/QSvgStyle/palette.pal file
     */
    Q_INVOKABLE void loadPalette(const QString& palette);
    Q_INVOKABLE QString currentPalette() const { return curPalette; };

    /**
     * Wrapper method around @ref loadPalette that reads
     * ~/.config/QSvgStyle/qsvgstyle.cfg configuration file and loads
     * the palette set with the variable palette=
     */
    void loadUserPalette();

    /**
     * Loads and sets the system palette
     */
    Q_INVOKABLE void loadSystemPalette();

    /**
     * Do not use palette
     */
    Q_INVOKABLE void unloadPalette();

  signals:
    /**
     * These signals are emitted on various QSvgStyle painting events
     * They are mainly used by the QSvgThemeViewer application to monitor
     * style actions
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
      * Extension of Qt::orientation
      */
    typedef enum {
        Horizontal = Qt::Horizontal,
        Vertical   = Qt::Vertical,
        TopBottom  = Vertical,
        BottomTop,
    } Orientation;

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
     */
    void renderElement(QPainter *painter,
                       const QString &element,
                       const QRect &bounds,
                       int hsize = 0,
                       int vsize = 0) const;

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
     * Returns the specific setting from the config file
     */
    inline QVariant getSpecificValue(const QString &key) const;
    /**
     * Returns the color spec of the given group
     */
    inline color_spec_t getColorSpec(const QString &group) const;

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
     * Recursively looks for the given widget inside the given layout
     * and returns the layout containing the widget.
     * If @a layout is NULL, looks for the widget inside the layout of its parent
     * @returns NULL if the widget is not found
     */
    QLayout * layoutForWidget(const QWidget *widget, QLayout *layout = 0) const;

    /**
     * Helper function that computes the 9 rects of a frame
     * NOTE: if @ref orn is @ref Vertical, returned results
     * are for the transposed @ref bounds. Drawing routines like @ref renderFrame
     * will manage to apply appriopriate rotations when drawing
     * NOTE: This function considers a Left to Right layout.
     * Appropriate transformations will be done in @ref renderFrame function
     * when drawing
     */
    void computeFrameRects(/* element bounds */ const QRect &bounds,
                           /* frame spec */ const frame_spec_t &fs,
                           /* orientation */ Orientation orn,
                           /* Results */
                           QRect &top,
                           QRect &bottom,
                           QRect &left,
                           QRect &right,
                           QRect &topleft,
                           QRect &topright,
                           QRect &bottomleft,
                           QRect &bottomright) const;

    /**
     * Helper function that computes the interior rect. The same notes
     * as for @ref computeFrameRects apply
     */
    void computeInteriorRect(/* element bounds */ const QRect &bounds,
                             /* frame spec */ frame_spec_t fs,
                             /* interior spec */ interior_spec_t is,
                             /* orientation */ Orientation orn,
                             /* Result */
                             QRect &r) const;

    /**
     * Generic method that draws a frame
     */
    void renderFrame(QPainter *p,
                    /* color spec */ const QBrush &b,
                    /* frame bounds */ const QRect &bounds,
                    /* frame spec */ const frame_spec_t &fs,
                    /* frame SVG element (basename) */ const QString &e,
                    /* direction */ Qt::LayoutDirection dir = Qt::LeftToRight,
                    /* orientation */ Orientation orn = Horizontal) const;
    /**
     * Generic method that draws a frame interior
     */
    void renderInterior(QPainter *p,
                       /* color spec */ const QBrush &b,
                       /* frame bounds */ const QRect &bounds,
                       /* frame spec */ const frame_spec_t &fs,
                       /* interior spec */ const interior_spec_t &is,
                       /* interior SVG element */ const QString &e,
                       /* direction */ Qt::LayoutDirection dir = Qt::LeftToRight,
                       /* orientation */ Orientation orn = Horizontal) const;
    /**
     * Generic method that draws an indicator (e.g. drop down arrows)
     */
    void renderIndicator(QPainter *p,
                       /* frame bounds */ const QRect &bounds,
                       /* frame spec */ const frame_spec_t &fs,
                       /* interior spec */ const interior_spec_t &is,
                       /* indicator spec */ const indicator_spec_t &ds,
                       /* indicator SVG element */ const QString &e,
                       /* direction */ Qt::LayoutDirection dir = Qt::LeftToRight,
                       Qt::Alignment alignment = Qt::AlignVCenter | Qt::AlignCenter) const;
    /**
     * Generic method that draws a label (text and/or icon)
     */
    void renderLabel(QPainter *p,
                     /* color spec */ const QBrush &b,
                     /* text direction */ Qt::LayoutDirection dir,
                     /* frame bounds */ const QRect &bounds,
                     /* frame spec */ const frame_spec_t &fs,
                     /* interior spec */ const interior_spec_t &is,
                     /* label spec */ const label_spec_t &ls,
                     /* text alignment */ int talign,
                     /* text */ const QString &text,
                     /* icon */ const QPixmap &icon = QPixmap(),
                     /* text-icon alignment */ const Qt::ToolButtonStyle tialign = Qt::ToolButtonTextBesideIcon) const;

    void colorizeIndicator(QPainter *p,
                        /* frame bounds */ const QRect &bounds,
                        /* frame spec */ const frame_spec_t &fs,
                        /* interior spec */ const interior_spec_t &is,
                        /* indicator spec */ const indicator_spec_t &ds,
                        /* brush */ const QBrush &b,
                        /* direction */ Qt::LayoutDirection dir = Qt::LeftToRight,
                        Qt::Alignment alignment = Qt::AlignVCenter | Qt::AlignCenter) const;


    /**
     * Generic method to compute the ideal
     * (in QSvgStyle : minimal and strictly sufficient) size of a widget
     */
    QSize sizeFromContents(/* font metrics to determine width/height */ const QFontMetrics &fm,
                     /* frame spec */ const frame_spec_t &fspec,
                     /* interior spec */ const interior_spec_t &ispec,
                     /* label spec */ const label_spec_t &lspec,
                     /* text */ const QString &text,
                     /* icon */ const QPixmap &icon,
                     /* text-icon alignment */ const Qt::ToolButtonStyle tialign = Qt::ToolButtonTextBesideIcon) const;
    /**
     * Returns the squared rect that can fit inside the given rect
     * The topleft of the result is the same as the topleft of @ref r
     */
    QRect squaredRect(const QRect &r) const;
    /**
     * Returns a transposed rect
     */
    QRect transposedRect(const QRect &r) const {
        return QRect(r.y(),r.x(),r.height(),r.width());
    }
    /**
     * Returns a QRect for drawing the interior inside the given @ref bounds
     * with regards to the frame spec @ref f
     * QSvgStyle draws interiors immediately after the frame with no margins
     * @warning this is a convenience function and returns the interior rect
     * assuming a horizontal LTR widget
     */
    QRect interiorRect(const QRect &bounds,
                       frame_spec_t fs,
                       interior_spec_t is) const {
      QRect r;
      computeInteriorRect(bounds,fs,is, Horizontal, r);
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
      QRect r = interiorRect(bounds,f,i).adjusted(t.hmargin,t.vmargin,-t.hmargin,-t.vmargin);
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

    /**
     * Converts a color_spec_t fg or bg field to a QBrush.
     * If the field is not set, takes the defaut supplied brush b
     */
    QBrush cs2b(value_t<int> c, const QBrush &b) const {
      return c.present ? QBrush(QRgba64::fromArgb32(c)) : b;
    }

    /** Helper functions that convert various QStyle enums to strings */
    QString PE_str(PrimitiveElement element) const;
    QString CE_str(ControlElement element) const;
    QString CC_str(ComplexControl element) const;
    QString SE_str(SubElement element) const;
    QString SC_str(ComplexControl control, SubControl subControl) const;
    QString CT_str(ContentsType type) const;

    friend class ThemeBuilderUI;

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
     * Slot called on timer timeout to animate busy progress bars
     */
    void slot_animateProgressBars();

  private:
    QString cls;
    QSvgRenderer *themeRndr;
    ThemeConfig *themeSettings;
    PaletteConfig *paletteSettings;
    StyleConfig *styleSettings;

    QString curTheme, curPalette;

    /* timer used for progress bars */
    QTimer *progresstimer;

    /* List of registered widgets for a animations */
    QList<QWidget *> animatedWidgets;

    /* List of busy progress bars */
    QMap<QWidget *,int> progressbars;

    /* QSvgStyle debugging capabilities */
    bool dbgWireframe, dbgOverdraw;
};

#endif
