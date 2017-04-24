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
#include "QSvgThemableStyle.h"

#include <stdlib.h>
#include <limits.h>

#include <QApplication>
#include <QWidget>
#include <QPainter>
#include <QRect>
#include <QSvgRenderer>
#include <QStyleOption>
#include <QSettings>
#include <QFile>
#include <QDir>
#include <QVariant>
#include <QBoxLayout>
#include <QGridLayout>
#include <QFont>
#include <QTimer>
#include <QList>
#include <QMap>
#include <QDebug>
#include <QMatrix>
#include <QtAlgorithms>
#include <QtMath>

#include <QSpinBox>
#include <QToolButton>
#include <QToolBar>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QProgressBar>
#include <QMenu>
#include <QAction>
#include <QGroupBox>
#include <QDockWidget>
#include <QScrollBar>
#include <QTreeWidget>
#include <QToolBox>

#include "ThemeConfig.h"
#include "PaletteConfig.h"
#include "StyleConfig.h"
#include "groups.h"

QSvgThemableStyle::QSvgThemableStyle()
  : QCommonStyle(),
    cls(QString(this->metaObject()->className())),
    themeRndr(NULL),
    themeSettings(NULL),
    paletteSettings(NULL),
    styleSettings(NULL),
    progresstimer(NULL),
    dbgWireframe(false),
    dbgOverdraw(false)
{
  loadUserConfig();

  progresstimer = new QTimer(this);

  connect(progresstimer,SIGNAL(timeout()), this,SLOT(slot_animateProgressBars()));
}

QSvgThemableStyle::~QSvgThemableStyle()
{
  delete themeSettings;
  delete themeRndr;
}

void QSvgThemableStyle::loadUserConfig()
{
  QString filename = StyleConfig::getUserConfigFile();

  delete styleSettings;
  styleSettings = NULL;

  if ( !QFile::exists(filename) ) {
    QDir().mkpath(StyleConfig::getUserConfigDir().absolutePath());
      QFile in(":/qsvgstyle.cfg");
    in.open(QIODevice::ReadOnly);
    QByteArray data = in.readAll();
    in.close();

    QFile f(filename);
    f.open(QIODevice::WriteOnly);
    f.write(data);
    f.close();
  }

  styleSettings = new StyleConfig(filename);

  loadUserTheme();
  loadUserPalette();
}

void QSvgThemableStyle::loadCustomStyleConfig(const QString& filename)
{
  if ( !QFile::exists(filename) )
    return;

  delete styleSettings;
  styleSettings = NULL;

  styleSettings = new StyleConfig(filename);

  qDebug() << "[QSvgStyle]" << "Loaded custom config file" << filename;

  loadTheme(styleSettings->getStyleSpec().theme);
  loadPalette(styleSettings->getStyleSpec().palette);
}

void QSvgThemableStyle::loadBuiltinTheme()
{
  if ( curTheme == "<builtin>" )
    return;

  delete themeRndr;
  themeRndr = NULL;

  delete themeSettings;
  themeSettings = NULL;

  themeSettings = new ThemeConfig(":/default.cfg");
  themeRndr = new QSvgRenderer();
  themeRndr->load(QString(":/default.svg"));

  curTheme = "<builtin>";
  qDebug() << "[QSvgStyle]" << "Loaded built in theme";
}

void QSvgThemableStyle::loadTheme(const QString& theme)
{
  if ( curTheme == theme )
    return;

  if ( theme.isNull() || theme.isEmpty() || theme == "<builtin>" ) {
    loadBuiltinTheme();
    return;
  }

  QList<theme_spec_t> tlist = StyleConfig::getThemeList();
  Q_FOREACH(theme_spec_t t, tlist) {
    if ( theme == t.name ) {
      delete themeSettings;
      themeSettings = NULL;

      themeSettings = new ThemeConfig(t.path);

      delete themeRndr;
      themeRndr = NULL;

      themeRndr = new QSvgRenderer();
      themeRndr->load(
        QFileInfo(t.path).absolutePath().append("/").append(
        QFileInfo(t.path).completeBaseName().append(".svg")));

      curTheme = theme;
      qDebug() << "[QSvgStyle]" << "Loaded theme " << theme;

      return;
    }
  }

  // not found
  qDebug() << "[QSvgStyle]" << "Theme" << theme << "not found";
  loadBuiltinTheme();
  return;
}

void QSvgThemableStyle::loadUserTheme()
{
  QString theme;

  QString filename = StyleConfig::getUserConfigFile();

  // load global config file
  if ( QFile::exists(filename) ) {
    StyleConfig cfg(filename);
    loadTheme(cfg.getStyleSpec().theme);
  } else
    loadBuiltinTheme();
}

void QSvgThemableStyle::loadCustomSVG(const QString& filename)
{
  if ( !QFile::exists(filename) )
    return;

  delete themeRndr;
  themeRndr = NULL;

  themeRndr = new QSvgRenderer();
  themeRndr->load(filename);

  qDebug() << "[QSvgStyle] loaded custom SVG file" << filename;
}

void QSvgThemableStyle::loadCustomThemeConfig(const QString& filename)
{
  if ( !QFile::exists(filename) )
    return;

  delete themeSettings;
  themeSettings = NULL;

  themeSettings = new ThemeConfig(filename);

  curTheme = QString("custom:%1").arg(filename);
  qDebug() << "[QSvgStyle] loaded custom theme file" << filename;
}

void QSvgThemableStyle::loadPalette(const QString& palette)
{
  if ( palette.isNull() || palette.isEmpty() || (palette == "<none>") ) {
    unloadPalette();
    return;
  }

  if ( palette == "<system>" ) {
    loadSystemPalette();
    return;
  }

  QList<palette_spec_t> plist = StyleConfig::getPaletteList();
  Q_FOREACH(palette_spec_t p, plist) {
    if ( palette == p.name ) {
      delete paletteSettings;
      paletteSettings = NULL;

      paletteSettings = new PaletteConfig(p.path);

      curPalette = palette;
      qDebug() << "[QSvgStyle]" << "Loaded palette " << palette;

      return;
    }
  }

  // not found
  qDebug() << "[QSvgStyle]" << "Palette" << palette << "not found";
  unloadPalette();
  return;
}

void QSvgThemableStyle::loadUserPalette()
{
  QString palette;

  QString filename = StyleConfig::getUserConfigFile();

  // load global config file
  if ( QFile::exists(filename) ) {
    StyleConfig cfg(filename);
    loadPalette(cfg.getStyleSpec().palette);
  } else
    unloadPalette();
}

void QSvgThemableStyle::loadSystemPalette()
{
  delete paletteSettings;
  paletteSettings = NULL;

  curPalette = "<system>";
}

void QSvgThemableStyle::unloadPalette()
{
  delete paletteSettings;
  paletteSettings = NULL;

  curPalette = "<none>";
}

void QSvgThemableStyle::loadCustomPaletteConfig(const QString& filename)
{
  if ( paletteSettings ) {
    unloadPalette();
  }

  if ( QFile::exists(filename) ) {
    paletteSettings = new PaletteConfig(filename);

    curPalette = QString("custom:%1").arg(filename);
    qDebug() << "[QSvgStyle] loaded custom palette file" << filename;
  }
}

bool QSvgThemableStyle::isContainerWidget(const QWidget * widget) const
{
  return !widget || (widget && (
    (widget->inherits("QFrame") &&
      !(
        widget->inherits("QTreeWidget") ||
        widget->inherits("QHeaderView") ||
        widget->inherits("QSplitter")
      )
    ) ||
    widget->inherits("QGroupBox") ||
    widget->inherits("QTabWidget") ||
    widget->inherits("QDockWidget") ||
    widget->inherits("QMainWindow") ||
    widget->inherits("QDialog") ||
    widget->inherits("QDesktopWidget") ||
    widget->inherits("QToolBar") ||
    // Ok this one is not a container widget but we want to treat it as such
    // because it has its own groove
    widget->inherits("QProgressBar") ||
    (QString(widget->metaObject()->className()) == "QWidget")
  ));
}

bool QSvgThemableStyle::isAnimatableWidget(const QWidget * widget) const
{
  // NOTE should we test against direct inheritance instead ?
  return widget && (
    widget->inherits("QPushButton") ||
    widget->inherits("QToolButton") ||
    widget->inherits("QProgressBar") ||
    widget->inherits("QLineEdit")
  );
}

void QSvgThemableStyle::polish(QWidget * widget)
{
  if ( !widget )
    return;

  // set WA_Hover attribute for non container widgets,
  // this way we will receive paint events when entering
  // and leaving the widget
  if ( !isContainerWidget(widget) ) {
    widget->setAttribute(Qt::WA_Hover, true);
  }

  // Remove WA_OpaquePaintEvent from scrollbars to correctly render them
  // when the svg items have non opaque colors
  if ( QScrollBar *s = qobject_cast< QScrollBar* >(widget) ) {
    s->setAttribute(Qt::WA_OpaquePaintEvent, false);
  }

  // Install event filter on progress bars to animate them
  if ( QProgressBar *b = qobject_cast< QProgressBar* >(widget) ) {
    b->installEventFilter(this);
  }

  // Enable menu tear off
  if ( QMenu *m = qobject_cast< QMenu* >(widget) ) {
    m->setTearOffEnabled(true);
  }

  // QLineEdit inside a SpinBox of variant VA_SPINBOX_BUTTONS_OPPOSITE : center text
  if ( QLineEdit *l = qobject_cast< QLineEdit * >(widget) ) {
    if ( QSpinBox *s = qobject_cast< QSpinBox * >(l->parent()) )
      if ( s->buttonSymbols() != QAbstractSpinBox::NoButtons )
        if ( getSpecificValue("specific.spinbox.variant").toInt() ==
            VA_SPINBOX_BUTTONS_OPPOSITE )
          l->setAlignment(Qt::AlignHCenter);
  }

  // QToolBox: set background role to Window as in Tab Widgets
  if ( QToolBox *t = qobject_cast< QToolBox * >(widget) ) {
    t->setBackgroundRole(QPalette::Window);
  }
}

void QSvgThemableStyle::unpolish(QWidget * widget)
{
  Q_UNUSED(widget);
/*   if (widget) {
     widget->setAttribute(Qt::WA_Hover, false);

     animatedWidgets.removeOne(widget);
   }*/

  if ( qobject_cast< const QProgressBar* >(widget) ) {
    progressbars.remove(widget);
  }

  widget->removeEventFilter(this);
}

void QSvgThemableStyle::slot_animateProgressBars()
{
  QMap<QWidget *,int>::iterator it;
  for (it=progressbars.begin(); it!=progressbars.end(); ++it) {

    QWidget *widget = it.key();
    if (widget->isVisible() && (qobject_cast< const QProgressBar* >(widget)->value() <= 0)) {
      // only update busy progress bars
      it.value() += 2;
      widget->update();
    }
  }
}

bool QSvgThemableStyle::eventFilter(QObject* o, QEvent* e)
{
  QWidget *w = qobject_cast< QWidget* >(o);

  switch ( e->type() ) {
  case QEvent::Show:
    if ( w ) {
      if ( qobject_cast< QProgressBar * >(o) ) {
        progressbars.insert(w, 0);
        if ( !progresstimer->isActive() )
          progresstimer->start(50);
      }
    }
    break;

  case QEvent::Hide:
  case QEvent::Destroy:
    if (w) {
      animatedWidgets.removeOne(w);
      progressbars.remove(w);
      if ( progressbars.size() == 0 )
        progresstimer->stop();
    }
    break;

  default:
    break;
  }

  // In any case, forward event
  return QObject::eventFilter(o,e);
}

void QSvgThemableStyle::drawPrimitive(PrimitiveElement e, const QStyleOption * option, QPainter * p, const QWidget * widget) const
{
  emit(sig_drawPrimitive_begin(PE_str(e)));

  // Copy some values into shorter variable names
  int x,y,w,h;
  QRect r = option->rect;
  r.getRect(&x,&y,&w,&h);
  QString st = state_str(option->state, widget);
  Qt::LayoutDirection dir = option->direction;
  bool focus = option->state & State_HasFocus;
  Orientation orn = option->state & State_Horizontal ? Horizontal : Vertical;
  bool en = option->state & State_Enabled;
  QPalette pal = option->palette;
  QPalette::ColorGroup cg = en ? QPalette::Normal : QPalette::Disabled;
  pal.setCurrentColorGroup(cg);
  QPalette::ColorRole brole = widget ? widget->backgroundRole() : QPalette::NoRole;

  Q_UNUSED(focus);
  Q_UNUSED(orn);

  // Get QSvgStyle configuration group used to render this element
  QString g = PE_group(e);

  // Configuration for group g
  frame_spec_t fs;
  interior_spec_t is;
  label_spec_t ls;
  indicator_spec_t ds;
  color_spec_t cs;

  if ( g.isEmpty() ) {
    // element not currently supported by QSvgStyle
    QCommonStyle::drawPrimitive(e,option,p,widget);
    goto end;
  }

  // Get configuration for group
  fs = getFrameSpec(g);
  is = getInteriorSpec(g);
  ls = getLabelSpec(g);
  ds = getIndicatorSpec(g);
  cs = getColorSpec(g);

  fs.pressed = option->state & State_Sunken;

  if ( const QStyleOptionComboBox *opt =
    qstyleoption_cast<const QStyleOptionComboBox *>(option) ) {
    fs.hasFrame = opt->frame;
  }

  if ( const QStyleOptionSpinBox *opt =
    qstyleoption_cast<const QStyleOptionSpinBox *>(option) ) {
    fs.hasFrame = opt->frame;
  }

  // Draw
  switch(e) {
    case PE_Widget : {
      // nothing
      break;
    }
    case PE_PanelButtonBevel :
    case PE_PanelButtonCommand : {
      // Interior for push buttons
      capsulePosition(widget,fs.hasCapsule,fs.capsuleH,fs.capsuleV);
      renderInterior(p,cs2b(cs.bg,pal.button()),r,fs,is,is.element+"-"+st,dir);
      break;
    }
    case PE_FrameDefaultButton : {
      // Frame for default buttons
      // use "default" status
      if ( option->state & State_On ) {
        st = "default";
        renderFrame(p,cs2b(cs.bg,pal.button()),r,fs,fs.element+"-"+st,dir);
      }
      break;
    }
    case PE_FrameButtonBevel : {
      // Frame for push buttons
      capsulePosition(widget,fs.hasCapsule,fs.capsuleH,fs.capsuleV);
      renderFrame(p,cs2b(cs.bg,pal.button()),r,fs,fs.element+"-"+st,dir);
      break;
    }
    case PE_PanelButtonTool : {
      // Interior for tool buttons
      // Do not compute capsule position for autoraise buttons
      if ( !(option->state & State_AutoRaise) )
        capsulePosition(widget,fs.hasCapsule,fs.capsuleH,fs.capsuleV);
      renderInterior(p,cs2b(cs.bg,pal.button()),r,fs,is,is.element+"-"+st,dir);
      break;
    }
    case PE_FrameButtonTool : {
      // Frame for tool buttons
      // Do not compute capsule position for autoraise buttons
      if ( !(option->state & State_AutoRaise) )
        capsulePosition(widget,fs.hasCapsule,fs.capsuleH,fs.capsuleV);
      renderFrame(p,cs2b(cs.bg,pal.button()),r,fs,fs.element+"-"+st,dir);
      break;
    }
    case PE_PanelTipLabel : {
      // frame and interior for tool tips
      renderFrame(p,cs2b(cs.bg,pal.brush(brole)),r,fs,fs.element+"-"+st,dir);
      renderInterior(p,cs2b(cs.bg,pal.brush(brole)),r,fs,is,is.element+"-"+st,dir);
      break;
    }
    case PE_IndicatorRadioButton : {
      ds.size = pixelMetric(PM_ExclusiveIndicatorHeight);
      // a radio button (exclusive choice)
      // QSvgStyle: no pressed or toggled status for radio buttons
      st = (option->state & State_Enabled) ?
          (option->state & State_MouseOver) ? "hovered" : "normal"
        : "disabled";
      if ( option->state & State_On )
        st = "checked-"+st;
      fs.hasFrame = false;
      renderIndicator(p,r,fs,is,ds,ds.element+"-"+st,dir);
      break;
    }
    case PE_IndicatorViewItemCheck :
      // a check box inside view items
    case PE_IndicatorCheckBox : {
      ds.size = pixelMetric(PM_IndicatorHeight);
      // a check box (multiple choices)
      // QSvgStyle: no pressed or toggled status for check boxes
      st = (option->state & State_Enabled) ?
          (option->state & State_MouseOver) ? "hovered" : "normal"
        : "disabled";
      if ( option->state & State_On )
        st = "checked-"+st;
      else if ( option->state & State_NoChange )
        st = "tristate-"+st;
      fs.hasFrame = false;
      renderIndicator(p,r,fs,is,ds,ds.element+"-"+st,dir);
      break;
    }
    case PE_IndicatorMenuCheckMark : {
      // Check box or radio button of a menu item
      if ( const QStyleOptionMenuItem *opt =
            qstyleoption_cast<const QStyleOptionMenuItem *>(option) ) {

        QStyleOptionMenuItem o(*opt);

        if ( opt->checked )
          o.state |= State_On;

        if ( opt->checkType == QStyleOptionMenuItem::Exclusive )
          drawPrimitive(PE_IndicatorRadioButton,&o,p,widget);
        else if ( opt->checkType == QStyleOptionMenuItem::NonExclusive )
          drawPrimitive(PE_IndicatorCheckBox,&o,p,widget);
      }
      break;
    }
    case PE_FrameFocusRect : {
      // The frame of a focus rectangle, used on focusable widgets
      renderFrame(p,cs2b(cs.bg,pal.brush(brole)),r,fs,fs.element+"-focus-normal",dir);
      break;
    }
    case PE_IndicatorBranch : {
      // Indicator of tree hierarchies
      if (option->state & State_Children) {
        if (option->state & State_Open)
          st = "minus-"+st;
        else
          st = "plus-"+st;
        renderIndicator(p,r,fs,is,ds,ds.element+"-"+st,dir);
      }
      break;
    }
    case PE_FrameMenu :  {
      // Frame for menus
      QStyleOption o(*option);
      o.state &= ~State_On;
      o.state |= State_Enabled;

      st = state_str(o.state,widget);

      renderFrame(p,cs2b(cs.bg,pal.brush(brole)),r,fs,fs.element+"-"+st,dir);
      break;
    }
    case PE_FrameWindow : {
      // Frame for windows
      renderFrame(p,cs2b(cs.bg,pal.brush(brole)),r,fs,fs.element+"-"+st,dir);
      break;
    }
    case PE_FrameTabBarBase : {
      // ???
      break;
    }
    case PE_Frame : {
      // Generic frame
      if ( const QStyleOptionFrame *opt =
        qstyleoption_cast<const QStyleOptionFrame *>(option) ) {
        QStyleOptionFrame o(*opt);
        // NOTE remove hovered, toggled and pressed states
        o.state &= ~(State_MouseOver | State_Sunken | State_On);
        st = state_str(o.state,widget);

        if ( opt->state & State_Sunken )
          renderFrame(p,cs2b(cs.bg,pal.brush(brole)),r,fs,fs.element+"-sunken-"+st,dir);
        else
          renderFrame(p,cs2b(cs.bg,pal.brush(brole)),r,fs,fs.element+"-raised-"+st,dir);

        if ( !qobject_cast< const QTreeWidget* >(widget) ) {
          if ( opt->state & State_Sunken )
            renderInterior(p,cs2b(cs.bg,pal.brush(brole)),r,fs,is,is.element+"-sunken-"+st,dir);
          if ( opt->state & State_Raised )
            renderInterior(p,cs2b(cs.bg,pal.brush(brole)),r,fs,is,is.element+"-raised-"+st,dir);
        }
      }
      break;
    }
    case PE_FrameDockWidget : {
      // Frame for "detached" dock widgets
      if ( const QStyleOptionDockWidget *opt =
        qstyleoption_cast<const QStyleOptionDockWidget *>(option) ) {
        if ( opt->verticalTitleBar )
          orn = Vertical;
        else
          orn = Horizontal;
        renderFrame(p,cs2b(cs.bg,pal.brush(brole)),r,fs,fs.element+"-"+st,dir,orn);
        renderInterior(p,cs2b(cs.bg,pal.brush(brole)),r,fs,is,is.element+"-"+st,dir,orn);
      }
      break;
    }
    case PE_FrameStatusBarItem : {
      // Frame for status bar items
      renderFrame(p,cs2b(cs.bg,pal.brush(brole)),r,fs,fs.element+"-"+st,dir);
      break;
    }
    case PE_FrameGroupBox : {
      // Frame and interior for group boxes
      renderFrame(p,cs2b(cs.bg,pal.brush(brole)),r,fs,fs.element+"-"+st,dir);
      renderInterior(p,cs2b(cs.bg,pal.brush(brole)),r,fs,is,is.element+"-"+st,dir);
      break;
    }
    case PE_FrameTabWidget : {
      // Frame for tab widgets (contents)
      renderFrame(p,cs2b(cs.bg,pal.brush(brole)),r,fs,fs.element+"-raised-"+st,dir);
      break;
    }
    case PE_FrameLineEdit : {
      // Frame for line edits
      // NOTE LineEdits have always State_Sunken (see
      // QLineEdit.cpp::initStyleOption()). Remove it
      // QSvgStyle remove also State_On
      QStyleOption o(*option);
      o.state &= ~(State_Sunken | State_On);
      st = state_str(o.state,widget);
      renderFrame(p,cs2b(cs.bg,pal.base()),r,fs,fs.element+"-"+st,dir);
      break;
    }
    case PE_PanelLineEdit : {
      // Interior and frame for line edits
      if ( const QStyleOptionFrame *opt =
           qstyleoption_cast<const QStyleOptionFrame *>(option) ) {
        if ( opt->lineWidth > 0 ) {
          // NOTE QLineEdit sets lineWidth to PM_DefaultFrameWidth
          // when line edit has a frame
          drawPrimitive(PE_FrameLineEdit,option,p,widget);
        } else
          fs.hasFrame = false;
      }
      // NOTE LineEdits have always State_Sunken (see
      // QLineEdit.cpp::initStyleOption()). Remove it
      // QSvgStyle remove also State_On
      QStyleOption o(*option);
      o.state &=  ~ (State_Sunken | State_On);
      st = state_str(o.state,widget);
      renderInterior(p,cs2b(cs.bg,pal.base()),r,fs,is,is.element+"-"+st,dir);
      break;
    }
    case PE_PanelToolBar : {
      // toolbar frame and interior
      renderFrame(p,cs2b(cs.bg,pal.brush(brole)),r,fs,fs.element+"-"+st,dir);
      renderInterior(p,cs2b(cs.bg,pal.brush(brole)),r,fs,is,is.element+"-"+st,dir);
      break;
    }
    case PE_IndicatorToolBarHandle : {
      // toolbar move handle
      renderElement(p,ds.element+"-handle-"+st,r);
      break;
    }
    case PE_IndicatorToolBarSeparator : {
      // toolbar separator
      renderElement(p,ds.element+"-separator-"+st,r);
      break;
    }
    case PE_IndicatorSpinPlus : {
      // Plus spin box indicator
      renderIndicator(p,r,fs,is,ds,ds.element+"-plus-"+st,dir);
      break;
    }
    case PE_IndicatorSpinMinus : {
      // Minus spin box indicator
      renderIndicator(p,r,fs,is,ds,ds.element+"-minus-"+st,dir);
      break;
    }
    case PE_IndicatorSpinUp : {
      // Up spin box indicator
      if ( getSpecificValue("specific.spinbox.variant").toInt() ==
         VA_SPINBOX_BUTTONS_OPPOSITE )
        renderIndicator(p,r,fs,is,ds,ds.element+"-right-"+st,dir);
      else
        renderIndicator(p,r,fs,is,ds,ds.element+"-up-"+st,dir);
      break;
    }
    case PE_IndicatorSpinDown : {
      // down spin box indicator
      if ( getSpecificValue("specific.spinbox.variant").toInt() ==
         VA_SPINBOX_BUTTONS_OPPOSITE )
        renderIndicator(p,r,fs,is,ds,ds.element+"-left-"+st,dir);
      else
        renderIndicator(p,r,fs,is,ds,ds.element+"-down-"+st,dir);
      break;
    }
    case PE_IndicatorHeaderArrow : {
      // Header section sort arrows
      if ( const QStyleOptionHeader *opt =
           qstyleoption_cast<const QStyleOptionHeader *>(option) ) {
        if (opt->sortIndicator == QStyleOptionHeader::SortDown)
          renderIndicator(p,r,fs,is,ds,ds.element+"-down-"+st,dir);
        else if (opt->sortIndicator == QStyleOptionHeader::SortUp)
          renderIndicator(p,r,fs,is,ds,ds.element+"-up-"+st,dir);
      }
      break;
    }
    case PE_IndicatorButtonDropDown : {
      // Drop down button arrow
      renderIndicator(p,r,fs,is,ds,ds.element+"-dropdown-"+st,dir);
      break;
    }
    case PE_PanelMenuBar : {
      // Menu bar "border" (see QMenuBar.cpp)
      QStyleOption o(*option);
      o.state |= State_Enabled;

      st = state_str(o.state,widget);

      renderFrame(p,cs2b(cs.bg,pal.brush(brole)),r,fs,fs.element+"-"+st,dir);
      break;
    }
    case PE_PanelMenu : {
      // Interior of a menu
      // NOTE do nothing. Menu will be filled by its menu items
      // which have their own interior
      break;
    }
    case PE_IndicatorTabTear : {
      // FIXME
      break;
    }
    case PE_IndicatorArrowUp : {
      // Arrow up indicator
      renderIndicator(p,r,fs,is,ds,ds.element+"-up-"+st,dir);
      break;
    }
    case PE_IndicatorArrowDown : {
      // Arrow down indicator
      renderIndicator(p,r,fs,is,ds,ds.element+"-down-"+st,dir);
      break;
    }
    case PE_IndicatorArrowLeft : {
      // Arrow left indicator
      renderIndicator(p,r,fs,is,ds,ds.element+"-left-"+st,dir);
      break;
    }
    case PE_IndicatorArrowRight : {
      // Arrow right indicator
      renderIndicator(p,r,fs,is,ds,ds.element+"-right-"+st,dir);
      break;
    }
    case PE_IndicatorColumnViewArrow : {
      // FIXME
      break;
    }
    case PE_IndicatorProgressChunk : {
      // The "filled" part of a progress bar
      renderInterior(p,cs2b(cs.bg,pal.highlight()),r,fs,is,is.element+"-"+st,dir);
      break;
    }
    case PE_PanelItemViewRow : {
      // A row of a item view list
      break;
    }
    case PE_PanelItemViewItem : {
      // An item of a view item
      if ( (option->state & State_Enabled) && (st == "normal") ) {
        if ( const QStyleOptionViewItem *opt =
          qstyleoption_cast<const QStyleOptionViewItem *>(option) ) {

          if ( opt->features & QStyleOptionViewItem::Alternate )
            st = "alt-"+st;
          }
      }
      renderInterior(p,cs2b(cs.bg,pal.brush(brole)),r,fs,is,is.element+"-"+st,dir);
      break;
    }
    case PE_PanelStatusBar : {
      // nothing
      break;
    }
    default :
      //qDebug() << "[QSvgStyle]" << __func__ << ": Unhandled primitive " << element;
      QCommonStyle::drawPrimitive(e,option,p,widget);
      break;
  }

end:
  emit(sig_drawPrimitive_end(PE_str(e)));
}

void QSvgThemableStyle::drawControl(ControlElement e, const QStyleOption * option, QPainter * p, const QWidget * widget) const
{
  emit(sig_drawControl_begin(CE_str(e)));

  // Copy some values into shorter variable names
  int x,y,w,h;
  QRect r = option->rect;
  r.getRect(&x,&y,&w,&h);
  QString st = state_str(option->state, widget);
  Qt::LayoutDirection dir = option->direction;
  bool focus = option->state & State_HasFocus;
  QIcon::Mode icm = state_iconmode(option->state);
  QIcon::State ics = state_iconstate(option->state);
  Orientation orn = option->state & State_Horizontal ? Horizontal : Vertical;
  bool en = option->state & State_Enabled;
  QPalette pal = option->palette;
  QPalette::ColorGroup cg = en ? QPalette::Normal : QPalette::Disabled;
  pal.setCurrentColorGroup(cg);
  QPalette::ColorRole brole = widget ? widget->backgroundRole() : QPalette::NoRole;

//   // Get QSvgStyle configuration group used to render this element
  QString g = CE_group(e);

  // Configuration for group g
  frame_spec_t fs;
  interior_spec_t is;
  label_spec_t ls;
  indicator_spec_t ds;
  color_spec_t cs;

  if ( g.isEmpty() ) {
    // element not currently supported by QSvgStyle
    QCommonStyle::drawControl(e,option,p,widget);
    goto end;
  }

  // Get configuration for group
  fs = getFrameSpec(g);
  is = getInteriorSpec(g);
  ls = getLabelSpec(g);
  ds = getIndicatorSpec(g);
  cs = getColorSpec(g);

  fs.pressed = option->state & State_Sunken;

  switch (e) {
    case CE_FocusFrame : {
      //qDebug() << "CE_FocusFrame" << (widget ? widget->objectName() : "???");
      break;
    }

    case CE_PushButtonBevel : {
      if ( const QStyleOptionButton *opt =
           qstyleoption_cast<const QStyleOptionButton *>(option) ) {

        QStyleOptionButton o(*opt);

        drawPrimitive(PE_FrameButtonBevel,&o,p,widget);

        if ( opt->features & QStyleOptionButton::Flat ) {
          // flat buttons
          if ( (option->state & State_Enabled) &&
                ((option->state & State_Sunken) ||
                (option->state & State_On) ||
                (option->state & State_MouseOver))
              ) {
            // Draw interior around normal non flat tool buttons
            drawPrimitive(PE_PanelButtonBevel,&o,p,widget);
          }
        } else {
          drawPrimitive(PE_PanelButtonBevel,option,p,widget);
        }
      }

      break;
    }

    case CE_MenuTearoff : {
      renderElement(p,is.element+"-tearoff",r,10,0);
      break;
    }

    case CE_MenuItem : {
      if ( const QStyleOptionMenuItem *opt =
           qstyleoption_cast<const QStyleOptionMenuItem *>(option) ) {

        QStyleOptionMenuItem o(*opt);

        // NOTE Cheat here: the frame is for the whole menu,
        // not the individual menu items
        fs.hasFrame = false;
        fs.top = fs.bottom = fs.left = fs.right = 0;

        if (opt->menuItemType == QStyleOptionMenuItem::Separator)
          // Menu separator
          renderElement(p,is.element+"-separator",r,
                        pixelMetric(PM_ProgressBarChunkWidth,opt,widget),0);
        else if (opt->menuItemType == QStyleOptionMenuItem::TearOff)
          // Menu tear off
          drawControl(CE_MenuTearoff,opt,p,widget);
        else {
          // Standard menu item
          // NOTE QSvgStyle ignore pressed state
          o.state &= ~State_Sunken;
          st = state_str(o.state,widget);
          renderInterior(p,cs2b(cs.bg,pal.brush(brole)),r,fs,is,is.element+"-"+st,dir);

          QRect rtext = r; // text rect (menu item label+shortcut)
          QRect rcheckmark, rarrow;

          // determine positions
          const QStringList l = opt->text.split('\t');
          if ( (l.size() > 0) && (opt->icon.isNull()) )
            // No icon ? leave space for icon anyway
            rtext.adjust(opt->maxIconWidth+ls.tispace,0,0,0);

          if ( opt->checkType != QStyleOptionMenuItem::NotCheckable )
            // remove room for check mark
            rtext = rtext.adjusted(0,0,-pixelMetric(PM_IndicatorWidth)-pixelMetric(PM_CheckBoxLabelSpacing),0);
          if ( opt->menuItemType == QStyleOptionMenuItem::SubMenu )
            // remove room for sub menu arrow
            rtext = rtext.adjusted(0,0,-ls.tispace-ds.size,0);

          rarrow = QRect(r.topRight(),QSize(ds.size,h))
                    .translated(-ds.size-fs.right-ls.hmargin,0);
          rcheckmark = QRect(r.topRight(),QSize(pixelMetric(PM_IndicatorWidth),h))
                    .translated(-pixelMetric(PM_IndicatorWidth)-fs.right-ls.hmargin,0);

          if (opt->menuItemType == QStyleOptionMenuItem::SubMenu)
            rcheckmark.translate(-ds.size-ls.tispace,0);

          // translate to visual rects inside r
          rtext = visualRect(dir,r,rtext);
          rcheckmark = visualRect(dir,r,rcheckmark);
          rarrow = visualRect(dir,r,rarrow);

          // draw menu text (label+shortcut)
          if (l.size() > 0) {
            // menu label
            renderLabel(p,cs2b(cs.fg,pal.text()),
                        dir,
                        rtext,
                        fs,is,ls,
                        Qt::AlignLeft|Qt::AlignVCenter | Qt::TextShowMnemonic,
                        l[0],
                        opt->icon.pixmap(opt->maxIconWidth,icm,ics));
          }

          if (l.size() > 1) {
            // shortcut
            renderLabel(p,cs2b(cs.fg,pal.text()),
                        dir,
                        rtext,
                        fs,is,ls,
                        Qt::AlignRight|Qt::AlignVCenter,
                        l[1]);
          }

          // menu check mark
          o.rect = rcheckmark;
          drawPrimitive(PE_IndicatorMenuCheckMark,&o,p,widget);

          // submenu arrow
          if (opt->menuItemType == QStyleOptionMenuItem::SubMenu) {
            o.rect = rarrow;
            if ( dir == Qt::LeftToRight )
              drawPrimitive(PE_IndicatorArrowRight,&o,p);
            else
              drawPrimitive(PE_IndicatorArrowLeft,&o,p);
          }
        }
      }

      break;
    }

    case CE_MenuEmptyArea : {
      break;
    }

    case CE_MenuBarItem : {
      if ( const QStyleOptionMenuItem *opt =
           qstyleoption_cast<const QStyleOptionMenuItem *>(option) ) {

        QStyleOptionMenuItem o(*opt);

        // NOTE QSvgSyle: ignore pressed state
        o.state &= ~State_Sunken;
        st = state_str(o.state,widget);

        // NOTE cheat here: frame is for whole menu bar, not individual
        // menu bar items
        fs.hasFrame = false;
        fs.top = fs.bottom = fs.left = fs.right = 0;

        renderInterior(p,cs2b(cs.bg,pal.brush(brole)),option->rect,fs,is,is.element+"-"+st,dir);
        renderLabel(p,cs2b(cs.fg,pal.text()),
                    dir,r,fs,is,ls,
                    Qt::AlignCenter | Qt::AlignVCenter | Qt::TextShowMnemonic,
                    opt->text);
      }

      break;
    }

    case CE_MenuBarEmptyArea : {
      // Menu bar interior
      renderInterior(p,cs2b(cs.bg,pal.brush(brole)),r,fs,is,is.element+"-"+st,dir);
      break;
    }

    case CE_MenuScroller : {
      drawPrimitive(PE_PanelButtonTool,option,p,widget);
      if (option->state & State_DownArrow)
        drawPrimitive(PE_IndicatorArrowDown,option,p,widget);
      else
        drawPrimitive(PE_IndicatorArrowUp,option,p,widget);

      break;
    }

    case CE_MenuHMargin:
    case CE_MenuVMargin:
      break;

    case CE_RadioButtonLabel : {
      if ( const QStyleOptionButton *opt =
           qstyleoption_cast<const QStyleOptionButton *>(option) ) {

        if ( opt->state & State_MouseOver )
          renderInterior(p,cs2b(cs.bg,pal.brush(brole)),r,fs,is,is.element+"-"+st,dir);

        renderLabel(p,cs2b(cs.fg,pal.text()),
                    dir,r,fs,is,ls,
                    Qt::AlignLeft | Qt::AlignVCenter | Qt::TextShowMnemonic,
                    opt->text,
                    opt->icon.pixmap(opt->iconSize,icm,ics));
      }
      break;
    }

    case CE_CheckBoxLabel : {
      if ( const QStyleOptionButton *opt =
          qstyleoption_cast<const QStyleOptionButton *>(option) ) {

        if ( opt->state & State_MouseOver )
          renderInterior(p,cs2b(cs.bg,pal.brush(brole)),r,fs,is,is.element+"-"+st,dir);

        renderLabel(p,cs2b(cs.fg,pal.text()),
                    dir,r,fs,is,ls,
                    Qt::AlignLeft | Qt::AlignVCenter | Qt::TextShowMnemonic,
                    opt->text,
                    opt->icon.pixmap(opt->iconSize,icm,ics));
      }
      break;
    }

    case CE_ComboBoxLabel : {
      if ( const QStyleOptionComboBox *opt =
           qstyleoption_cast<const QStyleOptionComboBox *>(option) ) {

        if ( !opt->editable ) {
          // NOTE Editable label is rendered by an embedded QLineEdit
          // inside the QComboBox object, except icon
          // See QComboBox's qcombobox.cpp::updateLineEditGeometry()
          renderLabel(p,cs2b(cs.fg,pal.text()),
                      dir,r,fs,is,ls,
                      Qt::AlignLeft | Qt::AlignVCenter | Qt::TextShowMnemonic,
                      opt->currentText,
                      opt->currentIcon.pixmap(opt->iconSize,icm,ics));
        } else {
          // NOTE Non editable combo boxes: the embedded QLineEdit is not
          // able to draw the item icon, so do it here
          renderLabel(p,cs2b(cs.fg,pal.text()),
                      dir,r,fs,is,ls,
                      Qt::AlignLeft | Qt::AlignVCenter | Qt::TextShowMnemonic,
                      " ", // NOTE renderLabel centers icons if text is empty
                      opt->currentIcon.pixmap(opt->iconSize,icm,ics));
        }
      }
      break;
    }

    case CE_TabBarTabShape : {
      if ( const QStyleOptionTab *opt =
           qstyleoption_cast<const QStyleOptionTab *>(option) ) {

        QStyleOptionTab o(*opt);

        if ( o.state & State_Sunken ) {
          // Remove "pressed" state and replace it by "toggled" state
          o.state &= ~State_Sunken;
          o.state |= State_On;
          st = state_str(o.state,widget);
        }

        fs.hasCapsule = true;
        int capsule = 2;

        if ( (opt->shape == QTabBar::RoundedNorth) ||
             (opt->shape == QTabBar::TriangularNorth) ||
             (opt->shape == QTabBar::RoundedSouth) ||
             (opt->shape == QTabBar::TriangularSouth) )
          orn = Horizontal;
        else
          orn = Vertical;

        if (opt->position == QStyleOptionTab::Beginning)
          capsule = -1;
        else if (opt->position == QStyleOptionTab::Middle)
          capsule = 0;
        else if (opt->position == QStyleOptionTab::End)
          capsule = 1;
        else if (opt->position == QStyleOptionTab::OnlyOneTab)
          capsule = 2;

        if ( (orn == Vertical) && (dir == Qt::RightToLeft) ) {
          // TabWidget bug :
          // In vertical position with RTL layout, the tabs do not
          // swap positions (first is always at top, last always at
          // bottom), contrary to horizontal RTL
          // given the flip done by render* functions, this won't give
          // the expected result
          // so cheat on the capsule
          if ( (capsule == -1) || (capsule == 1) )
            capsule = -capsule;
        }

        fs.capsuleH = capsule;

        if ( (opt->shape == QTabBar::RoundedNorth) ||
             (opt->shape == QTabBar::TriangularNorth)
        ) {
          fs.capsuleV = -1;
        }

        if ( (opt->shape == QTabBar::RoundedSouth) ||
             (opt->shape == QTabBar::TriangularSouth)
        ) {
          fs.capsuleV = 1;
        }

        if ( (opt->shape == QTabBar::RoundedWest) ||
             (opt->shape == QTabBar::TriangularWest)
        ) {
          fs.capsuleV = 1;
        }

        if ( (opt->shape == QTabBar::RoundedEast) ||
             (opt->shape == QTabBar::TriangularEast)
        ) {
          fs.capsuleV = -1;
        }

        // In order to colorize tabs, look up the tab widget contents
        // and apply its palette to the tab
        // That's the way Qt Designer applies palettes to individual tabs
        if ( const QTabBar *tb = qobject_cast<const QTabBar *>(widget) ) {
          // From tab tar
          if ( const QTabWidget *tw = qobject_cast<const QTabWidget *>(tb->parent()) ) {
            // to parent tab widget
            int i;
            for (i=0; i<tb->count(); i++) {
              if ( tb->tabText(i) == opt->text ) {
                // compare tab texts to determine tab index as QStyleOptionTab
                // does not suppy it. Assume that tabs have different names
                // which is a good heuristic in real apps
                QWidget *contents = tw->widget(i);
                if ( contents ) {
                  pal = contents->palette();
                  pal.setCurrentColorGroup(cg);
                  brole = contents->backgroundRole();
                }
                break;
              }
            }
          }
        }

        renderInterior(p,cs2b(cs.bg,pal.button()),r,fs,is,is.element+"-"+st,dir,orn);
        renderFrame(p,cs2b(cs.bg,pal.button()),r,fs,fs.element+"-"+st,dir,orn);

        if ( focus ) {
          QStyleOptionFocusRect fropt;
          fropt.QStyleOption::operator=(*opt);
          drawPrimitive(PE_FrameFocusRect, &fropt,p,widget);
        }
      }

      break;
    }

    case CE_TabBarTabLabel : {
      if ( const QStyleOptionTab *opt =
           qstyleoption_cast<const QStyleOptionTab *>(option) ) {

        if ( opt->shape == QTabBar::TriangularEast ||
             opt->shape == QTabBar::RoundedEast ||
             opt->shape == QTabBar::TriangularWest ||
             opt->shape == QTabBar::RoundedWest ) {
          p->save();
          int newX = w+x;
          int newY = y;
          int newRot = 90;
          if ( opt->shape == QTabBar::TriangularWest ||
               opt->shape == QTabBar::RoundedWest ) {
            newX = x;
            newY = y+h;
            newRot = -90;
          }
          QTransform m = QTransform::fromTranslate(newX,newY);
          m.rotate(newRot);
          p->setTransform(m,true);

          r.setRect(0,0,r.height(),r.width()); /* because of transform */
          /* TabWidget does not swap close button position on vertical
           * tabs with RTL layouts */
          dir = Qt::LeftToRight;
        }

        // In order to colorize tabs, look up the tab widget contents
        // and apply its palette to the tab
        // That's the way Qt Designer applies palettes to individual tabs
        if ( const QTabBar *tb = qobject_cast<const QTabBar *>(widget) ) {
          // From tab tar
          if ( const QTabWidget *tw = qobject_cast<const QTabWidget *>(tb->parent()) ) {
            // to parent tab widget
            int i;
            for (i=0; i<tb->count(); i++) {
              if ( tb->tabText(i) == opt->text ) {
                // compare tab texts to determine tab index as QStyleOptionTab
                // does not suppy it. Assume that tabs have different names
                // which is a good heuristic in real apps
                QWidget *contents = tw->widget(i);
                if ( contents ) {
                  pal = contents->palette();
                  pal.setCurrentColorGroup(cg);
                  brole = contents->backgroundRole();
                }
                break;
              }
            }
          }
        }

        renderLabel(p,cs2b(cs.fg,pal.text()),
                    dir,
                    r,
                    fs,is,ls,
                    Qt::AlignLeft | Qt::AlignVCenter| Qt::TextShowMnemonic,
                    opt->text,
                    opt->icon.pixmap(opt->iconSize,icm,ics));

        if ( opt->shape == QTabBar::TriangularEast ||
             opt->shape == QTabBar::RoundedEast ||
             opt->shape == QTabBar::TriangularWest ||
             opt->shape == QTabBar::RoundedWest ) {
          p->restore();
        }
      }

      break;
    }

    case CE_TabBarTab : {
      drawControl(CE_TabBarTabShape,option,p,widget);
      drawControl(CE_TabBarTabLabel,option,p,widget);

      break;
    }

    case CE_ToolBoxTabShape : {
      // In order to colorize tabs, look up the tool box widget contents
      // and apply its palette to the tab
      if ( const QStyleOptionToolBox *opt =
           qstyleoption_cast<const QStyleOptionToolBox *>(option) ) {

        if ( const QToolBox *tb = qobject_cast<const QToolBox *>(widget) ) {
          int i;
          for (i=0; i<tb->count(); i++) {
            if ( tb->itemText(i) == opt->text ) {
              // compare tab texts to determine tab index as QStyleOptionTab
              // does not suppy it. Assume that tabs have different names
              // which is a good heuristic in real apps
              QWidget *contents = tb->widget(i);
              if ( contents ) {
                pal = contents->palette();
                cg = contents->isEnabled() ? QPalette::Normal : QPalette::Disabled;
                pal.setCurrentColorGroup(cg);
                brole = contents->backgroundRole();
              }
              break;
            }
          }
        }

        renderFrame(p,cs2b(cs.bg,pal.button()),option->rect,fs,fs.element+"-"+st,dir);
        renderInterior(p,cs2b(cs.bg,pal.button()),option->rect,fs,is,is.element+"-"+st,dir);
      }

      break;
    }

    case CE_ToolBoxTabLabel : {
      if ( const QStyleOptionToolBox *opt =
           qstyleoption_cast<const QStyleOptionToolBox *>(option) ) {

        if ( const QToolBox *tb = qobject_cast<const QToolBox *>(widget) ) {
          int i;
          for (i=0; i<tb->count(); i++) {
            if ( tb->itemText(i) == opt->text ) {
              // compare tab texts to determine tab index as QStyleOptionTab
              // does not suppy it. Assume that tabs have different names
              // which is a good heuristic in real apps
              QWidget *contents = tb->widget(i);
              if ( contents ) {
                pal = contents->palette();
                cg = contents->isEnabled() ? QPalette::Normal : QPalette::Disabled;
                pal.setCurrentColorGroup(cg);
                brole = contents->backgroundRole();
              }
              break;
            }
          }
        }

        renderLabel(p,cs2b(cs.fg,pal.text()),
                    dir,r,fs,is,ls,
                    Qt::AlignCenter | Qt::TextShowMnemonic,
                    opt->text,
                    opt->icon.pixmap(pixelMetric(PM_SmallIconSize),icm,ics));

        if ( focus ) {
          QStyleOptionFocusRect fropt;
          fropt.QStyleOption::operator=(*opt);
          fropt.rect = interiorRect(r,fs,is);
          drawPrimitive(PE_FrameFocusRect, &fropt,p,widget);
        }
      }

      break;
    }

    case CE_ProgressBar : {
      // whole progress bar widget
      if ( const QStyleOptionProgressBar *opt =
          qstyleoption_cast<const QStyleOptionProgressBar *>(option) ) {

        QStyleOptionProgressBar o(*opt);

        o.rect = subElementRect(SE_ProgressBarGroove, opt, widget);
        drawControl(CE_ProgressBarGroove, &o, p, widget);

        o.rect = subElementRect(SE_ProgressBarContents, opt, widget);
        drawControl(CE_ProgressBarContents, &o, p, widget);

        if ( opt->textVisible ) {
          o.rect = subElementRect(SE_ProgressBarLabel, opt, widget);
          drawControl(CE_ProgressBarLabel, &o, p, widget);
        }
      }

      break;
    }

    case CE_ProgressBarGroove : {
      // "background" of a progress bar
      renderFrame(p,cs2b(cs.bg,pal.brush(brole)),r,fs,fs.element+"-"+st,dir,orn);
      renderInterior(p,cs2b(cs.bg,pal.brush(brole)),r,fs,is,is.element+"-"+st,dir,orn);

      break;
    }

    case CE_ProgressBarLabel : {
      if ( const QStyleOptionProgressBar *opt =
           qstyleoption_cast<const QStyleOptionProgressBar *>(option) ) {

        if ( orn == Vertical ) {
          p->save();
          int newX = w+x;
          int newY = y;
          int newRot = 90;
          if ( opt->bottomToTop ) {
            newX = x;
            newY = y+h;
            newRot = -90;
          }
          QTransform m = QTransform::fromTranslate(newX,newY);
          m.rotate(newRot);
          p->setTransform(m,true);

          r.setRect(0,0,r.height(),r.width());
        }
        renderLabel(p,cs2b(cs.fg,pal.text()),
                    dir,r,fs,is,ls,
                    opt->textAlignment,opt->text,
                    QPixmap());
        if ( orn == Vertical ) {
          p->restore();
        }
      }

      break;
    }

    case CE_ProgressBarContents : {
      // the progress indicator
      if ( const QStyleOptionProgressBar *opt =
           qstyleoption_cast<const QStyleOptionProgressBar *>(option) ) {

        if ( orn != Horizontal ) {
          // perform computations on horizontalized widget
          r = transposedRect(r);
          qSwap(x,y);
          qSwap(w,h);
        }

        QRect orig = r;
        is.px = pixelMetric(PM_ProgressBarChunkWidth);

        if ( opt->progress >= 0 ) {
          // Normal progress bar
          int empty = sliderPositionFromValue(opt->minimum,
                                              opt->maximum,
                                              opt->maximum-opt->progress,
                                              interiorRect(r,fs,is).width(),
                                              false);

          r = r.adjusted(0,0,-empty,0); // filled area

          if ( orn == Horizontal )
            r = visualRect(dir,orig,r);

          if ( opt->invertedAppearance )
            r = visualRect(Qt::RightToLeft,orig,r);

          renderInterior(p,cs2b(cs.bg,pal.brush(brole)),
                         (orn != Horizontal) ? transposedRect(r) : r,
                         fs,is,
                         is.element+"-elapsed-"+st,
                         dir,
                         orn);
        } else { // busy progressbar
          int variant = getSpecificValue("specific.progressbar.busy.variant").toInt();

          QWidget *wd = (QWidget *)widget;
          int animcount = progressbars[wd];
          int pm = pixelMetric(PM_ProgressBarChunkWidth)+fs.left+fs.right;

          switch (variant) {
            case VA_PROGRESSBAR_BUSY_WRAP : {
              r = r.adjusted(animcount%w,0,0,0);
              r.setWidth(pm);
              if ( r.x()+r.width()-1 > x+w-1 ) {
                // wrap busy indicator
                int ww = x+w-r.x();

                if ( orn == Horizontal )
                  r = visualRect(dir,orig,r);

                if ( opt->invertedAppearance )
                  r = visualRect(Qt::RightToLeft,orig,r);

                p->setClipRect(r.x(),r.y(),ww,r.height());
                renderInterior(p,cs2b(cs.bg,pal.brush(brole)),
                              (orn != Horizontal) ? transposedRect(r) : r,
                              fs,is,
                              is.element+"-elapsed-"+st,
                              dir,
                              orn);
                p->setClipRect(QRect());

                r = QRect(orig.x()-ww,orig.y(),pm,h);

                if ( orn == Horizontal )
                  r = visualRect(dir,orig,r);

                if ( opt->invertedAppearance )
                  r = visualRect(Qt::RightToLeft,orig,r);

                p->setClipRect(orig.x(),orig.y(),pm,h);
                renderInterior(p,cs2b(cs.bg,pal.brush(brole)),
                              (orn != Horizontal) ? transposedRect(r) : r,
                              fs,is,
                              is.element+"-elapsed-"+st,
                              dir,
                              orn);
                p->setClipRect(QRect());
              } else {
                if ( orn == Horizontal )
                  r = visualRect(dir,orig,r);

                if ( opt->invertedAppearance )
                  r = visualRect(Qt::RightToLeft,orig,r);

                renderInterior(p,cs2b(cs.bg,pal.brush(brole)),
                              (orn != Horizontal) ? transposedRect(r) : r,
                              fs,is,
                              is.element+"-elapsed-"+st,
                              dir,
                              orn);
              }
              break;
            }
            case VA_PROGRESSBAR_BUSY_BACKANDFORTH : {
              r = r.adjusted(animcount%(2*(w-pm)),0,0,0);
              if ( r.x() > x+w-pm )
                r.setX(x+2*(w-pm)-r.x());
              r.setWidth(pm);

              if ( orn == Horizontal )
                r = visualRect(dir,orig,r);

              if ( opt->invertedAppearance )
                r = visualRect(Qt::RightToLeft,orig,r);

              renderInterior(p,cs2b(cs.bg,pal.brush(brole)),
                            (orn != Horizontal) ? transposedRect(r) : r,
                            fs,is,
                            is.element+"-elapsed-"+st,
                            dir,
                            orn);
              break;
            }
            case VA_PROGRESSBAR_BUSY_FULLLENGTH :
            default: {
              int ni = animcount%pm;
              if ( getSpecificValue("specific.progressbar.busy.full.variant").toInt() ==
                   VA_PROGRESSBAR_BUSY_FULLLENGTH_DIRECTION_FWD )
                ni = pm-ni;
              r.adjust(-ni,0,w+ni,0);

              if ( orn == Horizontal )
                r = visualRect(dir,orig,r);

              if ( opt->invertedAppearance )
                r = visualRect(Qt::RightToLeft,orig,r);

              p->setClipRect(orig);
              renderInterior(p,cs2b(cs.bg,pal.brush(brole)),
                            (orn != Horizontal) ? transposedRect(r) : r,
                            fs,is,
                            is.element+"-elapsed-"+st,
                            dir,
                            orn);
              p->setClipRect(QRect());
              break;
            }
          }
        }
      }

      break;
    }

    case CE_Splitter : {
      renderFrame(p,cs2b(cs.bg,pal.brush(brole)),option->rect,fs,fs.element+"-"+st,dir,orn);
      renderInterior(p,cs2b(cs.bg,pal.brush(brole)),option->rect,fs,is,is.element+"-"+st,dir,orn);

      break;
    }

    case CE_ScrollBarAddLine : {
      if ( const QStyleOptionSlider *opt =
           qstyleoption_cast<const QStyleOptionSlider *>(option) ) {
        QStyleOptionSlider o(*opt);

        o.state &= ~(State_Sunken | State_Selected | State_On | State_MouseOver);
        if ( opt->activeSubControls & SC_ScrollBarAddLine )
          o.state = opt->state;
        st = state_str(o.state,widget);
        renderFrame(p,cs2b(cs.bg,pal.brush(brole)),option->rect,fs,fs.element+"-"+st,dir);
      renderInterior(p,cs2b(cs.bg,pal.brush(brole)),option->rect,fs,is,is.element+"-"+st,dir);
        if (option->state & State_Horizontal) {
            drawPrimitive(PE_IndicatorArrowRight,option,p,widget);
        } else
          drawPrimitive(PE_IndicatorArrowDown,option,p,widget);
      }
      break;
    }

    case CE_ScrollBarSubLine : {
      if ( const QStyleOptionSlider *opt =
        qstyleoption_cast<const QStyleOptionSlider *>(option) ) {
        QStyleOptionSlider o(*opt);

        o.state &= ~(State_Sunken | State_Selected | State_On | State_MouseOver);
        if ( opt->activeSubControls & SC_ScrollBarSubLine )
          o.state = opt->state;
        st = state_str(o.state,widget);
        renderFrame(p,cs2b(cs.bg,pal.brush(brole)),option->rect,fs,fs.element+"-"+st,dir);
        renderInterior(p,cs2b(cs.bg,pal.brush(brole)),option->rect,fs,is,is.element+"-"+st,dir);
        if (option->state & State_Horizontal) {
            drawPrimitive(PE_IndicatorArrowLeft,option,p,widget);
        } else
          drawPrimitive(PE_IndicatorArrowUp,option,p,widget);
      }
      break;
    }

    case CE_ScrollBarSlider : {
      if ( const QStyleOptionSlider *opt =
        qstyleoption_cast<const QStyleOptionSlider *>(option) ) {
        QStyleOptionSlider o(*opt);

        o.state &= ~(State_Sunken | State_Selected | State_On | State_MouseOver);
        if ( opt->activeSubControls & SC_ScrollBarSlider )
          o.state = opt->state;
        st = state_str(o.state,widget);
        renderFrame(p,cs2b(cs.bg,pal.brush(brole)),option->rect,fs,fs.element+"-cursor-"+st,dir);
        renderInterior(p,cs2b(cs.bg,pal.brush(brole)),option->rect,fs,is,is.element+"-cursor-"+st,dir);
      }
      break;
    }

    case CE_HeaderSection : {
      renderFrame(p,cs2b(cs.bg,pal.brush(brole)),r,fs,fs.element+"-"+st,dir);
      renderInterior(p,cs2b(cs.bg,pal.brush(brole)),r,fs,is,is.element+"-"+st,dir);
      break;
    }

    case CE_HeaderLabel : {
      if ( const QStyleOptionHeader *opt =
           qstyleoption_cast<const QStyleOptionHeader *>(option) ) {

        QStyleOptionHeader o(*opt);

        o.rect = subElementRect(SE_HeaderLabel,opt,widget);
        renderLabel(p,cs2b(cs.fg,pal.text()),
                    dir,o.rect,fs,is,ls,
                    opt->textAlignment,
                    opt->text,
                    opt->icon.pixmap(pixelMetric(PM_SmallIconSize),icm,ics));
        o.rect = subElementRect(SE_HeaderArrow,opt,widget);
        if ( opt->sortIndicator == QStyleOptionHeader::SortDown )
          drawPrimitive(PE_IndicatorArrowDown,&o,p,widget);
        else if ( opt->sortIndicator == QStyleOptionHeader::SortUp )
          drawPrimitive(PE_IndicatorArrowUp,&o,p,widget);
      }
      break;
    }

    case CE_Header : {
      drawControl(CE_HeaderSection,option,p,widget);
      drawControl(CE_HeaderLabel,option,p,widget);

      break;
    }

    case CE_ToolBar : {
      renderFrame(p,cs2b(cs.bg,pal.brush(brole)),option->rect,fs,fs.element+"-"+st,dir,orn);
      renderInterior(p,cs2b(cs.bg,pal.brush(brole)),option->rect,fs,is,is.element+"-"+st,dir,orn);
      break;
    }

    case CE_SizeGrip : {
      renderIndicator(p,option->rect,fs,is,ds,ds.element+"-sizegrip-"+st,dir);
      break;
    }

    case CE_PushButton : {
      if ( const QStyleOptionButton *opt =
           qstyleoption_cast<const QStyleOptionButton *>(option) ) {
        drawControl(CE_PushButtonBevel,opt,p,widget);
        drawControl(CE_PushButtonLabel,opt,p,widget);
        if ( focus ) {
          QStyleOptionFocusRect fropt;
          fropt.QStyleOption::operator=(*opt);
          fropt.rect = subElementRect(SE_PushButtonFocusRect, opt, widget);
          drawPrimitive(PE_FrameFocusRect, &fropt,p,widget);
        }
      }
      break;
    }

    case CE_PushButtonLabel : {
      if ( const QStyleOptionButton *opt =
           qstyleoption_cast<const QStyleOptionButton *>(option) ) {

        if ( opt->features & QStyleOptionButton::HasMenu ) {
          QStyleOptionButton o(*opt);
          renderLabel(p,cs2b(cs.fg,pal.text()),
                      dir,r.adjusted(0,0,-ds.size-2*ls.tispace,0),fs,is,ls,
                      Qt::AlignCenter | Qt::AlignVCenter | Qt::TextShowMnemonic,
                      opt->text,
                      opt->icon.pixmap(opt->iconSize,icm,ics));
          o.rect = QRect(x+w-ds.size-ls.tispace-fs.right,y,ds.size,h);
          drawPrimitive(PE_IndicatorArrowDown,&o,p,widget);
        } else {
          renderLabel(p,cs2b(cs.fg,pal.text()),
                      dir,r,fs,is,ls,
                      Qt::AlignCenter | Qt::AlignVCenter | Qt::TextShowMnemonic,
                      opt->text,
                      opt->icon.pixmap(opt->iconSize,icm,ics));
        }
      }

      break;
    }

    case CE_ToolButtonLabel : {
      ds = getIndicatorSpec(PE_group(PE_IndicatorArrowDown));
      if ( const QStyleOptionToolButton *opt =
          qstyleoption_cast<const QStyleOptionToolButton *>(option) ) {

        if (opt->arrowType == Qt::NoArrow)
          renderLabel(p,cs2b(cs.fg,pal.text()),
                      dir,r,fs,is,ls,
                      Qt::AlignCenter | Qt::TextShowMnemonic,
                      opt->text,
                      opt->icon.pixmap(opt->iconSize,icm,ics),
                      opt->toolButtonStyle);
        else {
          if ( dir == Qt::LeftToRight )
            renderLabel(p,cs2b(cs.fg,pal.text()),
                        dir,r.adjusted(ds.size+ls.tispace,0,0,0),fs,is,ls,
                        Qt::AlignCenter | Qt::TextShowMnemonic,
                        opt->text,
                        opt->icon.pixmap(opt->iconSize,icm,ics),
                        opt->toolButtonStyle);
          else
            renderLabel(p,cs2b(cs.fg,pal.text()),
                        dir,r.adjusted(0,0,-ds.size-ls.tispace,0),fs,is,ls,
                        Qt::AlignCenter | Qt::TextShowMnemonic,
                        opt->text,
                        opt->icon.pixmap(opt->iconSize,icm,ics),
                        opt->toolButtonStyle);
        }

        // alignement of arrow
        Qt::Alignment hAlign = visualAlignment(dir,Qt::AlignLeft);
        if ( opt->text.isEmpty() && opt->icon.isNull() )
          hAlign = Qt::AlignCenter;

        switch (opt->arrowType) {
          case Qt::NoArrow :
            break;
          case Qt::UpArrow :
            renderIndicator(p,r,fs,is,ds,ds.element+"-up-"+st,
                            dir,
                            hAlign | Qt::AlignVCenter);
            break;
          case Qt::DownArrow :
            renderIndicator(p,r,fs,is,ds,ds.element+"-down-"+st,
                            dir,
                            hAlign | Qt::AlignVCenter);
            break;
          case Qt::LeftArrow :
            renderIndicator(p,option->rect,fs,is,ds,ds.element+"-left-"+st,
                            dir,
                            hAlign | Qt::AlignVCenter);
            break;
          case Qt::RightArrow :
            renderIndicator(p,option->rect,fs,is,ds,ds.element+"-right-"+st,
                            dir,
                            hAlign | Qt::AlignVCenter);
            break;
        }
      }

      break;
    }

    case CE_DockWidgetTitle : {
      if ( const QStyleOptionDockWidget *opt =
           qstyleoption_cast<const QStyleOptionDockWidget *>(option) ) {

        if ( opt->verticalTitleBar )
          orn = Vertical;
        else
          orn = Horizontal;

        renderFrame(p,cs2b(cs.bg,pal.brush(brole)),r,fs,fs.element+"-"+st,dir,orn);
        renderInterior(p,cs2b(cs.bg,pal.brush(brole)),r,fs,is,is.element+"-"+st,dir,orn);

        if ( opt->verticalTitleBar ) {
          p->save();
          r = transposedRect(r);
          p->translate(r.left(), r.top() + r.width());
          p->rotate(-90);
          p->translate(-r.left(), -r.top());
        }
        renderLabel(p,cs2b(cs.fg,pal.text()),
                    dir,r,fs,is,ls,
                    Qt::AlignLeft | Qt::AlignVCenter | Qt::TextShowMnemonic,
                    opt->title);
        if ( opt->verticalTitleBar ) {
          p->restore();
        }
      }

      break;
    }

    case CE_ShapedFrame : {
      if ( const QStyleOptionFrame *opt =
           qstyleoption_cast<const QStyleOptionFrame *>(option) ) {

        if ( opt && (opt->frameShape == QFrame::HLine) ) {
          renderElement(p,
                        fs.element+"-"+"hsep",
                        option->rect,
                        0,0);
        } else if (opt && (opt->frameShape == QFrame::VLine) ) {
          renderElement(p,
                        fs.element+"-"+"vsep",
                        option->rect,
                        0,0);
        } else if (opt && (opt->frameShape != QFrame::NoFrame) ) {
          drawPrimitive(PE_Frame,opt,p,widget);
        }
      }

      break;
    }

    default :
      //qDebug() << "[QSvgStyle] " << __func__ << ": Unhandled control " << element;
      QCommonStyle::drawControl(e,option,p,widget);
  }

end:
  emit(sig_drawControl_end(CE_str(e)));
}

void QSvgThemableStyle::drawComplexControl(ComplexControl control, const QStyleOptionComplex * option, QPainter * p, const QWidget * widget) const
{
  emit(sig_drawComplexControl_begin(CC_str(control)));

  // Copy some values into shorter variable names
  int x,y,w,h;
  QRect r = option->rect;
  r.getRect(&x,&y,&w,&h);
  QString st = state_str(option->state, widget);
  Qt::LayoutDirection dir = option->direction;
  bool focus = option->state & State_HasFocus;
  QIcon::Mode icm = state_iconmode(option->state);
  QIcon::State ics = state_iconstate(option->state);
  QFontMetrics fm = option->fontMetrics;
  Orientation orn = option->state & State_Horizontal ? Horizontal : Vertical;
  bool en = option->state & State_Enabled;
  QPalette pal = option->palette;
  QPalette::ColorGroup cg = en ? QPalette::Normal : QPalette::Disabled;
  pal.setCurrentColorGroup(cg);
  QPalette::ColorRole brole = widget ? widget->backgroundRole() : QPalette::NoRole;

  Q_UNUSED(focus);
  Q_UNUSED(icm);
  Q_UNUSED(ics);

  // Get QSvgStyle configuration group used to render this element
  QString g = CC_group(control);

  // Configuration for group g
  frame_spec_t fs;
  interior_spec_t is;
  label_spec_t ls;
  indicator_spec_t ds;
  color_spec_t cs;

  if ( g.isEmpty() ) {
    // element not currently supported by QSvgStyle
    QCommonStyle::drawComplexControl(control,option,p,widget);
    goto end;
  }

  // Get configuration for group
  fs = getFrameSpec(g);
  is = getInteriorSpec(g);
  ls = getLabelSpec(g);
  ds = getIndicatorSpec(g);
  cs = getColorSpec(g);

  switch (control) {
    case CC_ToolButton : {
      if ( const QStyleOptionToolButton *opt =
          qstyleoption_cast<const QStyleOptionToolButton *>(option) ) {

        QStyleOptionToolButton o(*opt);

        QRect dropRect = subControlRect(CC_ToolButton,opt,SC_ToolButtonMenu,widget);
        QRect buttonRect = subControlRect(CC_ToolButton,opt,SC_ToolButton,widget);

        QStyle::State buttonState = opt->state;
        QStyle::State dropState = opt->state;

        if ( opt->features & QStyleOptionToolButton::Menu ) {
          // with drop down menu -> apply State only to the
          // selected subcontrol
          if ( !(opt->activeSubControls & QStyle::SC_ToolButton) )
            buttonState &= ~(State_Sunken | State_MouseOver);
          if ( !(opt->activeSubControls & QStyle::SC_ToolButtonMenu) )
            dropState &= ~(State_On | State_Sunken | State_MouseOver);
        }

        // draw frame and interior
        if ( option->state & State_AutoRaise ) {
          // Auto raise buttons (found on toolbars)
          if ( (option->state & State_Enabled) &&
                ((option->state & State_Sunken) ||
                (option->state & State_On) ||
                (option->state & State_MouseOver))
              ) {
            // Draw frame and interior around normal non autoraise tool buttons
            o.rect = r;
            o.state = opt->state;
            drawPrimitive(PE_FrameButtonTool,&o,p,widget);
            o.state = buttonState;
            drawPrimitive(PE_PanelButtonTool,&o,p,widget);
            if ( opt->features & QStyleOptionToolButton::Menu ) {
              o.rect = dropRect;
              o.state = dropState;
              drawPrimitive(PE_PanelButtonTool,&o,p,widget);
            }
          }
        } else {
          o.rect = r;
          o.state = opt->state;
          drawPrimitive(PE_FrameButtonTool,&o,p,widget);
          o.state = buttonState;
          drawPrimitive(PE_PanelButtonTool,&o,p,widget);
          if ( opt->features & QStyleOptionToolButton::Menu ) {
            o.rect = dropRect;
            o.state = dropState;
            drawPrimitive(PE_PanelButtonTool,&o,p,widget);
          }
        }

        // Draw label
        o.rect = buttonRect;
        o.state = opt->state;
        drawControl(CE_ToolButtonLabel,&o,p,widget);

        // Draw arrow
        o.rect = dropRect;
        if (opt->features & QStyleOptionToolButton::Menu) {
          // FIXME draw drop down button separator
          // Tool button with independant drop down button
          o.state = dropState;
          drawPrimitive(PE_IndicatorButtonDropDown,&o,p,widget);
        } else if (opt->features & QStyleOptionToolButton::HasMenu) {
          // Simple down arrow for tool buttons with menus
          o.state = opt->state;
          drawPrimitive(PE_IndicatorArrowDown,&o,p,widget);
        }

        if ( focus ) {
          QStyleOptionFocusRect fropt;
          fropt.QStyleOption::operator=(*opt);
          fropt.rect = interiorRect(r,fs,is);
          drawPrimitive(PE_FrameFocusRect, &fropt,p,widget);
        }
      }

      break;
    }

    case CC_SpinBox : {
      if (const QStyleOptionSpinBox *opt =
          qstyleoption_cast<const QStyleOptionSpinBox *>(option)) {

        QStyleOptionSpinBox o(*opt);

        // Remove sunken,pressed attributes when drawing frame
        o.state &= ~(State_Sunken | State_Selected);
        // draw frame
        if ( opt->frame ) {
          o.rect = subControlRect(CC_SpinBox,opt,SC_SpinBoxFrame,widget);
          drawPrimitive(PE_FrameLineEdit,&o,p,widget);
        } else
          fs.hasFrame = false;

        // draw spin buttons
        fs.hasFrame = false;
        if (opt->buttonSymbols != QAbstractSpinBox::NoButtons) {
          o.state = opt->state;
          o.state &= ~(State_Sunken | State_Selected | State_MouseOver);
          if ( opt->activeSubControls & SC_SpinBoxUp )
            o.state = opt->state;
          st = state_str(o.state,widget);
          o.rect = subControlRect(CC_SpinBox,opt,SC_SpinBoxUp,widget);
          renderInterior(p,cs2b(cs.bg,pal.button()),o.rect,fs,is,is.element+'-'+st,dir);

          if (opt->buttonSymbols == QAbstractSpinBox::UpDownArrows) {
            o.rect = subControlRect(CC_SpinBox,opt,SC_SpinBoxUp,widget);
            drawPrimitive(PE_IndicatorSpinUp,&o,p,widget);
          }

          if (opt->buttonSymbols == QAbstractSpinBox::PlusMinus) {
            o.rect = subControlRect(CC_SpinBox,opt,SC_SpinBoxUp,widget);
            drawPrimitive(PE_IndicatorSpinPlus,&o,p,widget);
          }

          o.state = opt->state;
          o.state &= ~(State_Sunken | State_Selected | State_MouseOver);
          if ( opt->activeSubControls & SC_SpinBoxDown )
            o.state = opt->state;
          st = state_str(o.state,widget);
          o.rect = subControlRect(CC_SpinBox,opt,SC_SpinBoxDown,widget);
          renderInterior(p,cs2b(cs.bg,pal.button()),o.rect,fs,is,is.element+'-'+st,dir);

          if (opt->buttonSymbols == QAbstractSpinBox::UpDownArrows) {
            o.rect = subControlRect(CC_SpinBox,opt,SC_SpinBoxDown,widget);
            drawPrimitive(PE_IndicatorSpinDown,&o,p,widget);
          }

          if (opt->buttonSymbols == QAbstractSpinBox::PlusMinus) {
            o.rect = subControlRect(CC_SpinBox,opt,SC_SpinBoxDown,widget);
            drawPrimitive(PE_IndicatorSpinMinus,&o,p,widget);
          }
        }
      }

      break;
    }

    case CC_ComboBox : {
      if ( const QStyleOptionComboBox *opt =
           qstyleoption_cast<const QStyleOptionComboBox *>(option) ) {

        QStyleOptionComboBox o(*opt);

        // Draw frame
        if ( opt->frame ) {
          o.rect = subControlRect(CC_ComboBox,opt,SC_ComboBoxFrame,widget);
        } else
          fs.hasFrame = false;

        if ( opt->frame ) {
          if ( opt->editable )
            drawPrimitive(PE_FrameLineEdit,&o,p,widget);
          else
            drawPrimitive(PE_FrameButtonBevel,&o,p,widget);
        }

        // Draw interior
        o.rect = subControlRect(CC_ComboBox,opt,SC_ComboBoxEditField,widget);

        if ( opt->editable ) {
          //drawPrimitive(PE_PanelLineEdit,&o,p,widget);
        } else {
          o.frame = false;
          drawPrimitive(PE_PanelButtonBevel,&o,p,widget);
        }

        // NOTE Don't draw the label, QComboBox will draw it for us
        // see Qt's implementation qcombobox.cpp::paintEvent()

        // draw drop down button
        // FIXME render separator
        if ( opt->editable ) {
          o.state &= ~State_Sunken;
        }

        o.frame = false;
        o.rect = subControlRect(CC_ComboBox,opt,SC_ComboBoxArrow,widget);
        drawPrimitive(PE_PanelButtonBevel,&o,p,widget);
        drawPrimitive(PE_IndicatorButtonDropDown,&o,p,widget);
      }

      break;
    }

    case CC_ScrollBar : {
      const QStyleOptionSlider *opt =
        qstyleoption_cast<const QStyleOptionSlider *>(option);

      if (opt) {
        QStyleOptionSlider o(*opt);

        // Groove
        // Remove pressed and selected state for groove
        o.state &= ~(State_Sunken | State_Selected | State_On | State_MouseOver);
        st = state_str(o.state,widget);
        o.rect = subControlRect(CC_ScrollBar,opt,SC_ScrollBarGroove,widget);
        renderFrame(p,cs2b(cs.bg,pal.brush(brole)),o.rect,fs,fs.element+"-"+st,dir,orn);
        renderInterior(p,cs2b(cs.bg,pal.brush(brole)),o.rect,fs,is,is.element+"-"+st,dir,orn);

        // 'Next' arrow
        o.state = opt->state;
        o.rect = subControlRect(CC_ScrollBar,opt,SC_ScrollBarAddLine,widget);
        drawControl(CE_ScrollBarAddLine,&o,p,widget);

        // 'Previous' arrow
        o.state = opt->state;
        o.rect = subControlRect(CC_ScrollBar,opt,SC_ScrollBarSubLine,widget);
        drawControl(CE_ScrollBarSubLine,&o,p,widget);

        // Cursor
        o.state = opt->state;
        o.rect = subControlRect(CC_ScrollBar,opt,SC_ScrollBarSlider,widget);
        drawControl(CE_ScrollBarSlider,&o,p,widget);
      }

      break;
    }

    case CC_Slider : {
      if ( const QStyleOptionSlider *opt =
           qstyleoption_cast<const QStyleOptionSlider *>(option) ) {

        QStyleOptionSlider o(*opt);

        // remove useless Sunken attribute
        o.state &= ~State_Sunken;
        st = state_str(o.state,widget);

        QRect groove = subControlRect(CC_Slider,opt,SC_SliderGroove,widget);
        QRect empty = groove;
        QRect full = empty;
        QRect slider = subControlRect(CC_Slider,opt,SC_SliderHandle,widget);

        // Groove
        renderFrame(p,cs2b(cs.bg,pal.brush(brole)),empty,fs,fs.element+"-"+st,dir,orn);

        // compute elapsed and empty part of groove
        if (opt->orientation == Qt::Horizontal) {
          if (!opt->upsideDown) {
            full.setWidth(slider.x());
            empty.adjust(slider.x(),0,0,0);
          } else {
            empty.setWidth(slider.x());
            full.adjust(slider.x(),0,0,0);
          }
        } else {
          if (!opt->upsideDown) {
            full.setHeight(slider.y());
            empty.adjust(0,slider.y(),0,0);
          } else {
            empty.setHeight(slider.y());
            full.adjust(0,slider.y(),0,0);
          }
        }

        // draw empty part
        fs.hasCapsule = true;
        if (opt->orientation == Qt::Horizontal)
          fs.capsuleV = 2;
        else
          fs.capsuleH = 2;

        if (opt->orientation == Qt::Horizontal) {
          if (!opt->upsideDown) {
            fs.capsuleH = 1;
          } else {
            fs.capsuleH = -1;
          }
        } else {
          if (!opt->upsideDown) {
            fs.capsuleV = 1;
          } else {
            fs.capsuleV = -1;
          }
        }

        renderInterior(p,cs2b(cs.bg,pal.brush(brole)),empty,fs,is,is.element+"-"+st,dir,orn);

        // draw elapsed part
        if (option->state & State_Horizontal) {
          if (!opt->upsideDown) {
            fs.capsuleH = -1;
          } else {
            fs.capsuleH = 1;
          }
        } else {
          if (!opt->upsideDown) {
            fs.capsuleV = -1;
          } else {
            fs.capsuleV = 1;
          }
        }

        renderInterior(p,cs2b(cs.bg,pal.brush(brole)),full,fs,is,is.element+"-elapsed-"+st,dir,orn);

        // ticks
        o.state &= ~(State_MouseOver | State_Sunken);
        st = state_str(o.state,widget);
        if ( opt->subControls & QStyle::SC_SliderTickmarks ) {
          int interval = opt->tickInterval;
          int range = orn == Horizontal ? groove.width() : groove.height();
          if ( interval <= 0 ) {
            interval = opt->singleStep;
            // make ticks not too close to each other
            if (sliderPositionFromValue(opt->minimum,opt->maximum, interval, range)
                - sliderPositionFromValue(opt->minimum,opt->maximum,0,range) < 3)
              interval = opt->pageStep;
          }
          if ( !interval )
            interval = 3;

          // draw
          int tickOffset = pixelMetric(PM_SliderTickmarkOffset, opt, widget);
          int tickPos = opt->tickPosition;

          int val = opt->minimum;
          int pos;
          while ( val <= opt->maximum+1 ) {
            pos = sliderPositionFromValue(opt->minimum,opt->maximum,
                                          val, range);

            if ( orn == Horizontal ) {
              if ( tickPos & QSlider::TicksBelow ) {
                renderElement(p,is.element+"-htick-"+st,
                              QRect(groove.x()+pos,groove.y()+groove.height()+tickOffset,
                                    1,3)
                              );
              }
              if ( tickPos & QSlider::TicksAbove ) {
                renderElement(p,is.element+"-htick-"+st,
                              QRect(groove.x()+pos,groove.y()-tickOffset-3,
                                    1,3)
                              );
              }
            } else {
              if ( tickPos & QSlider::TicksRight ) {
                renderElement(p,is.element+"-vtick-"+st,
                              QRect(groove.x()+groove.width()+tickOffset,groove.y()+pos,
                                    3,1)
                              );
              }
              if ( tickPos & QSlider::TicksLeft ) {
                renderElement(p,is.element+"-vtick-"+st,
                              QRect(groove.x()-tickOffset-3,groove.y()+pos,
                                    3,1)
                              );
              }
            }

            val += interval;
          }
        }

        // cursor
        o.state = option->state;
        st = state_str(o.state,widget);
        fs.hasFrame = false;
        fs.hasCapsule = false;
        o.rect = subControlRect(CC_Slider,opt,SC_SliderHandle,widget);
        renderInterior(p,cs2b(cs.bg,pal.brush(brole)),o.rect,fs,is,is.element+"-cursor-"+st,dir,orn);
      }

      break;
    }

    case CC_Dial : {
      const QStyleOptionSlider *opt =
          qstyleoption_cast<const QStyleOptionSlider *>(option);

      if (opt) {
        QStyleOptionSlider o(*opt);

        // QDials have vertical orientation which is wrong for us
        orn = Horizontal;

        QRect groove = squaredRect(subControlRect(CC_Dial,opt,SC_DialGroove,widget));
        o.rect = groove;
        renderInterior(p,cs2b(cs.bg,pal.brush(brole)),o.rect,fs,is,is.element+"-"+st,dir,orn);

        // TODO make configurable
        qreal startAngle = -60;
        qreal endAngle = 230;

        // tick marks
        if ( opt->subControls & QStyle::SC_DialTickmarks ) {
          o.state &= ~(State_MouseOver | State_Sunken);
          st = state_str(o.state,widget);

          // clip tickmarks in the startAngle..endAngle pie
//           QPainterPath clip;
//           clip.addPolygon(QPolygonF()
//             << groove.center()
//             << QPointF(x,y+h/2+qAbs(qSin(qDegreesToRadians(startAngle)))*w/2)
//             << QPointF(x,y)
//             << QPointF(x+w-1,y)
//             << QPointF(x+w-1,y+h/2+qAbs(qSin(qDegreesToRadians(endAngle)))*w/2)
//             << groove.center()
//           );
//           qDebug() << "sin start" << qSin(qDegreesToRadians(startAngle));
//           qDebug() << "sin end" << qSin(qDegreesToRadians(endAngle));
//           qDebug() << clip;
//           p->drawPath(clip);
//           p->save();
//           p->setClipPath(clip);
          renderInterior(p,cs2b(cs.bg,pal.brush(brole)),o.rect,fs,is,is.element+"-ticks-"+st,dir,orn);
//           p->restore();
        }

        // handle
        const qreal range = endAngle-startAngle;
        qreal pos = sliderPositionFromValue(opt->minimum,opt->maximum,
                                            opt->sliderValue, range,
                                            !opt->upsideDown);
        qreal angle = startAngle+pos;
         if ( dir == Qt::LeftToRight )
           angle = 180+startAngle+pos; // 0° at 9 o'clock
         else
           angle = 180-(startAngle+pos);  // we want CCW rotations in RTL

        o.rect = QRect(-groove.width()/2,-groove.height()/2,
                       groove.width(),groove.height());
        o.state = opt->state;
        st = state_str(o.state,widget);

        p->save();
        p->translate(QPoint(groove.center().x(),groove.center().y()));
        p->rotate(angle);
        renderInterior(p,cs2b(cs.bg,pal.brush(brole)),o.rect,fs,is,is.element+"-handle-"+st,dir,orn);
        p->restore();
      }

      break;
    }

    case CC_GroupBox : {
      if ( const QStyleOptionGroupBox *opt =
           qstyleoption_cast<const QStyleOptionGroupBox *>(option) ) {

        QStyleOptionGroupBox o(*opt);

        QRect r1,r2;

        r1 = subControlRect(CC_GroupBox,&o,SC_GroupBoxFrame,widget);
        r2 = subControlRect(CC_GroupBox,&o,SC_GroupBoxLabel,widget);

        // Draw frame and interior around contents
        renderFrame(p,cs2b(cs.bg,pal.brush(brole)),r1,fs,fs.element+"-"+st,dir);
        renderInterior(p,cs2b(cs.bg,pal.brush(brole)),r1,fs,is,is.element+"-"+st,dir);

        // Draw frame and interior around title
        fs.hasCapsule = true;
        fs.capsuleH = 2;
        fs.capsuleV = -1; // FIXME bottom titles
        renderFrame(p,cs2b(cs.bg,pal.brush(brole)),r2,fs,fs.element+"-"+st,dir);
        renderInterior(p,cs2b(cs.bg,pal.brush(brole)),r2,fs,is,is.element+"-"+st,dir);

        // Draw title
        fs.hasCapsule = false;
        r2 = subControlRect(CC_GroupBox,&o,SC_GroupBoxLabel,widget);
        if ( opt->subControls & SC_GroupBoxCheckBox ) {
          if ( dir == Qt::LeftToRight )
            renderLabel(p,cs2b(cs.fg,pal.text()),
                        dir,
                        r2.adjusted(pixelMetric(PM_IndicatorWidth)+pixelMetric(PM_CheckBoxLabelSpacing),0,0,0),
                        fs,is,ls,opt->textAlignment | Qt::TextShowMnemonic,
                        opt->text);
          else
            renderLabel(p,cs2b(cs.fg,pal.text()),
                        dir,
                        r2.adjusted(0,0,-pixelMetric(PM_IndicatorWidth)-pixelMetric(PM_CheckBoxLabelSpacing),0),
                        fs,is,ls,opt->textAlignment | Qt::TextShowMnemonic,
                        opt->text);
          o.rect= subControlRect(CC_GroupBox,opt,SC_GroupBoxCheckBox,widget);
          drawPrimitive(PE_IndicatorCheckBox,&o,p,NULL);
        } else
          renderLabel(p,cs2b(cs.fg,pal.text()),
                      dir,r2,fs,is,ls,
                      opt->textAlignment | Qt::TextShowMnemonic,
                      opt->text);

        if ( focus ) {
          QStyleOptionFocusRect fropt;
          fropt.QStyleOption::operator=(*opt);
          fropt.rect = r2;
          drawPrimitive(PE_FrameFocusRect, &fropt,p,widget);
        }
      }
      break;
    }

    default :
      //qDebug() << "[QSvgStyle] " << __func__ << ": Unhandled complex control " << control;
      QCommonStyle::drawComplexControl(control,option,p,widget);
  }

end:
  emit(sig_drawComplexControl_end(CC_str(control)));
}

int QSvgThemableStyle::pixelMetric(PixelMetric metric, const QStyleOption * option, const QWidget * widget) const
{
  switch (metric) {
    // Indicator width (checkboxes, radios, arrows, ...)
    case PM_IndicatorWidth :
    case PM_IndicatorHeight :
    case PM_ExclusiveIndicatorWidth :
    case PM_ExclusiveIndicatorHeight :
      return getSpecificValue("specific.radiocheckbox.indicator.size").toInt();

    // drop down menu + spin box up/down/plus/minus
    case PM_MenuButtonIndicator :
      return getSpecificValue("specific.dropdown.size").toInt();

    // Custom layout margins
    case PM_LayoutLeftMargin :
      return getSpecificValue("specific.layoutmargins.left").toInt();
    case PM_LayoutRightMargin :
      return getSpecificValue("specific.layoutmargins.right").toInt();
    case PM_LayoutTopMargin :
      return getSpecificValue("specific.layoutmargins.top").toInt();
    case PM_LayoutBottomMargin :
      return getSpecificValue("specific.layoutmargins.bottom").toInt();
    case PM_LayoutHorizontalSpacing :
      return getSpecificValue("specific.layoutmargins.hspace").toInt();
    case PM_LayoutVerticalSpacing :
      return getSpecificValue("specific.layoutmargins.vspace").toInt();

    case PM_MenuBarPanelWidth :
      return getFrameSpec(PE_group(PE_PanelMenuBar)).width;

    // These are the 'interior' margins of the menu bar
    case PM_MenuBarVMargin : return 0;
    case PM_MenuBarHMargin :
      return getSpecificValue("specific.menubar.hspace").toInt();
    // Spacing between menu bar items
    case PM_MenuBarItemSpacing :
      return getSpecificValue("specific.menubar.space").toInt();

    // Popup menu tear off height
    case PM_MenuTearoffHeight :
      return getSpecificValue("specific.menu.tearoff.height").toInt();

    case PM_ToolBarFrameWidth :
      return getFrameSpec(PE_group(PE_PanelToolBar)).width;
    // Margin between toolbar frame and buttons
    case PM_ToolBarItemMargin :
      return getSpecificValue("specific.toolbar.itemmargin").toInt();
    // The "move" handle of a toolbar
    case PM_ToolBarHandleExtent :
      return getSpecificValue("specific.toolbar.handle.width").toInt();
    // Item separator size
    case PM_ToolBarSeparatorExtent :
      return getSpecificValue("specific.toolbar.separator.width").toInt();
    // No spacing between items
    case PM_ToolBarItemSpacing :
      return getSpecificValue("specific.toolbar.space").toInt();
    // The "extension" button size on partial toolbars
    case PM_ToolBarExtensionExtent :
      return getSpecificValue("specific.toolbar.extension.width").toInt();
    case PM_ToolBarIconSize :
      return getSpecificValue("specific.toolbar.icon.size").toInt();

    case PM_TabBarTabHSpace : return 0;
    case PM_TabBarTabVSpace : return 0;
    case PM_TabBarScrollButtonWidth : return 20;
    case PM_TabBarBaseHeight : return 0;
    case PM_TabBarBaseOverlap : return 0;
    case PM_TabBarTabShiftHorizontal : return 0;
    case PM_TabBarTabShiftVertical : return 0;
    case PM_TabBarIconSize : return 16;

    case PM_SmallIconSize : return 16;
    case PM_LargeIconSize : return 32;

    case PM_FocusFrameHMargin :
    case PM_FocusFrameVMargin : return 0;

    case PM_CheckBoxLabelSpacing :
    case PM_RadioButtonLabelSpacing :
      return getSpecificValue("specific.radiocheckbox.label.tispace").toInt();

    case PM_SplitterWidth : return 6;

    case PM_ScrollBarExtent :
      return getSpecificValue("specific.scrollbar.thickness").toInt();
    case PM_ScrollBarSliderMin :
      return getSpecificValue("specific.scrollbar.slider.minsize").toInt();

    case PM_SliderThickness :
      return getSpecificValue("specific.slider.thickness").toInt();
    case PM_SliderLength :
    case PM_SliderControlThickness :
      return getSpecificValue("specific.slider.cursor.size").toInt();
    case PM_SliderTickmarkOffset :
      return getSpecificValue("specific.slider.ticks.offset").toInt();
    case PM_SliderSpaceAvailable:
      if (const QStyleOptionSlider *opt =
          qstyleoption_cast<const QStyleOptionSlider *>(option)) {
          if ( opt->orientation == Qt::Horizontal )
              return opt->rect.width()-pixelMetric(PM_SliderLength, opt, widget);
          else
              return opt->rect.height()-pixelMetric(PM_SliderLength, opt, widget);
      } else {
          return 0;
      }
      break;

    case PM_ProgressBarChunkWidth :
      return getSpecificValue("specific.progressbar.chunk.width").toInt();

    case PM_DefaultFrameWidth :
      // NOTE used by QLineEdit, QTabWidget and QMdiArea
      if ( qobject_cast< const QLineEdit* >(widget) )
        return getFrameSpec(PE_group(PE_FrameLineEdit)).width;
      else if ( qobject_cast< const QTabWidget* >(widget) )
        return getFrameSpec(PE_group(PE_FrameTabWidget)).width;
      else
        return getFrameSpec(PE_group(PE_Frame)).width;

    case PM_MenuPanelWidth :
      return getFrameSpec(PE_group(PE_FrameMenu)).width;

    case PM_ToolTipLabelFrameWidth :
      return getFrameSpec(PE_group(PE_PanelTipLabel)).width;

    case PM_DockWidgetTitleMargin : {
      // NOTE used by QDockWidgetLayout to compute title size
      int ret = 0;
      ret += getLabelSpec(CE_group(CE_DockWidgetTitle)).margin;
      // TODO make configurable whether title has frame in non floating docks
      if ( const QDockWidget *w =
        qobject_cast<const QDockWidget *>(widget)) {
        Q_UNUSED(w);
        //if ( w->isFloating() )
          ret += getFrameSpec(CE_group(CE_DockWidgetTitle)).width;
      }
      return ret;
    }
    break;
    case PM_DockWidgetFrameWidth :
      /// NOTE Only used for floatable docks
      return getFrameSpec(PE_group(PE_FrameDockWidget)).width;
    case PM_DockWidgetTitleBarButtonMargin : {
      // NOTE Dock widget button "margins" are used in their sizeHint() calculation
      // CHEAT: they must include both the label margins and the frame size
      // since their sizeHint() does not take into account frame size
      int ret = 0;
      if ( styleHint(QStyle::SH_DockWidget_ButtonsHaveFrame, 0, widget) )
        ret += getFrameSpec(CC_group(CC_ToolButton)).width;
      ret += getLabelSpec(CC_group(CC_ToolButton)).margin;
      ret *= 2; // HACK why ? sizeHint() already has it
      return ret;
    }
    break;

    case PM_TextCursorWidth : return 1;
    case PM_SizeGripSize :
      return 15;

    default : return QCommonStyle::pixelMetric(metric,option,widget);
  }
}

int QSvgThemableStyle::styleHint(StyleHint hint, const QStyleOption * option, const QWidget * widget, QStyleHintReturn * returnData) const
{
  switch (hint) {
    case SH_ComboBox_ListMouseTracking :
    case SH_Menu_MouseTracking :
    case SH_MenuBar_MouseTracking : return true;

    case SH_DockWidget_ButtonsHaveFrame : return true;

    case SH_TabBar_Alignment : return Qt::AlignCenter;

    default : return QCommonStyle::styleHint(hint,option,widget,returnData);
  }
}

QSize QSvgThemableStyle::sizeFromContents ( ContentsType type, const QStyleOption * option, const QSize & csz, const QWidget * widget) const
{
  if (!option)
    return csz;

  emit(sig_sizeFromContents_begin(CT_str(type)));

  // result
  QSize s;

  // Save some values into shorter variable names
  int x,y,w,h;
  option->rect.getRect(&x,&y,&w,&h);
  int csw = csz.width();
  int csh = csz.height();
  QFontMetrics fm = option->fontMetrics;

  // Get configuration group used to compute size
  QString g = CT_group(type);

  // Configuration for group g
  frame_spec_t fs;
  interior_spec_t is;
  label_spec_t ls;
  indicator_spec_t ds;

  if ( g.isEmpty() ) {
    // element not currently supported by QSvgStyle
    // QSvgStyle does not know how to compute the size of this type
    s = QCommonStyle::sizeFromContents(type,option,csz,widget);
    goto end;
  }

  // Get configuration for group
  fs = getFrameSpec(g);
  is = getInteriorSpec(g);
  ls = getLabelSpec(g);
  ds = getIndicatorSpec(g);

  switch (type) {
    case CT_LineEdit : {
      s = csz;
      if ( const QStyleOptionFrame *opt =
           qstyleoption_cast<const QStyleOptionFrame *>(option) ) {
        // add frame size
        if ( opt->lineWidth  ) // that's how QLineEdit tells us there is a frame
          s += QSize(fs.top+fs.bottom,fs.left+fs.right);

        // the text of a line edit is considered to be a label, so
        // add h/v label margins
        s += QSize(2*ls.hmargin,2*ls.vmargin);
      }
      break;
    }

    case CT_SpinBox : {
      if ( const QStyleOptionSpinBox *opt =
           qstyleoption_cast<const QStyleOptionSpinBox *>(option) ) {
        const QSpinBox *w = qobject_cast<const QSpinBox *>(widget);
        // FIXME: handle specialValueText()

        if ( !opt->frame )
          fs.hasFrame = false;

        s = sizeFromContents(fm,fs,is,ls,
                             QString("%1%2%3").arg(w ? w->prefix(): "").arg(w ? w->maximum() : 99).arg(w? w->suffix() : ""),
                             QPixmap());

        s = s.expandedTo(csz);

        s += QSize(4,0); // QLineEdit hard-coded margins
        if ( opt->buttonSymbols != QAbstractSpinBox::NoButtons ) {
          s += QSize(2*pixelMetric(PM_MenuButtonIndicator),0); // buttons
        }
        if ( !opt->frame)
          s = s.expandedTo(QSize(0,pixelMetric(PM_MenuButtonIndicator))); // minimum height
        else
          s = s.expandedTo(QSize(fs.left+fs.right,
                                 pixelMetric(PM_MenuButtonIndicator)+fs.top+fs.bottom));
      }

      break;
    }

    case CT_ComboBox : {
      if ( const QStyleOptionComboBox *opt =
           qstyleoption_cast<const QStyleOptionComboBox *>(option) ) {

        s = csz;
        if ( !opt->frame )
          fs.hasFrame = false;

        s = sizeFromContents(fm,fs,is,ls,
                             "W",
                             QPixmap(QSize(pixelMetric(PM_SmallIconSize),pixelMetric(PM_SmallIconSize))));

        s = s.expandedTo(QSize(csz.width(),0));

        if ( opt->editable )
          s += QSize(4,0); // QLineEdit hard-coded margins
        s += QSize(pixelMetric(PM_MenuButtonIndicator),0); // drop down button;

        s += QSize(8,0); // QComboBox missing in csz ?

        if ( !opt->frame )
          s = s.expandedTo(QSize(0,pixelMetric(PM_MenuButtonIndicator))); // minimum height
        else
          s = s.expandedTo(QSize(fs.left+fs.right,
                                 pixelMetric(PM_MenuButtonIndicator)+fs.top+fs.bottom));
      }

      break;
    }

    case CT_PushButton : {
      if ( const QStyleOptionButton *opt =
           qstyleoption_cast<const QStyleOptionButton *>(option) ) {

        s = sizeFromContents(fm,fs,is,ls,
                             (opt->text.isEmpty() && opt->icon.isNull()) ? "W"
                               : opt->text,
                             opt->icon.pixmap(opt->iconSize));

        if ( opt->features & QStyleOptionButton::HasMenu ) {
          s.rwidth() += 2*ls.tispace+ds.size;
        }
      }
      break;
    }

    case CT_CheckBox :  {
      if ( const QStyleOptionButton *opt =
           qstyleoption_cast<const QStyleOptionButton *>(option) ) {

        s = sizeFromContents(fm,fs,is,ls,
                             opt->text,
                             opt->icon.pixmap(opt->iconSize));
        s += QSize(pixelMetric(PM_CheckBoxLabelSpacing)+pixelMetric(PM_IndicatorWidth),0);
        s = s.expandedTo(QSize(pixelMetric(PM_IndicatorWidth),pixelMetric(PM_IndicatorHeight))); // minimal checkbox size is size of indicator
      }
      break;
    }

    case CT_RadioButton : {
      if ( const QStyleOptionButton *opt =
           qstyleoption_cast<const QStyleOptionButton *>(option) ) {

        s = sizeFromContents(fm,fs,is,ls,
                             opt->text,
                             opt->icon.pixmap(opt->iconSize));
        s += QSize(pixelMetric(PM_CheckBoxLabelSpacing)+pixelMetric(PM_ExclusiveIndicatorWidth),0);
        s = s.expandedTo(QSize(pixelMetric(PM_ExclusiveIndicatorWidth),pixelMetric(PM_ExclusiveIndicatorHeight))); // minimal checkbox size is size of indicator
      }
      break;
    }

    case CT_MenuItem : {
      if ( const QStyleOptionMenuItem *opt =
           qstyleoption_cast<const QStyleOptionMenuItem *>(option) ) {

        // NOTE Cheat here: the frame is for the whole menu,
        // not the individual menu items
        fs.hasFrame = false;
        fs.top = fs.bottom = fs.left = fs.right = 0;

        if (opt->menuItemType == QStyleOptionMenuItem::Separator)
          s = QSize(csw,2); /* there is no PM_MenuSeparatorHeight pixel metric */
        else {
          s = sizeFromContents(fm,fs,is,ls,opt->text,opt->icon.pixmap(opt->maxIconWidth));
        }

        // No icon ? add icon width nevertheless
        if (opt->icon.pixmap(opt->maxIconWidth).isNull())
          s.rwidth() += ls.tispace+opt->maxIconWidth;

        // add width for check mark
        if ( (opt->checkType == QStyleOptionMenuItem::Exclusive) ||
             (opt->checkType == QStyleOptionMenuItem::NonExclusive)
            ) {
          s.rwidth() += pixelMetric(PM_CheckBoxLabelSpacing)+pixelMetric(PM_IndicatorWidth);
        }

        // add width for sub menu arrow
        if ( opt->menuItemType == QStyleOptionMenuItem::SubMenu ) {
          s.rwidth() += ls.tispace+ds.size;
        }
      }

      break;
    }

    case CT_MenuBarItem : {
      if ( const QStyleOptionMenuItem *opt =
           qstyleoption_cast<const QStyleOptionMenuItem *>(option) ) {

        // NOTE Cheat here: frame is for whole menu bar, not
        // individual menu bar items
        fs.hasFrame = false;
        fs.left = fs.right = fs.top = fs.bottom = 0;

        s = sizeFromContents(fm,fs,is,ls,
                             opt->text,
                             opt->icon.pixmap(opt->maxIconWidth));
      }

      break;
    }

    case CT_ProgressBar : {
      if ( const QStyleOptionProgressBar *opt =
           qstyleoption_cast<const QStyleOptionProgressBar *>(option) )  {

        s = sizeFromContents(fm,fs,is,ls,
                             opt->textVisible ?
                               opt->text.isEmpty() ? "W" : opt->text
                               : QString::null,
                             QPixmap());

        if ( opt->orientation == Qt::Vertical )
          s.transpose();
      }

      break;
    }

    case CT_ToolButton : {
      if ( const QStyleOptionToolButton *opt =
           qstyleoption_cast<const QStyleOptionToolButton *>(option) ) {

        // fix indicator size
        ds = getIndicatorSpec(PE_group(PE_IndicatorArrowDown));

        QStyleOptionToolButton o(*opt);

        // minimum size
        QSize ms;
        if ( opt->text.isEmpty() && (opt->toolButtonStyle == Qt::ToolButtonTextOnly) )
          ms = sizeFromContents(fm,fs,is,ls,
                                "W",
                                opt->icon.pixmap(opt->iconSize),
                                Qt::ToolButtonTextOnly);
        if ( opt->icon.isNull() && (opt->toolButtonStyle == Qt::ToolButtonIconOnly) )
          ms = sizeFromContents(fm,fs,is,ls,
                                "W",
                                opt->icon.pixmap(opt->iconSize),
                                Qt::ToolButtonTextOnly);
        if ( opt->text.isEmpty() && opt->icon.isNull() )
          ms = sizeFromContents(fm,fs,is,ls,
                                "W",
                                opt->icon.pixmap(opt->iconSize),
                                Qt::ToolButtonTextOnly);

        s = sizeFromContents(fm,fs,is,ls,
                             opt->text,
                             opt->icon.pixmap(opt->iconSize),
                             opt->toolButtonStyle).expandedTo(ms);

        if (opt->arrowType != Qt::NoArrow) {
          // add room for arrow
          s = s + QSize(ds.size,0);
          // add spacing between arrow and label if necessary
          if ( (opt->toolButtonStyle != Qt::ToolButtonIconOnly) &&
               !opt->text.isEmpty() )
            s = s + QSize(ls.tispace,0);
          else if ( (opt->toolButtonStyle != Qt::ToolButtonTextOnly) &&
               !opt->icon.isNull() )
            s = s + QSize(ls.tispace,0);
        }

        // add room for simple down arrow or drop down arrow
        if (opt->features & QStyleOptionToolButton::Menu) {
          // Tool button with drop down button
          s.rwidth() += pixelMetric(PM_MenuButtonIndicator);
        } else if (opt->features & QStyleOptionToolButton::HasMenu) {
          // Tool button with down arrow
          s.rwidth() += 2*ls.tispace+ds.size;
        }
      }
      break;
    }

    case CT_TabBarTab : {
      if ( const QStyleOptionTab *opt =
           qstyleoption_cast<const QStyleOptionTab *>(option) ) {

        s = sizeFromContents(fm,fs,is,ls,
                             opt->text,
                             opt->icon.pixmap(opt->iconSize));

        if ( qobject_cast< const QTabBar* >(widget)->tabsClosable() ) {
            s.rwidth() += ls.tispace+opt->iconSize.width();
        }

        if ( opt->shape == QTabBar::TriangularEast ||
             opt->shape == QTabBar::TriangularWest ||
             opt->shape == QTabBar::RoundedEast ||
             opt->shape == QTabBar::RoundedWest ) {
          s.transpose();
        }
      }

      break;
    }

    case CT_HeaderSection : {
      if ( const QStyleOptionHeader *opt =
           qstyleoption_cast<const QStyleOptionHeader *>(option) ) {

        s = sizeFromContents(fm,fs,is,ls,
                             opt->text,opt->icon.pixmap(pixelMetric(PM_SmallIconSize)));
        if ( opt->sortIndicator != QStyleOptionHeader::None )
          s += QSize(ls.tispace+ds.size,0);
      }

      break;
    }

    case CT_Slider : {
      if (option->state & State_Horizontal)
        s = QSize(csw,pixelMetric(PM_SliderControlThickness,option,widget)+1);
      else
        s = QSize(pixelMetric(PM_SliderLength,option,widget)+1,csh);

      break;
    }

    case CT_GroupBox : {
      if ( const QStyleOptionGroupBox *opt =
           qstyleoption_cast<const QStyleOptionGroupBox *>(option) ) {

        if ( opt->subControls & SC_GroupBoxCheckBox )
          s = sizeFromContents(fm,fs,is,ls,opt->text,QPixmap())+QSize(pixelMetric(PM_CheckBoxLabelSpacing)+pixelMetric(PM_IndicatorWidth),0);
        else
          s = sizeFromContents(fm,fs,is,ls,opt->text,QPixmap());

        // add contents to st, 30 is title shift (left and right)
        s = QSize(qMax(s.width()+30+30,csz.width()+fs.left+fs.right),
                  csz.height()+s.height()+fs.top+fs.bottom);
      }

      break;
    }

    case CT_ItemViewItem :  {
      if ( const QStyleOptionViewItem *opt =
           qstyleoption_cast<const QStyleOptionViewItem *>(option) ) {

        s = sizeFromContents(fm,fs,is,ls,
                             opt->text,
                             opt->icon.pixmap(pixelMetric(PM_SmallIconSize)));

        if ( opt->features & QStyleOptionViewItem::HasCheckIndicator ) {
          s += QSize(pixelMetric(PM_CheckBoxLabelSpacing)+pixelMetric(PM_IndicatorWidth),0);
        }
        s = s.expandedTo(QSize(pixelMetric(PM_IndicatorWidth),pixelMetric(PM_IndicatorHeight))); // minimal checkbox size is size of indicator
      }
      break;
    }

    default : s = QCommonStyle::sizeFromContents(type,option,csz,widget);
  }

#ifdef __DEBUG__
//   if (widget){
//     QWidget *w = (QWidget *)widget;
//     w->setToolTip(QString("%1\n<b>sizeFromContents()</b>:%2,%3\n").arg(w->toolTip()).arg(s.width()).arg(s.height()));
//   }
#endif

end:
  emit(sig_sizeFromContents_end(CT_str(type)));
  return s;
}

QSize QSvgThemableStyle::sizeFromContents(const QFontMetrics &fm,
                     /* frame spec */ const frame_spec_t &fs,
                     /* interior spec */ const interior_spec_t &is,
                     /* label spec */ const label_spec_t &ls,
                     /* text */ const QString &text,
                     /* icon */ const QPixmap &icon,
                     /* text-icon alignment */ const Qt::ToolButtonStyle tialign) const
{
  Q_UNUSED(is);

  QSize s(0,0);
  if ( fs.hasFrame ) {
    s.rwidth() += fs.left+fs.right;
    s.rheight() += fs.top+fs.bottom;
  }

  int th = 0, tw = 0;
  if ( !text.isNull() || !icon.isNull() ) {
    s.rwidth() += 2*ls.hmargin;
    s.rheight() += 2*ls.vmargin;
  }

  if ( !text.isNull() ) {
    if (ls.hasShadow) {
      s.rwidth() += ls.xshift+ls.depth;
      s.rheight() += ls.yshift+ls.depth;
    }

    // compute width and height of text
    QSize ts = text.isNull() ? QSize(0,0) : fm.size(Qt::TextShowMnemonic,text);
    tw = ts.width();
    th = ts.height();
  }

  if (tialign == Qt::ToolButtonIconOnly) {
    s.rwidth() += icon.width();
    s.rheight() += icon.height();
  } else if (tialign == Qt::ToolButtonTextOnly) {
    s.rwidth() += tw;
    s.rheight() += th;
  } else if (tialign == Qt::ToolButtonTextBesideIcon) {
    s.rwidth() += (icon.isNull() ? 0 : icon.width()) + (icon.isNull() ? 0 : (text.isEmpty() ? 0 : ls.tispace)) + tw;
    s.rheight() += qMax(icon.height(),th);
  } else if (tialign == Qt::ToolButtonTextUnderIcon) {
    s.rwidth() += qMax(icon.width(),tw);
    s.rheight() += icon.height() + (icon.isNull() ? 0 : ls.tispace) + th;
  }

  // minimum size : frame + 2 pixels of interior
  if ( fs.hasFrame )
    s = s.expandedTo(QSize(fs.top+fs.bottom+2,fs.left+fs.right+2));
  else
    s = s.expandedTo(QSize(2,2));

  return s;
}

QRect QSvgThemableStyle::subElementRect(SubElement e, const QStyleOption * option, const QWidget * widget) const
{
  // result
  QRect ret = option->rect;

  // Copy some values into shorter variable names
  int x,y,w,h;
  QRect r = option->rect;
  r.getRect(&x,&y,&w,&h);
  QString st = state_str(option->state, widget);
  Qt::LayoutDirection dir = option->direction;
  bool focus = option->state & State_HasFocus;

  Q_UNUSED(dir);
  Q_UNUSED(focus);

  // Get QSvgStyle configuration group used to render this element
  QString g = SE_group(e);

  // Configuration for group g
  frame_spec_t fs;
  interior_spec_t is;
  label_spec_t ls;
  indicator_spec_t ds;

  if ( g.isEmpty() ) {
    // element not currently supported by QSvgStyle
    // subElementRect() already returns visual rects
    return QCommonStyle::subElementRect(e,option,widget);
  }

  // Get configuration for group
  fs = getFrameSpec(g);
  is = getInteriorSpec(g);
  ls = getLabelSpec(g);
  ds = getIndicatorSpec(g);

  switch (e) {
    case SE_PushButtonFocusRect : {
      ret = r;
      break;
    }
    case SE_ProgressBarGroove :
    case SE_ProgressBarContents :
    case SE_ProgressBarLabel: {
      ret = r;
      break;
    }
    case SE_LineEditContents : {
      if ( const QStyleOptionFrame *opt =
           qstyleoption_cast<const QStyleOptionFrame *>(option) ) {
        if ( opt->lineWidth <= 0 )
          fs.hasFrame = false;
      }
      ret = interiorRect(r, fs,is);
      break;
    }
    case SE_PushButtonContents : {
      ret = interiorRect(r,fs,is);
      break;
    }
    case SE_HeaderLabel : {
      if ( const QStyleOptionHeader *opt =
           qstyleoption_cast<const QStyleOptionHeader *>(option) ) {

        if ( opt->sortIndicator != QStyleOptionHeader::None )
          ret = r.adjusted(0,0,-ls.tispace-ds.size,0);
        else
          ret = r;
      }
      break;
    }
    case SE_HeaderArrow : {
      ret = labelRect(r,fs,is,ls);
      ret = ret.adjusted(ret.width()-ds.size-ls.tispace,0,0,0);
      break;
    }
    case SE_CheckBoxFocusRect : {
      ret = subElementRect(SE_CheckBoxContents,option,widget);
      break;
    }
    case SE_RadioButtonFocusRect : {
      ret = subElementRect(SE_RadioButtonContents,option,widget);
      break;
    }

    case SE_TabBarTabText : {
      ret = labelRect(r,fs,is,ls);
    }

    default :
      // subElementRect() already returns visual rects
      return QCommonStyle::subElementRect(e,option,widget);
  }

  return visualRect(dir,r,ret);
}

QRect QSvgThemableStyle::subControlRect(ComplexControl control, const QStyleOptionComplex * option, SubControl subControl, const QWidget * widget) const
{
  // result
  QRect ret = option->rect;

  // Copy some values into shorter variable names
  int x,y,w,h;
  QRect r = option->rect;
  r.getRect(&x,&y,&w,&h);
  QString st = state_str(option->state, widget);
  Qt::LayoutDirection dir = option->direction;
  bool focus = option->state & State_HasFocus;
  QIcon::Mode icm = state_iconmode(option->state);
  QIcon::State ics = state_iconstate(option->state);
  QFontMetrics fm = option->fontMetrics;

  Q_UNUSED(focus);
  Q_UNUSED(icm);
  Q_UNUSED(ics);

  // Get QSvgStyle configuration group used to render this element
  QString g = CC_group(control);

  // Configuration for group g
  frame_spec_t fs;
  interior_spec_t is;
  label_spec_t ls;
  indicator_spec_t ds;

  if ( g.isEmpty() ) {
    // element not currently supported by QSvgStyle
    // subControlRect() already returns visual rects
    return QCommonStyle::subControlRect(control,option,subControl,widget);
  }

  // Get configuration for group
  fs = getFrameSpec(g);
  is = getInteriorSpec(g);
  ls = getLabelSpec(g);
  ds = getIndicatorSpec(g);

  switch (control) {
    case CC_SpinBox : {
      // OK
      const QStyleOptionSpinBox *opt =
        qstyleoption_cast<const QStyleOptionSpinBox *>(option);

      int variant = getSpecificValue("specific.spinbox.variant").toInt();

      if ( !opt->frame )
        fs.hasFrame = false;

      switch (subControl) {
        case SC_SpinBoxFrame :
          ret = r;
          break;
        case SC_SpinBoxEditField :
          r = interiorRect(r,fs,is);
          r.getRect(&x,&y,&w,&h);

          if ( opt->buttonSymbols == QAbstractSpinBox::NoButtons )
            ret = r;
          else {
            switch(variant) {
              case VA_SPINBOX_BUTTONS_SIDEBYSIDE :
                ret = r.adjusted(0,0,-2*pixelMetric(PM_MenuButtonIndicator),0);
                break;
              case VA_SPINBOX_BUTTONS_OPPOSITE :
                ret = r.adjusted(pixelMetric(PM_MenuButtonIndicator),0,
                                 -pixelMetric(PM_MenuButtonIndicator),0);
                break;
              default:
                break;
            }
          }
          break;
        case SC_SpinBoxUp :
            r = interiorRect(option->rect, fs,is);
            r.getRect(&x,&y,&w,&h);
            switch(variant) {
              case VA_SPINBOX_BUTTONS_SIDEBYSIDE :
                ret = QRect(x+w-pixelMetric(PM_MenuButtonIndicator),
                        y,pixelMetric(PM_MenuButtonIndicator),h);
                break;
              case VA_SPINBOX_BUTTONS_OPPOSITE :
                ret = QRect(x+w-pixelMetric(PM_MenuButtonIndicator),
                        y,pixelMetric(PM_MenuButtonIndicator),h);
                break;
              default:
                break;
            }
          break;
        case SC_SpinBoxDown :
            r = interiorRect(option->rect, fs,is);
            r.getRect(&x,&y,&w,&h);
            switch(variant) {
              case VA_SPINBOX_BUTTONS_SIDEBYSIDE :
                ret = QRect(x+w-2*pixelMetric(PM_MenuButtonIndicator),y,
                        pixelMetric(PM_MenuButtonIndicator),h);
                break;
              case VA_SPINBOX_BUTTONS_OPPOSITE :
                ret = QRect(x,y,pixelMetric(PM_MenuButtonIndicator),h);
                break;
              default:
                break;
            }
          break;
        default :
          ret = QCommonStyle::subControlRect(control,option,subControl,widget);
      }
      break;
    }

    case CC_ComboBox : {
      // OK
      const QStyleOptionComboBox *opt =
        qstyleoption_cast<const QStyleOptionComboBox *>(option);
      if ( !opt->frame )
        fs.hasFrame = false;

      switch (subControl) {
        case SC_ComboBoxFrame :
          ret = r;
          break;
        case SC_ComboBoxEditField :
          r = interiorRect(r,fs,is);
          r.getRect(&x,&y,&w,&h);
          ret = r.adjusted(0,0,-pixelMetric(PM_MenuButtonIndicator),0);
          break;
        case SC_ComboBoxArrow :
          r = interiorRect(r,fs,is);
          r.getRect(&x,&y,&w,&h);
          ret = QRect(x+w-pixelMetric(PM_MenuButtonIndicator),y,
                      pixelMetric(PM_MenuButtonIndicator),h);
          break;
        case SC_ComboBoxListBoxPopup :
          ret = r;
          break;
        default :
          ret = QCommonStyle::subControlRect(control,option,subControl,widget);
      }
      break;
    }

    case CC_ScrollBar : {
      // OK
      const int extent = pixelMetric(PM_ScrollBarExtent,option,widget);
      if (option->state & State_Horizontal)
        switch (subControl) {
          case SC_ScrollBarGroove :
            ret = QRect(x+extent,y,w-2*extent,h);
            break;
          case SC_ScrollBarSubLine :
            ret = QRect(x,y,extent,extent);
            break;
          case SC_ScrollBarAddLine :
            ret = QRect(x+w-extent,y,extent,extent);
            break;
          case SC_ScrollBarSlider : {
            if ( const QStyleOptionSlider *opt =
                 qstyleoption_cast<const QStyleOptionSlider *>(option) ) {

              r = subControlRect(CC_ScrollBar,option,SC_ScrollBarGroove,widget);
              r = interiorRect(r,fs,is);
              r.getRect(&x,&y,&w,&h);

              const int minLength = pixelMetric(PM_ScrollBarSliderMin,option,widget);
              const int maxLength = w; // max slider length
              const int valueRange = opt->maximum - opt->minimum;
              int length = maxLength;
              if (opt->minimum != opt->maximum) {
                length = (opt->pageStep*maxLength) / (valueRange+opt->pageStep);

                if ( (length < minLength) || (valueRange > INT_MAX/2) )
                  length = minLength;
                if (length > maxLength)
                  length = maxLength;
              }

              const int start = sliderPositionFromValue(opt->minimum,
                                                        opt->maximum,
                                                        opt->sliderPosition,
                                                        maxLength - length,
                                                        opt->upsideDown);
              ret = QRect(x+start,y,length,h);
            }
            break;
          }

          default : ret = QCommonStyle::subControlRect(control,option,subControl,widget);

          break;
        }
      else
        switch (subControl) {
          case SC_ScrollBarGroove :
            ret = QRect(x,y+extent,w,h-2*extent);
            break;
          case SC_ScrollBarSubLine :
            ret = QRect(x,y,extent,extent);
            break;
          case SC_ScrollBarAddLine :
            ret = QRect(x,y+h-extent,extent,extent);
            break;
          case SC_ScrollBarSlider : {
            if ( const QStyleOptionSlider *opt =
                 qstyleoption_cast<const QStyleOptionSlider *>(option) ) {

              r = subControlRect(CC_ScrollBar,option,SC_ScrollBarGroove,widget);
              r = interiorRect(r,fs,is);
              r.getRect(&x,&y,&w,&h);

              const int minLength = pixelMetric(PM_ScrollBarSliderMin,option,widget);
              const int maxLength = h; // max slider length
              const int valueRange = opt->maximum - opt->minimum;
              int length = maxLength;
              if (opt->minimum != opt->maximum) {
                length = (opt->pageStep*maxLength) / (valueRange+opt->pageStep);

                if ( (length < minLength) || (valueRange > INT_MAX/2) )
                  length = minLength;
                if (length > maxLength)
                  length = maxLength;
              }

              const int start = sliderPositionFromValue(opt->minimum,opt->maximum,opt->sliderPosition,maxLength - length,opt->upsideDown);
              ret = QRect(x,y+start,w,length);
            } else
              ret = QRect();

            break;
          }

          default : ret = QCommonStyle::subControlRect(control,option,subControl,widget);

          break;
        }

      break;
    }

    case CC_Slider : {
      // OK
      const int avail = pixelMetric(PM_SliderSpaceAvailable,option,widget);
      const int thickness = pixelMetric(PM_SliderThickness, option, widget);
      const bool horiz = (option->state & State_Horizontal);
      switch (subControl) {
        case SC_SliderGroove :
          if (horiz)
            ret = alignedRect(option->direction, Qt::AlignCenter,
              QSize(avail,thickness), r);
          else
            ret = alignedRect(option->direction, Qt::AlignCenter,
              QSize(thickness,avail), r);
          break;
        case SC_SliderHandle : {
          if ( const QStyleOptionSlider *opt =
               qstyleoption_cast<const QStyleOptionSlider *>(option) ) {

            const int handleWidth = pixelMetric(PM_SliderLength, option, widget);
            const int handleHeight = pixelMetric(PM_SliderControlThickness, option, widget);

            subControlRect(CC_Slider,option,SC_SliderGroove,widget).getRect(&x,&y,&w,&h);


            const int sliderPos(sliderPositionFromValue(opt->minimum,
                                                        opt->maximum,
                                                        opt->sliderPosition,
                                                        horiz ? w : h,
                                                        opt->upsideDown));

            if (horiz)
              ret = QRect(x+sliderPos-handleWidth/2,y+(h-handleHeight)/2,
                          handleWidth,handleHeight);
            else
              ret = QRect(x+(w-handleHeight)/2,y+sliderPos-handleHeight/2,
                          handleHeight,handleWidth);
          }

          break;
        }

        default : ret = QCommonStyle::subControlRect(control,option,subControl,widget);
      }

      break;
    }

    case CC_ToolButton : {
      // OK
      ds = getIndicatorSpec(PE_group(PE_IndicatorArrowDown));
      switch (subControl) {
        case SC_ToolButton : {
          if ( const QStyleOptionToolButton *opt =
               qstyleoption_cast<const QStyleOptionToolButton *>(option) ) {

            // remove room for drop down buttons or down arrows
            if (opt->features & QStyleOptionToolButton::Menu)
              ret = r.adjusted(0,0,-pixelMetric(PM_MenuButtonIndicator),0);
            else if (opt->features & QStyleOptionToolButton::HasMenu)
              ret = r.adjusted(0,0,-ds.size-2*ls.tispace,0);
          }
          break;
        }
        case SC_ToolButtonMenu : {
          if ( const QStyleOptionToolButton *opt =
               qstyleoption_cast<const QStyleOptionToolButton *>(option) ) {

            if (opt->features & QStyleOptionToolButton::Menu)
              ret = r.adjusted(x+w-pixelMetric(PM_MenuButtonIndicator),0,0,0);
            else if (opt->features & QStyleOptionToolButton::HasMenu)
              ret = QRect(x+w-ls.tispace-ds.size-fs.right,y+h-ds.size-fs.bottom,ds.size,ds.size);
          }
          break;
        }
        default : ret = QCommonStyle::subControlRect(control,option,subControl,widget);
      }

      break;
    }

    case CC_GroupBox : {
      // OK
      QRect labelRect; // = text + checkbox if any
      QSize stitle; // size of title (text+checkbox if any)

      if (const QStyleOptionGroupBox *opt =
          qstyleoption_cast<const QStyleOptionGroupBox *>(option) ) {

        // title size
        stitle = sizeFromContents(fm,fs,is,ls,
                                  opt->text,QPixmap());
        if ( opt->subControls & SC_GroupBoxCheckBox ) {
          // ensure checkbox indicator fits within title interior
          stitle += QSize(pixelMetric(PM_IndicatorWidth)+pixelMetric(PM_CheckBoxLabelSpacing),0);
          stitle = stitle.expandedTo(QSize(0,fs.top+fs.bottom+pixelMetric(PM_IndicatorHeight)));
        }

        labelRect = alignedRect(Qt::LeftToRight,
                                opt->textAlignment & ~Qt::AlignVertical_Mask,
                                stitle,
                                r.adjusted(30,0,-30,0));
      }

      switch (subControl) {
      // FIXME take into account label V alignment
        case SC_GroupBoxCheckBox : {
          // align checkbox inside label rect
          ret = alignedRect(Qt::LeftToRight,Qt::AlignLeft | Qt::AlignVCenter,
                            QSize(pixelMetric(PM_IndicatorWidth),pixelMetric(PM_IndicatorHeight)),
                            labelRect.adjusted(fs.left+ls.hmargin,fs.top,-fs.right-ls.hmargin,-fs.bottom));
          break;
        }
        case SC_GroupBoxLabel : {
            // Shift for checkbox will be done be drawComplexControl()
            ret = labelRect;
            break;
        }
        case SC_GroupBoxFrame : {
          ret = r.adjusted(0,stitle.height(),0,0);
          break;
        }
        case SC_GroupBoxContents : {
          ret = interiorRect(r.adjusted(0,stitle.height(),0,0),fs,is);
          break;
        }

        default : ret = QCommonStyle::subControlRect(control,option,subControl,widget);
      }

      break;
    }

    case CC_Dial: {
      switch (subControl) {
        case SC_DialGroove : {
          ret = alignedRect(Qt::LeftToRight,Qt::AlignCenter,
                            QSize(qMin(w,h),qMin(w,h)),r);
          break;
        }
        default : ret = QCommonStyle::subControlRect(control,option,subControl,widget);
      }

      break;
    }

    default :
      // subControlRect() already returns visual rects
      return QCommonStyle::subControlRect(control,option,subControl,widget);
  }

  return visualRect(dir,r,ret);
}

#if QT_VERSION >= 0x050000
QIcon QSvgThemableStyle::standardIcon(StandardPixmap standardIcon, const QStyleOption * option, const QWidget * widget) const
#else
QIcon QSvgThemableStyle::standardIconImplementation ( QStyle::StandardPixmap standardIcon, const QStyleOption* option, const QWidget* widget ) const
#endif
{
  switch (standardIcon) {
    case SP_ToolBarHorizontalExtensionButton : {
      int s = pixelMetric(PM_ToolBarExtensionExtent);
      QPixmap pm(QSize(s,s));
      pm.fill(Qt::transparent);

      QPainter painter(&pm);

      QStyleOption opt;
      opt.rect = QRect(0,0,s,s);
      opt.state |= State_Enabled;

      if ( option ) {
        if ( option->direction == Qt::LeftToRight )
          drawPrimitive(PE_IndicatorArrowRight,&opt,&painter,0);
        else
          drawPrimitive(PE_IndicatorArrowLeft,&opt,&painter,0);
      } else {
        drawPrimitive(PE_IndicatorArrowRight,&opt,&painter,0);
      }

      return QIcon(pm);
    }
    case SP_ToolBarVerticalExtensionButton : {
      int s = pixelMetric(PM_ToolBarExtensionExtent);
      QPixmap pm(QSize(s,s));
      pm.fill(Qt::transparent);

      QPainter painter(&pm);

      QStyleOption opt;
      opt.rect = QRect(0,0,s,s);
      opt.state |= State_Enabled;

      drawPrimitive(PE_IndicatorArrowDown,&opt,&painter,0);

      return QIcon(pm);
    }
    case SP_TitleBarMinButton : {
      int s = 12;
      QPixmap pm(QSize(s,s));
      pm.fill(Qt::transparent);

      QPainter painter(&pm);

      QStyleOption opt;
      opt.rect = QRect(0,0,s,s);
      opt.state |= State_Enabled;

      renderElement(&painter,"mdi-minimize-normal",opt.rect);

      return QIcon(pm);
    }
    case SP_TitleBarMaxButton : {
      int s = 12;
      QPixmap pm(QSize(s,s));
      pm.fill(Qt::transparent);

      QPainter painter(&pm);

      QStyleOption opt;
      opt.rect = QRect(0,0,s,s);
      opt.state |= State_Enabled;

      renderElement(&painter,"mdi-maximize-normal",opt.rect);

      return QIcon(pm);
    }
    case SP_TitleBarCloseButton : {
      int s = 12;
      QPixmap pm(QSize(s,s));
      pm.fill(Qt::transparent);

      QPainter painter(&pm);

      QStyleOption opt;
      opt.rect = QRect(0,0,s,s);
      opt.state |= State_Enabled;

      renderElement(&painter,"mdi-close-normal",opt.rect);

      return QIcon(pm);
    }
    case SP_TitleBarMenuButton : {
      int s = 12;
      QPixmap pm(QSize(s,s));
      pm.fill(Qt::transparent);

      QPainter painter(&pm);

      QStyleOption opt;
      opt.rect = QRect(0,0,s,s);
      opt.state |= State_Enabled;

      renderElement(&painter,"mdi-menu-normal",opt.rect);

      return QIcon(pm);
    }
    case SP_TitleBarNormalButton : {
      int s = 12;
      QPixmap pm(QSize(s,s));
      pm.fill(Qt::transparent);

      QPainter painter(&pm);

      QStyleOption opt;
      opt.rect = QRect(0,0,s,s);
      opt.state |= State_Enabled;

      renderElement(&painter,"mdi-restore-normal",opt.rect);

      return QIcon(pm);
    }

    default :
#if QT_VERSION >= 0x050000
      return QCommonStyle::standardIcon(standardIcon,option,widget);
#else
      return QCommonStyle::standardIconImplementation(standardIcon,option,widget);
#endif
  }

#if QT_VERSION >= 0x050000
  return QCommonStyle::standardIcon(standardIcon,option,widget);
#else
  return QCommonStyle::standardIconImplementation(standardIcon,option,widget);
#endif
}

QRect QSvgThemableStyle::squaredRect(const QRect& r) const {
  int e = qMin(r.height(),r.width());
  return QRect(r.x(),r.y(),e,e);
}

void QSvgThemableStyle::renderElement(QPainter* p, const QString& element, const QRect& bounds, int hsize, int vsize) const
{
  int x,y,h,w;
  bounds.getRect(&x,&y,&w,&h);

  if ( !bounds.isValid() )
    return;

  QSvgRenderer *renderer = 0;

  if ( !themeRndr->elementExists(element) ) {
    // Missing element
    p->save();
    p->setPen(Qt::black);
    drawRealRect(p, bounds);
    p->drawLine(x,y,x+w-1,y+h-1);
    p->drawLine(x+w-1,y,x,y+h-1);
    p->restore();
    qDebug() << "element" << element << "not found in SVG file";
    return;
  }

  renderer = themeRndr;
  if (renderer) {
    if ( (hsize > 0) || (vsize > 0) ) {

      if ( (hsize > 0) && (vsize <= 0) ) {
        int hpatterns = (w/hsize)+1;

        p->save();
        p->setClipRect(QRect(x,y,w,h));
        for (int i=0; i<hpatterns; i++)
          renderer->render(p,element,QRect(x+i*hsize,y,hsize,h));
        p->restore();
      }

      if ( (hsize <= 0) && (vsize > 0) ) {
        int vpatterns = (h/vsize)+1;

        p->save();
        p->setClipRect(QRect(x,y,w,h));
        for (int i=0; i<vpatterns; i++)
          renderer->render(p,element,QRect(x,y+i*vsize,w,vsize));
        p->restore();
      }

      if ( (hsize > 0) && (vsize > 0) ) {
        int hpatterns = (w/hsize)+1;
        int vpatterns = (h/vsize)+1;

        p->save();
        p->setClipRect(bounds);
        for (int i=0; i<hpatterns; i++)
          for (int j=0; j<vpatterns; j++)
            renderer->render(p,element,QRect(x+i*hsize,y+j*vsize,hsize,vsize));
        p->restore();
      }
    } else {
      renderer->render(p,element,QRect(x,y,w,h));
    }
  }
}

void QSvgThemableStyle::computeFrameRects(const QRect& bounds,
                       const frame_spec_t& fs,
                       Orientation orn,
                       QRect& top,
                       QRect& bottom,
                       QRect& left,
                       QRect& right,
                       QRect& topleft,
                       QRect& topright,
                       QRect& bottomleft,
                       QRect& bottomright) const
{
  // drawing rect
  QRect r = bounds;

  top = bottom = left = right = topleft = topright = bottomleft = bottomright
    = QRect();

  if ( !fs.hasFrame )
    return;

  if ( orn != Horizontal ) {
    // Vertical orientation: perform calculations on "horizontalized"
    // rect
    r = transposedRect(r);
  }

  int x0,y0,x1,y1,w,h;
  r.getRect(&x0,&y0,&w,&h);
  x1 = r.bottomRight().x();
  y1 = r.bottomRight().y();

  // compute frame parts positions
  if ( !fs.hasCapsule ) {
    top         = QRect(x0+fs.left,y0,w-fs.left-fs.right,fs.top);
    bottom      = QRect(x0+fs.left,y1-fs.bottom+1,w-fs.left-fs.right,fs.bottom);
    left        = QRect(x0,y0+fs.top,fs.left,h-fs.top-fs.bottom);
    right       = QRect(x1-fs.right+1,y0+fs.top,fs.right,h-fs.top-fs.bottom);
    topleft     = QRect(x0,y0,fs.left,fs.top);
    topright    = QRect(x1-fs.right+1,y0,fs.right,fs.top);
    bottomleft  = QRect(x0,y1-fs.bottom+1,fs.left,fs.bottom);
    bottomright = QRect(x1-fs.right+1,y1-fs.bottom+1,fs.right,fs.bottom);
  } else {
    if ( (fs.capsuleH == 0) && (fs.capsuleV == 0) )
      // no frame at all (middle position)
      return;

    // adjustment
    int la = 0,ra = 0,ta = 0,ba = 0;

    if ( (fs.capsuleH == -1) || (fs.capsuleH == 2) )
      la = fs.left;
    if ( (fs.capsuleH == 1) || (fs.capsuleH == 2) )
      ra = fs.right;
    if ( (fs.capsuleV == -1)  || (fs.capsuleV == 2) )
      ta = fs.top;
    if ( (fs.capsuleV == 1) || (fs.capsuleV == 2) )
      ba = fs.bottom;

    // top
    if ( (fs.capsuleV == -1) || (fs.capsuleV == 2) ) {
      top = QRect(x0+la,y0,w-la-ra,fs.top);
      // topleft corner
      if (fs.capsuleH == -1)
        topleft = QRect(x0,y0,fs.left,fs.top);
      // topright corner
      if (fs.capsuleH == 1)
        topright = QRect(x1-fs.right+1,y0,fs.right,fs.top);
    }
    // bottom
    if ( (fs.capsuleV == 1) || (fs.capsuleV == 2) ) {
      bottom = QRect(x0+la,y1-ba+1,w-la-ra,fs.bottom);
      // bottomleft corner
      if (fs.capsuleH == -1)
        bottomleft = QRect(x0,y1-fs.bottom+1,fs.left,fs.bottom);
      // bottomright corner
      if (fs.capsuleH == 1)
        bottomright = QRect(x1-fs.right+1,y1-fs.bottom+1,fs.right,fs.bottom);
    }
    // left
    if ( (fs.capsuleH == -1) || (fs.capsuleH == 2) ) {
      left = QRect(x0,y0+ta,fs.left,h-ta-ba);
      // topleft corner
      if (fs.capsuleV == -1)
        topleft = QRect(x0,y0,fs.left,fs.top);
      // bottomleft corner
      if (fs.capsuleV == 1)
        bottomleft = QRect(x0,y1-fs.bottom+1,fs.left,fs.bottom);
    }
    // right
    if ( (fs.capsuleH == 1) || (fs.capsuleH == 2) ) {
      right = QRect(x1-fs.right+1,y0+ta,fs.right,h-ta-ba);
      // topright corner
      if (fs.capsuleV == -1)
        topright = QRect(x1-fs.right+1,y0,fs.right,fs.top);
      // bottomright corner
      if (fs.capsuleV == 1)
        bottomright = QRect(x1-fs.right+1,y1-fs.bottom+1,fs.right,fs.bottom);
    }
  }
}

void QSvgThemableStyle::renderFrame(QPainter *p,
                    /* color spec */ const QBrush &b,
                    /* frame bounds */ const QRect &bounds,
                    /* frame spec */ const frame_spec_t &fs,
                    /* SVG element */ const QString &e,
                    /* direction */ Qt::LayoutDirection dir,
                    /* orientation */ Orientation orn) const
{
  if (!fs.hasFrame)
    return;

  emit(sig_renderFrame_begin(e));

  int x0,y0,w,h;
  int intensity;
  bool use3dFrame;

  bounds.getRect(&x0,&y0,&w,&h);
  intensity = getSpecificValue("specific.palette.intensity").toInt();
  use3dFrame = getSpecificValue("specific.palette.3dframes").toBool();

  // rects to draw frame parts
  QRect top, bottom, left, right, topleft, topright, bottomleft, bottomright;

  computeFrameRects(bounds,fs,orn,
                    top,bottom,left,right,
                    topleft,topright,bottomleft,bottomright);

  if ( !topright.isNull() )
    topright.adjust(0,0,1,1);
  if ( !bottomleft.isNull() )
    bottomleft.adjust(0,0,1,1);

  QPainterPath lightPath; // top and left 3D effect
  if ( !top.isNull() )
    lightPath.addRect(top);
  if ( !topleft.isNull() )
    lightPath.addRect(topleft);
  if ( !left.isNull() )
    lightPath.addRect(left);
  if ( !bottomleft.isNull() ) {
    // make corners wider by +1 because the QPolygon fill is missing one pixel
    bottomleft.adjust(0,0,1,1);
    lightPath.addPolygon(QPolygon(
      QVector<QPoint>() <<
      bottomleft.topLeft() <<
      bottomleft.bottomLeft() <<
      bottomleft.topRight() <<
      bottomleft.topLeft()
    ));
    bottomleft.adjust(0,0,-1,-1);
  }
  if ( !topright.isNull() )
    lightPath.addPolygon(QPolygon(
      QVector<QPoint>() <<
      topright.topLeft() <<
      topright.topRight() <<
      topright.bottomLeft() <<
      topright.topLeft()
    ));

  QPainterPath darkPath; // bottom and right 3D effect
  if ( !bottom.isNull() )
    darkPath.addRect(bottom);
  if ( !bottomright.isNull() )
    darkPath.addRect(bottomright);
  if ( !right.isNull() )
    darkPath.addRect(right);
  if ( !topright.isNull() )
    darkPath.addPolygon(QPolygon(
      QVector<QPoint>() <<
      topright.topRight() <<
      topright.bottomRight() <<
      topright.bottomLeft() <<
      topright.topRight()
    ));
  if ( !bottomleft.isNull() )
    darkPath.addPolygon(QPolygon(
      QVector<QPoint>() <<
      bottomleft.topRight() <<
      bottomleft.bottomRight() <<
      bottomleft.bottomLeft() <<
      bottomleft.topRight()
    ));

  if ( !topright.isNull() )
    topright.adjust(0,0,-1,-1);
  if ( !bottomleft.isNull() )
    bottomleft.adjust(0,0,-1,-1);

  if ( dir == Qt::RightToLeft ) {
    p->save();
    if ( orn == Horizontal ) {
      p->scale(-1.0,1.0);
      p->translate(-2*x0-w,0);
    } else {
      p->scale(1.0,-1.0);
      p->translate(0,-2*y0-h);
    }
  }

  if ( orn == Vertical ) {
    p->save();
    p->scale(-1.0,1.0);
    p->translate(-2*x0-w,0);
    p->setMatrix(QMatrix(0,1,1,0,0,0),true);
  }

  // Render !
  if ( !dbgWireframe ) {
    renderElement(p,e+"-top",top,0,0);
    renderElement(p,e+"-bottom",bottom,0,0);
    renderElement(p,e+"-left",left,0,0);
    renderElement(p,e+"-right",right,0,0);
    renderElement(p,e+"-topleft",topleft,0,0);
    renderElement(p,e+"-topright",topright,0,0);
    renderElement(p,e+"-bottomleft",bottomleft,0,0);
    renderElement(p,e+"-bottomright",bottomright,0,0);
  }

  // Colorize !
  QBrush lightBrush(b), darkBrush(b);
  QColor lightColor, darkColor;

  if ( use3dFrame ) {
    lightColor = b.color().lighter();
    darkColor = b.color().darker();
  } else {
    lightColor = darkColor = b.color();
  }

  lightColor.setAlpha(intensity);
  lightBrush.setColor(lightColor);

  darkColor.setAlpha(intensity);
  darkBrush.setColor(darkColor);

  if ( !dbgWireframe && (curPalette != "<none>") ) {
    if ( !fs.pressed ) {
      p->fillPath(lightPath,lightColor);
      p->fillPath(darkPath,darkColor);
    } else {
      p->fillPath(lightPath,darkColor);
      p->fillPath(darkPath,lightColor);
    }
  }

  // debugging facilities
  if ( dbgWireframe || dbgOverdraw ) {
    p->save();
    if ( dbgWireframe) {
      p->setPen(QPen(Qt::blue));
      drawRealRect(p,top);
      drawRealRect(p,bottom);
      drawRealRect(p,left);
      drawRealRect(p,right);
      drawRealRect(p,topleft);
      drawRealRect(p,topright);
      drawRealRect(p,bottomleft);
      drawRealRect(p,bottomright);
    }
    if ( dbgOverdraw ) {
      p->fillRect(top,QColor(255,0,0,100));
      p->fillRect(bottom,QColor(255,0,0,100));
      p->fillRect(left,QColor(255,0,0,100));
      p->fillRect(right,QColor(255,0,0,100));
      p->fillRect(topleft,QColor(255,0,0,100));
      p->fillRect(topright,QColor(255,0,0,100));
      p->fillRect(bottomleft,QColor(255,0,0,100));
      p->fillRect(bottomright,QColor(255,0,0,100));
    }
    p->restore();
  }

  if ( orn == Vertical ) {
    p->restore();
  }

  if ( dir == Qt::RightToLeft ) {
    p->restore();
  }

  emit(sig_renderFrame_end(e));
}

void QSvgThemableStyle::computeInteriorRect(const QRect& bounds,
                                            frame_spec_t fs,
                                            interior_spec_t is,
                                            Orientation orn,
                                            QRect& r) const
{
  Q_UNUSED(is);

  // drawing rect
  r = bounds;

  // work on horizontalized rect
  if ( orn != Horizontal )
    r = transposedRect(bounds);

  QRect top, bottom, left, right, topleft, topright, bottomleft, bottomright;

  computeFrameRects(bounds,fs,orn, top,bottom,left,right,
                    topleft,topright,bottomleft,bottomright);

  QMargins m(left.width(),top.height(),right.width(),bottom.height());

  r = r.marginsRemoved(m);

  if ( !r.isValid() ) {
    r.setWidth(0);
    r.setHeight(0);
  }
}

void QSvgThemableStyle::renderInterior(QPainter *p,
                       /* color spec */ const QBrush &b,
                       /* frame bounds */ const QRect &bounds,
                       /* frame spec */ const frame_spec_t &fs,
                       /* interior spec */ const interior_spec_t &is,
                       /* SVG element */ const QString &e,
                       /* direction */ Qt::LayoutDirection dir,
                       /* orientation */ Orientation orn) const
{
  if (!is.hasInterior)
    return;

  emit(sig_renderInterior_begin(e));

  int x0,y0,w,h;
  int intensity;

  bounds.getRect(&x0,&y0,&w,&h);
  intensity = getSpecificValue("specific.palette.intensity").toInt();

  // drawing rect
  QRect r;

  computeInteriorRect(bounds,fs,is,orn, r);

  if ( dir == Qt::RightToLeft ) {
    p->save();
    if ( orn == Horizontal ) {
      p->scale(-1.0,1.0);
      p->translate(-2*x0-w,0);
    } else {
      p->scale(1.0,-1.0);
      p->translate(0,-2*y0-h);
    }
  }

  if ( orn == Vertical ) {
    p->save();
    p->scale(-1.0,1.0);
    p->translate(-2*x0-w,0);
    p->setMatrix(QMatrix(0,1,1,0,0,0),true);
  }

  // Render !
  if ( !dbgWireframe )
    renderElement(p,e,r,is.px,is.py);

  // Colorize !
  QBrush interiorBrush(b);

  QColor interiorColor = b.color();
  interiorColor.setAlpha(intensity);
  interiorBrush.setColor(interiorColor);

  if ( !dbgWireframe && (curPalette != "<none>") )
    p->fillRect(r,interiorColor);

  // debugging facilities
  if ( dbgWireframe || dbgOverdraw ) {
    p->save();
    if ( dbgWireframe) {
      p->setPen(QPen(Qt::red));
      drawRealRect(p,r);
    }
    if ( dbgOverdraw ) {
      p->fillRect(r,QBrush(QColor(255,0,0,100)));
    }
    p->restore();
  }

  if ( orn == Vertical ) {
    p->restore();
  }

  if ( dir == Qt::RightToLeft ) {
    p->restore();
  }

  emit(sig_renderInterior_end(e));
}

void QSvgThemableStyle::renderIndicator(QPainter *p,
                       /* frame bounds */ const QRect &bounds,
                       /* frame spec */ const frame_spec_t &fs,
                       /* interior spec */ const interior_spec_t &is,
                       /* indicator spec */ const indicator_spec_t &ds,
                       /* indicator SVG element */ const QString &e,
                       /* direction */ Qt::LayoutDirection dir,
                       Qt::Alignment alignment) const
{
  emit(sig_renderIndicator_begin(e));

  // drawing rect
  QRect r = squaredRect(interiorRect(bounds,fs,is));
  int s = (r.width() > ds.size) ? ds.size : r.width();
  r = alignedRect(Qt::LeftToRight,alignment,QSize(s,s),interiorRect(bounds,fs,is));

  int x0,y0,w,h;
  r.getRect(&x0,&y0,&w,&h);

  if ( dir == Qt::RightToLeft ) {
    p->save();
    p->scale(-1.0,1.0);
    p->translate(-2*x0-w,0);
  }

  renderElement(p,e,r,0,0);

  if ( dir == Qt::RightToLeft ) {
    p->restore();
  }

  // debugging facilities
  if ( dbgWireframe || dbgOverdraw ) {
    p->save();
    if ( dbgWireframe) {
      p->setPen(QPen(Qt::cyan));
      drawRealRect(p,bounds);
    }
    if ( dbgOverdraw ) {
      p->fillRect(r,QColor(255,0,0,100));
    }
    p->restore();
  }

  emit(sig_renderIndicator_end(e));
}

void QSvgThemableStyle::colorizeIndicator(QPainter *p,
                       /* frame bounds */ const QRect &bounds,
                       /* frame spec */ const frame_spec_t &fs,
                       /* interior spec */ const interior_spec_t &is,
                       /* indicator spec */ const indicator_spec_t &ds,
                       /* brush */ const QBrush &b,
                       /* direction */ Qt::LayoutDirection dir,
                       Qt::Alignment alignment) const
{
  int intensity;
  bool use3dFrame;

  intensity = getSpecificValue("specific.palette.intensity").toInt();
  use3dFrame = getSpecificValue("specific.palette.3dframes").toBool();

  // drawing rect
  QRect r = squaredRect(interiorRect(bounds,fs,is));
  int s = (r.width() > ds.size) ? ds.size : r.width();
  r = alignedRect(Qt::LeftToRight,alignment,QSize(s,s),interiorRect(bounds,fs,is));

  int x0,y0,w,h;
  r.getRect(&x0,&y0,&w,&h);

  QPainterPath lightPath, darkPath;

  darkPath.addPolygon(QPolygon(
    QVector<QPoint>() <<
      r.topLeft() <<
      r.topRight() <<
      r.bottomLeft() <<
      r.topLeft()
                      ));

  lightPath.addPolygon(QPolygon(
    QVector<QPoint>() <<
    r.topRight() <<
    r.bottomLeft() <<
    r.bottomRight() <<
    r.topRight()
  ));

  // Colorize
  QBrush lightBrush(b), darkBrush(b);
  QColor lightColor, darkColor;

  if ( use3dFrame ) {
    lightColor = b.color().lighter();
    darkColor = b.color().darker();
  } else {
    lightColor = darkColor = b.color();
  }

  lightColor.setAlpha(intensity);
  lightBrush.setColor(lightColor);

  darkColor.setAlpha(intensity);
  darkBrush.setColor(darkColor);

  if ( dir == Qt::RightToLeft ) {
    p->save();
    p->scale(-1.0,1.0);
    p->translate(-2*x0-w,0);
  }

  // Colorize
  p->fillPath(darkPath,darkColor);
  p->fillPath(lightPath,lightColor);

  if ( dir == Qt::RightToLeft ) {
    p->restore();
  }
}

void QSvgThemableStyle::renderLabel(QPainter* p,
                            const QBrush &b,
                            Qt::LayoutDirection dir,
                            const QRect& bounds,
                            const frame_spec_t& fs,
                            const interior_spec_t& is,
                            const label_spec_t& ls,
                            int talign,
                            const QString& text,
                            const QPixmap& icon,
                            const Qt::ToolButtonStyle tialign) const
{
  emit(sig_renderLabel_begin("text:"+(text.isEmpty() ? "<none>" : "\""+text+"\"")+
                             "/icon:"+(icon.isNull() ? "no" : "yes")));

  // compute text and icon rect
  QRect r(labelRect(bounds,fs,is,ls));

  QRect ricon = r;
  QRect rtext = r;

  if (tialign == Qt::ToolButtonTextBesideIcon) {
    ricon = alignedRect(Qt::LeftToRight,Qt::AlignVCenter | Qt::AlignLeft, QSize(icon.width(),icon.height()),r);
    rtext = QRect(r.x()+icon.width()+(icon.isNull() ? 0 : ls.tispace),r.y(),r.width()-ricon.width()-(icon.isNull() ? 0 : ls.tispace),r.height());
  } else if (tialign == Qt::ToolButtonTextUnderIcon) {
    ricon = alignedRect(Qt::LeftToRight,Qt::AlignTop | Qt::AlignHCenter, QSize(icon.width(),icon.height()),r);
    rtext = QRect(r.x(),r.y()+icon.height()+(icon.isNull() ? 0 : ls.tispace),r.width(),r.height()-ricon.height()-(icon.isNull() ? 0 : ls.tispace));
  } else if (tialign == Qt::ToolButtonIconOnly) {
    ricon = alignedRect(Qt::LeftToRight,Qt::AlignCenter, QSize(icon.width(),icon.height()),r);
  }

  if ( text.isNull() || text.isEmpty() ) {
    // When we have no text, center icon
    ricon = alignedRect(Qt::LeftToRight,Qt::AlignCenter, QSize(icon.width(),icon.height()),r);
  }

  rtext = visualRect(dir,bounds,rtext);
  ricon = visualRect(dir,bounds,ricon);

  if (tialign != Qt::ToolButtonIconOnly) {
    if ( !text.isNull() && !text.isEmpty() ) {
//       if (ls.hasShadow) {
//         p->save();
//         p->setPen(QPen(QColor(ls.r,ls.g,ls.b,ls.a)));
//         for (int i=0; i<ls.depth; i++)
//           p->drawText(rtext.adjusted(ls.xshift+i,ls.yshift+i,0,0),talign,text);
//         p->restore();
//       }
      if ( curPalette != "<none>" ) {
        p->setPen(b.color());
      } else {
        p->setPen(Qt::black);
      }
      p->drawText(rtext,visualAlignment(dir,static_cast<Qt::Alignment>(talign)),text);
    }
  }

  if (tialign != Qt::ToolButtonTextOnly) {
    if (!icon.isNull()) {
      p->drawPixmap(ricon,icon);
    }
  }

  // debugging facilities
  if ( dbgWireframe || dbgOverdraw ) {
    p->save();
    if ( dbgWireframe) {
      p->setPen(QPen(Qt::white));
      drawRealRect(p,r);
      if (tialign != Qt::ToolButtonIconOnly) {
        if (!text.isEmpty()) {
          p->setPen(QPen(Qt::green));
          drawRealRect(p,rtext);
        }
      }
      if (tialign != Qt::ToolButtonTextOnly) {
        if (!icon.isNull()) {
          p->setPen(QPen(QColor(255,0,255)));
          drawRealRect(p, ricon);
        }
      }
    }
    if ( dbgOverdraw ) {
//      p->setCompositionMode(QPainter::CompositionMode_Darken);
      p->fillRect(r,QBrush(QColor(255,0,0,100)));
    }
    p->restore();
  }

  emit(sig_renderLabel_end("text:"+text+"/icon:"+(icon.isNull() ? "yes":"no")));
}

inline frame_spec_t QSvgThemableStyle::getFrameSpec(const QString& group) const
{
  return themeSettings->getFrameSpec(group);
}

inline interior_spec_t QSvgThemableStyle::getInteriorSpec(const QString& group) const
{
  return themeSettings->getInteriorSpec(group);
}

inline indicator_spec_t QSvgThemableStyle::getIndicatorSpec(const QString& group) const
{
  return themeSettings->getIndicatorSpec(group);
}

inline label_spec_t QSvgThemableStyle::getLabelSpec(const QString& group) const
{
  return themeSettings->getLabelSpec(group);
}

inline QVariant QSvgThemableStyle::getSpecificValue(const QString &key) const
{
  return styleSettings->getSpecificValue(key);
}

inline color_spec_t QSvgThemableStyle::getColorSpec(const QString& group) const
{
  color_spec_t cs;
  return paletteSettings ? paletteSettings->getColorSpec(group) : cs;
}

QLayout * QSvgThemableStyle::layoutForWidget(const QWidget* widget, QLayout *l) const
{
  if ( !widget )
    return NULL;

  QLayout *pLayout = l; /* parent layout */

  if ( !pLayout && widget->parentWidget() )
    pLayout = widget->parentWidget()->layout();

  if ( !pLayout )
    return NULL;

  for (int i = 0; i < pLayout->count(); ++i) {
    // item is a widget -> compare
    if ( pLayout->itemAt(i)->widget() == widget )
      return pLayout;

    // item is a sub layout -> look inside
    if ( pLayout->itemAt(i)->layout() ) {
      QLayout *sub = layoutForWidget(widget, pLayout->itemAt(i)->layout());
      if ( sub )
        return sub;
    }
  }

  return NULL;
}

void QSvgThemableStyle::capsulePosition(const QWidget *widget, bool &capsule, int &h, int &v) const
{
  capsule = false;
  h = v = 2;

  QLayout *myLayout = layoutForWidget(widget);
  if ( !myLayout )
    return;

  if ( myLayout->spacing() != 0 ) { // space between widgets -> no capsule
    capsule = false;
    return;
  }

  //qDebug() << "widget" << widget->objectName() << "is in layout" << myLayout->objectName();

  int cnt = myLayout->count();
  if ( cnt == 1 ) { // only one widget -> easy
    capsule = true;
    h = v = 2;
    return;
  }

  QString myClass = widget->metaObject()->className();

  int myIdx = -1;
  for (int i=0; i<cnt; i++) {
    if ( myLayout->itemAt(i)->widget() == widget ) {
      myIdx = i;
      break;
    }
  }

  const QHBoxLayout *hbox = qobject_cast<const QHBoxLayout *>(myLayout);
  const QVBoxLayout *vbox = qobject_cast<const QVBoxLayout *>(myLayout);
  const QGridLayout *grid = qobject_cast<const QGridLayout *>(myLayout);

  QWidget *myLeftWidget = 0, *myRightWidget = 0;
  QWidget *myTopWidget = 0, *myBottomWidget = 0;

  // QHBoxLayout
  if ( hbox ) {
    if ( myIdx > 0 )
      myLeftWidget = myLayout->itemAt(myIdx-1) ?
        myLayout->itemAt(myIdx-1)->widget() : 0;
    if ( myIdx < cnt-1 )
      myRightWidget = myLayout->itemAt(myIdx+1) ?
        myLayout->itemAt(myIdx+1)->widget() : 0;
  }

  // QVBoxLayout
  if ( vbox ) {
    if ( myIdx > 0 )
      myTopWidget = myLayout->itemAt(myIdx-1) ?
        myLayout->itemAt(myIdx-1)->widget() : 0;
    if ( myIdx < cnt-1 )
      myBottomWidget = myLayout->itemAt(myIdx+1) ?
        myLayout->itemAt(myIdx+1)->widget() : 0;
  }

  // QGridLayout
  if ( grid ) {
    if ( (grid->horizontalSpacing() != 0) || (grid->verticalSpacing() != 0) ) {
      // space between widgets -> no capsule
      capsule = false;
      return;
    }

    int nrows = grid->rowCount();
    int ncols = grid->columnCount();

    int myRow, myCol, myRowSpan, myColSpan;
    grid->getItemPosition(myIdx, &myRow,&myCol, &myRowSpan,&myColSpan);

    //qDebug() << "myidx,myrow,mycol" << myIdx << myRow << myCol;

    if ( (myRowSpan != 1) && (myColSpan != 1) ) {
      // widgets that span accross multiple rows/cols cannot be in a capsule
      capsule = false;
      return;
    }

    if ( nrows == 1 ) {
      capsule = true;
      v = 2;
    }

    if ( ncols == 1 ) {
      capsule = true;
      h = 2;
    }

    if ( myCol > 0 )
      myLeftWidget = grid->itemAtPosition(myRow,myCol-1) ?
        grid->itemAtPosition(myRow,myCol-1)->widget() : 0;
    if ( myCol < ncols-1 )
      myRightWidget = grid->itemAtPosition(myRow,myCol+1) ?
        grid->itemAtPosition(myRow,myCol+1)->widget() : 0;

    if ( myRow > 0 )
      myTopWidget = grid->itemAtPosition(myRow-1,myCol) ?
        grid->itemAtPosition(myRow-1,myCol)->widget() : 0;
    if ( myRow < nrows-1 )
      myBottomWidget = grid->itemAtPosition(myRow+1,myCol) ?
        grid->itemAtPosition(myRow+1,myCol)->widget() : 0;
  }

  if ( grid || hbox ) {
    if ( myLeftWidget && (myLeftWidget->metaObject()->className() == myClass) &&
         myRightWidget && (myRightWidget->metaObject()->className() == myClass) ) {
      capsule = true;
      h = 0;
      goto next;
    }

    if ( myLeftWidget && (myLeftWidget->metaObject()->className() == myClass) ) {
      capsule = true;
      h = 1;
      goto next;
    }

    if ( myRightWidget && (myRightWidget->metaObject()->className() == myClass) ) {
      capsule = true;
      h = -1;
      goto next;
    }
  }

next:
  if ( grid || vbox ) {
    if ( myTopWidget && (myTopWidget->metaObject()->className() == myClass) &&
         myBottomWidget && (myBottomWidget->metaObject()->className() == myClass) ) {
      capsule = true;
      v = 0;
      goto end;
    }

    if ( myTopWidget && (myTopWidget->metaObject()->className() == myClass) ) {
      capsule = true;
      v = 1;
      goto end;
    }

    if ( myBottomWidget && (myBottomWidget->metaObject()->className() == myClass) ) {
      capsule = true;
      v = -1;
      goto end;
    }
  }

end:
  ;
}

void QSvgThemableStyle::drawRealRect(QPainter* p, const QRect& r) const
{
  int x0,y0,x1,y1,w,h;
  r.getRect(&x0,&y0,&w,&h);
  x1 = r.bottomRight().x();
  y1 = r.bottomRight().y();
  p->drawLine(x0,y0,x1,y0);
  p->drawLine(x0,y0,x0,y1);
  p->drawLine(x1,y0,x1,y1);
  p->drawLine(x0,y1,x1,y1);
}

/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
QString QSvgThemableStyle::state_str(State st, const QWidget* w) const
{
  QString status;

  if ( !isContainerWidget(w) ) {
    status = ( (st & State_Enabled) || (st & State_None) ) ?
      (st & State_Sunken) ? "pressed" :
      (st & State_On) ? "toggled" :
      (st & State_Selected) ? "toggled" :
      (st & State_MouseOver) ? "hovered" : "normal"
    : (st & State_On) ? "disabled-toggled" : "disabled";
  } else {
      // container widgets will have only normal, selected and disabled status
    status = ( (st & State_Enabled) || (st & State_None) ) ?
      (st & State_Selected) ? "toggled" : "normal"
    : "disabled";
  }

  return status;
}

QIcon::Mode QSvgThemableStyle::state_iconmode(State st) const
{
  return
    (st & State_Enabled) ?
      (st & State_Sunken) ? QIcon::Active :
      (st & State_Selected ) ? QIcon::Selected :
      (st & State_MouseOver) ? QIcon::Active : QIcon::Normal
    : QIcon::Disabled;
}

QIcon::State QSvgThemableStyle::state_iconstate(State st) const
{
  return (st & State_On) ? QIcon::On : QIcon::Off;
}

/* Auto generated */
QString QSvgThemableStyle::PE_str(PrimitiveElement element) const
{
  switch (element) {
    case PE_Frame : return "PE_Frame";
    case PE_FrameDefaultButton : return "PE_FrameDefaultButton";
    case PE_FrameDockWidget : return "PE_FrameDockWidget";
    case PE_FrameFocusRect : return "PE_FrameFocusRect";
    case PE_FrameGroupBox : return "PE_FrameGroupBox";
    case PE_FrameLineEdit : return "PE_FrameLineEdit";
    case PE_FrameMenu : return "PE_FrameMenu";
    case PE_FrameStatusBarItem : return "PE_FrameStatusBarItem (= PE_FrameStatusBar)";
    case PE_FrameTabWidget : return "PE_FrameTabWidget";
    case PE_FrameWindow : return "PE_FrameWindow";
    case PE_FrameButtonBevel : return "PE_FrameButtonBevel";
    case PE_FrameButtonTool : return "PE_FrameButtonTool";
    case PE_FrameTabBarBase : return "PE_FrameTabBarBase";
    case PE_PanelButtonCommand : return "PE_PanelButtonCommand";
    case PE_PanelButtonBevel : return "PE_PanelButtonBevel";
    case PE_PanelButtonTool : return "PE_PanelButtonTool";
    case PE_PanelMenuBar : return "PE_PanelMenuBar";
    case PE_PanelToolBar : return "PE_PanelToolBar";
    case PE_PanelLineEdit : return "PE_PanelLineEdit";
    case PE_IndicatorArrowDown : return "PE_IndicatorArrowDown";
    case PE_IndicatorArrowLeft : return "PE_IndicatorArrowLeft";
    case PE_IndicatorArrowRight : return "PE_IndicatorArrowRight";
    case PE_IndicatorArrowUp : return "PE_IndicatorArrowUp";
    case PE_IndicatorBranch : return "PE_IndicatorBranch";
    case PE_IndicatorButtonDropDown : return "PE_IndicatorButtonDropDown";
    case PE_IndicatorItemViewItemCheck : return "PE_IndicatorItemViewItemCheck (= PE_IndicatorViewItemCheck)";
    case PE_IndicatorCheckBox : return "PE_IndicatorCheckBox";
    case PE_IndicatorDockWidgetResizeHandle : return "PE_IndicatorDockWidgetResizeHandle";
    case PE_IndicatorHeaderArrow : return "PE_IndicatorHeaderArrow";
    case PE_IndicatorMenuCheckMark : return "PE_IndicatorMenuCheckMark";
    case PE_IndicatorProgressChunk : return "PE_IndicatorProgressChunk";
    case PE_IndicatorRadioButton : return "PE_IndicatorRadioButton";
    case PE_IndicatorSpinDown : return "PE_IndicatorSpinDown";
    case PE_IndicatorSpinMinus : return "PE_IndicatorSpinMinus";
    case PE_IndicatorSpinPlus : return "PE_IndicatorSpinPlus";
    case PE_IndicatorSpinUp : return "PE_IndicatorSpinUp";
    case PE_IndicatorToolBarHandle : return "PE_IndicatorToolBarHandle";
    case PE_IndicatorToolBarSeparator : return "PE_IndicatorToolBarSeparator";
    case PE_PanelTipLabel : return "PE_PanelTipLabel";
    case PE_IndicatorTabTearLeft : return "PE_IndicatorTabTearLeft";
    case PE_IndicatorTabTearRight : return "PE_IndicatorTabTearRight";
    case PE_PanelScrollAreaCorner : return "PE_PanelScrollAreaCorner";
    case PE_Widget : return "PE_Widget";
    case PE_IndicatorColumnViewArrow : return "PE_IndicatorColumnViewArrow";
    case PE_IndicatorItemViewItemDrop : return "PE_IndicatorItemViewItemDrop";
    case PE_PanelItemViewItem : return "PE_PanelItemViewItem";
    case PE_PanelItemViewRow : return "PE_PanelItemViewRow";
    case PE_PanelStatusBar : return "PE_PanelStatusBar";
    case PE_IndicatorTabClose : return "PE_IndicatorTabClose";
    case PE_PanelMenu : return "PE_PanelMenu";
    case PE_CustomBase : return "PE_CustomBase";
  }

  return "PE_Unknown";
}

QString QSvgThemableStyle::CE_str(QStyle::ControlElement element) const
{
  switch(element) {
    case CE_PushButton : return "CE_PushButton";
    case CE_PushButtonBevel : return "CE_PushButtonBevel";
    case CE_PushButtonLabel : return "CE_PushButtonLabel";
    case CE_CheckBox : return "CE_CheckBox";
    case CE_CheckBoxLabel : return "CE_CheckBoxLabel";
    case CE_RadioButton : return "CE_RadioButton";
    case CE_RadioButtonLabel : return "CE_RadioButtonLabel";
    case CE_TabBarTab : return "CE_TabBarTab";
    case CE_TabBarTabShape : return "CE_TabBarTabShape";
    case CE_TabBarTabLabel : return "CE_TabBarTabLabel";
    case CE_ProgressBar : return "CE_ProgressBar";
    case CE_ProgressBarGroove : return "CE_ProgressBarGroove";
    case CE_ProgressBarContents : return "CE_ProgressBarContents";
    case CE_ProgressBarLabel : return "CE_ProgressBarLabel";
    case CE_MenuItem : return "CE_MenuItem";
    case CE_MenuScroller : return "CE_MenuScroller";
    case CE_MenuVMargin : return "CE_MenuVMargin";
    case CE_MenuHMargin : return "CE_MenuHMargin";
    case CE_MenuTearoff : return "CE_MenuTearoff";
    case CE_MenuEmptyArea : return "CE_MenuEmptyArea";
    case CE_MenuBarItem : return "CE_MenuBarItem";
    case CE_MenuBarEmptyArea : return "CE_MenuBarEmptyArea";
    case CE_ToolButtonLabel : return "CE_ToolButtonLabel";
    case CE_Header : return "CE_Header";
    case CE_HeaderSection : return "CE_HeaderSection";
    case CE_HeaderLabel : return "CE_HeaderLabel";
    case CE_ToolBoxTab : return "CE_ToolBoxTab";
    case CE_SizeGrip : return "CE_SizeGrip";
    case CE_Splitter : return "CE_Splitter";
    case CE_RubberBand : return "CE_RubberBand";
    case CE_DockWidgetTitle : return "CE_DockWidgetTitle";
    case CE_ScrollBarAddLine : return "CE_ScrollBarAddLine";
    case CE_ScrollBarSubLine : return "CE_ScrollBarSubLine";
    case CE_ScrollBarAddPage : return "CE_ScrollBarAddPage";
    case CE_ScrollBarSubPage : return "CE_ScrollBarSubPage";
    case CE_ScrollBarSlider : return "CE_ScrollBarSlider";
    case CE_ScrollBarFirst : return "CE_ScrollBarFirst";
    case CE_ScrollBarLast : return "CE_ScrollBarLast";
    case CE_FocusFrame : return "CE_FocusFrame";
    case CE_ComboBoxLabel : return "CE_ComboBoxLabel";
    case CE_ToolBar : return "CE_ToolBar";
    case CE_ToolBoxTabShape : return "CE_ToolBoxTabShape";
    case CE_ToolBoxTabLabel : return "CE_ToolBoxTabLabel";
    case CE_HeaderEmptyArea : return "CE_HeaderEmptyArea";
    case CE_ColumnViewGrip : return "CE_ColumnViewGrip";
    case CE_ItemViewItem : return "CE_ItemViewItem";
    case CE_ShapedFrame : return "CE_ShapedFrame";
    case CE_CustomBase : return "CE_CustomBase";
  }

  return "CE_Unknown";
}

QString QSvgThemableStyle::SE_str(QStyle::SubElement element) const
{
  switch(element) {
    case SE_PushButtonContents : return "SE_PushButtonContents";
    case SE_PushButtonFocusRect : return "SE_PushButtonFocusRect";
    case SE_CheckBoxIndicator : return "SE_CheckBoxIndicator";
    case SE_CheckBoxContents : return "SE_CheckBoxContents";
    case SE_CheckBoxFocusRect : return "SE_CheckBoxFocusRect";
    case SE_CheckBoxClickRect : return "SE_CheckBoxClickRect";
    case SE_RadioButtonIndicator : return "SE_RadioButtonIndicator";
    case SE_RadioButtonContents : return "SE_RadioButtonContents";
    case SE_RadioButtonFocusRect : return "SE_RadioButtonFocusRect";
    case SE_RadioButtonClickRect : return "SE_RadioButtonClickRect";
    case SE_ComboBoxFocusRect : return "SE_ComboBoxFocusRect";
    case SE_SliderFocusRect : return "SE_SliderFocusRect";
    case SE_ProgressBarGroove : return "SE_ProgressBarGroove";
    case SE_ProgressBarContents : return "SE_ProgressBarContents";
    case SE_ProgressBarLabel : return "SE_ProgressBarLabel";
    case SE_ToolBoxTabContents : return "SE_ToolBoxTabContents";
    case SE_HeaderLabel : return "SE_HeaderLabel";
    case SE_HeaderArrow : return "SE_HeaderArrow";
    case SE_TabWidgetTabBar : return "SE_TabWidgetTabBar";
    case SE_TabWidgetTabPane : return "SE_TabWidgetTabPane";
    case SE_TabWidgetTabContents : return "SE_TabWidgetTabContents";
    case SE_TabWidgetLeftCorner : return "SE_TabWidgetLeftCorner";
    case SE_TabWidgetRightCorner : return "SE_TabWidgetRightCorner";
    case SE_ItemViewItemCheckIndicator : return "SE_ItemViewItemCheckIndicator (= SE_ViewItemCheckIndicator)";
    case SE_TabBarTearIndicatorLeft : return "SE_TabBarTearIndicatorLeft";
    case SE_TabBarTearIndicatorRight : return "SE_TabBarTearIndicatorRight";
    case SE_TreeViewDisclosureItem : return "SE_TreeViewDisclosureItem";
    case SE_LineEditContents : return "SE_LineEditContents";
    case SE_FrameContents : return "SE_FrameContents";
    case SE_DockWidgetCloseButton : return "SE_DockWidgetCloseButton";
    case SE_DockWidgetFloatButton : return "SE_DockWidgetFloatButton";
    case SE_DockWidgetTitleBarText : return "SE_DockWidgetTitleBarText";
    case SE_DockWidgetIcon : return "SE_DockWidgetIcon";
    case SE_CheckBoxLayoutItem : return "SE_CheckBoxLayoutItem";
    case SE_ComboBoxLayoutItem : return "SE_ComboBoxLayoutItem";
    case SE_DateTimeEditLayoutItem : return "SE_DateTimeEditLayoutItem";
    case SE_DialogButtonBoxLayoutItem : return "SE_DialogButtonBoxLayoutItem";
    case SE_LabelLayoutItem : return "SE_LabelLayoutItem";
    case SE_ProgressBarLayoutItem : return "SE_ProgressBarLayoutItem";
    case SE_PushButtonLayoutItem : return "SE_PushButtonLayoutItem";
    case SE_RadioButtonLayoutItem : return "SE_RadioButtonLayoutItem";
    case SE_SliderLayoutItem : return "SE_SliderLayoutItem";
    case SE_SpinBoxLayoutItem : return "SE_SpinBoxLayoutItem";
    case SE_ToolButtonLayoutItem : return "SE_ToolButtonLayoutItem";
    case SE_FrameLayoutItem : return "SE_FrameLayoutItem";
    case SE_GroupBoxLayoutItem : return "SE_GroupBoxLayoutItem";
    case SE_TabWidgetLayoutItem : return "SE_TabWidgetLayoutItem";
    case SE_TabBarScrollLeftButton : return "SE_TabBarScrollLeftButton";
    case SE_TabBarScrollRightButton : return "SE_TabBarScrollRightButton";
    case SE_ItemViewItemDecoration : return "SE_ItemViewItemDecoration";
    case SE_ItemViewItemText : return "SE_ItemViewItemText";
    case SE_ItemViewItemFocusRect : return "SE_ItemViewItemFocusRect";
    case SE_TabBarTabLeftButton : return "SE_TabBarTabLeftButton";
    case SE_TabBarTabRightButton : return "SE_TabBarTabRightButton";
    case SE_TabBarTabText : return "SE_TabBarTabText";
    case SE_ShapedFrameContents : return "SE_ShapedFrameContents";
    case SE_ToolBarHandle : return "SE_ToolBarHandle";
    case SE_CustomBase : return "SE_CustomBase";
  }

  return "SE_Unknown";
}

QString QSvgThemableStyle::CC_str(QStyle::ComplexControl element) const
{
  switch (element) {
    case CC_SpinBox : return "CC_SpinBox";
    case CC_ComboBox : return "CC_ComboBox";
    case CC_ScrollBar : return "CC_ScrollBar";
    case CC_Slider : return "CC_Slider";
    case CC_ToolButton : return "CC_ToolButton";
    case CC_TitleBar : return "CC_TitleBar";
    case CC_Dial : return "CC_Dial";
    case CC_GroupBox : return "CC_GroupBox";
    case CC_MdiControls : return "CC_MdiControls";
    case CC_CustomBase : return "CC_CustomBase";
  }

  return "CC_Unknown";
}

QString QSvgThemableStyle::SC_str(QStyle::ComplexControl control, QStyle::SubControl subControl) const
{
  switch (control) {
    case CC_SpinBox : switch (subControl) {
      case SC_SpinBoxUp : return "SC_SpinBoxUp";
      case SC_SpinBoxDown : return "SC_SpinBoxDown";
      case SC_SpinBoxFrame : return "SC_SpinBoxFrame";
      case SC_SpinBoxEditField : return "SC_SpinBoxEditField";
      case SC_None : return "SC_None";
      default : return "SC_Unknown";
    }
    case CC_ComboBox : switch (subControl) {
      case SC_ComboBoxFrame : return "SC_ComboBoxFrame";
      case SC_ComboBoxEditField : return "SC_ComboBoxEditField";
      case SC_ComboBoxArrow : return "SC_ComboBoxArrow";
      case SC_ComboBoxListBoxPopup : return "SC_ComboBoxListBoxPopup";
      case SC_None : return "SC_None";
      default : return "SC_Unknown";
    }
    case CC_ScrollBar : switch (subControl) {
      case SC_ScrollBarAddLine : return "SC_ScrollBarAddLine";
      case SC_ScrollBarSubLine : return "SC_ScrollBarSubLine";
      case SC_ScrollBarAddPage : return "SC_ScrollBarAddPage";
      case SC_ScrollBarSubPage : return "SC_ScrollBarSubPage";
      case SC_ScrollBarFirst : return "SC_ScrollBarFirst";
      case SC_ScrollBarLast : return "SC_ScrollBarLast";
      case SC_ScrollBarSlider : return "SC_ScrollBarSlider";
      case SC_ScrollBarGroove : return "SC_ScrollBarGroove";
      case SC_None : return "SC_None";
      default : return "SC_Unknown";
    }
    case CC_Slider : switch (subControl) {
      case SC_SliderGroove : return "SC_SliderGroove";
      case SC_SliderHandle : return "SC_SliderHandle";
      case SC_SliderTickmarks : return "SC_SliderTickmarks";
      case SC_None : return "SC_None";
      default : return "SC_Unknown";
    }
    case CC_ToolButton : switch (subControl) {
      case SC_ToolButton : return "SC_ToolButton";
      case SC_ToolButtonMenu : return "SC_ToolButtonMenu";
      case SC_None : return "SC_None";
      default : return "SC_Unknown";
    }
    case CC_TitleBar : switch (subControl) {
      case SC_TitleBarSysMenu : return "SC_TitleBarSysMenu";
      case SC_TitleBarMinButton : return "SC_TitleBarMinButton";
      case SC_TitleBarMaxButton : return "SC_TitleBarMaxButton";
      case SC_TitleBarCloseButton : return "SC_TitleBarCloseButton";
      case SC_TitleBarNormalButton : return "SC_TitleBarNormalButton";
      case SC_TitleBarShadeButton : return "SC_TitleBarShadeButton";
      case SC_TitleBarUnshadeButton : return "SC_TitleBarUnshadeButton";
      case SC_TitleBarContextHelpButton : return "SC_TitleBarContextHelpButton";
      case SC_TitleBarLabel : return "SC_TitleBarLabel";
      case SC_None : return "SC_None";
      default : return "SC_Unknown";
    }
    case CC_Dial : switch (subControl) {
      case SC_DialGroove : return "SC_DialGroove";
      case SC_DialHandle : return "SC_DialHandle";
      case SC_DialTickmarks : return "SC_DialTickmarks";
      case SC_None : return "SC_None";
      default : return "SC_Unknown";
    }
    case CC_GroupBox : switch (subControl) {
      case SC_GroupBoxCheckBox : return "SC_GroupBoxCheckBox";
      case SC_GroupBoxLabel : return "SC_GroupBoxLabel";
      case SC_GroupBoxContents : return "SC_GroupBoxContents";
      case SC_GroupBoxFrame : return "SC_GroupBoxFrame";
      case SC_None : return "SC_None";
      default : return "SC_Unknown";
    }
    case CC_MdiControls : switch (subControl) {
      case SC_MdiMinButton : return "SC_MdiMinButton";
      case SC_MdiNormalButton : return "SC_MdiNormalButton";
      case SC_MdiCloseButton : return "SC_MdiCloseButton";
      case SC_None : return "SC_None";
      default : return "SC_Unknown";
    };
    case CC_CustomBase : switch (subControl) {
      default : return "SC_Unknown";
    }
  }

  return "SC_Unknown";
}

QString QSvgThemableStyle::CT_str(QStyle::ContentsType type) const
{
  switch (type) {
    case CT_PushButton : return "CT_PushButton";
    case CT_CheckBox : return "CT_CheckBox";
    case CT_RadioButton : return "CT_RadioButton";
    case CT_ToolButton : return "CT_ToolButton";
    case CT_ComboBox : return "CT_ComboBox";
    case CT_Splitter : return "CT_Splitter";
    case CT_ProgressBar : return "CT_ProgressBar";
    case CT_MenuItem : return "CT_MenuItem";
    case CT_MenuBarItem : return "CT_MenuBarItem";
    case CT_MenuBar : return "CT_MenuBar";
    case CT_Menu : return "CT_Menu";
    case CT_TabBarTab : return "CT_TabBarTab";
    case CT_Slider : return "CT_Slider";
    case CT_ScrollBar : return "CT_ScrollBar";
    case CT_LineEdit : return "CT_LineEdit";
    case CT_SpinBox : return "CT_SpinBox";
    case CT_SizeGrip : return "CT_SizeGrip";
    case CT_TabWidget : return "CT_TabWidget";
    case CT_DialogButtons : return "CT_DialogButtons";
    case CT_HeaderSection : return "CT_HeaderSection";
    case CT_GroupBox : return "CT_GroupBox";
    case CT_MdiControls : return "CT_MdiControls";
    case CT_ItemViewItem : return "CT_ItemViewItem";
    case CT_CustomBase : return "CT_CustomBase";
  }

  return "CT_Unknown";
}

/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
