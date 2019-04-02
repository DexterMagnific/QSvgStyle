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

#include <QApplication>
#include <QWidget>
#include <QPainter>
#include <QRect>
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
#include <QStyleHints>
#include <QMetaObject>

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
#include <QRubberBand>
#include <QToolTip>
#include <QMenuBar>
#include <QHeaderView>

#include "QSvgCachedRenderer.h"
#include "ThemeConfig.h"
#include "StyleConfig.h"
#include "groups.h"

QSvgThemableStyle::QSvgThemableStyle()
  : QCommonStyle(),
    cls(QString(this->metaObject()->className())),
    themeRndr(nullptr),
    themeSettings(nullptr),
    styleSettings(nullptr),
    useConfigCache(true),
    useShapeCache(true),
    progresstimer(nullptr),
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
  styleSettings = nullptr;

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
  styleSettings->setUseCache(useConfigCache);

  loadUserTheme();
}

void QSvgThemableStyle::loadCustomStyleConfig(const QString& filename)
{
  if ( !QFile::exists(filename) )
    return;

  delete styleSettings;
  styleSettings = nullptr;

  styleSettings = new StyleConfig(filename);
  styleSettings->setUseCache(useConfigCache);

  qDebug() << "[QSvgStyle]" << "Loaded custom config file" << filename;

  loadTheme(styleSettings->getStyleSpec().theme);
}

void QSvgThemableStyle::loadBuiltinTheme()
{
  if ( curTheme == "<builtin>" )
    return;

  delete themeRndr;
  themeRndr = nullptr;

  delete themeSettings;
  themeSettings = nullptr;

  themeSettings = new ThemeConfig(":/default.cfg");
  themeRndr = new QSvgCachedRenderer();
  themeRndr->load(QString(":/default.svg"));

  curTheme = "<builtin>";
  qWarning() << "[QSvgStyle]" << "Loaded built in theme";
}

void QSvgThemableStyle::loadTheme(const QString& theme)
{
  if ( !curTheme.isEmpty() && (curTheme == theme) )
    return;

  if ( theme.isNull() || theme.isEmpty() || theme == "<builtin>" ) {
    loadBuiltinTheme();
    return;
  }

  QList<theme_spec_t> tlist = StyleConfig::getThemeList();
  Q_FOREACH(theme_spec_t t, tlist) {
    if ( theme == t.name ) {
      delete themeSettings;
      themeSettings = nullptr;

      themeSettings = new ThemeConfig(t.path);
      themeSettings->setUseCache(useConfigCache);

      delete themeRndr;
      themeRndr = nullptr;

      themeRndr = new QSvgCachedRenderer();
      themeRndr->load(
        QFileInfo(t.path).absolutePath().append("/").append(
        QFileInfo(t.path).completeBaseName().append(".svg")));

      curTheme = theme;
      qWarning() << "[QSvgStyle]" << "Loaded theme " << theme;

      return;
    }
  }

  // not found
  qWarning() << "[QSvgStyle]" << "Theme" << theme << "not found";
  loadBuiltinTheme();
  return;
}

void QSvgThemableStyle::loadUserTheme()
{
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
  themeRndr = nullptr;

  themeRndr = new QSvgCachedRenderer();
  themeRndr->load(filename);

  qDebug() << "[QSvgStyle] loaded custom SVG file" << filename;
}

void QSvgThemableStyle::loadCustomThemeConfig(const QString& filename)
{
  if ( !QFile::exists(filename) )
    return;

  delete themeSettings;
  themeSettings = nullptr;

  themeSettings = new ThemeConfig(filename);
  themeSettings->setUseCache(useConfigCache);

  curTheme = QString("custom:%1").arg(filename);
  qDebug() << "[QSvgStyle] loaded custom theme file" << filename;
}

void QSvgThemableStyle::setUseConfigCache(bool val)
{
  useConfigCache = val;

  if ( themeSettings )
    themeSettings->setUseCache(val);
  if ( styleSettings )
    styleSettings->setUseCache(val);
}

void QSvgThemableStyle::setUseShapeCache(bool val)
{
  useShapeCache = val;
  // TODO
}

bool QSvgThemableStyle::isContainerWidget(const QWidget * widget) const
{
  return !widget || (widget && (
    (widget->inherits("QFrame") &&
      !(
        widget->inherits("QTreeWidget") ||
        widget->inherits("QHeaderView") ||
        widget->inherits("QSplitter") ||
        widget->inherits("QToolBox")
      )
    ) ||
    widget->inherits("QGroupBox") ||
    widget->inherits("QTabWidget") ||
    widget->inherits("QDockWidget") ||
    widget->inherits("QMainWindow") ||
    widget->inherits("QDialog") ||
    widget->inherits("QDesktopWidget") ||
    widget->inherits("QToolBar") ||
    widget->inherits("QStatusBar") ||
    // Ok this one is not a container widget but we want to treat it as such
    // because it has its own groove
    widget->inherits("QProgressBar")
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

  // Enable menu tear off, enable translucency
  if ( QMenu *m = qobject_cast< QMenu* >(widget) ) {
    if ( getThemeTweak("specific.menu.forcetearoff").toBool() )
      m->setTearOffEnabled(true);
    m->setAttribute(Qt::WA_TranslucentBackground, true);
  }

#if 0
  // Enable menu tear off, enable translucency
  if ( QMenuBar *b = qobject_cast< QMenuBar* >(widget) ) {
    // FIXME
    //b->setAttribute(Qt::WA_TranslucentBackground, true);
    //b->setAutoFillBackground(true);
  }
#endif

  // QLineEdit inside a SpinBox of variant VA_SPINBOX_BUTTONS_OPPOSITE : center text
  if ( QLineEdit *l = qobject_cast< QLineEdit * >(widget) ) {
    if ( QSpinBox *s = qobject_cast< QSpinBox * >(l->parent()) )
      if ( s->buttonSymbols() != QAbstractSpinBox::NoButtons )
        if ( getThemeTweak("specific.spinbox.variant").toInt() ==
            VA_SPINBOX_BUTTONS_OPPOSITE )
          l->setAlignment(Qt::AlignHCenter);
  }

  // QToolBox: set background role to Window as in Tab Widgets
  if ( QToolBox *t = qobject_cast< QToolBox * >(widget) ) {
    t->setBackgroundRole(QPalette::Window);
  }

  // QComboBox: set background role to Button
  if ( QComboBox *c = qobject_cast< QComboBox * >(widget) ) {
    c->setBackgroundRole(QPalette::Button);
  }

  // QSpinBox: set background role to Button
  if ( QSpinBox *s = qobject_cast< QSpinBox * >(widget) ) {
    s->setBackgroundRole(QPalette::Button);
  }

  // QHeader: set background role to Button
  if ( QHeaderView *s = qobject_cast< QHeaderView * >(widget) ) {
    s->setBackgroundRole(QPalette::Button);
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
  const Qt::LayoutDirection dir = option->direction;
  const bool focus = option->state & State_HasFocus;
  Orientation orn = option->state & State_Horizontal ? Horizontal : Vertical;
  QBrush bg;

  // Get QSvgStyle configuration group used to render this element
  QString g = PE_group(e);

  // Configuration for group g
  frame_spec_t fs;
  interior_spec_t is;
  label_spec_t ls;
  indicator_spec_t ds;
  palette_spec_t ps;
  font_spec_t ts;

  if ( g.isEmpty() ) {
    // element not currently supported by QSvgStyle
    qWarning() << "[QSvgStyle] No group for" << e;
    QCommonStyle::drawPrimitive(e,option,p,widget);
    goto end;
  }

  // Get configuration for group
  fs = getFrameSpec(g);
  is = getInteriorSpec(g);
  ls = getLabelSpec(g);
  ds = getIndicatorSpec(g);
  ps = getPaletteSpec(g);
  ts = getFontSpec(g);

  bg = bgBrush(ps,option,widget, st);
  setupPainterFromFontSpec(p,ts, st);
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
  switch(static_cast<int>(e)) {
    case PE_Widget : {
      // nothing
      break;
    }
    case PE_PanelButtonBevel :
    case PE_PanelButtonCommand : {
      // Interior for push buttons
      computeButtonCapsule(widget,fs.hasCapsule,fs.capsuleH,fs.capsuleV);
      renderInterior(p,bg,r,fs,is,is.element+"-"+st,dir);

      bool isDefault = false;

      // Draw either default overlay or focus overlay
      if ( const QStyleOptionButton *opt =
           qstyleoption_cast<const QStyleOptionButton *>(option) ) {
        if ( opt->features & QStyleOptionButton::DefaultButton )
          isDefault = true;
      }

      if ( isDefault ) {
        renderInterior(p,QBrush(),r,fs,is,is.element+"-default",dir);
      } else if ( focus ) {
        renderInterior(p,QBrush(),r,fs,is,is.element+"-focused",dir);
      }
      break;
    }
    case PE_FrameDefaultButton : {
      // Frame for default buttons
      // use "default" status
      st = "default";
      computeButtonCapsule(widget,fs.hasCapsule,fs.capsuleH,fs.capsuleV);
      renderFrame(p,bg,r,fs,fs.element+"-"+st,dir);
      break;
    }
    case PE_FrameButtonBevel : {
      // Frame for push buttons
      computeButtonCapsule(widget,fs.hasCapsule,fs.capsuleH,fs.capsuleV);
      renderFrame(p,bg,r,fs,fs.element+"-"+st,dir);

      bool isDefault = false;

      // Draw either default overlay or focus overlay
      if ( const QStyleOptionButton *opt =
           qstyleoption_cast<const QStyleOptionButton *>(option) ) {
        if ( opt->features & QStyleOptionButton::DefaultButton )
          isDefault = true;
       }

      if ( isDefault ) {
        renderFrame(p,QBrush(),r,fs,fs.element+"-default",dir);
      } else if ( focus ) {
        renderFrame(p,QBrush(),r,fs,fs.element+"-focused",dir);
      }
      break;
    }
    case PE_PanelButtonTool : {
      // Interior for tool buttons
      // Do not compute capsule position for autoraise buttons
      if ( !(option->state & State_AutoRaise) )
        computeButtonCapsule(widget,fs.hasCapsule,fs.capsuleH,fs.capsuleV);
      renderInterior(p,bg,r,fs,is,is.element+"-"+st,dir);
      if ( focus ) {
        renderInterior(p,QBrush(),r,fs,is,is.element+"-focused",dir);
      }
      break;
    }
    case PE_FrameButtonTool : {
      // Frame for tool buttons
      // Do not compute capsule position for autoraise buttons
      if ( !(option->state & State_AutoRaise) )
        computeButtonCapsule(widget,fs.hasCapsule,fs.capsuleH,fs.capsuleV);
      renderFrame(p,bg,r,fs,fs.element+"-"+st,dir);
      if ( focus ) {
        renderFrame(p,QBrush(),r,fs,fs.element+"-focused",dir);
      }
      break;
    }
    case PE_PanelTipLabel : {
      // frame and interior for tool tips
      renderFrame(p,bg,r,fs,fs.element+"-"+st,dir);
      renderInterior(p,bg,r,fs,is,is.element+"-"+st,dir);
      break;
    }
    case PE_IndicatorRadioButton : {
      // a radio button (exclusive choice)
      // QSvgStyle: no pressed or toggled status for radio buttons
      st = (option->state & State_Enabled) ?
          (option->state & State_Selected) ? "toggled" :
          (option->state & State_MouseOver) ? "hovered" : "normal"
        : "disabled";
      if ( option->state & State_On )
        st = "checked-"+st;
      fs.hasFrame = false;
      renderIndicator(p,r,fs,is,ds,ds.element+"-"+st,dir);
      break;
    }
    case PE_IndicatorViewItemCheck : {
      // a check box inside view items
      st = (option->state & State_Enabled) ?
          (option->state & State_Selected) ? "toggled" : "normal"
        : "disabled";
      if ( option->state & State_On )
        st = "checked-"+st;
      else if ( option->state & State_NoChange )
        st = "tristate-"+st;
      fs.hasFrame = false;
      renderIndicator(p,r,fs,is,ds,ds.element+"-checkbox-"+st,dir);
      break;
    }
    case PE_IndicatorCheckBox : {
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
    case PE_IndicatorGroupBoxCheckMark : {
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

        st = (option->state & State_Enabled) ?
              (option->state & State_Selected) ? "toggled" : "normal"
                                               : "disabled";

        if ( opt->checked )
          st = "checked-"+st;

        fs.hasFrame = false;
        if ( opt->checkType == QStyleOptionMenuItem::Exclusive )
          renderIndicator(p,r,fs,is,ds,ds.element+"-radio-"+st,dir);
        else if ( opt->checkType == QStyleOptionMenuItem::NonExclusive )
          renderIndicator(p,r,fs,is,ds,ds.element+"-checkbox-"+st,dir);
      }
      break;
    }
    case PE_FrameFocusRect : {
      // Nothing. QSvgStyle supports focus on a per group basis
      break;
    }
    case PE_IndicatorBranch : {
      // FIXME branches overlap expand/collapse indicators
      if ( const QStyleOptionViewItem *opt =
        qstyleoption_cast<const QStyleOptionViewItem *>(option) ) {

        QStyleOptionViewItem o = *opt;
        o.state &= ~State_MouseOver;
        st = state_str(o.state,widget);
      }
      // Vertical branch
      if ( option->state & State_Sibling ) {
        // Maybe make the width/height configurable in a future version
        QRect r_vbranch = alignedRect(dir,Qt::AlignCenter,QSize(1,r.height()),r);
        renderElement(p,ds.element+"-vbranch-"+st,r_vbranch);
      }
      // Horizontal branch, depends on whether item is last one
      if ( option->state & State_Item ) {
        if ( option->state & State_Sibling ) {
          // not the last child
          QRect r_hbranch = alignedRect(dir,Qt::AlignRight|Qt::AlignVCenter,QSize(r.width()/2,1),r);
          renderElement(p,ds.element+"-hbranch-"+st,r_hbranch);
        } else {
          // last child
          QRect r_hbranch = alignedRect(dir,Qt::AlignRight|Qt::AlignVCenter,QSize(r.width()/2,1),r);
          renderElement(p,ds.element+"-hbranch-"+st,r_hbranch);
          QRect r_vbranch = alignedRect(dir,Qt::AlignHCenter|Qt::AlignTop,QSize(1,r.height()/2),r);
          renderElement(p,ds.element+"-vbranch-"+st,r_vbranch);
        }
      }
      // Expand/Collapse indicators
      fs.hasFrame = false;
      if (option->state & State_Children) {
        if (option->state & State_Open)
          renderIndicator(p,r,fs,is,ds,ds.element+"-collapse-"+st,dir);
        else
          renderIndicator(p,r,fs,is,ds,ds.element+"-expand-"+st,dir);
      }
      break;
    }
    case PE_IndicatorDockWidgetResizeHandle : {
      // resize handle of dock separators
      fs.hasFrame  = false;
      is.hasInterior = true;
      if ( orn == Horizontal ) {
        r = alignedRect(dir, Qt::AlignCenter, QSize(pixelMetric(PM_DockWidgetHandleExtent),h), r);
        renderInterior(p,QBrush(),r,fs,is,ds.element+"-hhandle-"+st,dir);
      } else {
        r = alignedRect(dir, Qt::AlignCenter, QSize(w,pixelMetric(PM_DockWidgetHandleExtent)), r);
        renderInterior(p,QBrush(),r,fs,is,ds.element+"-vhandle-"+st,dir);
      }
      break;
    }
    case PE_IndicatorTabClose : {
      // tab close buttons. used when icon theme does not supply one
      int variant = getThemeTweak("specific.tab.variant").toInt();
      if ( option->state & State_Selected ) {
        if ( (variant == VA_TAB_GROUP_NON_SELECTED) ||
             (variant == VA_TAB_INDIVIDUAL )
             ) {
          r.adjust(0,5,0,0);
        }
      }

      fs.hasFrame = false;
      // do not set toggled
      st = state_str(option->state & ~(State_On | State_Selected), widget);
      ds.size = pixelMetric(PM_TabCloseIndicatorWidth);
      renderIndicator(p,r,fs,is,ds,ds.element+"-tabclose-"+st, dir);
      break;
    }
    case PE_PanelScrollAreaCorner : {
      // scroll area corner
      // nothing
      break;
    }
    case PE_FrameMenu :  {
      // Frame for menus
      QStyleOption o(*option);
      o.state &= ~State_On;
      o.state |= State_Enabled;

      st = state_str(o.state,widget);

      renderFrame(p,bg,r,fs,fs.element+"-"+st,dir);
      break;
    }
    case PE_FrameWindow : {
      // Frame for windows
      renderFrame(p,bg,r,fs,fs.element+"-"+st,dir);
      break;
    }
    case PE_FrameTabBarBase : {
      // FIXME
      renderFrame(p,bg,r,fs,fs.element+"-"+st,dir);
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
          renderFrame(p,bg,r,fs,fs.element+"-sunken-"+st,dir);
        else
          renderFrame(p,bg,r,fs,fs.element+"-raised-"+st,dir);

        if ( !qobject_cast< const QTreeWidget* >(widget) ) {
          if ( opt->state & State_Sunken )
            renderInterior(p,bg,r,fs,is,is.element+"-sunken-"+st,dir);
          if ( opt->state & State_Raised )
            renderInterior(p,bg,r,fs,is,is.element+"-raised-"+st,dir);
        }
      }
      break;
    }
    case PE_FrameDockWidget : {
      // Frame for floating dock widgets
      renderFrame(p,bg,r,fs,fs.element+"-"+st,dir,orn);
      renderInterior(p,bg,r,fs,is,is.element+"-"+st,dir,orn);
      break;
    }
    case PE_FrameStatusBarItem : {
      // Frame for status bar items
      renderFrame(p,bg,r,fs,fs.element+"-"+st,dir);
      break;
    }
    case PE_FrameGroupBox : {
      // Frame and interior for group boxes contents
      renderFrame(p,bg,r,fs,fs.element+"-"+st,dir);
      renderInterior(p,bg,r,fs,is,is.element+"-"+st,dir);
      break;
    }
    case PE_FrameTabWidget : {
      // Frame and interior for tab widgets (contents)
      orn = Horizontal;
      if ( const QTabWidget *tw = qobject_cast<const QTabWidget *>(widget) ) {
        const QTabBar *tb = tw->tabBar();
        QTabBar::Shape s = tb->shape();
        if ( s == QTabBar::RoundedEast ||
             s == QTabBar::RoundedWest ||
             s == QTabBar::TriangularEast ||
             s == QTabBar::TriangularWest )
          orn = Vertical;

        QRect selectedRect = tb->tabRect(tb->currentIndex());
        fs.hasCuts = true;
        fs.h0 = tb->mapToParent(selectedRect.topLeft()).x();
        fs.h1 = tb->mapToParent(selectedRect.topRight()).x();
      } else
        orn = Horizontal;

      renderFrame(p,bg,r,fs,fs.element+"-"+st,dir,orn);
      renderInterior(p,bg,r,fs,is,is.element+"-"+st,dir,orn);
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
      bg = bgBrush(ps,option,widget,st);
      renderFrame(p,bg,r,fs,fs.element+"-"+st,dir);
      if ( focus ) {
        renderFrame(p,QBrush(),r,fs,fs.element+"-focused",dir);
      }
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
      bg = bgBrush(ps,option,widget,st);
      renderInterior(p,bg,r,fs,is,is.element+"-"+st,dir);
      if ( focus ) {
        renderInterior(p,QBrush(),r,fs,is,is.element+"-focused",dir);
      }
      break;
    }
    case PE_PanelToolBar : {
      // toolbar frame and interior
      renderFrame(p,bg,r,fs,fs.element+"-"+st,dir);
      renderInterior(p,bg,r,fs,is,is.element+"-"+st,dir);
      break;
    }
    case PE_IndicatorToolBarHandle : {
      // toolbar move handle
      // FIXME RTL handles
      if ( orn == Horizontal )
        renderElement(p,ds.element+"-vhandle-"+st,r);
      else
        renderElement(p,ds.element+"-hhandle-"+st,r);
      break;
    }
    case PE_IndicatorToolBarSeparator : {
      // toolbar separator
      // FIXME RTL separators
      if ( orn == Horizontal )
        renderElement(p,ds.element+"-vsep-"+st,r);
      else
        renderElement(p,ds.element+"-hsep-"+st,r);
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
      if ( getThemeTweak("specific.spinbox.variant").toInt() ==
         VA_SPINBOX_BUTTONS_OPPOSITE )
        renderIndicator(p,r,fs,is,ds,ds.element+"-right-"+st,dir);
      else
        renderIndicator(p,r,fs,is,ds,ds.element+"-up-"+st,dir);
      break;
    }
    case PE_IndicatorSpinDown : {
      // down spin box indicator
      if ( getThemeTweak("specific.spinbox.variant").toInt() ==
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
          renderIndicator(p,r,fs,is,ds,ds.element+"-asc-"+st,dir);
        else if (opt->sortIndicator == QStyleOptionHeader::SortUp)
          renderIndicator(p,r,fs,is,ds,ds.element+"-desc-"+st,dir);
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

      renderFrame(p,bg,r,fs,fs.element+"-"+st,dir);
      break;
    }
    case PE_PanelMenu : {
      // Interior of a menu
      // Qt::Menu has always State_None
      if ( option->state == State_None )
        st = "normal";
      renderInterior(p,bg,r,fs,is,is.element+"-"+st,dir);
      break;
    }
    case PE_IndicatorTabTear : {
      // FIXME
      break;
    }
    case PE_IndicatorArrowUp : {
      // Arrow up indicator
      fs.hasFrame = false;
      is.hasInterior = false;
      renderIndicator(p,r,fs,is,ds,ds.element+"-up-"+st,dir);
      break;
    }
    case PE_IndicatorArrowDown : {
      // Arrow down indicator
      fs.hasFrame = false;
      is.hasInterior = false;
      renderIndicator(p,r,fs,is,ds,ds.element+"-down-"+st,dir);
      break;
    }
    case PE_IndicatorArrowLeft : {
      // Arrow left indicator
      fs.hasFrame = false;
      is.hasInterior = false;
      renderIndicator(p,r,fs,is,ds,ds.element+"-left-"+st,dir);
      break;
    }
    case PE_IndicatorArrowRight : {
      // Arrow right indicator
      fs.hasFrame = false;
      is.hasInterior = false;
      renderIndicator(p,r,fs,is,ds,ds.element+"-right-"+st,dir);
      break;
    }
    case PE_IndicatorColumnViewArrow : {
      fs.hasFrame = false;
      is.hasInterior = false;
      renderIndicator(p,r,fs,is,ds,ds.element+"-child-"+st,dir);
      break;
    }
    case PE_IndicatorProgressChunk : {
      // The "filled" part of a progress bar
      // FIXME ce_progressbarcontents should use this one
      renderInterior(p,bg,r,fs,is,is.element+"-"+st,dir);
      break;
    }
    case PE_PanelItemViewRow : {
      // A row of a item view list
      break;
    }
    case PE_PanelItemViewItem : {
      // An item of a view item
        if ( const QStyleOptionViewItem *opt =
          qstyleoption_cast<const QStyleOptionViewItem *>(option) ) {

          if ( (opt->features & QStyleOptionViewItem::Alternate) && (st == "normal") )
            st = "alt-"+st;

          fs.hasCapsule = true;
          fs.capsuleH = 2;
          fs.capsuleV = 2;

          if ( opt->viewItemPosition == QStyleOptionViewItem::Beginning )
            fs.capsuleH = -1;
          else if ( opt->viewItemPosition == QStyleOptionViewItem::End )
            fs.capsuleH = 1;
          else if ( opt->viewItemPosition == QStyleOptionViewItem::Middle )
            fs.capsuleH = 0;

          // view items have their own brushes
          QBrush br = p->brush();
          QPen pn = p->pen();
          p->setBrush(opt->backgroundBrush);
          p->setPen(Qt::NoPen);
          p->drawRect(r);
          p->setBrush(br);
          p->setPen(pn);

          if ( st != "normal" ) {
            // only paint when state in not normal, otherwise keep
            // container widget background
            renderFrame(p,bg,r,fs,fs.element+"-"+st,dir);
            renderInterior(p,bg,r,fs,is,is.element+"-"+st,dir);
          }

          if ( focus ) {
            renderFrame(p,bg,r,fs,fs.element+"-focused",dir);
            renderInterior(p,QBrush(),r,fs,is,is.element+"-focused",dir);
          }
        }
      break;
    }
    case PE_PanelStatusBar : {
      renderInterior(p,bg,r,fs,is,is.element+"-"+st,dir);
      break;
    }
    default :
      qWarning() << "[QSvgStyle] Unhandled primitive" << e;
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
  const bool focus = option->state & State_HasFocus;
  QIcon::Mode icm = state_iconmode(option->state);
  QIcon::State ics = state_iconstate(option->state);
  Orientation orn = option->state & State_Horizontal ? Horizontal : Vertical;
  QBrush bg, fg;

  // Get QSvgStyle configuration group used to render this element
  QString g = CE_group(e);

  // Configuration for group g
  frame_spec_t fs;
  interior_spec_t is;
  label_spec_t ls;
  indicator_spec_t ds;
  palette_spec_t ps;
  font_spec_t ts;

  if ( g.isEmpty() ) {
    // element not currently supported by QSvgStyle
    qWarning() << "[QSvgStyle] No group for" << e;
    QCommonStyle::drawControl(e,option,p,widget);
    goto end;
  }

  // Get configuration for group
  fs = getFrameSpec(g);
  is = getInteriorSpec(g);
  ls = getLabelSpec(g);
  ds = getIndicatorSpec(g);
  ps = getPaletteSpec(g);
  ts = getFontSpec(g);

  bg = bgBrush(ps,option,widget, st);
  fg = fgBrush(ps,option,widget, st);
  setupPainterFromFontSpec(p,ts, st);
  fs.pressed = option->state & State_Sunken;

  switch (static_cast<int>(e)) {
    case CE_FocusFrame : {
      //qDebug() << "CE_FocusFrame" << (widget ? widget->objectName() : "???");
      break;
    }

    case CE_PushButtonBevel : {
      if ( const QStyleOptionButton *opt =
           qstyleoption_cast<const QStyleOptionButton *>(option) ) {

        QStyleOptionButton o(*opt);

        bool nonflat = false;

        if ( opt->features & QStyleOptionButton::Flat ) {
          // flat buttons: only applicable to normal state
          if ( (option->state & State_Enabled) &&
                ((option->state & State_Sunken) ||
                (option->state & State_On) ||
                (option->state & State_MouseOver))
              ) {
            nonflat = true;
          }
        } else {
          nonflat = true;
        }

        if ( opt->features & QStyleOptionButton::DefaultButton )
          drawPrimitive(PE_FrameDefaultButton,&o,p,widget);

        if ( nonflat ) {
          drawPrimitive(PE_FrameButtonBevel,&o,p,widget);
          drawPrimitive(PE_PanelButtonBevel,option,p,widget);
        }
      }

      break;
    }

    case CE_MenuTearoff : {
      renderElement(p,ds.element+"-tearoff-normal",r,10,0);
      break;
    }

    case CE_MenuItem : {
      if ( const QStyleOptionMenuItem *opt =
           qstyleoption_cast<const QStyleOptionMenuItem *>(option) ) {

        QStyleOptionMenuItem o(*opt);

        if (opt->menuItemType == QStyleOptionMenuItem::Separator) {
          if ( opt->text.isEmpty() ) {
            // Menu separator
            renderElement(p,ds.element+"-separator-normal",r);
          } else {
            // Menu section
            QStyleOptionButton o;
            o.QStyleOption::operator=(*opt);
            o.text = opt->text;
            o.icon = opt->icon;
            o.state |= State_On;
            o.iconSize = QSize(opt->maxIconWidth,opt->maxIconWidth);
            drawControl(CE_PushButton,&o,p,widget);
          }
        } else if (opt->menuItemType == QStyleOptionMenuItem::TearOff) {
          // Menu tear off
          drawControl(CE_MenuTearoff,opt,p,widget);
        } else {
          // Standard menu item
          // NOTE QSvgStyle ignores pressed state
          o.state &= ~State_Sunken;
          st = state_str(o.state,widget);

          // l[0] : text, l[1] shortcut if any
          const QStringList l = opt->text.split('\t');

          if ( getThemeTweak("specific.menu.usecapsule").toBool() && widget ) {
            // use V capsules. Lookup for menu item position
            fs.hasCapsule = true;
            fs.capsuleV = 2; // for now
            fs.capsuleH = 2;

            const QMenu *m = qobject_cast<const QMenu *>(widget);
            if ( m ) {
              int nb = m->actions().size();
              int pos = -1;

              foreach (QAction *a, m->actions()) {
                pos++;
                if ( !a->isSeparator() && (l.size() > 0) &&
                     (a->text() == l[0]) ) {
                  break; // found
                }
              }

              if ( nb == 1 )
                fs.capsuleV = 2; // only one
              else if ( pos == 0 )
                fs.capsuleV = -1; // first
              else if ( pos == nb-1 )
                fs.capsuleV = 1; // last
              else
                fs.capsuleV = 0; // middle
            }
          }

          renderFrame(p,bg,r,fs,fs.element+"-"+st,dir);
          renderInterior(p,bg,r,fs,is,is.element+"-"+st,dir);

          fs.hasCapsule = false;

          QRect rtext = r; // text rect (menu item label+shortcut)
          QRect rcheckmark, rchild;

          // determine positions
          if ( (l.size() > 0) && (opt->icon.isNull()) )
            // No icon ? leave space for icon anyway
            rtext.adjust(opt->maxIconWidth+ls.tispace,0,0,0);

          if ( opt->checkType != QStyleOptionMenuItem::NotCheckable )
            // remove room for check mark
            rtext = rtext.adjusted(0,0,-ds.size-pixelMetric(PM_CheckBoxLabelSpacing),0);
          if ( opt->menuItemType == QStyleOptionMenuItem::SubMenu )
            // remove room for sub menu arrow
            rtext = rtext.adjusted(0,0,-ds.size-pixelMetric(PM_CheckBoxLabelSpacing),0);

          rchild = QRect(r.topRight(),QSize(ds.size,h))
                    .translated(-ds.size-fs.right-ls.hmargin,0);
          rcheckmark = QRect(r.topRight(),QSize(ds.size,h))
                    .translated(-ds.size-fs.right-ls.hmargin,0);

          if (opt->menuItemType == QStyleOptionMenuItem::SubMenu)
            rcheckmark.translate(-ds.size-pixelMetric(PM_CheckBoxLabelSpacing),0);

          // translate to visual rects inside r
          rtext = visualRect(dir,r,rtext);
          rcheckmark = visualRect(dir,r,rcheckmark);
          rchild = visualRect(dir,r,rchild);

          // draw menu text (label+shortcut)
          if (l.size() > 0) {
            // menu label
            renderLabel(p,fg,
                        dir,
                        rtext,
                        fs,is,ls,
                        Qt::AlignLeft|Qt::AlignVCenter | Qt::TextShowMnemonic,
                        l[0],
                        opt->icon.pixmap(opt->maxIconWidth,icm,ics));
          }

          if (l.size() > 1) {
            // shortcut
            renderLabel(p,fg,
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
            o.rect = rchild;
            drawPrimitive(PE_IndicatorColumnViewArrow,&o,p);
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

        renderFrame(p,bg,option->rect,fs,fs.element+"-"+st,dir);
        renderInterior(p,bg,option->rect,fs,is,is.element+"-"+st,dir);
        renderLabel(p,fg,
                    dir,r,fs,is,ls,
                    Qt::AlignCenter | Qt::AlignVCenter | Qt::TextShowMnemonic,
                    opt->text);
      }

      break;
    }

    case CE_MenuBarEmptyArea : {
      // Menu bar interior
      // Qt:: Menu bar has always State_None
      if ( option->state == State_None )
        st = "normal";
      renderInterior(p,bg,r,fs,is,is.element+"-"+st,dir);
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

    case CE_RadioButton : {
      if ( const QStyleOptionButton *opt =
          qstyleoption_cast<const QStyleOptionButton *>(option) ) {

        QStyleOptionButton o = *opt;

        o.rect = subElementRect(SE_RadioButtonIndicator, opt, widget);
        drawPrimitive(PE_IndicatorRadioButton, &o, p, widget);

        o.rect = subElementRect(SE_RadioButtonContents, opt, widget);
        drawControl(CE_RadioButtonLabel, &o, p, widget);

        if ( focus ) {
          o.rect = subElementRect(SE_RadioButtonFocusRect, opt, widget);
          renderFrame(p,QBrush(),o.rect,fs,fs.element+"-focused",dir);
          renderInterior(p,QBrush(),o.rect,fs,is,is.element+"-focused",dir);
        }
      }

      break;
    }

    case CE_RadioButtonLabel : {
      if ( const QStyleOptionButton *opt =
           qstyleoption_cast<const QStyleOptionButton *>(option) ) {

        if ( focus )
          setupPainterFromFontSpec(p,ts, "focused");

        if ( opt->state & State_MouseOver )
          renderInterior(p,bg,r,fs,is,is.element+"-"+st,dir);

        renderLabel(p,fg,
                    dir,r,fs,is,ls,
                    Qt::AlignLeft | Qt::AlignVCenter | Qt::TextShowMnemonic,
                    opt->text,
                    opt->icon.pixmap(opt->iconSize,icm,ics));
      }
      break;
    }

    case CE_CheckBox : {
      if ( const QStyleOptionButton *opt =
          qstyleoption_cast<const QStyleOptionButton *>(option) ) {

        QStyleOptionButton o = *opt;

        o.rect = subElementRect(SE_CheckBoxIndicator, opt, widget);
        drawPrimitive(PE_IndicatorCheckBox, &o, p, widget);

        o.rect = subElementRect(SE_CheckBoxContents, opt, widget);
        drawControl(CE_CheckBoxLabel, &o, p, widget);

        if ( focus ) {
          o.rect = subElementRect(SE_CheckBoxFocusRect, opt, widget);
          renderFrame(p,QBrush(),o.rect,fs,fs.element+"-focused",dir);
          renderInterior(p,QBrush(),o.rect,fs,is,is.element+"-focused",dir);
        }
      }

      break;
    }

    case CE_CheckBoxLabel : {
      if ( const QStyleOptionButton *opt =
          qstyleoption_cast<const QStyleOptionButton *>(option) ) {

        if ( focus )
          setupPainterFromFontSpec(p,ts, "focused");

        if ( opt->state & State_MouseOver )
          renderInterior(p,bg,r,fs,is,is.element+"-"+st,dir);

        renderLabel(p,fg,
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

        if ( focus )
          setupPainterFromFontSpec(p,ts, "focused");

        if ( !opt->editable ) {
          // NOTE Editable label is rendered by an embedded QLineEdit
          // inside the QComboBox object, except icon
          // See QComboBox's qcombobox.cpp::updateLineEditGeometry()
          renderLabel(p,fg,
                      dir,r,fs,is,ls,
                      Qt::AlignLeft | Qt::AlignVCenter | Qt::TextShowMnemonic,
                      opt->currentText,
                      opt->currentIcon.pixmap(opt->iconSize,icm,ics));
        } else {
          // NOTE Non editable combo boxes: the embedded QLineEdit is not
          // able to draw the item icon, so do it here
          renderLabel(p,fg,
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
          // Remove "selected" state and replace it by "toggled" state
          o.state &= ~State_Sunken;
          o.state |= State_On;
          st = state_str(o.state,widget);
        }

        if ( (opt->shape == QTabBar::RoundedNorth) ||
             (opt->shape == QTabBar::TriangularNorth) ||
             (opt->shape == QTabBar::RoundedSouth) ||
             (opt->shape == QTabBar::TriangularSouth) )
          orn = Horizontal;
        else
          orn = Vertical;

        int variant = getThemeTweak("specific.tab.variant").toInt();
        int capsule = 2;
        int adjust = 0;
        fs.hasCapsule = true;

        if ( (variant == VA_TAB_INDIVIDUAL) ||
             (variant == VA_TAB_GROUP_NON_SELECTED) ) {
          adjust = 5;  // selected tab is higher than others
        }

        if ( variant == VA_TAB_GROUP_ALL ) {
          if (opt->position == QStyleOptionTab::Beginning)
            capsule = -1;
          else if (opt->position == QStyleOptionTab::Middle)
            capsule = 0;
          else if (opt->position == QStyleOptionTab::End)
            capsule = 1;
          else if (opt->position == QStyleOptionTab::OnlyOneTab)
            capsule = 2;
        }

        if ( variant == VA_TAB_GROUP_NON_SELECTED ) {
          if ( o.state & State_On || o.state & State_Selected )
            // selected
            capsule = 2;
          else {
            if (opt->position == QStyleOptionTab::Beginning) {
              if (opt->selectedPosition == QStyleOptionTab::NextIsSelected)
                capsule = 2;
              else
                capsule = -1;
            }
            else if (opt->position == QStyleOptionTab::End) {
              if (opt->selectedPosition == QStyleOptionTab::PreviousIsSelected)
                capsule = 2;
              else
                capsule = 1;
            }
            else if (opt->position == QStyleOptionTab::Middle) {
              if (opt->selectedPosition == QStyleOptionTab::PreviousIsSelected)
                capsule = -1;
              else if (opt->selectedPosition == QStyleOptionTab::NextIsSelected)
                capsule = 1;
              else if (opt->selectedPosition == QStyleOptionTab::NotAdjacent)
                capsule = 0;
              else
                capsule = 2;
            }
          }
        }

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
          if ( !(o.state & State_Selected) && !(o.state & State_On) ) {
            r.adjust(0,adjust,0,0);
          }
        }

        if ( (opt->shape == QTabBar::RoundedSouth) ||
             (opt->shape == QTabBar::TriangularSouth)
        ) {
          fs.capsuleV = 1;
          if ( !(o.state & State_Selected) && !(o.state & State_On) ) {
            r.adjust(0,0,0,-adjust);
          }
        }

        if ( (opt->shape == QTabBar::RoundedWest) ||
             (opt->shape == QTabBar::TriangularWest)
        ) {
          fs.capsuleV = 1;
          if ( !(o.state & State_Selected) && !(o.state & State_On) ) {
            r.adjust(adjust,0,0,0);
          }
        }

        if ( (opt->shape == QTabBar::RoundedEast) ||
             (opt->shape == QTabBar::TriangularEast)
        ) {
          fs.capsuleV = -1;
          if ( !(o.state & State_Selected) && !(o.state & State_On) ) {
            r.adjust(0,0,-adjust,0);
          }
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
                  // FIXME dirty hack
                  QPalette::ColorRole oldrole = contents->backgroundRole();
                  contents->setBackgroundRole(QPalette::Button);
                  bg = bgBrush(ps,option,contents, st);
                  contents->setBackgroundRole(oldrole);
                } else
                  bg.setStyle(Qt::NoBrush);

                break;
              }
            }
          }
        }

        renderInterior(p,bg,r,fs,is,is.element+"-"+st,dir,orn);
        renderFrame(p,bg,r,fs,fs.element+"-"+st,dir,orn);

        if ( focus ) {
          renderInterior(p,QBrush(),r,fs,is,is.element+"-focused",dir,orn);
          renderFrame(p,QBrush(),r,fs,fs.element+"-focused",dir,orn);
        }
      }

      break;
    }

    case CE_TabBarTabLabel : {
      if ( const QStyleOptionTab *opt =
           qstyleoption_cast<const QStyleOptionTab *>(option) ) {

        QStyleOptionTab o(*opt);

        if ( opt->state & State_Sunken ) {
          // Remove "pressed" state and replace it by "toggled" state
          o.state &= ~State_Sunken;
          o.state |= State_On;
          st = state_str(o.state,widget);

          icm = state_iconmode(o.state);
          ics = state_iconstate(o.state);
        }

        int variant = getThemeTweak("specific.tab.variant").toInt();
        int adjust = 0;

        if ( (variant == VA_TAB_INDIVIDUAL) ||
             (variant == VA_TAB_GROUP_NON_SELECTED) ) {
          adjust = 5;  // selected tab is higher than others
        }

        if ( !(o.state & State_Selected) && !(o.state & State_On) ) {

          if ( (opt->shape == QTabBar::RoundedNorth) ||
               (opt->shape == QTabBar::TriangularNorth)
               ) {
            r.adjust(0,adjust,0,0);
          }

          if ( (opt->shape == QTabBar::RoundedSouth) ||
               (opt->shape == QTabBar::TriangularSouth)
               ) {
            r.adjust(0,0,0,-adjust);
          }

          if ( (opt->shape == QTabBar::RoundedWest) ||
               (opt->shape == QTabBar::TriangularWest)
               ) {
            r.adjust(-adjust,0,0,0);
          }

          if ( (opt->shape == QTabBar::RoundedEast) ||
               (opt->shape == QTabBar::TriangularEast)
               ) {
            r.adjust(0,0,adjust,0);
          }
        }

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
                if ( contents )
                  fg = fgBrush(ps,option,contents, st);
                else
                  fg.setStyle(Qt::NoBrush);
                break;
              }
            }
          }
        }

        if ( focus )
          setupPainterFromFontSpec(p,ts, "focused");

        renderLabel(p,fg,
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

    case CE_ToolBoxTab : {
      drawControl(CE_ToolBoxTabShape,option,p,widget);
      drawControl(CE_ToolBoxTabLabel,option,p,widget);

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
                // FIXME dirty hack
                QPalette::ColorRole oldrole = contents->backgroundRole();
                contents->setBackgroundRole(QPalette::Button);
                bg = bgBrush(ps,option,contents, st);
                contents->setBackgroundRole(oldrole);
              } else
                bg.setStyle(Qt::NoBrush);
              break;
            }
          }
        }

        renderFrame(p,bg,option->rect,fs,fs.element+"-"+st,dir);
        renderInterior(p,bg,option->rect,fs,is,is.element+"-"+st,dir);
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
              if ( contents )
                fg = fgBrush(ps,option,contents, st);
              else
                fg.setStyle(Qt::NoBrush);
              break;
            }
          }
        }

        if ( focus )
          setupPainterFromFontSpec(p,ts, "focused");

        renderLabel(p,fg,
                    dir,r,fs,is,ls,
                    Qt::AlignCenter | Qt::TextShowMnemonic,
                    opt->text,
                    opt->icon.pixmap(pixelMetric(PM_SmallIconSize),icm,ics));
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
      renderFrame(p,bg,r,fs,fs.element+"-"+st,dir,orn);
      renderInterior(p,bg,r,fs,is,is.element+"-"+st,dir,orn);
      // FIXME render the progress bar contents frame here
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
        if ( getThemeTweak("specific.progressbar.variant") == VA_PROGRESSBAR_THIN ) {
          fs.hasFrame = false;
        }

        if ( focus )
          setupPainterFromFontSpec(p,ts, "focused");

        renderLabel(p,fg,
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
      // FIXME use pe_progressbarchunk
      // FIXME do not render the frame here
      // the progress indicator
      if ( const QStyleOptionProgressBar *opt =
           qstyleoption_cast<const QStyleOptionProgressBar *>(option) ) {

        QRect orig = r;

        // Get interior
        r = interiorRect(r,fs,is);
        r.getRect(&x,&y,&w,&h);

        if ( orn != Horizontal ) {
          // perform computations on horizontalized widget
          r = transposedRect(r);
          orig = transposedRect(orig);
          qSwap(x,y);
          qSwap(w,h);
        }

        is.px = pixelMetric(PM_ProgressBarChunkWidth);

        if ( opt->progress >= 0 ) {
          // Normal progress bar
          int empty = sliderPositionFromValue(opt->minimum,
                                              opt->maximum,
                                              opt->maximum-opt->progress,
                                              r.width(),
                                              false);

          r = r.adjusted(0,0,-empty,0); // filled area

          // add frame again
          r = r.adjusted(-fs.left,-fs.top,fs.right,fs.bottom);

          // right part of frame: only show if progress is full
          QRect cr = r; // Clip rect for r
          if ( opt->progress < opt->maximum )
            cr = cr.adjusted(0,0,-fs.right,0);

          if ( orn == Horizontal ) {
            r = visualRect(dir,orig,r);
            cr = visualRect(dir,orig,cr);
          }

          if ( opt->invertedAppearance ) {
            r = visualRect(Qt::RightToLeft,orig,r);
            cr = visualRect(Qt::RightToLeft,orig,cr);
          }

          if ( orn != Horizontal )
            cr = transposedRect(cr);

          p->save();
          p->setClipRect(cr);
          renderFrame(p,bg,
                         (orn != Horizontal) ? transposedRect(r) : r,
                         fs,fs.element+"-elapsed-"+st,
                         dir,
                         orn);
          p->restore();

          renderInterior(p,bg,
                         (orn != Horizontal) ? transposedRect(r) : r,
                         fs,is,
                         is.element+"-elapsed-"+st,
                         dir,
                         orn);
        } else { // busy progressbar
          int variant = getThemeTweak("specific.progressbar.busy.variant").toInt();

          QWidget *wd = (QWidget *)widget;
          int animcount = progressbars[wd];
          int pm = w;
          if ( is.px > 0 )
            pm = is.px; // cursor size

          switch (variant) {
            case VA_PROGRESSBAR_BUSY_WRAP : {
              r = r.adjusted(animcount%w,0,0,0);
              r.setWidth(pm);

              // add frame again
              r = r.adjusted(-fs.left,-fs.top,fs.right,fs.bottom);

              QRect cr = r; // Clip rect for r

              if ( r.x()+r.width()-1 > x+w-1 ) {
                // two half cursors
                // wrap busy indicator: second cursor size
                int pm2 = x+w-r.x();

                cr.setWidth(pm2);

                // left part of frame: only show if cursor is at the beginning
                if ( r.left() <= orig.left() )
                  cr = cr.adjusted(fs.left,0,0,0);

                // right part of frame: only show if cursor is at the end
                if ( r.right() >= orig.right() )
                  cr = cr.adjusted(0,0,-fs.right,0);

                if ( orn == Horizontal ) {
                  r = visualRect(dir,orig,r);
                  cr = visualRect(dir,orig,cr);
                }

                if ( opt->invertedAppearance ) {
                  r = visualRect(Qt::RightToLeft,orig,r);
                  cr = visualRect(Qt::RightToLeft,orig,cr);
                }

                p->save();
                p->setClipRect(r.x(),r.y(),pm2,r.height());
                renderInterior(p,bg,
                              (orn != Horizontal) ? transposedRect(r) : r,
                              fs,is,
                              is.element+"-elapsed-"+st,
                              dir,
                              orn);
                p->restore();

                // now the second cursor
                r = QRect(orig.x()-pm2,orig.y(),pm,h);
                // add frame again
                r = r.adjusted(-fs.left,-fs.top,fs.right,fs.bottom);
                cr = r;

                // left part of frame: only show if cursor is at the beginning
                if ( r.left() <= orig.left() )
                  cr = cr.adjusted(fs.left,0,0,0);

                // right part of frame: only show if cursor is at the end
                if ( r.right() >= orig.right() )
                  cr = cr.adjusted(0,0,-fs.right,0);

                if ( orn == Horizontal ) {
                  r = visualRect(dir,orig,r);
                  cr = visualRect(dir,orig,cr);
                }

                if ( opt->invertedAppearance ) {
                  r = visualRect(Qt::RightToLeft,orig,r);
                  cr = visualRect(Qt::RightToLeft,orig,cr);
                }

                p->save();
                p->setClipRect(orig.x(),orig.y(),pm,h);
                renderInterior(p,bg,
                              (orn != Horizontal) ? transposedRect(r) : r,
                              fs,is,
                              is.element+"-elapsed-"+st,
                              dir,
                              orn);
                p->restore();
              } else {
                // single cursor
                // left part of frame: only show if cursor is at the beginning
                if ( r.left() > orig.left() )
                  cr = cr.adjusted(fs.left,0,0,0);

                // right part of frame: only show if cursor is at the end
                if ( r.right() < orig.right() )
                  cr = cr.adjusted(0,0,-fs.right,0);

                if ( orn == Horizontal ) {
                  r = visualRect(dir,orig,r);
                  cr = visualRect(dir,orig,cr);
                }

                if ( opt->invertedAppearance ) {
                  r = visualRect(Qt::RightToLeft,orig,r);
                  cr = visualRect(Qt::RightToLeft,orig,cr);
                }

                if ( orn != Horizontal )
                  cr = transposedRect(cr);

                p->save();
                p->setClipRect(cr);
                renderFrame(p,bg,
                               (orn != Horizontal) ? transposedRect(r) : r,
                               fs,fs.element+"-elapsed-"+st,
                               dir,
                               orn);

                renderInterior(p,bg,
                              (orn != Horizontal) ? transposedRect(r) : r,
                              fs,is,
                              is.element+"-elapsed-"+st,
                              dir,
                              orn);
                p->restore();
              }
              break;
            }
            case VA_PROGRESSBAR_BUSY_BACKANDFORTH : {
              r = r.adjusted(animcount%(2*(w-pm+1)),0,0,0);
              if ( r.x()+pm-1 > x+w-1 )
                r.setX(x+2*(w-pm+1)-r.x());
              r.setWidth(pm);

              // add frame again
              r = r.adjusted(-fs.left,-fs.top,fs.right,fs.bottom);

              QRect cr = r; // Clip rect for r
              // left part of frame: only show if cursor is at the beginning
              if ( r.left() > orig.left() )
                cr = cr.adjusted(fs.left,0,0,0);

              // right part of frame: only show if cursor is at the end
              if ( r.right() < orig.right() )
                cr = cr.adjusted(0,0,-fs.right,0);

              if ( orn == Horizontal ) {
                r = visualRect(dir,orig,r);
                cr = visualRect(dir,orig,cr);
              }

              if ( opt->invertedAppearance ) {
                r = visualRect(Qt::RightToLeft,orig,r);
                cr = visualRect(Qt::RightToLeft,orig,cr);
              }

              if ( orn != Horizontal )
                cr = transposedRect(cr);

              p->save();
              p->setClipRect(cr);
              renderFrame(p,bg,
                             (orn != Horizontal) ? transposedRect(r) : r,
                             fs,fs.element+"-elapsed-"+st,
                             dir,
                             orn);

              renderInterior(p,bg,
                            (orn != Horizontal) ? transposedRect(r) : r,
                            fs,is,
                            is.element+"-elapsed-"+st,
                            dir,
                            orn);
              p->restore();

              break;
            }
            case VA_PROGRESSBAR_BUSY_FULLLENGTH :
            default: {
              int ni = animcount%pm;
              if ( getThemeTweak("specific.progressbar.busy.full.variant").toInt() ==
                   VA_PROGRESSBAR_BUSY_FULLLENGTH_DIRECTION_FWD )
                ni = pm-ni;
              r.adjust(-ni,0,w+ni,0);

              // add frame again
              r = r.adjusted(-fs.left,-fs.top,fs.right,fs.bottom);

              QRect cr = orig.adjusted(fs.left,0,-fs.right,0); // Clip rect for r

              if ( orn == Horizontal ) {
                r = visualRect(dir,orig,r);
                cr = visualRect(dir,orig,cr);
              }

              if ( opt->invertedAppearance ) {
                r = visualRect(Qt::RightToLeft,orig,r);
                cr = visualRect(Qt::RightToLeft,orig,cr);
              }

              if ( orn != Horizontal )
                cr = transposedRect(cr);

              // render whole frame
              renderFrame(p,bg,
                             (orn != Horizontal) ? transposedRect(orig) : orig,
                             fs,fs.element+"-elapsed-"+st,
                             dir,
                             orn);

              p->save();
              p->setClipRect(cr);
              renderInterior(p,bg,
                            (orn != Horizontal) ? transposedRect(r) : r,
                            fs,is,
                            is.element+"-elapsed-"+st,
                            dir,
                            orn);
              p->restore();

              break;
            }
          }
        }
      }

      break;
    }

    case CE_Splitter : {
      renderFrame(p,bg,option->rect,fs,fs.element+"-"+st,dir,orn);
      renderInterior(p,bg,option->rect,fs,is,is.element+"-"+st,dir,orn);

      break;
    }

    case CE_RubberBand : {
      if ( const QStyleOptionRubberBand *opt =
           qstyleoption_cast<const QStyleOptionRubberBand *>(option) ) {

      QStyleOptionRubberBand o(*opt);
      o.state &= ~State_MouseOver;
      st = state_str(o.state,widget);

      renderFrame(p,bg,option->rect,fs,fs.element+"-"+st,dir,orn);

      if ( opt->shape == QRubberBand::Rectangle )
        renderInterior(p,bg,option->rect,fs,is,is.element+"-"+st,dir,orn);
      }

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
        renderFrame(p,bg,option->rect,fs,fs.element+"-"+st,dir,orn);
        renderInterior(p,bg,option->rect,fs,is,is.element+"-"+st,dir,orn);
        // FIXME use own indicators
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
        renderFrame(p,bg,option->rect,fs,fs.element+"-"+st,dir,orn);
        renderInterior(p,bg,option->rect,fs,is,is.element+"-"+st,dir,orn);
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
        renderFrame(p,bg,option->rect,fs,fs.element+"-"+st,dir,orn);
        renderInterior(p,bg,option->rect,fs,is,is.element+"-"+st,dir,orn);
        if ( focus ) {
          renderFrame(p,QBrush(),option->rect,fs,fs.element+"-focused",dir,orn);
          renderInterior(p,QBrush(),option->rect,fs,is,is.element+"-focused",dir,orn);
        }
      }
      break;
    }

    case CE_HeaderEmptyArea :
    case CE_HeaderSection : {
      if ( const QStyleOptionHeader *opt =
           qstyleoption_cast<const QStyleOptionHeader *>(option) ) {

        fs.hasCapsule = true;
        fs.capsuleH = 2;
        fs.capsuleV = 2;

        if ( opt->position == QStyleOptionHeader::Beginning )
          fs.capsuleH = -1;
        if ( opt->position == QStyleOptionHeader::End )
          fs.capsuleH = 1;
        if ( opt->position == QStyleOptionHeader::Middle )
          fs.capsuleH = 0;

        renderFrame(p,bg,r,fs,fs.element+"-"+st,dir);
        renderInterior(p,bg,r,fs,is,is.element+"-"+st,dir);
      }
      break;
    }

    case CE_HeaderLabel : {
      if ( const QStyleOptionHeader *opt =
           qstyleoption_cast<const QStyleOptionHeader *>(option) ) {

        QStyleOptionHeader o(*opt);

        if ( focus )
          setupPainterFromFontSpec(p,ts, "focused");

        o.rect = subElementRect(SE_HeaderLabel,opt,widget);
        // FIXME does not honor icon alignment
        renderLabel(p,fg,
                    dir,o.rect,fs,is,ls,
                    opt->textAlignment,
                    opt->text,
                    opt->icon.pixmap(pixelMetric(PM_SmallIconSize),icm,ics));
        o.rect = subElementRect(SE_HeaderArrow,opt,widget);
        drawPrimitive(PE_IndicatorHeaderArrow,&o,p,widget);
      }
      break;
    }

    case CE_Header : {
      drawControl(CE_HeaderSection,option,p,widget);
      drawControl(CE_HeaderLabel,option,p,widget);

      break;
    }

    case CE_ToolBar : {
      renderFrame(p,bg,option->rect,fs,fs.element+"-"+st,dir,orn);
      renderInterior(p,bg,option->rect,fs,is,is.element+"-"+st,dir,orn);
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
      }
      break;
    }

    case CE_PushButtonLabel : {
      if ( const QStyleOptionButton *opt =
           qstyleoption_cast<const QStyleOptionButton *>(option) ) {

        bool isDefault = false;

        // Draw either default overlay or focus overlay
        if ( opt->features & QStyleOptionButton::DefaultButton )
          isDefault = true;

        if ( isDefault ) {
          st = "default";
        } else if ( focus ) {
          st = "focused";
        }

        setupPainterFromFontSpec(p,ts, st);

        if ( opt->features & QStyleOptionButton::HasMenu ) {
          QStyleOptionButton o(*opt);
          renderLabel(p,fg,
                      dir,r.adjusted(0,0,-ds.size-ls.tispace,0),fs,is,ls,
                      Qt::AlignCenter | Qt::AlignVCenter | Qt::TextShowMnemonic,
                      opt->text,
                      opt->icon.pixmap(opt->iconSize,icm,ics));
          o.rect = QRect(x+w-ds.size-fs.right-ls.hmargin,y,ds.size,h);
          drawPrimitive(PE_IndicatorArrowDown,&o,p,widget);
        } else {
          renderLabel(p,fg,
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

        if ( focus )
          setupPainterFromFontSpec(p,ts, "focused");

        if (opt->arrowType == Qt::NoArrow)
          renderLabel(p,fg,
                      dir,r,fs,is,ls,
                      Qt::AlignCenter | Qt::TextShowMnemonic,
                      opt->text,
                      opt->icon.pixmap(opt->iconSize,icm,ics),
                      opt->toolButtonStyle);
        else {
          if ( dir == Qt::LeftToRight )
            renderLabel(p,fg,
                        dir,r.adjusted(ds.size+ls.tispace,0,0,0),fs,is,ls,
                        Qt::AlignCenter | Qt::TextShowMnemonic,
                        opt->text,
                        opt->icon.pixmap(opt->iconSize,icm,ics),
                        opt->toolButtonStyle);
          else
            renderLabel(p,fg,
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

        renderFrame(p,bg,r,fs,fs.element+"-"+st,dir,orn);
        renderInterior(p,bg,r,fs,is,is.element+"-"+st,dir,orn);

        if ( opt->verticalTitleBar ) {
          p->save();
          r = transposedRect(r);
          p->translate(r.left(), r.top() + r.width());
          p->rotate(-90);
          p->translate(-r.left(), -r.top());
        }

        if ( focus )
          setupPainterFromFontSpec(p,ts, "focused");

        renderLabel(p,fg,
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
                        fs.element+"-hsep-"+st,
                        option->rect,
                        0,0);
        } else if (opt && (opt->frameShape == QFrame::VLine) ) {
          renderElement(p,
                        fs.element+"-vsep-"+st,
                        option->rect,
                        0,0);
        } else if (opt && (opt->frameShape != QFrame::NoFrame) ) {
          drawPrimitive(PE_Frame,opt,p,widget);
        }
      }

      break;
    }

    case CE_ItemViewItem : {
      if ( const QStyleOptionViewItem *opt =
           qstyleoption_cast<const QStyleOptionViewItem *>(option) ) {

        QStyleOptionViewItem o(*opt);

        drawPrimitive(PE_PanelItemViewItem,opt,p,widget);

        QRect rlabel = subElementRect(SE_ItemViewItemText,opt,widget);
        QRect rcheckmark = subElementRect(SE_ItemViewItemCheckIndicator,opt,widget);

        if ( opt->features & QStyleOptionViewItem::HasCheckIndicator ) {
          o.rect = rcheckmark;
          if ( opt->checkState == Qt::PartiallyChecked )
            o.state |= State_NoChange;
          if ( opt->checkState == Qt::Checked )
            o.state |= State_On;
          drawPrimitive(PE_IndicatorViewItemCheck,&o,p,widget);
        }

        // FIXME obey QTreeWidgetItem's foreground()
        p->setFont(opt->font);
        fg = opt->palette.foreground();
        renderLabel(p,fg,
                    dir,rlabel,fs,is,ls,
                    opt->displayAlignment,
                    opt->text,
                    opt->icon.pixmap(opt->decorationSize));
      }

      break;
    }

    case CE_GroupBoxTitle : {
      if ( const QStyleOptionGroupBox *opt =
           qstyleoption_cast<const QStyleOptionGroupBox *>(option) ) {

        QStyleOptionGroupBox o(*opt);

        // Draw frame and interior around title
        fs.hasCapsule = true;
        fs.capsuleH = 2;
        fs.capsuleV = -1;

        QRect r1 = subControlRect(CC_GroupBox,opt,SC_GroupBoxLabel,widget);

        // remove toggled for frame and interior
        st = state_str(o.state & ~State_On, widget);

        renderFrame(p,bg,r1,fs,fs.element+"-"+st,dir);
        renderInterior(p,bg,r1,fs,is,is.element+"-"+st,dir);

        if ( focus ) {
          renderFrame(p,QBrush(),r1,fs,fs.element+"-focused",dir);
          renderInterior(p,QBrush(),r1,fs,is,is.element+"-focused",dir);
        }

        if ( focus )
          setupPainterFromFontSpec(p,ts, "focused");

        // Draw title
        fs.hasCapsule = false;
        if ( opt->subControls & SC_GroupBoxCheckBox ) {
          if ( dir == Qt::LeftToRight )
            renderLabel(p,fg,
                        dir,
                        r1.adjusted(ds.size+pixelMetric(PM_CheckBoxLabelSpacing),0,0,0),
                        fs,is,ls,opt->textAlignment | Qt::TextShowMnemonic,
                        opt->text);
          else
            renderLabel(p,fg,
                        dir,
                        r1.adjusted(0,0,-ds.size-pixelMetric(PM_CheckBoxLabelSpacing),0),
                        fs,is,ls,opt->textAlignment | Qt::TextShowMnemonic,
                        opt->text);
          o.rect = subControlRect(CC_GroupBox,opt,SC_GroupBoxCheckBox,widget);
          drawPrimitive(static_cast<PrimitiveElement>(PE_IndicatorGroupBoxCheckMark),
                        &o,p,widget);
        } else
          renderLabel(p,fg,
                      dir,r1,fs,is,ls,
                      opt->textAlignment | Qt::TextShowMnemonic,
                      opt->text);
      }

      break;
    }

    default :
      qWarning() << "[QSvgStyle] Unhandled control" << e;
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
  const Qt::LayoutDirection dir = option->direction;
  const bool focus = option->state & State_HasFocus;
  Orientation orn = option->state & State_Horizontal ? Horizontal : Vertical;
  QBrush bg, fg;

  // Get QSvgStyle configuration group used to render this element
  QString g = CC_group(control);

  // Configuration for group g
  frame_spec_t fs;
  interior_spec_t is;
  label_spec_t ls;
  indicator_spec_t ds;
  palette_spec_t ps;
  font_spec_t ts;

  if ( g.isEmpty() ) {
    // element not currently supported by QSvgStyle
    qWarning() << "[QSvgStyle] No group for" << control;
    QCommonStyle::drawComplexControl(control,option,p,widget);
    goto end;
  }

  // Get configuration for group
  fs = getFrameSpec(g);
  is = getInteriorSpec(g);
  ls = getLabelSpec(g);
  ds = getIndicatorSpec(g);
  ps = getPaletteSpec(g);
  ts = getFontSpec(g);

  bg = bgBrush(ps,option,widget, st);
  fg = fgBrush(ps,option,widget, st);
  setupPainterFromFontSpec(p,ts, st);

  switch (control) {
    case CC_ToolButton : {
      if ( const QStyleOptionToolButton *opt =
          qstyleoption_cast<const QStyleOptionToolButton *>(option) ) {

        QStyleOptionToolButton o(*opt);

        // FIXME if button has no frame, no margins to add
        QRect dropRect = subControlRect(CC_ToolButton,opt,SC_ToolButtonMenu,widget)
            .marginsAdded(QMargins(fs.left,fs.top,fs.right,fs.bottom));
        QRect buttonRect = subControlRect(CC_ToolButton,opt,SC_ToolButton,widget)
            .marginsAdded(QMargins(fs.left,fs.top,fs.right,fs.bottom));

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
        }

        // draw spin buttons
        o.frame = false;
        if (opt->buttonSymbols != QAbstractSpinBox::NoButtons) {
          o.state = opt->state;
          o.state &= ~(State_Sunken | State_Selected | State_MouseOver);
          if ( opt->activeSubControls & SC_SpinBoxUp )
            o.state = opt->state;
          st = state_str(o.state,widget);
          o.rect = subControlRect(CC_SpinBox,opt,SC_SpinBoxUp,widget);
          drawPrimitive(PE_PanelButtonBevel,&o,p,widget);

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
          drawPrimitive(PE_PanelButtonBevel,&o,p,widget);

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
        }

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
        renderFrame(p,bg,o.rect,fs,fs.element+"-"+st,dir,orn);
        renderInterior(p,bg,o.rect,fs,is,is.element+"-"+st,dir,orn);

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

        // FIXME should we clip the rect instead ?
        renderFrame(p,bg,empty,fs,fs.element+"-"+st,dir,orn);
        renderInterior(p,bg,empty,fs,is,is.element+"-"+st,dir,orn);

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

        // FIXME should we clip the rect instead ?
        renderFrame(p,bg,full,fs,fs.element+"-elapsed-"+st,dir,orn);
        renderInterior(p,bg,full,fs,is,is.element+"-elapsed-"+st,dir,orn);

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
                renderElement(p,ds.element+"-htick-"+st,
                              QRect(groove.x()+pos,groove.y()+groove.height()+tickOffset,
                                    1,3)
                              );
              }
              if ( tickPos & QSlider::TicksAbove ) {
                renderElement(p,ds.element+"-htick-"+st,
                              QRect(groove.x()+pos,groove.y()-tickOffset-3,
                                    1,3)
                              );
              }
            } else {
              if ( tickPos & QSlider::TicksRight ) {
                renderElement(p,ds.element+"-vtick-"+st,
                              QRect(groove.x()+groove.width()+tickOffset,groove.y()+pos,
                                    3,1)
                              );
              }
              if ( tickPos & QSlider::TicksLeft ) {
                renderElement(p,ds.element+"-vtick-"+st,
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
        if ( opt->tickPosition == QSlider::NoTicks )
          renderInterior(p,bg,o.rect,fs,is,is.element+"-cursor-tickless-"+st,dir,orn);
        else
          renderInterior(p,bg,o.rect,fs,is,is.element+"-cursor-"+st,dir,orn);
        if ( focus ) {
          if ( opt->tickPosition == QSlider::NoTicks )
            renderInterior(p,QBrush(),o.rect,fs,is,is.element+"-cursor-tickless-focused",dir,orn);
          else
            renderInterior(p,QBrush(),o.rect,fs,is,is.element+"-cursor-focused",dir,orn);
        }
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
        renderInterior(p,bg,o.rect,fs,is,is.element+"-"+st,dir,orn);

        // TODO make configurable
        int startAngle = -60;
        int endAngle = 230;

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
          renderInterior(p,bg,o.rect,fs,is,is.element+"-ticks-"+st,dir,orn);
//           p->restore();
        }

        // handle
        const int range = endAngle-startAngle;
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
        renderInterior(p,bg,o.rect,fs,is,is.element+"-handle-"+st,dir,orn);
        p->restore();
      }

      break;
    }

    case CC_GroupBox : {
      if ( const QStyleOptionGroupBox *opt =
           qstyleoption_cast<const QStyleOptionGroupBox *>(option) ) {

        QStyleOptionGroupBox o(*opt);

        QRect r1;
        r1 = subControlRect(CC_GroupBox,&o,SC_GroupBoxFrame,widget);

        // Draw frame and interior around contents
        // remove toggled for checkable group boxes
        o.state &= ~State_On;
        if ( !(opt->features && QStyleOptionFrame::Flat) ) {
          if ( (opt->subControls & SC_GroupBoxCheckBox) && !(opt->state & State_On) ) {
            // not checked -> draw contents frame with disabled state
            //st = "disabled";
            o.state &= ~State_Enabled;
          }
          o.rect = r1;
          drawPrimitive(PE_FrameGroupBox,&o,p,widget);
          o = *opt;
        }

        // Draw title
        if ( opt->subControls & SC_GroupBoxLabel)
          drawControl(static_cast<ControlElement>(CE_GroupBoxTitle),&o,p,widget);
      }

      break;
    }

    case CC_TitleBar : {
      if ( const QStyleOptionTitleBar *opt =
           qstyleoption_cast<const QStyleOptionTitleBar *>(option) ) {

        if ( styleHint(SH_TitleBar_NoBorder) )
          fs.hasFrame = false;
        else if ( !(opt->titleBarState & Qt::WindowMinimized) )
          fs.bottom = 0;

        if (opt->titleBarState & Qt::WindowActive)
          st = "hovered";
        else
          st = "normal";

        const int btnsz = pixelMetric(PM_TitleBarButtonSize,opt,widget);
        const int iconsz = pixelMetric(PM_TitleBarButtonIconSize,opt,widget);
        const QIcon::Mode icm = state_iconmode(option->state);
        const QIcon::State ics = state_iconstate(option->state);

        // interior
        renderInterior(p,bg,r,fs,is,is.element+"-"+st,dir);

        if ( focus )
          setupPainterFromFontSpec(p,ts, "focused");

        // title
        ls.hmargin = 0; // this has been taken into account in SC_TitleBarLabel
        QRect rtext = subControlRect(CC_TitleBar, opt, SC_TitleBarLabel, widget);
        renderLabel(p,fg,
                    dir,rtext,fs,is,ls,
                    Qt::AlignCenter | Qt::TextSingleLine,
                    opt->text);

        QPixmap pm;

        // close button
        if ( (opt->subControls & SC_TitleBarCloseButton) &&
             opt->titleBarFlags & Qt::WindowSystemMenuHint ) {
          const QRect rbtn = subControlRect(CC_TitleBar, opt, SC_TitleBarCloseButton, widget);
          if ( (opt->titleBarFlags & Qt::WindowType_Mask) == Qt::Tool
              || qobject_cast<const QDockWidget *>(widget)
              )
            pm = standardIcon(SP_DockWidgetCloseButton, opt, widget).pixmap(btnsz,btnsz,icm,ics);
          else
            pm = standardIcon(SP_TitleBarCloseButton, opt, widget).pixmap(btnsz,btnsz,icm,ics);

          drawItemPixmap(p, rbtn, Qt::AlignCenter, pm);
        }

        // max button
        if ( (opt->subControls & SC_TitleBarMaxButton) &&
             opt->titleBarFlags & Qt::WindowMaximizeButtonHint &&
             !(opt->titleBarState & Qt::WindowMaximized) ) {
          const QRect rbtn = subControlRect(CC_TitleBar, opt, SC_TitleBarMaxButton, widget);
          pm = standardIcon(SP_TitleBarMaxButton, opt, widget).pixmap(btnsz,btnsz,icm,ics);

          drawItemPixmap(p, rbtn, Qt::AlignCenter, pm);
        }

        // min button
        if ( (opt->subControls & SC_TitleBarMinButton) &&
             opt->titleBarFlags & Qt::WindowMinimizeButtonHint &&
             !(opt->titleBarState & Qt::WindowMinimized) ) {
          const QRect rbtn = subControlRect(CC_TitleBar, opt, SC_TitleBarMinButton, widget);
          pm = standardIcon(SP_TitleBarMinButton, opt, widget).pixmap(btnsz,btnsz,icm,ics);

          drawItemPixmap(p, rbtn, Qt::AlignCenter, pm);
        }

        // normal (restore) button
        if ( (opt->subControls & SC_TitleBarNormalButton) &&
             (((opt->titleBarFlags & Qt::WindowMinimizeButtonHint) &&
               (opt->titleBarState & Qt::WindowMinimized)) ||
              ((opt->titleBarFlags & Qt::WindowMaximizeButtonHint) &&
               (opt->titleBarState & Qt::WindowMaximized))) ) {
          const QRect rbtn = subControlRect(CC_TitleBar, opt, SC_TitleBarNormalButton, widget);
          pm = standardIcon(SP_TitleBarNormalButton, opt, widget).pixmap(btnsz,btnsz,icm,ics);

          drawItemPixmap(p, rbtn, Qt::AlignCenter, pm);
        }

        // shade button
        if ( (opt->subControls & SC_TitleBarShadeButton) &&
             opt->titleBarFlags & Qt::WindowShadeButtonHint &&
             !(opt->titleBarState & Qt::WindowMinimized) ) {
          const QRect rbtn = subControlRect(CC_TitleBar, opt, SC_TitleBarShadeButton, widget);
          pm = standardIcon(SP_TitleBarShadeButton, opt, widget).pixmap(btnsz,btnsz,icm,ics);

          drawItemPixmap(p, rbtn, Qt::AlignCenter, pm);
        }

        // unshade button
        if ( (opt->subControls & SC_TitleBarUnshadeButton) &&
             opt->titleBarFlags & Qt::WindowShadeButtonHint &&
             opt->titleBarState & Qt::WindowMinimized ) {
          const QRect rbtn = subControlRect(CC_TitleBar, opt, SC_TitleBarUnshadeButton, widget);
          pm = standardIcon(SP_TitleBarUnshadeButton, opt, widget).pixmap(btnsz,btnsz,icm,ics);

          drawItemPixmap(p, rbtn, Qt::AlignCenter, pm);
        }

        // help button
        if ( (opt->subControls & SC_TitleBarContextHelpButton) &&
             opt->titleBarFlags & Qt::WindowContextHelpButtonHint &&
             opt->titleBarState & Qt::WindowMinimized ) {
          const QRect rbtn = subControlRect(CC_TitleBar, opt, SC_TitleBarContextHelpButton, widget);
          pm = standardIcon(SP_TitleBarContextHelpButton, opt, widget).pixmap(btnsz,btnsz,icm,ics);

          drawItemPixmap(p, rbtn, Qt::AlignCenter, pm);
        }

        // system menu button
        if ( (opt->subControls & SC_TitleBarSysMenu) &&
             opt->titleBarFlags & Qt::WindowSystemMenuHint ) {
          const QRect rbtn = subControlRect(CC_TitleBar, opt, SC_TitleBarSysMenu, widget);
          //pm = standardIcon(SP_TitleBarMenuButton, opt, widget).pixmap(pmsz,pmsz,icm,ics);
          pm = opt->icon.pixmap(iconsz,iconsz,icm,ics);

          drawItemPixmap(p, rbtn, Qt::AlignCenter, pm);
        }
      }
      break;
    }

    default :
      qWarning() << "[QSvgStyle] Unhandled complex control" << control;
      QCommonStyle::drawComplexControl(control,option,p,widget);
  }

end:
  emit(sig_drawComplexControl_end(CC_str(control)));
}

int QSvgThemableStyle::pixelMetric(PixelMetric metric, const QStyleOption * option, const QWidget * widget) const
{
  const QFontMetrics fm = widget ? widget->fontMetrics() :
                             option ? option->fontMetrics :
                                      qApp->fontMetrics();

  switch (metric) {
    // Indicator width (checkboxes, radios)
    case PM_IndicatorWidth :
    case PM_IndicatorHeight :
      return getIndicatorSpec(PE_group(PE_IndicatorCheckBox)).size;
    case PM_ExclusiveIndicatorWidth :
    case PM_ExclusiveIndicatorHeight :
      return getIndicatorSpec(PE_group(PE_IndicatorRadioButton)).size;

    // drop down menu + spin box up/down/plus/minus button size (not indicator)
    case PM_MenuButtonIndicator :
      return getThemeTweak("specific.dropdown.size").toInt();

    // Custom layout margins
    case PM_LayoutLeftMargin :
      return getThemeTweak("specific.layoutmargins.left").toInt();
    case PM_LayoutRightMargin :
      return getThemeTweak("specific.layoutmargins.right").toInt();
    case PM_LayoutTopMargin :
      return getThemeTweak("specific.layoutmargins.top").toInt();
    case PM_LayoutBottomMargin :
      return getThemeTweak("specific.layoutmargins.bottom").toInt();
    case PM_LayoutHorizontalSpacing :
      return getThemeTweak("specific.layoutmargins.hspace").toInt();
    case PM_LayoutVerticalSpacing :
      return getThemeTweak("specific.layoutmargins.vspace").toInt();
    // These two are obsolete but still used by many apps
    case PM_DefaultLayoutSpacing :
      return qMax(
            getThemeTweak("specific.layoutmargins.hspace").toInt(),
            getThemeTweak("specific.layoutmargins.vspace").toInt()
            );
    case PM_DefaultChildMargin :
      return qMax(
            qMax(getThemeTweak("specific.layoutmargins.left").toInt(),
                 getThemeTweak("specific.layoutmargins.right").toInt()),
            qMax(getThemeTweak("specific.layoutmargins.top").toInt(),
                 getThemeTweak("specific.layoutmargins.bottom").toInt())
            );

    case PM_MenuBarPanelWidth :
      return getFrameSpec(PE_group(PE_PanelMenuBar)).width;

    case PM_ComboBoxFrameWidth :
      return getFrameSpec(PE_group(PE_Frame)).width;

    // These are the 'interior' margins of the menu bar
    case PM_MenuBarVMargin : return 0;
    case PM_MenuBarHMargin :
      return getThemeTweak("specific.menubar.hspace").toInt();
    // Spacing between menu bar items
    case PM_MenuBarItemSpacing :
      return getThemeTweak("specific.menubar.space").toInt();

    // Popup menu tear off height
    case PM_MenuTearoffHeight :
      return getThemeTweak("specific.menu.tearoff.height").toInt();

    case PM_ToolBarFrameWidth :
      return getFrameSpec(PE_group(PE_PanelToolBar)).width;
    // Margin between toolbar frame and buttons
    case PM_ToolBarItemMargin :
      return getThemeTweak("specific.toolbar.itemmargin").toInt();
    // The "move" handle of a toolbar
    case PM_ToolBarHandleExtent :
      return getThemeTweak("specific.toolbar.handle.width").toInt();
    // Item separator size
    case PM_ToolBarSeparatorExtent :
      return getThemeTweak("specific.toolbar.separator.width").toInt();
    // No spacing between items
    case PM_ToolBarItemSpacing :
      return getThemeTweak("specific.toolbar.space").toInt();
    // The "extension" button size on partial toolbars
    case PM_ToolBarExtensionExtent :
      return getThemeTweak("specific.toolbar.extension.width").toInt();
    case PM_ToolBarIconSize :
      return getThemeTweak("specific.toolbar.icon.size").toInt();

    case PM_TabBarTabHSpace : return 0;
    case PM_TabBarTabVSpace : return 0;
    case PM_TabBarScrollButtonWidth : return 20;
    case PM_TabBarBaseHeight : return 0;
    case PM_TabBarBaseOverlap : return 00;
    case PM_TabBarTabOverlap : return 0;
    case PM_TabBarTabShiftHorizontal : return 0;
    case PM_TabBarTabShiftVertical : return 0;
    case PM_TabBarIconSize : return 16;
    case PM_TabBar_ScrollButtonOverlap: return 0;
    case PM_TabCloseIndicatorHeight :
    case PM_TabCloseIndicatorWidth :
      return getIndicatorSpec(PE_group(PE_IndicatorTabClose)).size;

    // Icon sizes
    case PM_SmallIconSize : return 16;
    case PM_LargeIconSize : return 32;
    case PM_ButtonIconSize : return 16;
    case PM_ListViewIconSize : return 16;
    case PM_IconViewIconSize : return 64;
    case PM_MessageBoxIconSize : return 64;
    case PM_TitleBarButtonIconSize : return 32;
    case PM_TitleBarButtonSize :
      return getIndicatorSpec(PE_group(PE_FrameWindow)).size;

    // Button related
    case PM_ButtonMargin :
    case PM_FocusFrameHMargin :
    case PM_FocusFrameVMargin :
    case PM_ButtonShiftHorizontal :
    case PM_ButtonShiftVertical : return 0;

    case PM_CheckBoxLabelSpacing :
    case PM_RadioButtonLabelSpacing :
      return getThemeTweak("specific.radiocheckbox.label.tispace").toInt();

    case PM_SplitterWidth : return 6;

    case PM_ScrollBarExtent :
      return getThemeTweak("specific.scrollbar.thickness").toInt();
    case PM_ScrollBarSliderMin :
      return getThemeTweak("specific.scrollbar.slider.minsize").toInt();

    case PM_SliderThickness :
      return getThemeTweak("specific.slider.thickness").toInt();
    case PM_SliderLength :
    case PM_SliderControlThickness :
      return getThemeTweak("specific.slider.cursor.size").toInt();
    case PM_SliderTickmarkOffset :
      return getThemeTweak("specific.slider.ticks.offset").toInt();
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
      return getThemeTweak("specific.progressbar.chunk.width").toInt();

    case PM_DefaultFrameWidth :
      // NOTE used by QLineEdit, QTabWidget and QMdiArea
      if ( qobject_cast< const QLineEdit* >(widget) )
        return getFrameSpec(PE_group(PE_FrameLineEdit)).width;
      else if ( qobject_cast< const QTabWidget* >(widget) )
        return getFrameSpec(PE_group(PE_FrameTabWidget)).width;
      else
        return getFrameSpec(PE_group(PE_Frame)).width;

    case PM_SpinBoxFrameWidth:
      return getFrameSpec(CC_group(CC_SpinBox)).width;

    case PM_MenuPanelWidth :
    case PM_MenuDesktopFrameWidth :
      return getFrameSpec(PE_group(PE_FrameMenu)).width;
    case PM_MenuHMargin :
    case PM_MenuVMargin : return 0;
    case PM_SubMenuOverlap : return 0;

    case PM_ToolTipLabelFrameWidth :
      return getFrameSpec(PE_group(PE_PanelTipLabel)).width;

    case PM_DockWidgetTitleMargin : {
      // NOTE used by QDockWidgetLayout to compute title size
      int ret = 0;
      ret += getLabelSpec(CE_group(CE_DockWidgetTitle)).vmargin;
      ret += getFrameSpec(CE_group(CE_DockWidgetTitle)).width;
      return ret;
    }
    break;
    case PM_DockWidgetFrameWidth :
      /// NOTE Only used when dock is floating
      return getFrameSpec(PE_group(PE_FrameDockWidget)).width;
    case PM_DockWidgetTitleBarButtonMargin : {
      // NOTE Dock widget button "margins" are used in their sizeHint() calculation
      // CHEAT: they must include both the label margins and the frame size
      // since their sizeHint() does not take into account frame size
      int ret = 0;
      if ( styleHint(QStyle::SH_DockWidget_ButtonsHaveFrame, nullptr, widget) )
        ret += getFrameSpec(CC_group(CC_ToolButton)).width;
      ret += getLabelSpec(CC_group(CC_ToolButton)).vmargin;
      return ret;
    }
    break;
    case PM_DockWidgetSeparatorExtent:
      return getThemeTweak("specific.dock.separator.size").toInt();
    case PM_DockWidgetHandleExtent:
      return getThemeTweak("specific.dock.handle.width").toInt();

    case PM_TextCursorWidth : return 1;

    case PM_SizeGripSize :
      return getIndicatorSpec(CE_group(CE_SizeGrip)).size;

    // Header sort indicator size
    case PM_HeaderMarkSize :
      return getIndicatorSpec(CE_group(CE_Header)).size;
    case PM_HeaderGripMargin :
      return getFrameSpec(CE_group(CE_Header)).width*2;
    case PM_HeaderMargin : return 0;

    // Mdi Windows
    case PM_MdiSubWindowFrameWidth :
      return getFrameSpec(PE_group(PE_FrameWindow)).width;
    case PM_TitleBarHeight : {
      if ( const QStyleOptionTitleBar *opt =
           qstyleoption_cast<const QStyleOptionTitleBar *>(option) ) {
        const QString group = PE_group(PE_FrameWindow);
        frame_spec_t fs = getFrameSpec(group);
        interior_spec_t is = getInteriorSpec(group);
        label_spec_t ls = getLabelSpec(group);

        if ( styleHint(SH_TitleBar_NoBorder) )
          fs.hasFrame = false;
        else if ( !(opt->titleBarState & Qt::WindowMinimized) )
          // non minimized ?
          fs.bottom = 0;

        QSize sz = sizeFromContents(fm,fs,is,ls,
                                  opt->text.isEmpty() ? "W" : opt->text,
                                  pixelMetric(PM_TitleBarButtonIconSize));

        return sz.height();
      } else {
        return fm.height();
      }
    }

    // We're happy with the values returned by QCommonStyle
    case PM_MaximumDragDistance :
    case PM_TreeViewIndentation :
    case PM_ScrollView_ScrollBarOverlap :
    case PM_ScrollView_ScrollBarSpacing :
    case PM_HeaderDefaultSectionSizeHorizontal :
    case PM_HeaderDefaultSectionSizeVertical :
    case PM_MdiSubWindowMinimizedWidth :
      return QCommonStyle::pixelMetric(metric,option,widget);

    default :
      qWarning() << "[QSvgStyle] Unhandled pixelmetric" << metric;
      return QCommonStyle::pixelMetric(metric,option,widget);
  }
}

int QSvgThemableStyle::styleHint(StyleHint hint, const QStyleOption * option, const QWidget * widget, QStyleHintReturn * returnData) const
{
  switch (hint) {
    case SH_ComboBox_ListMouseTracking :
    case QStyle::SH_Menu_SupportsSections :
    case SH_Menu_MouseTracking :
    case SH_MenuBar_MouseTracking : return true;

    case SH_DockWidget_ButtonsHaveFrame : return false;

    case SH_TabBar_Alignment : return Qt::AlignCenter;

    case SH_TitleBar_NoBorder : return true;

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
      if ( const QStyleOptionFrame *opt =
           qstyleoption_cast<const QStyleOptionFrame *>(option) ) {
        const QLineEdit *l = qobject_cast<const QLineEdit *>(widget);

        if ( !opt->lineWidth  ) // that's how QLineEdit tells us there is a frame
          fs.hasFrame = false;

        s = sizeFromContents(fm,fs,is,ls,
                             l && !l->placeholderText().isEmpty() ?
                               l->placeholderText() : "Some minimal text", 0);

        if ( l && l->isClearButtonEnabled() ) {
          s += QSize(32+ls.tispace,0); /* QLineEdit hardcoded clear button size */
          s = s.expandedTo(QSize(0,ds.size+fs.top+fs.bottom));
        }

        s += QSize(4,0); // QLineEdit hardcoded margins
      }
      break;
    }

    case CT_SpinBox : {
      if ( const QStyleOptionSpinBox *opt =
           qstyleoption_cast<const QStyleOptionSpinBox *>(option) ) {

        s = csz;
        if ( opt->frame )
          s += QSize(fs.left+fs.right,fs.top+fs.bottom);

        if ( opt->buttonSymbols != QAbstractSpinBox::NoButtons ) {
          if ( getThemeTweak("specific.spinbox.variant").toInt() == VA_SPINBOX_BUTTONS_STACKED )
            s += QSize(pixelMetric(PM_MenuButtonIndicator),0); // buttons
          else
            s += QSize(2*pixelMetric(PM_MenuButtonIndicator),0); // buttons
        }

        if ( !opt->frame)
          s = s.expandedTo(QSize(0,pixelMetric(PM_MenuButtonIndicator))); // minimum height
        else
          s = s.expandedTo(QSize(fs.left+fs.right,
                                 pixelMetric(PM_MenuButtonIndicator)+fs.top+fs.bottom));

        s += QSize(4,0); // QLineEdit hardcoded margins
      }

      break;
    }

    case CT_ComboBox : {
      if ( const QStyleOptionComboBox *opt =
           qstyleoption_cast<const QStyleOptionComboBox *>(option) ) {
        const QComboBox *w = qobject_cast<const QComboBox *>(widget);

        if ( !opt->frame )
          fs.hasFrame = false;

        if ( w ) {
          s = sizeFromContents(fm,fs,is,ls,
                               "W",0); // min size
          bool iconFound = false;
          for (int i=0; i<w->count(); i++) {
            if ( !w->itemIcon(i).isNull() )
              iconFound = true;
            s = sizeFromContents(fm,fs,is,ls,
                                 w->itemText(i),
                                 opt->iconSize.width()).expandedTo(s);
          }
          if ( w->count() && !iconFound )
            s -= QSize(opt->iconSize.width(), 0); // all items without icons
        } else {
          s = csz;
        }

        if ( opt->editable )
          s += QSize(4,0); // QLineEdit hardcoded margins

        s += QSize(pixelMetric(PM_MenuButtonIndicator),0); // drop down button

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
                             opt->icon.isNull() ? 0 : opt->iconSize.width());

        if ( opt->features & QStyleOptionButton::HasMenu ) {
          s.rwidth() += ls.tispace+ds.size;
        }
      }

      break;
    }

    case CT_CheckBox :  {
      if ( const QStyleOptionButton *opt =
           qstyleoption_cast<const QStyleOptionButton *>(option) ) {

        s = sizeFromContents(fm,fs,is,ls,
                             opt->text,
                             opt->icon.isNull() ? 0 : opt->iconSize.width());
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
                             opt->icon.isNull() ? 0 : opt->iconSize.width());
        s += QSize(pixelMetric(PM_CheckBoxLabelSpacing)+pixelMetric(PM_ExclusiveIndicatorWidth),0);
        s = s.expandedTo(QSize(pixelMetric(PM_ExclusiveIndicatorWidth),pixelMetric(PM_ExclusiveIndicatorHeight))); // minimal checkbox size is size of indicator
      }
      break;
    }

    case CT_MenuItem : {
      if ( const QStyleOptionMenuItem *opt =
           qstyleoption_cast<const QStyleOptionMenuItem *>(option) ) {

        if (opt->menuItemType == QStyleOptionMenuItem::Separator) {
          if ( opt->text.isEmpty() ) {
            // separator
            s = QSize(csw,getThemeTweak("specific.menu.separator.height").toInt());
          } else {
            // menu section with title
            QStyleOptionButton o;
            o.QStyleOption::operator =(*opt);
            o.text = opt->text;
            o.icon = opt->icon;
            o.iconSize = QSize(opt->maxIconWidth,opt->maxIconWidth);
            s = sizeFromContents(CT_PushButton,&o,csz,widget);
          }
        } else {
          s = sizeFromContents(fm,fs,is,ls,opt->text,opt->maxIconWidth);
        }

        // add icon width even if no icon
        s.rwidth() += ls.tispace+opt->maxIconWidth;

        // add width for check mark
        if ( (opt->checkType == QStyleOptionMenuItem::Exclusive) ||
             (opt->checkType == QStyleOptionMenuItem::NonExclusive)
            ) {
          s.rwidth() += pixelMetric(PM_CheckBoxLabelSpacing)+ds.size;
        }

        // add width for sub menu arrow
        if ( opt->menuItemType == QStyleOptionMenuItem::SubMenu ) {
          s.rwidth() += pixelMetric(PM_CheckBoxLabelSpacing)+ds.size;
        }
      }

      break;
    }

    case CT_MenuBarItem : {
      if ( const QStyleOptionMenuItem *opt =
           qstyleoption_cast<const QStyleOptionMenuItem *>(option) ) {

        s = sizeFromContents(fm,fs,is,ls,
                             opt->text,
                             opt->maxIconWidth);
      }

      break;
    }

    case CT_ProgressBar : {
      if ( const QStyleOptionProgressBar *opt =
           qstyleoption_cast<const QStyleOptionProgressBar *>(option) )  {

        if ( getThemeTweak("specific.progressbar.variant").toInt() == VA_PROGRESSBAR_THIN ) {
          // thin progressbar : text above bar
          QSize barSz = sizeFromContents(fm,fs,is,ls, QString::null);
          fs.hasFrame = false;
          is.hasInterior = false;
          QSize textSz;
          // this is to avoid the (2,2) minimum size of sizeFromContents()
          if ( opt->textVisible && !opt->text.isEmpty() )
            textSz = sizeFromContents(fm,fs,is,ls,
                                      opt->textVisible ?
                                        opt->text.isEmpty() ? "W" : opt->text
                                                            : QString::null);
          barSz = barSz.expandedTo(
                QSize(textSz.width(),
                      getThemeTweak("specific.progressbar.thin.minheight").toInt()));
          s = QSize(barSz.width(),barSz.height()+textSz.height());
        } else {
          s = sizeFromContents(fm,fs,is,ls,
                               opt->textVisible ?
                               opt->text.isEmpty() ? "W" : opt->text
                               : QString::null);
        }

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

        // minimum size
        QSize ms;
        if ( opt->text.isEmpty() && (opt->toolButtonStyle == Qt::ToolButtonTextOnly) )
          ms = sizeFromContents(fm,fs,is,ls,
                                "W",
                                opt->iconSize.width(),
                                Qt::ToolButtonTextOnly);
        if ( opt->icon.isNull() && (opt->toolButtonStyle == Qt::ToolButtonIconOnly) )
          ms = sizeFromContents(fm,fs,is,ls,
                                "W",
                                opt->iconSize.width(),
                                Qt::ToolButtonTextOnly);
        if ( opt->text.isEmpty() && opt->icon.isNull() )
          ms = sizeFromContents(fm,fs,is,ls,
                                "W",
                                opt->iconSize.width(),
                                Qt::ToolButtonTextOnly);

        s = sizeFromContents(fm,fs,is,ls,
                             opt->text,
                             opt->iconSize.width(),
                             opt->toolButtonStyle).expandedTo(ms);

        if (opt->arrowType != Qt::NoArrow) {
          // add room for arrow
          s.rwidth() += ds.size;
          // add spacing between arrow and label if necessary
          if ( (opt->toolButtonStyle != Qt::ToolButtonIconOnly) &&
               !opt->text.isEmpty() )
            s.rwidth() += ls.tispace;
          else if ( (opt->toolButtonStyle != Qt::ToolButtonTextOnly) &&
               !opt->icon.isNull() )
            s.rwidth() += ls.tispace;
        }

        // add room for simple down arrow or drop down arrow
        if (opt->features & QStyleOptionToolButton::Menu) {
          // Tool button with drop down button
          s.rwidth() += pixelMetric(PM_MenuButtonIndicator);
        } else if (opt->features & QStyleOptionToolButton::HasMenu) {
          // Tool button with down arrow
          s.rwidth() += ls.tispace+ds.size;
        }
      }
      break;
    }

    case CT_TabBarTab : {
      if ( const QStyleOptionTab *opt =
           qstyleoption_cast<const QStyleOptionTab *>(option) ) {

        // Size of this tab
        s = sizeFromContents(fm,fs,is,ls,
                             opt->text,
                             opt->icon.isNull() ? 0 : opt->iconSize.width());

        if ( const QTabBar *w = qobject_cast< const QTabBar* >(widget) ) {
            if ( w->tabsClosable() )
              s.rwidth() += ls.tispace+pixelMetric(PM_TabCloseIndicatorWidth);
        }

        if ( getThemeTweak("specific.tab.variant") == VA_TAB_INDIVIDUAL ||
             getThemeTweak("specific.tab.variant") == VA_TAB_GROUP_NON_SELECTED ) {
          // allow active tab to be higher than others
          // this is substracted from non toggled tabs when drawing
          s.rheight() += 5;
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
                             opt->text,pixelMetric(PM_SmallIconSize));
        if ( opt->sortIndicator != QStyleOptionHeader::None )
          s += QSize(2*ls.tispace+ds.size,0);
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

    case CT_GroupBox : { // whole group box (incl. title)
      if ( const QStyleOptionGroupBox *opt =
           qstyleoption_cast<const QStyleOptionGroupBox *>(option) ) {

        // Title
        QSize stitle(0,0);
        g = CE_group(static_cast<ControlElement>(CE_GroupBoxTitle));
        fs = getFrameSpec(g);
        is = getInteriorSpec(g);
        ls = getLabelSpec(g);

        if ( opt->subControls & SC_GroupBoxLabel ) {
          if ( opt->subControls & SC_GroupBoxCheckBox )
            stitle = sizeFromContents(fm,fs,is,ls,opt->text)+QSize(pixelMetric(PM_CheckBoxLabelSpacing)+ds.size,0);
          else
            stitle = sizeFromContents(fm,fs,is,ls,opt->text);

          // 30 is title margin (left and right)
          stitle += QSize(30+30,0);
        }

        QSize scontents;
        g = PE_group(PE_FrameGroupBox);
        fs = getFrameSpec(g);
        is = getInteriorSpec(g);
        ls = getLabelSpec(g);

        scontents = csz+QSize(fs.left+fs.right,fs.top+fs.bottom);

        // unite
        s = QSize(qMax(stitle.width(),scontents.width()),
                  stitle.height()+scontents.height());
      }

      break;
    }

    case CT_TabWidget : { // tab widget contents only
      if ( const QStyleOptionTabWidgetFrame *opt =
           qstyleoption_cast<const QStyleOptionTabWidgetFrame *>(option) ) {

        s = csz+QSize(fs.left+fs.right,fs.top+fs.bottom);
        s = QSize(qMax(s.width(),opt->tabBarSize.width()),
                  s.height());
      }

      break;
    }

    case CT_ItemViewItem :  {
      if ( const QStyleOptionViewItem *opt =
           qstyleoption_cast<const QStyleOptionViewItem *>(option) ) {

        // use item's own font
        fm = opt->fontMetrics;
        s = sizeFromContents(fm,fs,is,ls,
                             opt->text,
                             opt->icon.isNull() ? 0 : pixelMetric(PM_ListViewIconSize));

        if ( opt->features & QStyleOptionViewItem::HasCheckIndicator ) {
          s.rwidth() += pixelMetric(PM_CheckBoxLabelSpacing)+qMin(s.height(),(int)ds.size);
        }
      }
      break;
    }

    default :
      s = QCommonStyle::sizeFromContents(type,option,csz,widget);
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
                     /* icon */ int iconsz,
                     /* text-icon alignment */ const Qt::ToolButtonStyle tialign) const
{
  Q_UNUSED(is);

  QSize s(0,0);

  // compute size of interior elements first
  int th = 0, tw = 0;
  if ( !text.isEmpty() || iconsz ) {
    s.rwidth() += 2*ls.hmargin;
    s.rheight() += 2*ls.vmargin;
  }

  if ( !text.isEmpty() ) {
    if (ls.hasShadow) {
      s.rwidth() += ls.xshift+ls.depth;
      s.rheight() += ls.yshift+ls.depth;
    }

    // compute width and height of text
    QSize ts = text.isEmpty() ? QSize(0,0) : fm.size(Qt::TextShowMnemonic,text);
    tw = ts.width();
    th = ts.height();
  }

  if (tialign == Qt::ToolButtonIconOnly) {
    s.rwidth() += iconsz;
    s.rheight() += iconsz;
  } else if (tialign == Qt::ToolButtonTextOnly) {
    s.rwidth() += tw;
    s.rheight() += th;
  } else if (tialign == Qt::ToolButtonTextBesideIcon) {
    s.rwidth() += iconsz + (!iconsz ? 0 : (text.isEmpty() ? 0 : ls.tispace)) + tw;
    s.rheight() += qMax(iconsz,th);
  } else if (tialign == Qt::ToolButtonTextUnderIcon) {
    s.rwidth() += qMax(iconsz,tw);
    s.rheight() += iconsz + (!iconsz ? 0 : ls.tispace) + th;
  }

  // then add frame
  if ( fs.hasFrame ) {
    s.rwidth() += fs.left+fs.right;
    s.rheight() += fs.top+fs.bottom;
  }

  // minimum size : frame + 2 pixels of interior
  if ( fs.hasFrame )
    s = s.expandedTo(QSize(fs.left+fs.right+2,fs.top+fs.bottom+2));
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
  const QFontMetrics fm = option->fontMetrics;
  const Qt::LayoutDirection dir = option->direction;
  const Orientation orn = option->state & State_Horizontal ? Horizontal : Vertical;

  // Get QSvgStyle configuration group used to render this element
  const QString g = SE_group(e);

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

  ret = r;

  switch (e) {
    case SE_PushButtonFocusRect : {
      ret = r;
      break;
    }
    case SE_ProgressBarGroove :
    case SE_ProgressBarContents : {
      if ( orn != Horizontal ) {
        // perform computations on horizontalized widget
        ret = transposedRect(r);
        qSwap(x,y);
        qSwap(w,h);
      }
      if ( getThemeTweak("specific.progressbar.variant").toInt() == VA_PROGRESSBAR_THIN ) {
        QSize barSz = sizeFromContents(fm,fs,is,ls, QString::null);
        ret.setTop(ret.bottom()-
                   qMax(barSz.height(),
                         getThemeTweak("specific.progressbar.thin.minheight").toInt()));
      }
      if ( orn != Horizontal )
        ret = transposedRect(ret);
      break;
    }
    case SE_ProgressBarLabel: {
      if ( orn != Horizontal ) {
        // perform computations on horizontalized widget
        ret = transposedRect(r);
        qSwap(x,y);
        qSwap(w,h);
      }
      if ( getThemeTweak("specific.progressbar.variant") == VA_PROGRESSBAR_THIN ) {
        QRect barRect = subElementRect(SE_ProgressBarGroove,option,widget);
        if ( orn != Horizontal )
          barRect = transposedRect(barRect);
        ret.setHeight(h-barRect.height());
      }
      if ( orn != Horizontal )
        ret = transposedRect(ret);

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
      }
      break;
    }
    case SE_HeaderArrow : {
      ret = labelRect(r,fs,is,ls);
      ret = ret.adjusted(ret.width()-ds.size-ls.tispace,0,0,0);
      break;
    }
    case SE_CheckBoxFocusRect : {
      // already a visual rect
      return subElementRect(SE_CheckBoxContents,option,widget);
      break;
    }
    case SE_RadioButtonFocusRect : {
      return subElementRect(SE_RadioButtonContents,option,widget);
      break;
    }

    case SE_TabWidgetTabBar : {
      // already a visual rect
      ret = QCommonStyle::subElementRect(SE_TabWidgetTabBar,option,widget);
      // limit width/height and remove left and right (top, bottom) of tab contents if necessary
      if ( const QTabWidget *tw = qobject_cast<const QTabWidget *>(widget) ) {
        QTabBar::Shape s = tw->tabBar()->shape();
        if ( s == QTabBar::RoundedEast ||
             s == QTabBar::RoundedWest ||
             s == QTabBar::TriangularEast ||
             s == QTabBar::TriangularWest ) {
          if ( ret.top() < y+fs.left )
            ret.setLeft(y+fs.left);
          if ( ret.bottom() > y+h-fs.right )
            ret.setBottom(y+h-fs.right );
        } else {
          if ( ret.left() < x+fs.left )
            ret.setLeft(x+fs.left);
          if ( ret.right() > x+w-fs.right )
            ret.setRight(x+w-fs.right );
        }
      }
      break;
    }

    case SE_TabBarTabText : {
      ret = labelRect(r,fs,is,ls);
      break;
    }

    case SE_TabWidgetTabContents : {
      // already a visual rect
      ret = subElementRect(SE_TabWidgetTabPane,option,widget);
      return interiorRect(ret,fs,is);
      break;
    }

    case SE_ItemViewItemCheckIndicator : {
      ret = r.adjusted(ls.hmargin,0,0,0);
      ret.setWidth(qMin(ret.height(),static_cast<int>(ds.size)));
      break;
    }

    case SE_ItemViewItemDecoration :
    case SE_ItemViewItemText :
    case SE_ItemViewItemFocusRect : {
      r = interiorRect(r,fs,is);
      // QSvgStyle does not make a distinction between icon and text,
      // all these are label and there is a single function
      // to draw both: renderLabel()
      if ( const QStyleOptionViewItem *opt =
           qstyleoption_cast<const QStyleOptionViewItem *>(option) ) {
        //ret = r.adjusted(ls.hmargin,0,0,0);
        if ( opt->features & QStyleOptionViewItem::HasCheckIndicator )
          ret = ret.adjusted(pixelMetric(PM_CheckBoxLabelSpacing)+qMin(ret.height(),static_cast<int>(ds.size)),0,0,0);
      }
      break;
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
  Q_UNUSED(st);

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

      int variant = getThemeTweak("specific.spinbox.variant").toInt();

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
              case VA_SPINBOX_BUTTONS_STACKED :
                ret = r.adjusted(0,0,-pixelMetric(PM_MenuButtonIndicator),0);
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
              case VA_SPINBOX_BUTTONS_STACKED :
                ret = QRect(x+w-pixelMetric(PM_MenuButtonIndicator),
                        y,pixelMetric(PM_MenuButtonIndicator),h/2);
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
              case VA_SPINBOX_BUTTONS_STACKED :
                ret = QRect(x+w-pixelMetric(PM_MenuButtonIndicator),
                        y+h/2,pixelMetric(PM_MenuButtonIndicator),h/2);
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
            ret = r;
            if ( getThemeTweak("specific.scrollbar.variant").toInt() == VA_SCROLLBAR_BUTTONS )
              ret.adjust(extent,0,-extent,0);
            break;
          case SC_ScrollBarSubLine :
            if ( getThemeTweak("specific.scrollbar.variant").toInt() == VA_SCROLLBAR_BUTTONS )
              ret = QRect(x,y,extent,extent);
            else
              ret = QRect();
            break;
          case SC_ScrollBarAddLine :
            if ( getThemeTweak("specific.scrollbar.variant").toInt() == VA_SCROLLBAR_BUTTONS )
              ret = QRect(x+w-extent,y,extent,extent);
            else
              ret = QRect();
            break;
          case SC_ScrollBarSlider : {
            if ( const QStyleOptionSlider *opt =
                 qstyleoption_cast<const QStyleOptionSlider *>(option) ) {

              r = subControlRect(CC_ScrollBar,option,SC_ScrollBarGroove,widget);

              if ( getThemeTweak("specific.scrollbar.slider.area").toInt() == VA_SCROLLBAR_CURSOR_INSIDE_GROOVE )
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
            ret = r;
            if ( getThemeTweak("specific.scrollbar.variant").toInt() == VA_SCROLLBAR_BUTTONS )
              ret.adjust(0,extent,0,-extent);
            break;
          case SC_ScrollBarSubLine :
            if ( getThemeTweak("specific.scrollbar.variant").toInt() == VA_SCROLLBAR_BUTTONS )
              ret = QRect(x,y,extent,extent);
            else
              ret = QRect();
            break;
          case SC_ScrollBarAddLine :
            if ( getThemeTweak("specific.scrollbar.variant").toInt() == VA_SCROLLBAR_BUTTONS )
              ret = QRect(x,y+h-extent,extent,extent);
            else
              ret = QRect();
            break;
          case SC_ScrollBarSlider : {
            if ( const QStyleOptionSlider *opt =
                 qstyleoption_cast<const QStyleOptionSlider *>(option) ) {

              r = subControlRect(CC_ScrollBar,option,SC_ScrollBarGroove,widget);

              if ( getThemeTweak("specific.scrollbar.slider.area").toInt() == VA_SCROLLBAR_CURSOR_INSIDE_GROOVE )
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

            r = interiorRect(r,fs,is);
            r.getRect(&x,&y,&w,&h);

            // remove room for drop down buttons or down arrows
            if (opt->features & QStyleOptionToolButton::Menu)
              ret = r.adjusted(0,0,-pixelMetric(PM_MenuButtonIndicator),0);
            else if (opt->features & QStyleOptionToolButton::HasMenu)
              ret = r.adjusted(0,0,-ds.size-ls.tispace,0);
            else
              ret = r;
          }
          break;
        }
        case SC_ToolButtonMenu : {
          if ( const QStyleOptionToolButton *opt =
               qstyleoption_cast<const QStyleOptionToolButton *>(option) ) {

            r = interiorRect(r,fs,is);
            r.getRect(&x,&y,&w,&h);
            if (opt->features & QStyleOptionToolButton::Menu)
              ret = QRect(x+w-pixelMetric(PM_MenuButtonIndicator),y,
                         pixelMetric(PM_MenuButtonIndicator),h);
            else if (opt->features & QStyleOptionToolButton::HasMenu)
              ret = QRect(x+w-ls.tispace-ds.size,y+h-ds.size-2,ds.size,ds.size);
            else
              ret = r;
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
      QSize stitle(0,0); // size of title (text+checkbox if any)

      if (const QStyleOptionGroupBox *opt =
          qstyleoption_cast<const QStyleOptionGroupBox *>(option) ) {

        // title size
        if ( opt->subControls & SC_GroupBoxLabel ) {
          stitle = sizeFromContents(fm,fs,is,ls,
                                    opt->text);
          if ( opt->subControls & SC_GroupBoxCheckBox ) {
            // ensure checkbox indicator fits within title interior
            stitle += QSize(ds.size+pixelMetric(PM_CheckBoxLabelSpacing),0);
            stitle = stitle.expandedTo(QSize(0,fs.top+fs.bottom+ds.size));
          }

          labelRect = alignedRect(Qt::LeftToRight,
                                  opt->textAlignment & ~Qt::AlignVertical_Mask,
                                  stitle,
                                  r.adjusted(30,0,-30,0));
        }
      }

      switch (subControl) {
        case SC_GroupBoxCheckBox : {
          // align checkbox inside label rect
          ret = alignedRect(Qt::LeftToRight,Qt::AlignLeft | Qt::AlignVCenter,
                            QSize(ds.size,ds.size),
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
          g = PE_group(PE_FrameGroupBox);
          fs = getFrameSpec(g);
          is = getInteriorSpec(g);
          ls = getLabelSpec(g);

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

    case CC_TitleBar: {
      if ( const QStyleOptionTitleBar *opt =
           qstyleoption_cast<const QStyleOptionTitleBar *>(option) ) {

        if ( styleHint(SH_TitleBar_NoBorder) )
          fs.hasFrame = false;
        else if ( !(opt->titleBarState & Qt::WindowMinimized) )
          fs.bottom = 0;

        QRect ir = interiorRect(r,fs,is);
        ir.getRect(&x,&y,&w,&h);

        // button layout
        // Sy-Sh|USh---Title---H-m-M|R-C
        // Sy: System Menu
        // Sh : Shade or Unshade
        // H : Context Help
        // m : Minimize
        // M|R : Maximize or Restore
        // C : Close

        const int bw = pixelMetric(PM_TitleBarButtonSize); // button width

        // union rect for buttons located left
        QRect leftBtns = alignedRect(Qt::LeftToRight,Qt::AlignLeft | Qt::AlignVCenter,
                                     QSize(0,h),
                                     ir).translated(ls.hmargin-ls.tispace,0);
        // union rect for buttons located right
        QRect rightBtns = alignedRect(Qt::LeftToRight,Qt::AlignRight | Qt::AlignVCenter,
                                      QSize(0,h),
                                      ir).translated(-ls.hmargin+ls.tispace,0);

        // left buttons
        QRect sysBtn, shadeBtn;
        sysBtn = shadeBtn = leftBtns;

        // right buttons
        QRect helpBtn, minBtn, maxBtn, normalBtn, closeBtn;
        closeBtn = maxBtn = minBtn = normalBtn = helpBtn = rightBtns;

        // keep ordered as is layout, label last
        // --- left buttons
        // system menu button
        if ( (opt->subControls & SC_TitleBarSysMenu) &&
             opt->titleBarFlags & Qt::WindowSystemMenuHint ) {
          sysBtn.translate(leftBtns.width()+ls.tispace,0);
          sysBtn.setWidth(pixelMetric(PM_TitleBarButtonIconSize));
          leftBtns.setRight(sysBtn.right());
        }

        // shade button
        if ( (opt->subControls & SC_TitleBarShadeButton) &&
             opt->titleBarFlags & Qt::WindowShadeButtonHint &&
             !(opt->titleBarState & Qt::WindowMinimized) ) {
          shadeBtn.translate(leftBtns.width()+ls.tispace,0);
          shadeBtn.setWidth(pixelMetric(PM_TitleBarButtonSize));
          leftBtns.setRight(shadeBtn.right());
        }

        // unshade button
        if ( (opt->subControls & SC_TitleBarUnshadeButton) &&
             opt->titleBarFlags & Qt::WindowShadeButtonHint &&
             opt->titleBarState & Qt::WindowMinimized ) {
          shadeBtn.translate(leftBtns.width()+ls.tispace,0);
          shadeBtn.setWidth(pixelMetric(PM_TitleBarButtonSize));
          leftBtns.setRight(shadeBtn.right());
        }

        // --- right buttons (reverse order)
        // close button
        if ( (opt->subControls & SC_TitleBarCloseButton) &&
             opt->titleBarFlags & Qt::WindowSystemMenuHint ) {
          closeBtn.translate(-rightBtns.width()-bw-ls.tispace,0);
          closeBtn.setWidth(pixelMetric(PM_TitleBarButtonSize));
          rightBtns.setLeft(closeBtn.left());
        }

        // max button
        if ( (opt->subControls & SC_TitleBarMaxButton) &&
             opt->titleBarFlags & Qt::WindowMaximizeButtonHint &&
             !(opt->titleBarState & Qt::WindowMaximized) ) {
          maxBtn.translate(-rightBtns.width()-bw-ls.tispace,0);
          maxBtn.setWidth(pixelMetric(PM_TitleBarButtonSize));
          rightBtns.setLeft(maxBtn.left());
        }

        // normal (restore) button
        if ( (opt->subControls & SC_TitleBarNormalButton) &&
             (
               ((opt->titleBarFlags & Qt::WindowMinimizeButtonHint) &&
                (opt->titleBarState & Qt::WindowMinimized)) ||
               ((opt->titleBarFlags & Qt::WindowMaximizeButtonHint) &&
                (opt->titleBarState & Qt::WindowMaximized))
             )
           ) {
          normalBtn.translate(-rightBtns.width()-bw-ls.tispace,0);
          normalBtn.setWidth(pixelMetric(PM_TitleBarButtonSize));
          rightBtns.setLeft(normalBtn.left());
        }

        // min button
        if ( (opt->subControls & SC_TitleBarMinButton) &&
             opt->titleBarFlags & Qt::WindowMinimizeButtonHint &&
             !(opt->titleBarState & Qt::WindowMinimized) ) {
          minBtn.translate(-rightBtns.width()-bw-ls.tispace,0);
          minBtn.setWidth(pixelMetric(PM_TitleBarButtonSize));
          rightBtns.setLeft(minBtn.left());
        }

        // help button
        if ( (opt->subControls & SC_TitleBarContextHelpButton) &&
             opt->titleBarFlags & Qt::WindowContextHelpButtonHint &&
             opt->titleBarState & Qt::WindowMinimized ) {
          helpBtn.translate(-rightBtns.width()-bw-ls.tispace,0);
          helpBtn.setWidth(pixelMetric(PM_TitleBarButtonSize));
          rightBtns.setLeft(helpBtn.left());
        }

        // --- now the label
        QRect lr = r.adjusted(leftBtns.width()+ls.tispace+ls.hmargin,0,
                              -rightBtns.width()-ls.tispace-ls.hmargin,0);

        switch (subControl) {
          case SC_TitleBarSysMenu :
            ret = sysBtn;
            break;
          case SC_TitleBarCloseButton :
            ret = closeBtn;
            break;
          case SC_TitleBarContextHelpButton :
            ret = helpBtn;
            break;
          case SC_TitleBarMaxButton :
            ret = maxBtn;
            break;
          case SC_TitleBarNormalButton :
            ret = normalBtn;
            break;
          case SC_TitleBarMinButton :
            ret = minBtn;
            break;
          case SC_TitleBarShadeButton :
          case SC_TitleBarUnshadeButton :
            ret = shadeBtn;
            break;
          case SC_TitleBarLabel :
            ret = lr;
            break;

          default : ret = QCommonStyle::subControlRect(control,option,subControl,widget);
        }
      }

      break;
    }

    default :
      // subControlRect() already returns visual rects
      return QCommonStyle::subControlRect(control,option,subControl,widget);
  }

  return visualRect(dir,r,ret);
}

QIcon QSvgThemableStyle::standardIcon(StandardPixmap standardIcon, const QStyleOption * option, const QWidget * widget) const
{
  int sz;
  QString base;
  QIcon icon;

  switch (standardIcon) {
    case SP_ToolBarHorizontalExtensionButton :
    case SP_ToolBarVerticalExtensionButton  :
      sz = getIndicatorSpec(CE_group(CE_ToolBar)).size;
      base = getIndicatorSpec(PE_group(PE_PanelToolBar)).element + "-ext";
      break;

    case SP_TitleBarMinButton :
      sz = pixelMetric(PM_TitleBarButtonSize);
      base = getIndicatorSpec(PE_group(PE_FrameWindow)).element;
      base += "-min";
      break;

    case SP_TitleBarMaxButton :
      sz = pixelMetric(PM_TitleBarButtonSize);
      base = getIndicatorSpec(PE_group(PE_FrameWindow)).element;
      base += "-max";
      break;

    case SP_DockWidgetCloseButton :
    case SP_TitleBarCloseButton :
      sz = pixelMetric(PM_TitleBarButtonSize);
      base = getIndicatorSpec(PE_group(PE_FrameWindow)).element;
      base += "-close";
      break;

    case SP_TitleBarMenuButton :
      sz = pixelMetric(PM_TitleBarButtonSize);
      base = getIndicatorSpec(PE_group(PE_FrameWindow)).element;
      base += "-menu";
      break;

    case SP_TitleBarNormalButton :
      sz = pixelMetric(PM_TitleBarButtonSize);
      base = getIndicatorSpec(PE_group(PE_FrameWindow)).element;
      base += "-restore";
      break;

    case SP_TitleBarShadeButton :
      sz = pixelMetric(PM_TitleBarButtonSize);
      base = getIndicatorSpec(PE_group(PE_FrameWindow)).element;
      base += "-shade";
      break;

    case SP_TitleBarUnshadeButton :
      sz = pixelMetric(PM_TitleBarButtonSize);
      base = getIndicatorSpec(PE_group(PE_FrameWindow)).element;
      base += "-unshade";
      break;

    case SP_TitleBarContextHelpButton :
      sz = pixelMetric(PM_TitleBarButtonSize);
      base = getIndicatorSpec(PE_group(PE_FrameWindow)).element;
      base += "-help";
      break;

    case SP_LineEditClearButton :
      sz = getIndicatorSpec(PE_group(PE_FrameLineEdit)).size;
      base = getIndicatorSpec(PE_group(PE_FrameLineEdit)).element;
      base += "-clear";
      break;

    default :
      icon = QCommonStyle::standardIcon(standardIcon,option,widget);
  }

  if( !icon.isNull() )
    // default QCommonStyle icon
    return icon;

  // build icon
  QRect r(0,0,sz,sz);
  QPainter p;

  QPixmap pm_off(sz,sz);
  pm_off.fill(Qt::transparent);
  p.begin(&pm_off);
  renderElement(&p,base+"-disabled",r);
  p.end();
  icon.addPixmap(pm_off,QIcon::Disabled);

  QPixmap pm_normal(sz,sz);
  pm_normal.fill(Qt::transparent);
  p.begin(&pm_normal);
  renderElement(&p,base+"-normal",r);
  p.end();
  icon.addPixmap(pm_normal,QIcon::Normal);

  QPixmap pm_hovered(sz,sz);
  pm_hovered.fill(Qt::transparent);
  p.begin(&pm_hovered);
  renderElement(&p,base+"-hovered",r);
  p.end();
  icon.addPixmap(pm_hovered,QIcon::Active);

  QPixmap pm_pressed(sz,sz);
  pm_pressed.fill(Qt::transparent);
  p.begin(&pm_pressed);
  renderElement(&p,base+"-pressed",r);
  p.end();
  icon.addPixmap(pm_pressed,QIcon::Selected);

  QPixmap pm_toggled(sz,sz);
  pm_toggled.fill(Qt::transparent);
  p.begin(&pm_toggled);
  renderElement(&p,base+"-toggled",r);
  p.end();
  icon.addPixmap(pm_toggled,QIcon::Selected,QIcon::On);

  return icon;
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

  if ( !themeRndr->elementExists(element) ) {
    // Missing element
    p->save();
    p->setPen(Qt::black);
    drawRealRect(p, bounds);
    p->drawLine(x,y,x+w-1,y+h-1);
    p->drawLine(x+w-1,y,x,y+h-1);
    p->restore();
    emit sig_missingElement(element);
    qWarning() << "[QSvgStyle] object" << element << "missing in SVG file";
    return;
  }

  if (themeRndr) {
    if ( (hsize > 0) || (vsize > 0) ) {

      if ( (hsize > 0) && (vsize <= 0) ) {
        int hpatterns = (w/hsize)+1;

        p->save();
        p->setClipRect(QRect(x,y,w,h), Qt::IntersectClip);
        for (int i=0; i<hpatterns; i++)
          themeRndr->render(p,element,QRect(x+i*hsize,y,hsize,h));
        p->restore();
      }

      if ( (hsize <= 0) && (vsize > 0) ) {
        int vpatterns = (h/vsize)+1;

        p->save();
        p->setClipRect(QRect(x,y,w,h), Qt::IntersectClip);
        for (int i=0; i<vpatterns; i++)
          themeRndr->render(p,element,QRect(x,y+i*vsize,w,vsize));
        p->restore();
      }

      if ( (hsize > 0) && (vsize > 0) ) {
        int hpatterns = (w/hsize)+1;
        int vpatterns = (h/vsize)+1;

        p->save();
        p->setClipRect(bounds, Qt::IntersectClip);
        for (int i=0; i<hpatterns; i++)
          for (int j=0; j<vpatterns; j++)
            themeRndr->render(p,element,QRect(x+i*hsize,y+j*vsize,hsize,vsize));
        p->restore();
      }
    } else {
      themeRndr->render(p,element,QRect(x,y,w,h));
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
      if ( (fs.capsuleH == -1) || (fs.capsuleH == 2) )
        topleft = QRect(x0,y0,fs.left,fs.top);
      // topright corner
      if ( (fs.capsuleH == 1) || (fs.capsuleH == 2) )
        topright = QRect(x1-fs.right+1,y0,fs.right,fs.top);
    }
    // bottom
    if ( (fs.capsuleV == 1) || (fs.capsuleV == 2) ) {
      bottom = QRect(x0+la,y1-ba+1,w-la-ra,fs.bottom);
      // bottomleft corner
      if ( (fs.capsuleH == -1) || (fs.capsuleH == 2) )
        bottomleft = QRect(x0,y1-fs.bottom+1,fs.left,fs.bottom);
      // bottomright corner
      if ( (fs.capsuleH == 1) || (fs.capsuleH == 2) )
        bottomright = QRect(x1-fs.right+1,y1-fs.bottom+1,fs.right,fs.bottom);
    }
    // left
    if ( (fs.capsuleH == -1) || (fs.capsuleH == 2) ) {
      left = QRect(x0,y0+ta,fs.left,h-ta-ba);
      // topleft corner
      if ( (fs.capsuleV == -1) || (fs.capsuleV == 2) )
        topleft = QRect(x0,y0,fs.left,fs.top);
      // bottomleft corner
      if ( (fs.capsuleV == 1)|| (fs.capsuleV == 2) )
        bottomleft = QRect(x0,y1-fs.bottom+1,fs.left,fs.bottom);
    }
    // right
    if ( (fs.capsuleH == 1) || (fs.capsuleH == 2) ) {
      right = QRect(x1-fs.right+1,y0+ta,fs.right,h-ta-ba);
      // topright corner
      if ( (fs.capsuleV == -1) || (fs.capsuleV == 2) )
        topright = QRect(x1-fs.right+1,y0,fs.right,fs.top);
      // bottomright corner
      if ( (fs.capsuleV == 1) || (fs.capsuleV == 2) )
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
  bool use3dFrame, usePalette;

  bounds.getRect(&x0,&y0,&w,&h);
  intensity = getThemeTweak("specific.palette.intensity").toInt();
  use3dFrame = getThemeTweak("specific.palette.3dframes").toBool();
  usePalette = getThemeTweak("specific.palette.usepalette").toBool();
  usePalette = true;

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
  QPainterPath darkPath; // bottom and right 3D effect

  // top 3d
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

  // bottom 3d
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

  /* Cuts */
  if ( fs.hasCuts ) {
    QRect topClip = top;
    QRegion clip = bounds;

    if ( (fs.h0 >= x0) && (fs.h1 >= fs.h0) ) {
      topClip.setLeft(fs.h0);
      topClip.setRight(fs.h1);

      clip -= topClip;
    }

    p->save();
    p->setClipRegion(clip, Qt::IntersectClip);
  }


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
  if ( usePalette && (b.style() != Qt::NoBrush) ) {
    QBrush lightBrush(b), darkBrush(b);
    QColor lightColor, darkColor;
    // Clipping region for accurate colorization
    QRegion region;

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
      region += themeRndr->elementRegion(e+"-top", top);
      region += themeRndr->elementRegion(e+"-bottom", bottom);
      region += themeRndr->elementRegion(e+"-left", left);
      region += themeRndr->elementRegion(e+"-right", right);
      region += themeRndr->elementRegion(e+"-topleft", topleft);
      region += themeRndr->elementRegion(e+"-topright", topright);
      region += themeRndr->elementRegion(e+"-bottomleft", bottomleft);
      region += themeRndr->elementRegion(e+"-bottomright", bottomright);

      p->save();
      p->setClipRegion(region, Qt::IntersectClip);

      if ( !fs.pressed ) {
        p->fillPath(lightPath,lightColor);
        p->fillPath(darkPath,darkColor);
      } else {
        p->fillPath(lightPath,darkColor);
        p->fillPath(darkPath,lightColor);
      }

      p->restore();
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

  if ( fs.hasCuts ) {
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

  // FIXME even if we are in a capsule, interiorRect should be the same
  // as without capsule
  QMargins m(left.width(),top.height(),right.width(),bottom.height());
  //QMargins m(fs.left,fs.top,fs.right,fs.bottom);

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
  bool usePalette;

  bounds.getRect(&x0,&y0,&w,&h);
  intensity = getThemeTweak("specific.palette.intensity").toInt();
  usePalette = getThemeTweak("specific.palette.usepalette").toBool();
  usePalette = true;

  // drawing rect
  QRect r;

  computeInteriorRect(bounds,fs,is,orn, r);

  if  ( !r.isValid() )
    return;

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

  if ( usePalette && (b.style() != Qt::NoBrush) ) {
    // Colorize !
    QBrush interiorBrush(b);

    QColor interiorColor = b.color();
    interiorColor.setAlpha(intensity);
    interiorBrush.setColor(interiorColor);

    if ( !dbgWireframe && (curPalette != "<none>") ) {
      p->save();
      QRegion region = themeRndr->elementRegion(e, r);
      //qWarning() << "Region for" << e << "is" << region;
      p->setClipRegion(region, Qt::IntersectClip);
      p->fillRect(r,interiorColor);
      p->restore();
    }
  }

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

  intensity = getThemeTweak("specific.palette.intensity").toInt();
  use3dFrame = getThemeTweak("specific.palette.3dframes").toBool();

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
                            unsigned int talign,
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

  if ( text.isEmpty() ) {
    // When we have no text, center icon
    ricon = alignedRect(Qt::LeftToRight,Qt::AlignCenter, QSize(icon.width(),icon.height()),r);
  }

  rtext = visualRect(dir,bounds,rtext);
  ricon = visualRect(dir,bounds,ricon);

  if (tialign != Qt::ToolButtonIconOnly) {
    if ( !text.isEmpty() ) {
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

inline palette_spec_t QSvgThemableStyle::getPaletteSpec(const QString& group) const
{
  return themeSettings->getPaletteSpec(group);
}

inline font_spec_t QSvgThemableStyle::getFontSpec(const QString& group) const
{
  return themeSettings->getFontSpec(group);
}

inline QVariant QSvgThemableStyle::getThemeTweak(const QString &key) const
{
  return themeSettings->getThemeTweak(key);
}

inline QVariant QSvgThemableStyle::getStyleTweak(const QString &key) const
{
  return styleSettings->getStyleTweak(key);
}

QLayout * QSvgThemableStyle::layoutForWidget(const QWidget* widget, QLayout *l) const
{
  if ( !widget )
    return nullptr;

  QLayout *pLayout = l; /* parent layout */

  if ( !pLayout && widget->parentWidget() )
    pLayout = widget->parentWidget()->layout();

  if ( !pLayout )
    return nullptr;

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

  return nullptr;
}

void QSvgThemableStyle::computeButtonCapsule(const QWidget *widget, bool &capsule, int &h, int &v) const
{
  capsule = false;
  h = v = 2;

  if ( getThemeTweak("specific.button.usecapsule").toBool() == false )
    return;

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

  QWidget *myLeftWidget = nullptr, *myRightWidget = nullptr;
  QWidget *myTopWidget = nullptr, *myBottomWidget = nullptr;

  // QHBoxLayout
  if ( hbox ) {
    if ( myIdx > 0 ) {
      myLeftWidget = myLayout->itemAt(myIdx-1) ?
            myLayout->itemAt(myIdx-1)->widget() : nullptr;
    }
    if ( myIdx < cnt-1 )
      myRightWidget = myLayout->itemAt(myIdx+1) ?
        myLayout->itemAt(myIdx+1)->widget() : nullptr;
  }

  // QVBoxLayout
  if ( vbox ) {
    if ( myIdx > 0 )
      myTopWidget = myLayout->itemAt(myIdx-1) ?
        myLayout->itemAt(myIdx-1)->widget() : nullptr;
    if ( myIdx < cnt-1 ) {
      myBottomWidget = myLayout->itemAt(myIdx+1) ?
            myLayout->itemAt(myIdx+1)->widget() : nullptr;
    }
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

QBrush QSvgThemableStyle::bgBrush(const palette_spec_t &ps,
                                  const QStyleOption *opt,
                                  const QWidget *widget,
                                  const QString &status) const
{
  Q_UNUSED(widget);

  QBrush r;

  QPalette pal = widget ? widget->palette() : opt ? opt->palette : QGuiApplication::palette();
  const QPalette::ColorGroup cg = widget ?
        widget->isEnabled() ? QPalette::Normal : QPalette::Disabled
                            : (opt->state & State_Enabled) ?
                                QPalette::Normal : QPalette::Disabled;

  const QPalette::ColorRole bgrole =
      widget ? widget->backgroundRole() : QPalette::NoRole;

  QString val;

  if ( status == "normal" ) {
    val = ps.normal.bg;
  } else if ( status == "hovered" ) {
    val = ps.hovered.bg;
  } else if ( status == "pressed" ) {
    val = ps.pressed.bg;
  } else if ( status == "toggled" ) {
    val = ps.toggled.bg;
  } else if ( status == "disabled" ) {
    val = ps.disabled.bg;
  } else if ( status == "disabled-toggled" ) {
    val = ps.disabled_toggled.bg;
  } else if ( status == "focused" ) {
    val = ps.focused.bg;
  } else if ( status == "default" ) {
    val = ps.defaultt.bg;
  }

  if ( val.isEmpty() || (val == "<none>") )
    return r; // no brush
  else if ( val == "<system>" ) {
    // use widget's palette
    r = pal.color(cg,bgrole);
  } else {
    // r,g,b,a color -> parse
    QStringList l = val.split(',');
    if ( l.size() == 4 ) {
      QColor c(l[0].toInt(),l[1].toInt(),l[2].toInt(),l[3].toInt());

      r.setColor(c);
      r.setStyle(Qt::SolidPattern);
    }
  }

  return r;
}

QBrush QSvgThemableStyle::fgBrush(const palette_spec_t &ps,
                                  const QStyleOption *opt,
                                  const QWidget *widget,
                                  const QString &status) const
{
  Q_UNUSED(widget);
  QBrush r;

  QPalette pal = widget ? widget->palette() : opt->palette;
  const QPalette::ColorGroup cg = widget ?
        widget->isEnabled() ? QPalette::Normal : QPalette::Disabled
                            : (opt->state & State_Enabled) ?
                                QPalette::Normal : QPalette::Disabled;

  pal.setCurrentColorGroup(cg);
  const QPalette::ColorRole fgrole = widget ? widget->foregroundRole() : QPalette::NoRole;
  QString val;

  if ( status == "normal" ) {
    val = ps.normal.fg;
  } else if ( status == "hovered" ) {
    val = ps.hovered.fg;
  } else if ( status == "pressed" ) {
    val = ps.pressed.fg;
  } else if ( status == "toggled" ) {
    val = ps.toggled.fg;
  } else if ( status == "disabled" ) {
    val = ps.disabled.fg;
  } else if ( status == "disabled-toggled" ) {
    val = ps.disabled_toggled.fg;
  } else if ( status == "focused" ) {
    val = ps.focused.fg;
  } else if ( status == "default" ) {
    val = ps.defaultt.fg;
  }

  if ( val.isEmpty() || (val == "<none>") )
    return r; // no brush
  else if ( val == "<system>" ) {
    // use widget's palette
    r = pal.color(cg,fgrole);
  } else {
    // r,g,b,a color -> parse
    QStringList l = val.split(',');
    if ( l.size() == 4 ) {
      QColor c(l[0].toInt(),l[1].toInt(),l[2].toInt(),l[3].toInt());
      r.setColor(c);
      r.setStyle(Qt::SolidPattern);
    }
  }

  return r;
}

void QSvgThemableStyle::setupPainterFromFontSpec(QPainter *p,
                                                 const font_spec_t &ts,
                                                 const QString &status) const
{
  if ( !p )
    return;

  QFont f = p->font();
  font_attr_spec_t val;
  val.bold = false;
  val.italic = false;
  val.underline = false;

  if ( status == "normal" ) {
    val = ts.normal;
  } else if ( status == "hovered" ) {
    val = ts.hovered;
  } else if ( status == "pressed" ) {
    val = ts.pressed;
  } else if ( status == "toggled" ) {
    val = ts.toggled;
  } else if ( status == "disabled" ) {
    val = ts.disabled;
  } else if ( status == "disabled-toggled" ) {
    val = ts.disabled_toggled;
  } else if ( status == "focused" ) {
    val = ts.focused;
  } else if ( status == "default" ) {
    val = ts.defaultt;
  }

  if ( val.bold.present ) {
    if ( val.bold )
      f.setBold(true);
    else
      f.setBold(false);
  }

  if ( val.italic.present ) {
    if ( val.italic )
      f.setItalic(true);
    else
      f.setItalic(false);
  }

  if ( val.underline.present ) {
    if ( val.underline )
      f.setUnderline(true);
    else
      f.setUnderline(false);
  }

  p->setFont(f);
}

QString QSvgThemableStyle::state_str(State st, const QWidget* w) const
{
  QString status;

  if ( !isContainerWidget(w) ) {
    // Keep the order
    status = (st & State_Enabled) ?
      (st & State_Sunken) ? "pressed" :
      (st & State_On) ? "toggled" :
      (st & State_Selected) ? "toggled" :
      (st & State_MouseOver) ? "hovered" : "normal"
    : (st & State_On) ? "disabled-toggled" :
      (st & State_Selected) ? "disabled-toggled" : "disabled";
  } else {
      // no pressed/hovered state for containers
    status = (st & State_Enabled) ?
      (st & State_Selected) ? "toggled" :
      (st & State_On) ? "toggled" : "normal"
    : (st & State_On) ? "disabled-toggled" :
      (st & State_Selected) ? "disabled-toggled" : "disabled";
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
  return (st & State_Enabled) ? QIcon::On : QIcon::Off;
}
