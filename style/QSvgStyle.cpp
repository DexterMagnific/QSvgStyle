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
#include "QSvgStyle.h"

#include <stdlib.h>
#include <limits.h>

#ifdef QS_INSTRUMENTATION
#include <sys/time.h>
#endif

#include <QApplication>
#include <QWidget>
#include <QPainter>
#include <QRect>
#include <QSvgRenderer>
#include <QStyleOption>
#include <QSettings>
#include <QFile>
#include <QVariant>
#include <QBoxLayout>
#include <QGridLayout>
#include <QFont>
#include <QTimer>
#include <QList>
#include <QMap>

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

#include "../themeconfig/ThemeConfig.h"

//#define __DEBUG__

#ifdef __DEBUG__

#define DEBUG(...) printf("QSvgStyle "__VA_ARGS__)

#define fspecStr(fspec) \
    "frame={ " \
    + QString("hasFrame=%1").arg(fspec.hasFrame) + ", " \
    + "inherits=" + fspec.inherits + ", " \
    + "element="  + fspec.element + ", " \
    + QString("size(t,b,l,r)=(%1,%2,%3,%4)").arg(fspec.top).arg(fspec.bottom).arg(fspec.left).arg(fspec.right) + ", " \
    + QString("capsule(h,v)=(%1,%2)").arg(fspec.capsuleH).arg(fspec.capsuleV) + "), " \
    + QString("animFrames=%1").arg(fspec.animationFrames) + ", " \
    + QString("loopAnim=%1").arg(fspec.loopAnimation) \
    + " }"

#define ispecStr(ispec) \
    "interior={ " \
    + QString("hasInterior=%1").arg(ispec.hasInterior) + ", " \
    + "inherits=" + ispec.inherits + ", " \
    + "element="  + ispec.element + ", " \
    + QString("hasMargins=%1").arg(ispec.hasMargin) + ", " \
    + QString("pattern(h,v)=(%1,%2)").arg(ispec.px).arg(ispec.py) + ", " \
    + QString("animFrames=%1").arg(ispec.animationFrames) + ", " \
    + QString("loopAnim=%1").arg(ispec.loopAnimation) \
    + " }"

#define dspecStr(dspec) \
    "indicator={ " \
    + "inherits=" + dspec.inherits + ", " \
    + "element="  + dspec.element + ", " \
    + QString("hasMargins=%1").arg(ispec.hasMargin) + ", " \
    + " }"

#else
#define DEBUG(...)
#define fspecStr(...)
#define ispecStr(...)
#define dspecStr(...)
#endif

#ifdef QS_INSTRUMENTATION
static unsigned int level; /* indentation level */

static void indent() { level++; }
static void unindent() { level--; }
#define __debug(...) printf(__VA_ARGS__);

#define __enter_func__() \
  struct timeval __start_time; \
  gettimeofday(&__start_time, NULL);\
  printf("\n%*s%s {", level*2,"", __func__); \
  indent();

#define __exit_func() \
  struct timeval __exit_time; \
  struct timeval __diff_time; \
  gettimeofday(&__exit_time, NULL);\
  timersub(&__exit_time,&__start_time, &__diff_time); \
  unindent(); \
  printf("\n%*s} = %ld s %ld µs", level*2,"", __diff_time.tv_sec, __diff_time.tv_usec);

#define __print_group() \
  printf("\n%*sgroup=%s", level*2,"", (const char *)(group.toAscii()));\
  if (widget) {\
    printf("\n%*swidget=%s", level*2,"", (const char *)(widget->objectName().toAscii()));\
  }

#else
#define __enter_func__()
#define __exit_func()
#define __debug(...)
#define __print_group()
#endif

QSvgStyle::QSvgStyle()
  : QCommonStyle(),
    cls(QString(this->metaObject()->className())),
    defaultRndr(NULL),
    themeRndr(NULL),
    defaultSettings(NULL),
    themeSettings(NULL),
    settings(NULL),
    timer(NULL),
    progresstimer(NULL),
    animationEnabled(false),
    animationcount(0)
{
  __enter_func__();

  setBuiltinTheme();
  setUserTheme();

  timer = new QTimer(this);
  progresstimer = new QTimer(this);

  connect(timer,SIGNAL(timeout()), this,SLOT(slot_animate()));
  connect(progresstimer,SIGNAL(timeout()), this,SLOT(slot_animateProgressBars()));

  theme_spec_t tspec = settings->getThemeSpec();
  if (tspec.step.present)
    timer->start(tspec.step);
  else
    timer->start(250);

  __exit_func();
}

QSvgStyle::~QSvgStyle()
{
  delete defaultSettings;
  delete themeSettings;

  delete defaultRndr;
  delete themeRndr;
}

void QSvgStyle::setBuiltinTheme()
{
  if (defaultSettings) {
    delete defaultSettings;
    defaultSettings = NULL;
  }
  if (defaultRndr) {
    delete defaultRndr;
    defaultRndr = NULL;
  }
  if ( themeRndr ) {
    delete themeRndr;
    themeRndr = NULL;
  }
  if ( themeSettings ) {
    delete themeSettings;
    themeSettings = NULL;
  }

  defaultSettings = new ThemeConfig(":default.cfg");
  defaultRndr = new QSvgRenderer();
  defaultRndr->load(QString(":default.svg"));

  settings = defaultSettings;

  qDebug() << "["+cls+"]" << "Loaded built in theme";
}

void QSvgStyle::setTheme(const QString& theme)
{
  QString xdg_cfg;

  // get $XDG_CONFIG_HOME value
  char *_xdg_cfg = getenv("XDG_CONFIG_HOME");
  if ( _xdg_cfg )
    xdg_cfg = QString(_xdg_cfg);
  else
    return;

  if ( !defaultSettings || !defaultRndr )
    setBuiltinTheme();

  if ( themeSettings ) {
    delete themeSettings;
    themeSettings = NULL;
  }
  if ( themeRndr ) {
    delete themeRndr;
    themeRndr = NULL;
  }

  if ( !theme.isNull() &&
       !theme.isEmpty() &&
       QFile::exists(QString("%1/QSvgStyle/%2/%2.cfg").arg(xdg_cfg).arg(theme)) &&
       QFile::exists(QString("%1/QSvgStyle/%2/%2.svg").arg(xdg_cfg).arg(theme))
     ) {
    themeSettings = new ThemeConfig(QString("%1/QSvgStyle/%2/%2.cfg").arg(xdg_cfg).arg(theme));
    themeRndr = new QSvgRenderer();
    themeRndr->load(QString("%1/QSvgStyle/%2/%2.svg").arg(xdg_cfg).arg(theme));

    themeSettings->setParent(defaultSettings);
    settings = themeSettings;

    qDebug() << "["+cls+"]" << "Loaded user theme " << theme;
  }
}

void QSvgStyle::setUserTheme()
{
  QString theme;
  QSettings *globalSettings;
  QString xdg_cfg;

  // get $XDG_CONFIG_HOME value
  char *_xdg_cfg = getenv("XDG_CONFIG_HOME");
  if ( _xdg_cfg )
    xdg_cfg = QString(_xdg_cfg);
  else
    return;

  // load global config file
  if ( QFile::exists(QString("%1/QSvgStyle/qsvgstyle.cfg").arg(xdg_cfg)) ) {
    globalSettings = new QSettings(QString("%1/QSvgStyle/qsvgstyle.cfg").arg(xdg_cfg),QSettings::NativeFormat);

    if (globalSettings->contains("theme"))
      setTheme(globalSettings->value("theme").toString());

    delete globalSettings;
  }
}

bool QSvgStyle::isContainerWidget(const QWidget * widget) const
{
  return !widget || (widget && (
    widget->inherits("QFrame") ||
    widget->inherits("QGroupBox") ||
    widget->inherits("QTabWidget") ||
    widget->inherits("QDockWidget") ||
    widget->inherits("QMainWindow") ||
    widget->inherits("QDialog") ||
    widget->inherits("QDesktopWidget") ||
    (QString(widget->metaObject()->className()) == "QWidget")
  ));
}

bool QSvgStyle::isAnimatableWidget(const QWidget * widget) const
{
  // NOTE should we test against direct inheritance instead ?
  return widget && (
    widget->inherits("QPushButton") ||
    widget->inherits("QToolButton") ||
    widget->inherits("QProgressBar") ||
    widget->inherits("QLineEdit")
  );
}

void QSvgStyle::polish(QWidget * widget)
{
  if ( !widget )
    return;

  // set WA_Hover attribute for non container widgets,
  // this way we will receive paint events when entering
  // and leaving the widget
  if ( !isContainerWidget(widget) ) {
    widget->setAttribute(Qt::WA_Hover, true);
  }

  QString group = QString::null;

  if ( qobject_cast< const QPushButton* >(widget) )
    group = "PanelButtonCommand";

  if ( qobject_cast< const QToolButton* >(widget) )
    group = "PanelButtonTool";

  if ( qobject_cast< const QLineEdit* >(widget) )
    group = "LineEdit";

  if ( qobject_cast< const QProgressBar* >(widget) ) {
    group = "ProgressbarContents";
    widget->installEventFilter(this);
  }

  if ( qobject_cast< const QComboBox* >(widget) )
    group = "ComboBox";

  if ( qobject_cast< QMenu* >(widget) ) {
    (qobject_cast< QMenu* >(widget))->setTearOffEnabled(true);
  }

//     if ( qobject_cast< const QTabWidget* >(widget) )
//       group = "Tab";

  if ( !group.isNull() ) {
    const frame_spec_t fspec = getFrameSpec(group);
    const interior_spec_t ispec = getInteriorSpec(group);
    const indicator_spec_t dspec = getIndicatorSpec(group);

    if ( (fspec.animationFrames > 1) || (ispec.animationFrames > 1) || (dspec.animationFrames > 1) ) {
      animatedWidgets.append(widget);
    }
  }
}

void QSvgStyle::unpolish(QWidget * widget)
{
  Q_UNUSED(widget);
/*   if (widget) {
     widget->setAttribute(Qt::WA_Hover, false);

     animatedWidgets.removeOne(widget);
   }*/

  if ( qobject_cast< const QProgressBar* >(widget) ) {
    progressbars.remove(widget);
  }
}

void QSvgStyle::slot_animate()
{
#ifdef QS_INSTRUMENTATION
  printf("\n");
#endif

  if ( !animationEnabled )
    return;

  animationcount++;

  foreach (QWidget *widget, animatedWidgets) {
    if (widget->testAttribute(Qt::WA_UnderMouse)) {
      DEBUG("Repainting %s\n",(const char *)widget->objectName().toAscii());
      widget->repaint();
    }
  }
}

void QSvgStyle::slot_animateProgressBars()
{
  QMap<QWidget *,int>::iterator it;
  for (it=progressbars.begin(); it!=progressbars.end(); ++it) {

    QWidget *widget = it.key();
    if (widget->isVisible()) {
      it.value() += 2;
      widget->repaint();
    }
  }
}

bool QSvgStyle::eventFilter(QObject* o, QEvent* e)
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
    // forward event
    return QObject::eventFilter(o,e);
  }

  return QObject::eventFilter(o,e);
}

void QSvgStyle::drawPrimitive(PrimitiveElement e, const QStyleOption * option, QPainter * p, const QWidget * widget) const
{
  __enter_func__();
  emit(sig_drawPrimitive_begin(PE_str(e)));

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
  QString g = PE_group(e);

  // Configuration for group g
  frame_spec_t fs;
  interior_spec_t is;
  label_spec_t ls;
  indicator_spec_t ds;
  size_spec_t ss;

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
  ss = getSizeSpec(g);

  __print_group();

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
      renderInterior(p,r,fs,is,is.element+"-"+st);
      break;
    }
    case PE_FrameDefaultButton : {
      // Frame for default buttons
      // use "default" status
      if ( option->state & State_On ) {
        st = "default";
        renderFrame(p,r,fs,fs.element+"-"+st);
      }
      break;
    }
    case PE_FrameButtonBevel : {
      // Frame for push buttons
      capsulePosition(widget,fs.hasCapsule,fs.capsuleH,fs.capsuleV);
      renderFrame(p,r,fs,fs.element+"-"+st);
      break;
    }
    case PE_PanelButtonTool : {
      // Interior for tool buttons
      capsulePosition(widget,fs.hasCapsule,fs.capsuleH,fs.capsuleV);
      renderInterior(p,r,fs,is,is.element+"-"+st);
      break;
    }
    case PE_FrameButtonTool : {
      // Frame for tool buttons
      capsulePosition(widget,fs.hasCapsule,fs.capsuleH,fs.capsuleV);
      renderFrame(p,r,fs,fs.element+"-"+st);
      break;
    }
    case PE_IndicatorRadioButton : {
      // a radio button (exclusive choice)
      // QSvgStyle: no pressed or toggled status for radio buttons
      st = (option->state & State_Enabled) ?
          (option->state & State_MouseOver) ? "hovered" : "normal"
        : "disabled";
      if ( option->state & State_On )
        st = "checked-"+st;
      renderInterior(p,r,fs,is,is.element+"-"+st);
      break;
    }
    case PE_IndicatorViewItemCheck :
      // a check box inside view items
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
      renderInterior(p,r,fs,is,is.element+"-"+st);
      break;
    }
    case PE_IndicatorMenuCheckMark : {
      // Check box or radio button of a menu item
      if ( const QStyleOptionMenuItem *opt =
            qstyleoption_cast<const QStyleOptionMenuItem *>(option) ) {

        if ( opt->checkType == QStyleOptionMenuItem::Exclusive )
          drawPrimitive(PE_IndicatorRadioButton,opt,p,widget);
        else if ( opt->checkType == QStyleOptionMenuItem::NonExclusive )
          drawPrimitive(PE_IndicatorCheckBox,opt,p,widget);
      }
      break;
    }
    case PE_FrameFocusRect : {
      // The frame of a focus rectangle, used on focusable widgets
      renderFrame(p,r,fs,fs.element);
      break;
    }
    case PE_IndicatorBranch : {
      // Indicator of tree hierarchies
      if (option->state & State_Children) {
        if (option->state & State_Open)
          st = "minus-"+st;
        else
          st = "plus-"+st;
        renderIndicator(p,r,fs,is,ds,ds.element+"-"+st);
      }
      break;
    }
    case PE_FrameMenu :  {
      // Frame for menus
      renderFrame(p,r,fs,fs.element+"-"+st);
      break;
    }
    case PE_FrameWindow : {
      // Frame for windows
      renderFrame(p,r,fs,fs.element+"-"+st);
      break;
    }
    case PE_FrameTabBarBase : {
      // ???
      break;
    }
    case PE_Frame : {
      // Generic frame
      renderFrame(p,r,fs,fs.element+"-"+st);
      if ( const QStyleOptionFrame *opt =
           qstyleoption_cast<const QStyleOptionFrame *>(option) ) {
        if ( (opt->state & State_Sunken) || (opt->state & State_Raised) ) {
          renderInterior(p,r,fs,is,is.element+"-"+st);
        }
      }
      break;
    }
    case PE_FrameDockWidget : {
      // Frame for dock widgets
      renderFrame(p,r,fs,fs.element+"-"+st);
      break;
    }
    case PE_FrameStatusBarItem : {
      // Frame for status bar items
      renderFrame(p,r,fs,fs.element+"-"+st);
      break;
    }
    case PE_FrameGroupBox : {
      // Frame and interior for group boxes
      renderFrame(p,r,fs,fs.element+"-"+st);
      renderInterior(p,r,fs,is,is.element+"-"+st);
      break;
    }
    case PE_FrameTabWidget : {
      // Frame and interior for tab widgets (contents)
      renderFrame(p,r,fs,fs.element+"-"+st);
      renderInterior(p,r,fs,is,is.element+"-"+st);
      break;
    }
    case PE_FrameLineEdit : {
      // Frame for line edits
      renderFrame(p,r,fs,fs.element+"-"+st);
      break;
    }
    case PE_PanelLineEdit : {
      // Interior and frame for line edits
      if ( const QStyleOptionFrame *opt =
           qstyleoption_cast<const QStyleOptionFrame *>(option) ) {
        if ( opt->lineWidth > 0 )
          renderFrame(p,r,fs,fs.element+"-"+st);
      }
      renderInterior(p,r,fs,is,is.element+"-"+st);
      break;
    }
    case PE_PanelToolBar : {
      // toolbar frame and interior
      renderFrame(p,r,fs,fs.element+"-"+st);
      renderInterior(p,r,fs,is,is.element+"-"+st);
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
      renderIndicator(p,r,fs,is,ds,ds.element+"-plus-"+st);
      break;
    }
    case PE_IndicatorSpinMinus : {
      // Minus spin box indicator
      renderIndicator(p,r,fs,is,ds,ds.element+"-minus-"+st);
      break;
    }
    case PE_IndicatorSpinUp : {
      // Up spin box indicator
      renderIndicator(p,r,fs,is,ds,ds.element+"-up-"+st);
      break;
    }
    case PE_IndicatorSpinDown : {
      // down spin box indicator
      renderIndicator(p,r,fs,is,ds,ds.element+"-down-"+st);
      break;
    }
    case PE_IndicatorHeaderArrow : {
      // Header section sort arrows
      if ( const QStyleOptionHeader *opt =
           qstyleoption_cast<const QStyleOptionHeader *>(option) ) {
        if (opt->sortIndicator == QStyleOptionHeader::SortDown)
          renderIndicator(p,r,fs,is,ds,ds.element+"-down-"+st);
        else if (opt->sortIndicator == QStyleOptionHeader::SortUp)
          renderIndicator(p,r,fs,is,ds,ds.element+"-up-"+st);
      }
      break;
    }
    case PE_IndicatorButtonDropDown : {
      // Drop down button arrow
      renderIndicator(p,r,fs,is,ds,ds.element+"-dropdown-"+st);
      break;
    }
    case PE_PanelMenuBar : {
      // FIXME
      break;
    }
    case PE_IndicatorTabTear : {
      // FIXME
      break;
    }
    case PE_IndicatorArrowUp : {
      // Arrow up indicator
      renderIndicator(p,r,fs,is,ds,ds.element+"-up-"+st);
      break;
    }
    case PE_IndicatorArrowDown : {
      // Arrow down indicator
      renderIndicator(p,r,fs,is,ds,ds.element+"-down-"+st);
      break;
    }
    case PE_IndicatorArrowLeft : {
      // Arrow left indicator
      renderIndicator(p,r,fs,is,ds,ds.element+"-left-"+st);
      break;
    }
    case PE_IndicatorArrowRight : {
      // Arrow right indicator
      renderIndicator(p,r,fs,is,ds,ds.element+"-right-"+st);
      break;
    }
    case PE_IndicatorColumnViewArrow : {
      // FIXME
      break;
    }
    case PE_IndicatorProgressChunk : {
      // The "filled" part of a progress bar
      renderInterior(p,r,fs,is,is.element+"-"+st);
      break;
    }
    case PE_PanelItemViewRow : {
      // A row of a item view list
      if ( option->state & State_Enabled ) {
        if ( const QStyleOptionViewItemV4 *opt =
             qstyleoption_cast<const QStyleOptionViewItemV4 *>(option) ) {

          if ( opt->features & QStyleOptionViewItemV2::Alternate )
            st = st+"-alt";
        }
      }
      renderInterior(p,r,fs,is,is.element+"-"+st);
      break;
    }
    case PE_PanelItemViewItem : {
      // An item of a view item
      // FIXME
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
  __exit_func();
}

void QSvgStyle::drawControl(ControlElement e, const QStyleOption * option, QPainter * p, const QWidget * widget) const
{
  __enter_func__();
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

  Q_UNUSED(dir);
  Q_UNUSED(focus);

  // Get QSvgStyle configuration group used to render this element
  QString g = CE_group(e);

  // Configuration for group g
  frame_spec_t fs;
  interior_spec_t is;
  label_spec_t ls;
  indicator_spec_t ds;
  size_spec_t ss;

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
  ss = getSizeSpec(g);

  __print_group();

  switch (e) {
    case CE_PushButtonBevel : {
      drawPrimitive(PE_FrameButtonBevel,option,p,widget);
      drawPrimitive(PE_PanelButtonBevel,option,p,widget);
      break;
    }
    case CE_MenuTearoff : {
      const QStyleOptionMenuItem *opt =
          qstyleoption_cast<const QStyleOptionMenuItem *>(option);

      if (opt) {
        const QString group = "MenuItem";

        const indicator_spec_t ds = getIndicatorSpec(group);

        __print_group();

        renderElement(p,ds.element+"-tearoff",option->rect,10,0);
      }

      break;
    }
    case CE_MenuItem : {
      const QStyleOptionMenuItem *opt =
          qstyleoption_cast<const QStyleOptionMenuItem *>(option);

      if (opt) {
        const QString group = "MenuItem";

        frame_spec_t fs = getFrameSpec(group);
        const interior_spec_t is = getInteriorSpec(group);
        const indicator_spec_t ds = getIndicatorSpec(group);
        const label_spec_t ls = getLabelSpec(group);

        __print_group();

        if (opt->menuItemType == QStyleOptionMenuItem::Separator)
          renderElement(p,ds.element+"-separator",option->rect,20,0);
        else if (opt->menuItemType == QStyleOptionMenuItem::TearOff)
          renderElement(p,ds.element+"-tearoff",option->rect,20,0);
        else {
          renderFrame(p,option->rect,fs,fs.element+"-"+st);
          renderInterior(p,option->rect,fs,is,is.element+"-"+st);

          const QStringList l = opt->text.split('\t');

          if (l.size() > 0) {
            // menu label
            if (opt->icon.isNull())
              renderLabel(p,dir,option->rect.adjusted(opt->maxIconWidth+ls.tispace,0,0,0),fs,is,ls,Qt::AlignLeft|Qt::AlignVCenter | Qt::TextShowMnemonic |Qt::TextSingleLine,l[0], !(option->state & State_Enabled));
            else
              renderLabel(p,dir,option->rect,fs,is,ls,Qt::AlignLeft|Qt::AlignVCenter | Qt::TextShowMnemonic |Qt::TextSingleLine,l[0],!(option->state & State_Enabled),opt->icon.pixmap(opt->maxIconWidth,icm,ics));
          }
          if (l.size() > 1)
            // shortcut
            renderLabel(p,dir,option->rect.adjusted(opt->maxIconWidth,0,-15,0),fs,is,ls,Qt::AlignRight|Qt::AlignVCenter | Qt::TextShowMnemonic| Qt::TextSingleLine,l[1],!(option->state & State_Enabled));

          QStyleOptionMenuItem o(*opt);
          o.rect = alignedRect(QApplication::layoutDirection(),Qt::AlignRight | Qt::AlignVCenter,QSize(10,10),labelRect(o.rect,fs,is,ls));
          if (opt->menuItemType == QStyleOptionMenuItem::SubMenu) {
            if ( dir == Qt::LeftToRight )
              drawPrimitive(PE_IndicatorArrowRight,&o,p);
            else
              drawPrimitive(PE_IndicatorArrowLeft,&o,p);
          }

          if (opt->checkType == QStyleOptionMenuItem::Exclusive) {
            if (opt->checked)
              o.state |= State_On;
            drawPrimitive(PE_IndicatorRadioButton,&o,p,widget);
          }

          if (opt->checkType == QStyleOptionMenuItem::NonExclusive) {
            if (opt->checked)
              o.state |= State_On;
            drawPrimitive(PE_IndicatorCheckBox,&o,p,widget);
          }
        }
      }

      break;
    }

    case CE_MenuEmptyArea : {
      break;
    }

    case CE_MenuBarItem : {
      const QStyleOptionMenuItem *opt =
          qstyleoption_cast<const QStyleOptionMenuItem *>(option);

      if (opt) {
        QString group = "MenuBar";
        frame_spec_t fs = getFrameSpec(group);
        fs.hasCapsule = true;
        fs.capsuleH = 0;
        fs.capsuleV = 2;
        group = "MenuBarItem";
        const interior_spec_t is = getInteriorSpec(group);
        const label_spec_t ls = getLabelSpec(group);

        __print_group();

        renderFrame(p,option->rect,fs,fs.element+"-"+st);
        renderInterior(p,option->rect,fs,is,is.element+"-"+st);

        renderLabel(p,dir,option->rect,fs,is,ls,Qt::AlignLeft | Qt::AlignVCenter | Qt::TextShowMnemonic | Qt::TextSingleLine,opt->text,!(option->state & State_Enabled));
      }

      break;
    }

    case CE_MenuBarEmptyArea : {
        const QString group = "MenuBar";

        const frame_spec_t fs = getFrameSpec(group);
        const interior_spec_t is = getInteriorSpec(group);

        __print_group();

        // NOTE this does not use the status (otherwise always disabled)
        renderFrame(p,option->rect,fs,fs.element+"-normal");
        renderInterior(p,option->rect,fs,is,is.element+"-normal");

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

        renderLabel(p,dir,r,fs,is,ls,
                    Qt::AlignLeft | Qt::AlignVCenter | Qt::TextShowMnemonic,
                    opt->text,!(option->state & State_Enabled),
                    opt->icon.pixmap(opt->iconSize,icm,ics));
      }
      break;
    }
    case CE_CheckBoxLabel : {
      if ( const QStyleOptionButton *opt =
          qstyleoption_cast<const QStyleOptionButton *>(option) ) {

        renderLabel(p,dir,r,fs,is,ls,
                    Qt::AlignLeft | Qt::AlignVCenter | Qt::TextShowMnemonic,
                    opt->text,!(option->state & State_Enabled),
                    opt->icon.pixmap(opt->iconSize,icm,ics));
      }
      break;
    }
    case CE_ComboBoxLabel : {
      if ( const QStyleOptionComboBox *opt =
           qstyleoption_cast<const QStyleOptionComboBox *>(option) ) {

        if ( !opt->editable ) {
          // NOTE Editable label is rendered by a embedded QLineEdit
          // inside the QComboBox object, except icon
          // See QComboBox's qcombobox.cpp::updateLineEditGeometry()
          renderLabel(p,dir,r,fs,is,ls,
                      Qt::AlignLeft | Qt::AlignVCenter | Qt::TextShowMnemonic,
                      opt->currentText,
                      !(option->state & State_Enabled),
                      opt->currentIcon.pixmap(opt->iconSize,icm,ics));
        } else {
          // NOTE Non editable combo boxes: the embedded QLineEdit is not
          // able to draw the item icon, so do it here
          renderLabel(p,dir,r,fs,is,ls,
                      Qt::AlignLeft | Qt::AlignVCenter | Qt::TextShowMnemonic,
                      " ", // NOTE renderLabel centers icons if text is empty
                      !(option->state & State_Enabled),
                      opt->currentIcon.pixmap(opt->iconSize,icm,ics));
        }
      }
      break;
    }

    case CE_TabBarTabShape : {
      const QString group = "Tab";

      frame_spec_t fs = getFrameSpec(group);
      const interior_spec_t is = getInteriorSpec(group);

      __print_group();

      const QStyleOptionTab *opt =
          qstyleoption_cast<const QStyleOptionTab *>(option);

      if (opt) {
        fs.hasCapsule = true;
        int capsule = 2;

        if (opt->position == QStyleOptionTab::Beginning)
          capsule = -1;
        else if (opt->position == QStyleOptionTab::Middle)
          capsule = 0;
        else if (opt->position == QStyleOptionTab::End)
          capsule = 1;
        else if (opt->position == QStyleOptionTab::OnlyOneTab)
          capsule = 2;

        if ( (opt->shape == QTabBar::RoundedNorth) ||
             (opt->shape == QTabBar::TriangularNorth)
        ) {
          fs.capsuleH = capsule;
          fs.capsuleV = -1;
        }

        if ( (opt->shape == QTabBar::RoundedSouth) ||
             (opt->shape == QTabBar::TriangularSouth)
        ) {
          fs.capsuleH = capsule;
          fs.capsuleV = 1;
        }

        if ( (opt->shape == QTabBar::RoundedWest) ||
             (opt->shape == QTabBar::TriangularWest)
        ) {
          fs.capsuleV = capsule;
          fs.capsuleH = -1;
        }

        if ( (opt->shape == QTabBar::RoundedEast) ||
             (opt->shape == QTabBar::TriangularEast)
        ) {
          fs.capsuleV = capsule;
          fs.capsuleH = 1;
        }
      }

      renderInterior(p,option->rect,fs,is,is.element+"-"+st);
      renderFrame(p,option->rect,fs,fs.element+"-"+st);

      break;
    }

    case CE_TabBarTabLabel : {
      const QStyleOptionTab *opt =
        qstyleoption_cast<const QStyleOptionTab *>(option);

      if (opt) {
        const QString group = "Tab";

        const frame_spec_t fs = getFrameSpec(group);
        const interior_spec_t is = getInteriorSpec(group);
        const label_spec_t ls = getLabelSpec(group);

        __print_group();

        renderLabel(p,dir,option->rect,fs,is,ls,Qt::AlignLeft | Qt::AlignVCenter | Qt::TextShowMnemonic,opt->text,!(option->state & State_Enabled),opt->icon.pixmap(pixelMetric(PM_TabBarIconSize),icm,ics));
      }

      break;
    }

    case CE_ToolBoxTabShape : {
      const QString group = "ToolboxTab";

      frame_spec_t fs = getFrameSpec(group);
      const interior_spec_t is = getInteriorSpec(group);

      __print_group();

      renderFrame(p,option->rect,fs,fs.element+"-"+st);
      renderInterior(p,option->rect,fs,is,is.element+"-"+st);

      break;
    }

    case CE_ToolBoxTabLabel : {
      const QStyleOptionToolBox *opt =
          qstyleoption_cast<const QStyleOptionToolBox *>(option);

      if (opt) {
        const QString group = "ToolboxTab";

        frame_spec_t fs = getFrameSpec(group);
        const interior_spec_t is = getInteriorSpec(group);
        const label_spec_t ls = getLabelSpec(group);

        __print_group();

        renderLabel(p,dir,option->rect,fs,is,ls,Qt::AlignCenter | Qt::TextShowMnemonic,opt->text,!(option->state & State_Enabled),opt->icon.pixmap(pixelMetric(PM_TabBarIconSize),icm,ics));
      }

      break;
    }

    case CE_ProgressBarGroove : {
      const QString group = "Progressbar";

      frame_spec_t fs = getFrameSpec(group);
      const interior_spec_t is = getInteriorSpec(group);

      __print_group();

      renderFrame(p,option->rect,fs,fs.element+"-"+st);
      renderInterior(p,option->rect,fs,is,is.element+"-"+st);

      break;
    }

    case CE_ProgressBarContents : {
      const QStyleOptionProgressBar *opt =
          qstyleoption_cast<const QStyleOptionProgressBar *>(option);

      if (opt) {
        const QString group = "ProgressbarContents";

        frame_spec_t fs = getFrameSpec(group);
        const interior_spec_t is = getInteriorSpec(group);

        __print_group();

        if ( opt->progress >= 0 ) {
          int empty = sliderPositionFromValue(opt->minimum,opt->maximum,opt->maximum-opt->progress,w,false);

          renderFrame(p,option->rect.adjusted(0,0,-empty,0),fs,fs.element+"-"+st);
          renderInterior(p,option->rect.adjusted(0,0,-empty,0),fs,is,is.element+"-"+st);
        } else { // busy progressbar
          QWidget *w = (QWidget *)widget;

          int animcount = progressbars[w];
          int pm = pixelMetric(PM_ProgressBarChunkWidth);
          QRect r = option->rect.adjusted(animcount,0,0,0);
          r.setX(option->rect.x()+(animcount%option->rect.width()));
          r.setWidth(pm);
          if ( r.x()+r.width()-1 > option->rect.x()+option->rect.width()-1 ) {
            // wrap busy indicator
            fs.hasCapsule = true;
            fs.capsuleH = 1;
            fs.capsuleV = 2;
            r.setWidth(option->rect.x()+option->rect.width()-r.x());
            renderFrame(p,r,fs,fs.element+"-"+st);
            renderInterior(p,r,fs,is,is.element+"-"+st);

            fs.capsuleH = -1;
            r = QRect(option->rect.x(),option->rect.y(),pm-r.width(),option->rect.height());
            renderFrame(p,r,fs,fs.element+"-"+st);
            renderInterior(p,r,fs,is,is.element+"-"+st);
          } else {
            renderFrame(p,r,fs,fs.element+"-"+st);
            renderInterior(p,r,fs,is,is.element+"-"+st);
          }
        }
      }

      break;
    }

    case CE_ProgressBarLabel : {
      const QStyleOptionProgressBar *opt =
          qstyleoption_cast<const QStyleOptionProgressBar *>(option);

      if (opt && opt->textVisible) {
        const QString group = "Progressbar";

        const frame_spec_t fs = getFrameSpec(group);
        const interior_spec_t is = getInteriorSpec(group);
        const label_spec_t ls = getLabelSpec(group);

        __print_group();

        renderLabel(p,dir,option->rect,fs,is,ls,opt->textAlignment,opt->text,!(option->state & State_Enabled));
      }

      break;
    }

    case CE_Splitter : {
      const QString group = "Splitter";

      const frame_spec_t fs = getFrameSpec(group);
      const interior_spec_t is = getInteriorSpec(group);

      __print_group();

      renderFrame(p,option->rect,fs,fs.element+"-"+st);
      renderInterior(p,option->rect,fs,is,is.element+"-"+st, (h > w) ? Qt::Vertical : Qt::Horizontal);

      break;
    }

    case CE_ScrollBarAddLine : {
      const QString group = "Scrollbar";

      const frame_spec_t fs = getFrameSpec(group);
      const interior_spec_t is = getInteriorSpec(group);
      const indicator_spec_t ds = getIndicatorSpec(group);

      __print_group();

      renderFrame(p,option->rect,fs,fs.element+"-"+st);
      renderInterior(p,option->rect,fs,is,is.element+"-"+st);
      if (option->state & State_Horizontal)
        renderIndicator(p,option->rect,fs,is,ds,ds.element+"-right-"+st);
      else
        renderIndicator(p,option->rect,fs,is,ds,ds.element+"-down-"+st);

      break;
    }

    case CE_ScrollBarSubLine : {
      const QString group = "Scrollbar";

      const frame_spec_t fs = getFrameSpec(group);
      const interior_spec_t is = getInteriorSpec(group);
      const indicator_spec_t ds = getIndicatorSpec(group);

      __print_group();

      renderFrame(p,option->rect,fs,fs.element+"-"+st);
      renderInterior(p,option->rect,fs,is,is.element+"-"+st);
      if (option->state & State_Horizontal)
        renderIndicator(p,option->rect,fs,is,ds,ds.element+"-left-"+st);
      else
        renderIndicator(p,option->rect,fs,is,ds,ds.element+"-up-"+st);

      break;
    }

    case CE_ScrollBarSlider : {
      const QString group = "ScrollbarSlider";

      const frame_spec_t fs = getFrameSpec(group);
      const interior_spec_t is = getInteriorSpec(group);

      __print_group();

      renderFrame(p,option->rect,fs,fs.element+"-"+st);
      renderInterior(p,option->rect,fs,is,is.element+"-"+st);

      break;
    }

    case CE_HeaderSection : {
      renderFrame(p,option->rect,fs,fs.element+"-"+st);
      renderInterior(p,option->rect,fs,is,is.element+"-"+st);
      break;
    }

    case CE_HeaderLabel : {
      if ( const QStyleOptionHeader *opt =
           qstyleoption_cast<const QStyleOptionHeader *>(option) ) {

        renderLabel(p,dir,option->rect,fs,is,ls,opt->textAlignment,opt->text,!(option->state & State_Enabled),opt->icon.pixmap(pixelMetric(PM_SmallIconSize),icm,ics));
      }
      break;
    }

    case CE_ToolBar : {
      const QString group = "Toolbar";

      const frame_spec_t fs = getFrameSpec(group);
      const interior_spec_t is = getInteriorSpec(group);
      const indicator_spec_t ds = getIndicatorSpec(group);

      __print_group();

      renderFrame(p,option->rect,fs,fs.element+"-"+st);
      renderInterior(p,option->rect,fs,is,is.element+"-"+st, (option->state & State_Horizontal) ? Qt::Horizontal : Qt::Vertical);

      break;
    }

    case CE_SizeGrip : {
      const QString group = "SizeGrip";

      const frame_spec_t fs = getFrameSpec(group);
      const interior_spec_t is = getInteriorSpec(group);
      const indicator_spec_t ds = getIndicatorSpec(group);

      __print_group();

      renderFrame(p,option->rect,fs,fs.element+"-"+st);
      renderInterior(p,option->rect,fs,is,is.element+"-"+st);
      renderIndicator(p,option->rect,fs,is,ds,ds.element+"-"+st);

      break;
    }

    case CE_PushButton : {
      drawControl(CE_PushButtonBevel,option,p,widget);
      drawControl(CE_PushButtonLabel,option,p,widget);
      break;
    }
    case CE_PushButtonLabel : {
      if ( const QStyleOptionButton *opt =
           qstyleoption_cast<const QStyleOptionButton *>(option) ) {

        if ( opt->features & QStyleOptionButton::HasMenu ) {
	  QStyleOptionButton o(*opt);
	  renderLabel(p,dir,r.adjusted(0,0,-ds.size-ls.tispace,0),fs,is,ls,
                      Qt::AlignCenter | Qt::AlignVCenter | Qt::TextShowMnemonic,
                      opt->text,!(option->state & State_Enabled),
                      opt->icon.pixmap(opt->iconSize,icm,ics));
	  o.rect = QRect(x+w-ls.tispace-ds.size-fs.right,y,ds.size,h);
          drawPrimitive(PE_IndicatorArrowDown,&o,p,widget);
	} else {
	  renderLabel(p,dir,r,fs,is,ls,
                      Qt::AlignCenter | Qt::AlignVCenter | Qt::TextShowMnemonic,
                      opt->text,!(option->state & State_Enabled),
                      opt->icon.pixmap(opt->iconSize,icm,ics));
	}
      }

      break;
    }
    case CE_ToolButtonLabel : {
      if ( const QStyleOptionToolButton *opt =
          qstyleoption_cast<const QStyleOptionToolButton *>(option) ) {

        if (opt->arrowType == Qt::NoArrow)
          renderLabel(p,dir,r,fs,is,ls,
                      Qt::AlignCenter | Qt::TextShowMnemonic,
                      opt->text,!(option->state & State_Enabled),
                      opt->icon.pixmap(opt->iconSize,icm,ics),
                      opt->toolButtonStyle);
        else {
          if ( dir == Qt::LeftToRight )
            renderLabel(p,dir,r.adjusted(ds.size+ls.tispace,0,0,0),fs,is,ls,
                        Qt::AlignCenter | Qt::TextShowMnemonic,
                        opt->text,!(option->state & State_Enabled),
                        opt->icon.pixmap(opt->iconSize,icm,ics),
                        opt->toolButtonStyle);
          else
            renderLabel(p,dir,r.adjusted(0,0,-ds.size-ls.tispace,0),fs,is,ls,
                        Qt::AlignCenter | Qt::TextShowMnemonic,
                        opt->text,!(option->state & State_Enabled),
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
                            hAlign | Qt::AlignVCenter);
            break;
          case Qt::DownArrow :
            renderIndicator(p,r,fs,is,ds,ds.element+"-down-"+st,
                            hAlign | Qt::AlignVCenter);
            break;
          case Qt::LeftArrow :
            renderIndicator(p,option->rect,fs,is,ds,ds.element+"-left-"+st,
                            hAlign | Qt::AlignVCenter);
            break;
          case Qt::RightArrow :
            renderIndicator(p,option->rect,fs,is,ds,ds.element+"-right-"+st,
                            hAlign | Qt::AlignVCenter);
            break;
        }
      }

      break;
    }

    case CE_DockWidgetTitle : {
      const QStyleOptionDockWidget *opt =
          qstyleoption_cast<const QStyleOptionDockWidget *>(option);

      if (opt) {
        const QString group = "Dock";

        const frame_spec_t fs = getFrameSpec(group);
        const interior_spec_t is = getInteriorSpec(group);
        const label_spec_t ls = getLabelSpec(group);

        const QDockWidget *w = qobject_cast<const QDockWidget *>(widget);
        __print_group();

        renderFrame(p,option->rect,fs,fs.element+"-"+st);
        if ( w ) {
          renderInterior(p,option->rect,fs,is,is.element+"-"+st, (w->features() & QDockWidget::DockWidgetVerticalTitleBar) ? Qt::Vertical : Qt::Horizontal);
        } else {
          renderInterior(p,option->rect,fs,is,is.element+"-"+st);
        }
        renderLabel(p,dir,option->rect,fs,is,ls,Qt::AlignLeft | Qt::AlignVCenter | Qt::TextShowMnemonic,opt->title,!(option->state & State_Enabled));
      }

      break;
    }

    case CE_ShapedFrame : {
      const QString group = "GenericFrame";

      const frame_spec_t fs = getFrameSpec(group);
      const interior_spec_t is = getInteriorSpec(group);

      __print_group();

      const QStyleOptionFrameV3 *opt =
        qstyleoption_cast<const QStyleOptionFrameV3 *>(option);

      if ( opt && (opt->frameShape == QFrame::HLine) ) {
        renderElement(p,
                      fs.element+"-"+"hsep",
                      option->rect,
                      0,0,Qt::Horizontal,animationcount%fs.animationFrames);
      } else if (opt && (opt->frameShape == QFrame::VLine) ) {
        renderElement(p,
                      fs.element+"-"+"vsep",
                      option->rect,
                      0,0,Qt::Horizontal,animationcount%fs.animationFrames);
      } else if (opt && (opt->frameShape != QFrame::NoFrame) ) {
        drawPrimitive(PE_Frame,opt,p,widget);
      }

      break;
    }

    default :
      //qDebug() << "[QSvgStyle] " << __func__ << ": Unhandled control " << element;
      QCommonStyle::drawControl(e,option,p,widget);
  }

end:
  emit(sig_drawControl_end(CE_str(e)));
  __exit_func();
}

void QSvgStyle::drawComplexControl(ComplexControl control, const QStyleOptionComplex * option, QPainter * p, const QWidget * widget) const
{
  __enter_func__();
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
  size_spec_t ss;

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
  ss = getSizeSpec(g);

  __print_group();

  switch (control) {
    case CC_ToolButton : {
      if ( const QStyleOptionToolButton *opt =
          qstyleoption_cast<const QStyleOptionToolButton *>(option) ) {

        QStyleOptionToolButton o(*opt);

        // draw frame and interior
        if ( option->state & State_AutoRaise ) {
          // Auto raise buttons (found on toolbars)
          if ( (option->state & State_Enabled) &&
                ((option->state & State_Sunken) ||
                (option->state & State_On) ||
                (option->state & State_MouseOver))
              ) {
            // Draw frame and interior around normal non autoraise tool buttons
            drawPrimitive(PE_FrameButtonTool,&o,p,widget);
            drawPrimitive(PE_PanelButtonTool,&o,p,widget);
          }
        } else {
          drawPrimitive(PE_FrameButtonTool,&o,p,widget);
          drawPrimitive(PE_PanelButtonTool,&o,p,widget);
        }

        // Draw label
        o.rect = subControlRect(CC_ToolButton,opt,SC_ToolButton,widget);
        drawControl(CE_ToolButtonLabel,&o,p,widget);

        // Draw arrow
        o.rect = subControlRect(CC_ToolButton,opt,SC_ToolButtonMenu,widget);
        if (opt->features & QStyleOptionToolButton::Menu) {
          // FIXME draw drop down button separator
          // Tool button with independant drop down button
          drawPrimitive(PE_IndicatorButtonDropDown,&o,p,widget);
        } else if (opt->features & QStyleOptionToolButton::HasMenu)
          // Simple down arrow for tool buttons with menus
          drawPrimitive(PE_IndicatorArrowDown,&o,p,widget);
      }

      break;
    }

    case CC_SpinBox : {
      if (const QStyleOptionSpinBox *opt =
          qstyleoption_cast<const QStyleOptionSpinBox *>(option)) {

        QStyleOptionSpinBox o(*opt);

        // Remove sunken,pressed,mouse over attributes


        // draw frame
        o.rect = subControlRect(CC_SpinBox,opt,SC_SpinBoxFrame,widget);
        drawPrimitive(PE_FrameLineEdit,&o,p,widget);

        if (opt->buttonSymbols == QAbstractSpinBox::UpDownArrows) {
          o.state &= ~(State_Sunken | State_Selected | State_MouseOver);
          if ( opt->activeSubControls & SC_SpinBoxUp )
            o.state = opt->state;
          o.rect = subControlRect(CC_SpinBox,opt,SC_SpinBoxUp,widget);
          drawPrimitive(PE_PanelButtonBevel,&o,p,widget);
          drawPrimitive(PE_IndicatorSpinUp,&o,p,widget);

          o.state &= ~(State_Sunken | State_Selected | State_MouseOver);
          if ( opt->activeSubControls & SC_SpinBoxDown )
            o.state = opt->state;
          o.rect = subControlRect(CC_SpinBox,opt,SC_SpinBoxDown,widget);
          drawPrimitive(PE_PanelButtonBevel,&o,p,widget);
          drawPrimitive(PE_IndicatorSpinDown,&o,p,widget);
        }
        if (opt->buttonSymbols == QAbstractSpinBox::PlusMinus) {
          o.rect = subControlRect(CC_SpinBox,opt,SC_SpinBoxUp,widget);
          drawPrimitive(PE_IndicatorSpinPlus,&o,p,widget);
          o.rect = subControlRect(CC_SpinBox,opt,SC_SpinBoxDown,widget);
          drawPrimitive(PE_IndicatorSpinMinus,&o,p,widget);
        }
      }

      break;
    }

    case CC_ComboBox : {
      if ( const QStyleOptionComboBox *opt =
           qstyleoption_cast<const QStyleOptionComboBox *>(option) ) {

        QStyleOptionComboBox o(*opt);

        // Draw frame and interior
        o.rect = subControlRect(CC_ComboBox,opt,SC_ComboBoxFrame,widget);
        // FIXME don't draw frame in case of !opt->frame
        if ( !opt->editable )
          drawControl(CE_PushButtonBevel,&o,p,widget);
        else {
          drawPrimitive(PE_FrameLineEdit,&o,p,widget);
          drawPrimitive(PE_PanelLineEdit,&o,p,widget);
        }

        // NOTE Don't draw the label, QComboBox will draw it for us
        // see Qt's implementation qcombobox.cpp::paintEvent()

        // draw drop down button
        // FIXME render separator
        o.rect = subControlRect(CC_ComboBox,opt,SC_ComboBoxArrow,widget);
        drawPrimitive(PE_IndicatorButtonDropDown,&o,p,widget);
      }

      break;
    }

    case CC_ScrollBar : {
      const QStyleOptionSlider *opt =
        qstyleoption_cast<const QStyleOptionSlider *>(option);

      if (opt) {
        QStyleOptionSlider o(*opt);

        const QString group = "ScrollbarGroove";

        const frame_spec_t fspec = getFrameSpec(group);
        const interior_spec_t ispec = getInteriorSpec(group);

        o.rect = subControlRect(CC_ScrollBar,opt,SC_ScrollBarGroove,widget);
        renderFrame(p,o.rect,fspec,fspec.element+"-"+st);
        renderInterior(p,o.rect,fspec,ispec,ispec.element+"-"+st);

        o.rect = subControlRect(CC_ScrollBar,opt,SC_ScrollBarAddLine,widget);
        drawControl(CE_ScrollBarAddLine,&o,p,widget);

        o.rect = subControlRect(CC_ScrollBar,opt,SC_ScrollBarSubLine,widget);
        drawControl(CE_ScrollBarSubLine,&o,p,widget);

        o.rect = subControlRect(CC_ScrollBar,opt,SC_ScrollBarSlider,widget);
        drawControl(CE_ScrollBarSlider,&o,p,widget);
      }

      break;
    }

    case CC_Slider : {
      const QStyleOptionSlider *opt =
        qstyleoption_cast<const QStyleOptionSlider *>(option);

      if (opt) {
        QStyleOptionSlider o(*opt);

        QString group = "Slider";

        frame_spec_t fspec = getFrameSpec(group);
        interior_spec_t ispec = getInteriorSpec(group);

        QRect empty = subControlRect(CC_Slider,opt,SC_SliderGroove,widget);
        QRect full = subControlRect(CC_Slider,opt,SC_SliderGroove,widget);
        QRect slider = subControlRect(CC_Slider,opt,SC_SliderHandle,widget);

        // take into account inversion
        if (option->state & State_Horizontal)
          if (!opt->upsideDown) {
            full.setWidth(slider.x());
            empty.adjust(slider.x(),0,0,0);
          } else {
            empty.setWidth(slider.x());
            full.adjust(slider.x(),0,0,0);
          }
        else
          if (!opt->upsideDown) {
            full.setHeight(slider.y());
            empty.adjust(0,slider.y(),0,0);
          } else {
            empty.setHeight(slider.y());
            full.adjust(0,slider.y(),0,0);
          }

        fspec.hasCapsule = true;
        if (option->state & State_Horizontal)
          fspec.capsuleV = 2;
        else
          fspec.capsuleH = 2;

        if (option->state & State_Enabled) {
          if (option->state & State_MouseOver) {
            if (option->state & State_Horizontal)
              if (!opt->upsideDown) {
                fspec.capsuleH = 1;
              } else {
                fspec.capsuleH = -1;
              }
            else
              if (!opt->upsideDown) {
                fspec.capsuleV = 1;
              } else {
                fspec.capsuleV = -1;
              }
            renderFrame(p,empty,fspec,fspec.element+"-focused");
            renderInterior(p,empty,fspec,ispec,ispec.element+"-focused",(option->state & State_Horizontal) ? Qt::Vertical : Qt::Horizontal);
            if (option->state & State_Horizontal)
              if (!opt->upsideDown) {
                fspec.capsuleH = -1;
              } else {
                fspec.capsuleH = 1;
              }
            else
              if (!opt->upsideDown) {
                fspec.capsuleV = -1;
              } else {
                fspec.capsuleV = 1;
              }
            renderFrame(p,full,fspec,fspec.element+"-toggled");
            renderInterior(p,full,fspec,ispec,ispec.element+"-toggled",(option->state & State_Horizontal) ? Qt::Vertical : Qt::Horizontal);
          } else {
            if (option->state & State_Horizontal)
              if (!opt->upsideDown) {
                fspec.capsuleH = 1;
              } else {
                fspec.capsuleH = -1;
              }
            else
              if (!opt->upsideDown) {
                fspec.capsuleV = 1;
              } else {
                fspec.capsuleV = -1;
              }
            renderFrame(p,empty,fspec,fspec.element+"-normal");
            renderInterior(p,empty,fspec,ispec,ispec.element+"-normal",(option->state & State_Horizontal) ? Qt::Vertical : Qt::Horizontal);
            if (option->state & State_Horizontal)
              if (!opt->upsideDown) {
                fspec.capsuleH = -1;
              } else {
                fspec.capsuleH = 1;
              }
            else
              if (!opt->upsideDown) {
                fspec.capsuleV = -1;
              } else {
                fspec.capsuleV = 1;
              }
            renderFrame(p,full,fspec,fspec.element+"-toggled");
            renderInterior(p,full,fspec,ispec,ispec.element+"-toggled",(option->state & State_Horizontal) ? Qt::Vertical : Qt::Horizontal);
          }
        } else {
          if (option->state & State_Horizontal)
            if (!opt->upsideDown) {
              fspec.capsuleH = 1;
            } else {
              fspec.capsuleH = -1;
            }
          else
            if (!opt->upsideDown) {
              fspec.capsuleV = 1;
            } else {
              fspec.capsuleV = -1;
            }
          renderFrame(p,empty,fspec,fspec.element+"-disabled");
          renderInterior(p,empty,fspec,ispec,ispec.element+"-disabled",(option->state & State_Horizontal) ? Qt::Vertical : Qt::Horizontal);
          if (option->state & State_Horizontal)
            if (!opt->upsideDown) {
              fspec.capsuleH = -1;
            } else {
              fspec.capsuleH = 1;
            }
          else
            if (!opt->upsideDown) {
              fspec.capsuleV = -1;
            } else {
                fspec.capsuleV = 1;
            }
          renderFrame(p,full,fspec,fspec.element+"-disabled");
          renderInterior(p,full,fspec,ispec,ispec.element+"-disabled",(option->state & State_Horizontal) ? Qt::Vertical : Qt::Horizontal);
        }

        group = "SliderCursor";

        fspec = getFrameSpec(group);
        ispec = getInteriorSpec(group);
        indicator_spec_t dspec = getIndicatorSpec(group);

        o.rect = subControlRect(CC_Slider,opt,SC_SliderHandle,widget);

        renderFrame(p,o.rect,fspec,fspec.element+"-"+st);
        renderInterior(p,o.rect,fspec,ispec,ispec.element+"-"+st);
      }

      break;
    }

    case CC_Dial : { // FIXME
      const QStyleOptionSlider *opt =
          qstyleoption_cast<const QStyleOptionSlider *>(option);

      if (opt) {
        QStyleOptionSlider o(*opt);

        QRect empty(squaredRect(subControlRect(CC_Dial,opt,SC_DialGroove,widget)));
        QRect full(squaredRect(subControlRect(CC_Dial,opt,SC_DialHandle,widget)));

        renderElement(p,"dial-empty",empty);
            p->save();
            p->setClipRect(full);
        renderElement(p,"dial-full",empty);
            p->restore();
      }

      break;
    }

    case CC_TitleBar : {
      const QStyleOptionTitleBar *opt =
        qstyleoption_cast<const QStyleOptionTitleBar *>(option);

      if (opt) {
        QStyleOptionTitleBar o(*opt);

        QString group = "TitleBar";

        frame_spec_t fspec = getFrameSpec(group);
        interior_spec_t ispec = getInteriorSpec(group);
        label_spec_t lspec = getLabelSpec(group);

        renderFrame(p,o.rect,fspec,fspec.element+"-"+st);
        renderInterior(p,o.rect,fspec,ispec,ispec.element+"-"+st);

        o.rect = subControlRect(CC_TitleBar,opt,SC_TitleBarLabel,widget);

        renderLabel(p,dir,o.rect,fspec,ispec,lspec,Qt::AlignLeft | Qt::AlignVCenter, o.text, !(option->state & State_Enabled),o.icon.pixmap(pixelMetric(PM_TitleBarHeight)));

        drawComplexControl(CC_MdiControls,opt,p,widget);
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
        renderFrame(p,r1,fs,fs.element+"-"+st);
        renderInterior(p,r1,fs,is,is.element+"-"+st);

        // Draw frame and interior around title
        fs.hasCapsule = true;
        fs.capsuleH = 2;
        fs.capsuleV = -1; // FIXME bottom titles
        renderFrame(p,r2,fs,fs.element+"-"+st);
        renderInterior(p,r2,fs,is,is.element+"-"+st);

        // Draw title
        fs.hasCapsule = false;
        r2 = subControlRect(CC_GroupBox,&o,SC_GroupBoxLabel,widget);
        if ( opt->subControls & SC_GroupBoxCheckBox ) {
          if ( dir == Qt::LeftToRight )
            renderLabel(p,dir,
                        r2.adjusted(pixelMetric(PM_IndicatorWidth)+pixelMetric(PM_CheckBoxLabelSpacing),0,0,0),
                        fs,is,ls,opt->textAlignment | Qt::TextShowMnemonic,
                        opt->text,!opt->state & State_Enabled);
          else
            renderLabel(p,dir,
                        r2.adjusted(0,0,-pixelMetric(PM_IndicatorWidth)-pixelMetric(PM_CheckBoxLabelSpacing),0),
                        fs,is,ls,opt->textAlignment | Qt::TextShowMnemonic,
                        opt->text,!opt->state & State_Enabled);
          o.rect= subControlRect(CC_GroupBox,opt,SC_GroupBoxCheckBox,widget);
          drawPrimitive(PE_IndicatorCheckBox,&o,p,NULL);
        } else
          renderLabel(p,dir,r2,fs,is,ls,
                      opt->textAlignment | Qt::TextShowMnemonic,
                      opt->text,!opt->state & State_Enabled);
      }
      break;
    }

    // case CC_MdiControls : {

    //   break;
    // }

    default :
      //qDebug() << "[QSvgStyle] " << __func__ << ": Unhandled complex control " << control;
      QCommonStyle::drawComplexControl(control,option,p,widget);
  }

end:
  emit(sig_drawComplexControl_end(CC_str(control)));
  __exit_func();
}

int QSvgStyle::pixelMetric(PixelMetric metric, const QStyleOption * option, const QWidget * widget) const
{
  switch (metric) {
    case PM_ButtonMargin : return 0;

    case PM_ButtonShiftHorizontal :
    case PM_ButtonShiftVertical : return 1;

    case PM_DefaultFrameWidth : {
      QString group = "GenericFrame";
      const frame_spec_t fspec = getFrameSpec(group);

      return qMax(qMax(fspec.top,fspec.bottom),qMax(fspec.left,fspec.right));
    }

    case PM_SpinBoxFrameWidth :
    case PM_ComboBoxFrameWidth : return 0;

    case PM_MdiSubWindowFrameWidth : return 2;
    case PM_MdiSubWindowMinimizedWidth : return 50;

    case PM_LayoutLeftMargin :
    case PM_LayoutRightMargin :
    case PM_LayoutTopMargin :
    case PM_LayoutBottomMargin : return 4;

    case PM_LayoutHorizontalSpacing :
    case PM_LayoutVerticalSpacing : return 2;

    case PM_MenuBarPanelWidth :
    case PM_MenuBarVMargin :
    case PM_MenuBarHMargin :  return 0;

    case PM_MenuBarItemSpacing : return 2;

    case PM_MenuTearoffHeight : return 7;

    case PM_MenuPanelWidth : {
      QString group = "Menu";
      const frame_spec_t fspec = getFrameSpec(group);
      const interior_spec_t ispec = getInteriorSpec(group);

      int v = qMax(fspec.top,fspec.bottom);
      int h = qMax(fspec.left,fspec.right);
      return qMax(v,h);
    }

    case PM_ToolBarFrameWidth : return 0;
    case PM_ToolBarHandleExtent : return 8;
    case PM_ToolBarSeparatorExtent : return 2;
    case PM_ToolBarItemSpacing : return 0;
    case PM_ToolBarExtensionExtent : return 20;

    case PM_TabBarTabHSpace : return 0;
    case PM_TabBarTabVSpace : return 0;
    case PM_TabBarScrollButtonWidth : return 20;
    case PM_TabBarBaseHeight : return 0;
    case PM_TabBarBaseOverlap : return 0;
    case PM_TabBarTabShiftHorizontal : return 0;
    case PM_TabBarTabShiftVertical : return 0;

    case PM_ToolBarIconSize : return 16;
    case PM_ToolBarItemMargin : {
      QString group = "Toolbar";
      const frame_spec_t fspec = getFrameSpec(group);
      const interior_spec_t ispec = getInteriorSpec(group);

      int v = qMax(fspec.top,fspec.bottom);
      int h = qMax(fspec.left,fspec.right);
      return qMax(v,h);
    }

    case PM_TabBarIconSize : return 16;
    case PM_SmallIconSize : return 16;
    case PM_LargeIconSize : return 32;

    case PM_FocusFrameHMargin :
    case PM_FocusFrameVMargin : return 0;

    case PM_CheckBoxLabelSpacing :
    case PM_RadioButtonLabelSpacing : return 5;

    case PM_SplitterWidth : return 6;

    case PM_ScrollBarExtent : return 10;
    case PM_ScrollBarSliderMin : return 10;

    case PM_SliderThickness : return 4;

    case PM_ProgressBarChunkWidth : return 20;

    case PM_SliderLength : return 15;
    case PM_SliderControlThickness : return 15;

    case PM_DockWidgetFrameWidth : {
      QString group = "Dock";
      const frame_spec_t fspec = getFrameSpec(group);
      const interior_spec_t ispec = getInteriorSpec(group);
      const label_spec_t lspec = getLabelSpec(group);

      int v = qMax(fspec.top+lspec.top,fspec.bottom+lspec.bottom);
      int h = qMax(fspec.left+lspec.left,fspec.right+lspec.right);
      return qMax(v,h);
    }

    case PM_DockWidgetTitleMargin : {
      QString group = "Dock";
      const frame_spec_t fspec = getFrameSpec(group);
      const interior_spec_t ispec = getInteriorSpec(group);
      const label_spec_t lspec = getLabelSpec(group);

      int v = qMax(lspec.top,lspec.bottom);
      int h = qMax(lspec.left,lspec.right);
      return qMax(v,h);
    }

    case PM_TextCursorWidth : return 1;

    default : return QCommonStyle::pixelMetric(metric,option,widget);
  }
}

int QSvgStyle::styleHint(StyleHint hint, const QStyleOption * option, const QWidget * widget, QStyleHintReturn * returnData) const
{
  switch (hint) {
    case SH_ComboBox_ListMouseTracking :
    case SH_Menu_MouseTracking :
    case SH_MenuBar_MouseTracking : return true;

    case SH_TabBar_Alignment : return Qt::AlignCenter;

    //case SH_ScrollBar_BackgroundMode : return Qt::OpaqueMode;

    default : return QCommonStyle::styleHint(hint,option,widget,returnData);
  }
}

QCommonStyle::SubControl QSvgStyle::hitTestComplexControl ( ComplexControl control, const QStyleOptionComplex * option, const QPoint & position, const QWidget * widget) const
{
  return QCommonStyle::hitTestComplexControl(control,option,position,widget);
}

QSize QSvgStyle::sizeFromContents ( ContentsType type, const QStyleOption * option, const QSize & csz, const QWidget * widget) const
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
  size_spec_t ss;

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
  ss = getSizeSpec(g);

  switch (type) {
    case CT_LineEdit : {
      s = csz;
      if ( qstyleoption_cast<const QStyleOptionFrame *>(option) ) {
        // add frame size
        s += QSize(fs.top+fs.bottom,fs.left+fs.right);
      }
      break;
    }

    case CT_SpinBox : {
      // FIXME
      const QStyleOptionSpinBox *opt =
        qstyleoption_cast<const QStyleOptionSpinBox *>(option);

      if (opt) {
        QFont f = QApplication::font();
        if (widget)
          f = widget->font();

        const QString group = "LineEdit";

        const frame_spec_t fs = getFrameSpec(group);
        const interior_spec_t is = getInteriorSpec(group);
        const label_spec_t ls = getLabelSpec(group);
        const size_spec_t ss = getSizeSpec(group);

        if (widget) {
          const QSpinBox *w = qobject_cast<const QSpinBox *>(widget);
          if (w)
            s = sizeFromContents(fm,fs,is,ls,ss,w->prefix()+QString("%1").arg(w->maximum())+w->suffix(),QPixmap())+QSize(40,0)+QSize(8,0);
        } else
          s = QCommonStyle::sizeFromContents(CT_SpinBox,opt,csz,widget)+QSize(40,0);
      }

      break;
    }

    case CT_ComboBox : {
      if ( const QStyleOptionComboBox *opt =
           qstyleoption_cast<const QStyleOptionComboBox *>(option) ) {
        Q_UNUSED(opt);
        s = sizeFromContents(fm,fs,is,ls,ss,
                             "W",
                             QPixmap());

        s = s.expandedTo(csz);
        s += QSize(20,0); // drop down button;
      }

      break;
    }

    case CT_PushButton : {
      if ( const QStyleOptionButton *opt =
           qstyleoption_cast<const QStyleOptionButton *>(option) ) {

        s = sizeFromContents(fm,fs,is,ls,ss,
                             (opt->text.isEmpty() && opt->icon.isNull()) ? "W"
                               : opt->text,
                             opt->icon.pixmap(opt->iconSize));

	if ( opt->features & QStyleOptionButton::HasMenu ) {
	  s.rwidth() += ls.tispace+ds.size;
	}
      }
      break;
    }
    case CT_CheckBox :
    case CT_RadioButton : {
      if ( const QStyleOptionButton *opt =
           qstyleoption_cast<const QStyleOptionButton *>(option) ) {

        s = sizeFromContents(fm,fs,is,ls,ss,
                             opt->text,
                             opt->icon.pixmap(opt->iconSize));
        s += QSize(pixelMetric(PM_CheckBoxLabelSpacing)+pixelMetric(PM_IndicatorWidth),0);
	s = s.expandedTo(QSize(pixelMetric(PM_IndicatorWidth),pixelMetric(PM_IndicatorWidth))); // minimal checkbox size is size of indicator
      }
      break;
    }

    case CT_MenuItem : {
      const QStyleOptionMenuItem *opt =
        qstyleoption_cast<const QStyleOptionMenuItem *>(option);

      if (opt) {
        QFont f = QApplication::font();
        if (widget)
          f = widget->font();

        const QString group = "MenuItem";

        const frame_spec_t fs = getFrameSpec(group);
        const interior_spec_t is = getInteriorSpec(group);
        const label_spec_t ls = getLabelSpec(group);
        const size_spec_t ss = getSizeSpec(group);

        if (opt->menuItemType == QStyleOptionMenuItem::Separator)
          s = QSize(csw,2); /* FIXME there is no PM_MenuSeparatorHeight pixel metric */
        else {
          s = sizeFromContents(fm,fs,is,ls,ss,opt->text,opt->icon.pixmap(opt->maxIconWidth));
        }

        if (opt->icon.pixmap(opt->maxIconWidth).isNull())
          s.rwidth() += ls.tispace+opt->maxIconWidth;

        if ( (opt->menuItemType == QStyleOptionMenuItem::SubMenu) ||
             (opt->checkType == QStyleOptionMenuItem::Exclusive) ||
             (opt->checkType == QStyleOptionMenuItem::NonExclusive)
            ) {
          s.rwidth() += 15+ls.tispace;
        }
      }

      break;
    }

    case CT_MenuBarItem : {
      const QStyleOptionMenuItem *opt =
        qstyleoption_cast<const QStyleOptionMenuItem *>(option);

      if (opt) {
        QFont f = QApplication::font();
        if (widget)
          f = widget->font();

        QString group = "MenuBar";
        frame_spec_t fs = getFrameSpec(group);
        group = "MenuBarItem";
        const interior_spec_t is = getInteriorSpec(group);
        const label_spec_t ls = getLabelSpec(group);
        const size_spec_t ss = getSizeSpec(group);

        s = sizeFromContents(fm,fs,is,ls,ss,opt->text,opt->icon.pixmap(opt->maxIconWidth));
      }

      break;
    }

    case CT_ProgressBar : {
      if ( const QStyleOptionProgressBar *opt =
           qstyleoption_cast<const QStyleOptionProgressBar *>(option) )  {

        s = sizeFromContents(fm,fs,is,ls,ss,
                             (opt->textVisible &&
                              opt->text.isEmpty()) ? "W" : opt->text,
                             QPixmap());
      }

      break;
    }

    case CT_ToolButton : {
      if ( const QStyleOptionToolButton *opt =
           qstyleoption_cast<const QStyleOptionToolButton *>(option) ) {

	// minimum size
	QSize ms;
	if ( opt->text.isEmpty() && (opt->toolButtonStyle == Qt::ToolButtonTextOnly) )
	  ms = sizeFromContents(fm,fs,is,ls,ss,
                                "W",
                                opt->icon.pixmap(opt->iconSize),
                                Qt::ToolButtonTextOnly);
	if ( opt->icon.isNull() && (opt->toolButtonStyle == Qt::ToolButtonIconOnly) )
	  ms = sizeFromContents(fm,fs,is,ls,ss,
                                "W",
                                opt->icon.pixmap(opt->iconSize),
                                Qt::ToolButtonTextOnly);
	if ( opt->text.isEmpty() && opt->icon.isNull() )
	  ms = sizeFromContents(fm,fs,is,ls,ss,
                                "W",
                                opt->icon.pixmap(opt->iconSize),
                                Qt::ToolButtonTextOnly);

	s = sizeFromContents(fm,fs,is,ls,ss,
                             opt->text,
                             opt->icon.pixmap(opt->iconSize),
                             opt->toolButtonStyle);

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

	  ms = ms+QSize(ls.tispace+ds.size,0);
	}

	// add room for simple down arrow or drop down arrow
        if (opt->features & QStyleOptionToolButton::Menu) {
          // Tool button with drop down button
          s.rwidth() += 20;
        } else if (opt->features & QStyleOptionToolButton::HasMenu) {
          // Tool button with down arrow
          s.rwidth() += ls.tispace+ds.size;
        }

        s = s.expandedTo(ms);
      }

      break;
    }

    case CT_TabBarTab : {
      const QStyleOptionTab *opt =
        qstyleoption_cast<const QStyleOptionTab *>(option);

      if (opt) {
        QFont f = QApplication::font();
        if (widget)
          f = widget->font();

        const QString group = "Tab";

        const frame_spec_t fs = getFrameSpec(group);
        const interior_spec_t is = getInteriorSpec(group);
        const label_spec_t ls = getLabelSpec(group);
        const size_spec_t ss = getSizeSpec(group);

        s = sizeFromContents(fm,fs,is,ls,ss,opt->text,opt->icon.pixmap(pixelMetric(PM_ToolBarIconSize)));

        if (widget) {
          const QTabBar *w = qobject_cast<const QTabBar*>(widget);
          if (w && w->tabsClosable())
            s.rwidth() += pixelMetric(PM_TabCloseIndicatorWidth,option,widget)+ls.tispace;
        }
      }

      break;
    }

    case CT_HeaderSection : {
      if ( const QStyleOptionHeader *opt =
           qstyleoption_cast<const QStyleOptionHeader *>(option) ) {

        s = sizeFromContents(fm,fs,is,ls,ss,opt->text,opt->icon.pixmap(pixelMetric(PM_SmallIconSize)));
      }

      break;
    }

    case CT_Slider : {
      if (option->state & State_Horizontal)
        s = QSize(csw,pixelMetric(PM_SliderControlThickness,option,widget)+2); // +2 for frame
      else
        s = QSize(pixelMetric(PM_SliderControlThickness,option,widget),csh+2);

      break;
    }

    case CT_GroupBox : {
      if ( const QStyleOptionGroupBox *opt =
           qstyleoption_cast<const QStyleOptionGroupBox *>(option) ) {

	if ( opt->subControls & SC_GroupBoxCheckBox )
          s = sizeFromContents(fm,fs,is,ls,ss,opt->text,QPixmap())+QSize(pixelMetric(PM_CheckBoxLabelSpacing)+pixelMetric(PM_IndicatorWidth),0);
        else
	  s = sizeFromContents(fm,fs,is,ls,ss,opt->text,QPixmap());

        // add contents to st, 30 is title shift (left and right)
	s = QSize(qMax(s.width()+30+30,csz.width()+fs.left+fs.right),
                  csz.height()+s.height()+fs.top+fs.bottom);
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

QSize QSvgStyle::sizeFromContents(const QFontMetrics &fm,
                     /* frame spec */ const frame_spec_t &fs,
                     /* interior spec */ const interior_spec_t &is,
                     /* label spec */ const label_spec_t &ls,
                     /* size spec */ const size_spec_t &ss,
                     /* text */ const QString &text,
                     /* icon */ const QPixmap &icon,
                     /* text-icon alignment */ const Qt::ToolButtonStyle tialign) const
{
  Q_UNUSED(is);

  QSize s;
  s.setWidth(fs.left+fs.right+ls.left+ls.right);
  s.setHeight(fs.top+fs.bottom+ls.top+ls.bottom);
  if (ls.hasShadow) {
    s.rwidth() += ls.xshift+ls.depth;
    s.rheight() += ls.yshift+ls.depth;
  }

  // compute width and height of text. QFontMetrics seems to ignore \n
  // and returns wrong values

  int tw = 0;
  int th = 0;

  if ( !text.isNull() && !text.isEmpty() ) {
    /* remove & mnemonic character and tabs (for menu items) */
    // FIXME don't remove & if it is not followed by a character
    QString t = QString(text).remove('\t');
    {
      int i=0;
      while ( i<t.size() ) {
        if ( t.at(i) == '&' ) {
          // see if next character is not a space
          if ( (i+1<t.size()) && (!t.at(i+1).isSpace()) ) {
            t.remove(i,1);
            i++;
          }
        }
        i++;
      }
    }

    /* deal with \n */
    QStringList l = t.split('\n');

    th = fm.height()*(l.size());
    for (int i=0; i<l.size(); i++) {
      tw = qMax(tw,fm.width(l[i]));
    }
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

  if ( (ss.minH > 0) && (s.height() < ss.minH) )
    s.setHeight(ss.minH);

  if ( (ss.minW > 0) && (s.width() < ss.minW) )
    s.setWidth(ss.minW);

  if (ss.fixedH > 0)
    s.setHeight(ss.fixedH);

  if (ss.fixedW > 0)
    s.setWidth(ss.fixedW);

  return s;
}

QRect QSvgStyle::subElementRect(SubElement e, const QStyleOption * option, const QWidget * widget) const
{
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
  size_spec_t ss;

  if ( g.isEmpty() ) {
    // element not currently supported by QSvgStyle
    goto end;
  }

  // Get configuration for group
  fs = getFrameSpec(g);
  is = getInteriorSpec(g);
  ls = getLabelSpec(g);
  ds = getIndicatorSpec(g);
  ss = getSizeSpec(g);

  __print_group();

  switch (e) {
    case SE_ProgressBarContents : {
      return interiorRect(option->rect, fs,is);
    }
    case SE_LineEditContents : {
      return interiorRect(option->rect, fs,is);
    }
    case SE_PushButtonContents : {
      return interiorRect(option->rect,fs,is);
    }

    default : return QCommonStyle::subElementRect(e,option,widget);
  }

end:
  return QCommonStyle::subElementRect(e,option,widget);
}

QRect QSvgStyle::subControlRect(ComplexControl control, const QStyleOptionComplex * option, SubControl subControl, const QWidget * widget) const
{
  // FIXME return visual rects

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
  size_spec_t ss;

  if ( g.isEmpty() ) {
    // element not currently supported by QSvgStyle
    ret = QCommonStyle::subControlRect(control,option,subControl,widget);
    goto end;
  }

  // Get configuration for group
  fs = getFrameSpec(g);
  is = getInteriorSpec(g);
  ls = getLabelSpec(g);
  ds = getIndicatorSpec(g);
  ss = getSizeSpec(g);

  __print_group();

  switch (control) {
    case CC_SpinBox :
      // OK
      switch (subControl) {
        case SC_SpinBoxFrame :
          ret = r;
          break;
        case SC_SpinBoxEditField :
          ret = r.adjusted(0,0,-40,0);
          break;
        case SC_SpinBoxUp :
          // we adjust thr rect by fs left and right because when we
          // draw the button panel it will be shrunken again by the frame
          // left and eight (it is an interior)
          ret = QRect(x+w-20-fs.right,y,20,h).adjusted(-fs.left,0,fs.right,0);
          break;
        case SC_SpinBoxDown :
          ret = QRect(x+w-40-fs.right,y,20,h).adjusted(-fs.left,0,fs.right,0);
          break;
        default :
          ret = QCommonStyle::subControlRect(control,option,subControl,widget);
      }
      break;

    case CC_ComboBox :
      // OK
      switch (subControl) {
        case SC_ComboBoxFrame :
          ret = r;
          break;
        case SC_ComboBoxEditField :
          ret = r.adjusted(0,0,-20,0);
          break;
        case SC_ComboBoxArrow :
          ret = r.adjusted(x+w-20,0,0,0);
          break;
        case SC_ComboBoxListBoxPopup :
          ret = r;
          break;
        default :
          ret = QCommonStyle::subControlRect(control,option,subControl,widget);
      }
      break;

    case CC_ScrollBar : {
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
            const QStyleOptionSlider *opt =
                qstyleoption_cast<const QStyleOptionSlider *>(option);

            if (opt) {
              QRect r = subControlRect(CC_ScrollBar,option,SC_ScrollBarGroove,widget);
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

              const int start = sliderPositionFromValue(opt->minimum,opt->maximum,opt->sliderPosition,maxLength - length,opt->upsideDown);
              ret = QRect(x+start,y,length,h);
            } else
              ret = QRect();

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
            const QStyleOptionSlider *opt =
                qstyleoption_cast<const QStyleOptionSlider *>(option);

            if (opt) {
              QRect r = subControlRect(CC_ScrollBar,option,SC_ScrollBarGroove,widget);
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
      const int thick = pixelMetric(PM_SliderThickness,option,widget);
      const bool horiz = (option->state & State_Horizontal);
      switch (subControl) {
        case SC_SliderGroove :
          if (horiz)
            ret = QRect(x,y+(h-thick)/2,w,thick);
          else
            ret = QRect(x+(w-thick)/2,y,thick,h);
          break;
        case SC_SliderHandle : {
          if ( const QStyleOptionSlider *opt =
               qstyleoption_cast<const QStyleOptionSlider *>(option) ) {

            subControlRect(CC_Slider,option,SC_SliderGroove,widget).getRect(&x,&y,&w,&h);

            const int len = pixelMetric(PM_SliderLength, option, widget);
            const int thickness = pixelMetric(PM_SliderControlThickness, option, widget);
            const int sliderPos(sliderPositionFromValue(opt->minimum, opt->maximum, opt->sliderPosition, (horiz ? w : h) - len, opt->upsideDown));

            if (horiz)
              ret = QRect(x+sliderPos,y+(h-thickness)/2,len,thickness);
            else
              ret = QRect(x+(w-len)/2,y+sliderPos,thickness,len);
          }

          break;
        }

        default : ret = QCommonStyle::subControlRect(control,option,subControl,widget);
      }

      break;
    }

    case CC_ToolButton : {
      // OK
      switch (subControl) {
        case SC_ToolButton : {
          if ( const QStyleOptionToolButton *opt =
               qstyleoption_cast<const QStyleOptionToolButton *>(option) ) {

            // remove room for drop down buttons or down arrows
            if (opt->features & QStyleOptionToolButton::Menu)
                  ret = r.adjusted(0,0,-20,0);
            if (opt->features & QStyleOptionToolButton::HasMenu)
                  ret = r.adjusted(0,0,-ds.size-ls.tispace,0);
          }
          break;
        }
        case SC_ToolButtonMenu : {
          if ( const QStyleOptionToolButton *opt =
               qstyleoption_cast<const QStyleOptionToolButton *>(option) ) {

            if (opt->features & QStyleOptionToolButton::Menu)
              ret = r.adjusted(x+w-20,0,0,0);
            else if (opt->features & QStyleOptionToolButton::HasMenu)
              ret = QRect(x+w-ls.tispace-ds.size-fs.right,y+10,ds.size,h-10);
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
        stitle = sizeFromContents(fm,fs,is,ls,ss,
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
                            labelRect.adjusted(fs.left+ls.left,fs.top,-fs.right-ls.right,-fs.bottom));
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

    default : ret = QCommonStyle::subControlRect(control,option,subControl,widget);
  }

end:
  return visualRect(dir, r,ret);
}

QIcon QSvgStyle::standardIconImplementation ( QStyle::StandardPixmap standardIcon, const QStyleOption* option, const QWidget* widget ) const
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

      drawPrimitive(PE_IndicatorArrowRight,&opt,&painter,0);

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

    default : return QCommonStyle::standardIconImplementation(standardIcon,option,widget);
  }

  return QCommonStyle::standardIconImplementation(standardIcon,option,widget);
}

QRect QSvgStyle::squaredRect(const QRect& r) const {
  int e = (r.width() > r.height()) ? r.height() : r.width();
  return QRect(r.x(),r.y(),e,e);
}

void QSvgStyle::renderElement(QPainter* painter, const QString& element, const QRect& bounds, int hsize, int vsize, Qt::Orientation orientation, int frameno) const
{
  Q_UNUSED(orientation);

  int x,y,h,w;
  bounds.getRect(&x,&y,&w,&h);

  if ( (x < 0) || (y < 0) || (w <= 0) || (h <= 0) )
    return;

  QSvgRenderer *renderer = 0;
  QPainter *p = painter;
  QPainter *p2 = 0;
  QPixmap pm2;

  // theme specific rendering
  if (themeRndr && themeRndr->isValid() && themeRndr->elementExists(element)) {
      renderer = themeRndr;
  } else {
    // Fall back to default rendering
    if (defaultRndr && defaultRndr->isValid() && defaultRndr->elementExists(element))
      renderer = defaultRndr;
  }

  QString _element = element;
  if ( (frameno != -1) && (frameno != 0) && renderer ) {
    for (int i=frameno; i>=1; i--) {
      if ( renderer->elementExists(QString(element+"-frame%1").arg(i)) ) {
        _element = QString(element+"-frame%1").arg(i);
        break;
      }
    }
  }

  if ( orientation == Qt::Vertical ) {
    // render the item rotated by -90 degrees into a pixmap first
    pm2 = QPixmap(w,h);
    pm2.fill(Qt::transparent);
    p2 = new QPainter(&pm2);
    p2->translate(0,h);
    p2->rotate(-90);

    // QPixmap origin is (0,0)
    x = y = 0;

    // swap w,h for the code below
    SWAP(w,h);

    // paint into pm1
    p = p2;
  }

  if (renderer) {
    if ( (hsize > 0) || (vsize > 0) ) {

      if ( (hsize > 0) && (vsize <= 0) ) {
        int hpatterns = (w/hsize)+1;

        p->save();
        p->setClipRect(QRect(x,y,w,h));
        for (int i=0; i<hpatterns; i++)
          renderer->render(p,_element,QRect(x+i*hsize,y,hsize,h));
        p->restore();
      }

      if ( (hsize <= 0) && (vsize > 0) ) {
        int vpatterns = (h/vsize)+1;

        p->save();
        p->setClipRect(QRect(x,y,w,h));
        for (int i=0; i<vpatterns; i++)
          renderer->render(p,_element,QRect(x,y+i*vsize,w,vsize));
        p->restore();
      }

      if ( (hsize > 0) && (vsize > 0) ) {
        int hpatterns = (w/hsize)+1;
        int vpatterns = (h/vsize)+1;

        p->save();
        p->setClipRect(bounds);
        for (int i=0; i<hpatterns; i++)
          for (int j=0; j<vpatterns; j++)
            renderer->render(p,_element,QRect(x+i*hsize,y+j*vsize,hsize,vsize));
        p->restore();
      }
    } else {
      renderer->render(p,_element,QRect(x,y,w,h));
    }

    if ( orientation == Qt::Vertical ) {
      // restore coords
      bounds.getRect(&x,&y,&w,&h);

      painter->drawPixmap(bounds,pm2,QRect(0,0,w,h));

      delete p2;
    }

    #ifdef __DEBUG__
    painter->save();
    painter->setPen(QPen(Qt::black));
    //painter->drawText(bounds, QString("%1").arg(frameno));
    painter->restore();
    #endif
  }
}

void QSvgStyle::renderFrame(QPainter *painter,
                    /* frame bounds */ const QRect &bounds,
                    /* frame spec */ const frame_spec_t &fspec,
                    /* SVG element */ const QString &element,
                    /* orientation */ Qt::Orientation orientation) const
{
  Q_UNUSED(orientation);

  if (!fspec.hasFrame)
    return;

  __enter_func__();
  emit(sig_renderFrame_begin(element));

  int x0,y0,x1,y1,w,h;
  bounds.getRect(&x0,&y0,&w,&h);
  x1 = bounds.bottomRight().x();
  y1 = bounds.bottomRight().y();

  if (!fspec.hasCapsule) {
    // top
    if ( (fspec.y0c0 == -1) || (fspec.y0c1 == -1) || (fspec.y0c1 <= fspec.y0c0) ) {
      renderElement(painter,element+"-top",QRect(x0+fspec.left,y0,w-fspec.left-fspec.right,fspec.top),0,0,Qt::Horizontal,animationcount%fspec.animationFrames);
    } else {
      // with cut
      renderElement(painter,element+"-top",QRect(x0+fspec.left,y0,fspec.y0c0-x0-fspec.left+1,fspec.top),0,0,Qt::Horizontal,animationcount%fspec.animationFrames);
      renderElement(painter,element+"-top",QRect(fspec.y0c1,y0,w-fspec.y0c1-fspec.right+1,fspec.top),0,0,Qt::Horizontal,animationcount%fspec.animationFrames);

      // inverted corners at cut
      renderElement(painter,element+"-bottomright-inverted",QRect(fspec.y0c0,y0,fspec.right,fspec.bottom),0,0,Qt::Horizontal,animationcount%fspec.animationFrames);
      renderElement(painter,element+"-bottomleft-inverted",QRect(fspec.y0c1-fspec.right+1,y0,fspec.left,fspec.bottom),0,0,Qt::Horizontal,animationcount%fspec.animationFrames);
    }
    // bottom
    if ( (fspec.y1c0 == -1) || (fspec.y1c1 == -1) || (fspec.y1c1 <= fspec.y1c0) ) {
      renderElement(painter,element+"-bottom",QRect(x0+fspec.left,y1-fspec.bottom+1,w-fspec.left-fspec.right,fspec.bottom),0,0,Qt::Horizontal,animationcount%fspec.animationFrames);
    } else {
      // with cut
      renderElement(painter,element+"-bottom",QRect(x0+fspec.left,y1-fspec.bottom+1,fspec.y1c0-x0-fspec.left+1,fspec.bottom),0,0,Qt::Horizontal,animationcount%fspec.animationFrames);
      renderElement(painter,element+"-bottom",QRect(fspec.y1c1,y1-fspec.bottom+1,w-fspec.y1c1-fspec.right+1,fspec.bottom),0,0,Qt::Horizontal,animationcount%fspec.animationFrames);

      // inverted corners at cut
      renderElement(painter,element+"-topright-inverted",QRect(fspec.y1c0,y1-fspec.bottom+1,fspec.right,fspec.bottom),0,0,Qt::Horizontal,animationcount%fspec.animationFrames);
      renderElement(painter,element+"-topleft-inverted",QRect(fspec.y1c1-fspec.right+1,y1-fspec.bottom+1,fspec.left,fspec.bottom),0,0,Qt::Horizontal,animationcount%fspec.animationFrames);
    }
    if ( (fspec.x0c0 == -1) || (fspec.x0c1 == -1) || (fspec.x0c1 <= fspec.x0c0) ) {
      // left
      renderElement(painter,element+"-left",QRect(x0,y0+fspec.top,fspec.left,h-fspec.top-fspec.bottom),0,0,Qt::Horizontal,animationcount%fspec.animationFrames);
    } else {
      // with cut
      renderElement(painter,element+"-left",QRect(x0,y0+fspec.top,fspec.left,fspec.x0c0-y0-fspec.top),0,0,Qt::Horizontal,animationcount%fspec.animationFrames);
      renderElement(painter,element+"-left",QRect(x0,fspec.x0c1,fspec.left,h-fspec.x0c1-fspec.bottom+1),0,0,Qt::Horizontal,animationcount%fspec.animationFrames);

      // inverted corners at cut
      renderElement(painter,element+"-bottomright-inverted",QRect(x0,fspec.x0c0,fspec.right,fspec.bottom),0,0,Qt::Horizontal,animationcount%fspec.animationFrames);
      renderElement(painter,element+"-topright-inverted",QRect(x0,fspec.x0c1-fspec.top+1,fspec.right,fspec.top),0,0,Qt::Horizontal,animationcount%fspec.animationFrames);
    }
    // right
    if ( (fspec.x1c0 == -1) || (fspec.x1c1 == -1) || (fspec.x1c1 <= fspec.x1c0) ) {
      renderElement(painter,element+"-right",QRect(x1-fspec.right+1,y0+fspec.top,fspec.right,h-fspec.top-fspec.bottom),0,0,Qt::Horizontal,animationcount%fspec.animationFrames);
    } else {
      // with cut
      renderElement(painter,element+"-right",QRect(x1-fspec.right+1,y0+fspec.top,fspec.right,fspec.x1c0-y0-fspec.top+1),0,0,Qt::Horizontal,animationcount%fspec.animationFrames);
      renderElement(painter,element+"-right",QRect(x1-fspec.right+1,fspec.x1c1,fspec.right,h-fspec.x1c1-fspec.bottom+1),0,0,Qt::Horizontal,animationcount%fspec.animationFrames);

      // inverted corners at cut
      renderElement(painter,element+"-bottomleft-inverted",QRect(x1-fspec.right+1,fspec.x1c0,fspec.left,fspec.bottom),0,0,Qt::Horizontal,animationcount%fspec.animationFrames);
      renderElement(painter,element+"-topleft-inverted",QRect(x1-fspec.right+1,fspec.x1c1-fspec.top+1,fspec.left,fspec.top),0,0,Qt::Horizontal,animationcount%fspec.animationFrames);
    }
    // topleft
    renderElement(painter,element+"-topleft",QRect(x0,y0,fspec.left,fspec.top),0,0,Qt::Horizontal,animationcount%fspec.animationFrames);
    // topright
    renderElement(painter,element+"-topright",QRect(x1-fspec.right+1,y0,fspec.right,fspec.top),0,0,Qt::Horizontal,animationcount%fspec.animationFrames);
    // bottomleft
    renderElement(painter,element+"-bottomleft",QRect(x0,y1-fspec.bottom+1,fspec.left,fspec.bottom),0,0,Qt::Horizontal,animationcount%fspec.animationFrames);
    // bottomright
    renderElement(painter,element+"-bottomright",QRect(x1-fspec.right+1,y1-fspec.bottom+1,fspec.right,fspec.bottom),0,0,Qt::Horizontal,animationcount%fspec.animationFrames);
  } else {
    int left = 0, right = 0, top = 0, bottom = 0;
    // horizontal separator
    if ( (fspec.capsuleH == 1) || (fspec.capsuleH == 0) ) {
      renderElement(painter,element+"-left",QRect(x0,y0+fspec.top,fspec.left,h-fspec.top-fspec.bottom),0,0,Qt::Horizontal);
    }

    if ( (fspec.capsuleH == 0) && (fspec.capsuleV == 0) )
      return;

    // edges
    if ( (fspec.capsuleH == -1) || (fspec.capsuleH == 2) )
      left = fspec.left;
    if ( (fspec.capsuleH == 1) || (fspec.capsuleH == 2) )
      right = fspec.right;
    if ( (fspec.capsuleV == -1)  || (fspec.capsuleV == 2) )
      top = fspec.top;
    if ( (fspec.capsuleV == 1) || (fspec.capsuleV == 2) )
      bottom = fspec.bottom;

    // top
    if ( (fspec.capsuleV == -1) || (fspec.capsuleV == 2) ) {
      renderElement(painter,element+"-top",QRect(x0+left,y0,w-left-right,fspec.top),0,0,Qt::Horizontal);

      // topleft corner
      if (fspec.capsuleH == -1)
        renderElement(painter,element+"-topleft",QRect(x0,y0,fspec.left,fspec.top),0,0,Qt::Horizontal);

      // topright corner
        if (fspec.capsuleH == 1)
          renderElement(painter,element+"-topright",QRect(x1-fspec.right+1,y0,fspec.right,fspec.top),0,0,Qt::Horizontal);
    }

    // bottom
    if ( (fspec.capsuleV == 1) || (fspec.capsuleV == 2) ) {
      renderElement(painter,element+"-bottom",QRect(x0+left,y1-bottom+1,w-left-right,fspec.bottom),0,0,Qt::Horizontal);

      // bottomleft corner
      if (fspec.capsuleH == -1)
        renderElement(painter,element+"-bottomleft",QRect(x0,y1-fspec.bottom+1,fspec.left,fspec.bottom),0,0,Qt::Horizontal);

      // bottomright corner
        if (fspec.capsuleH == 1)
          renderElement(painter,element+"-bottomright",QRect(x1-fspec.right+1,y1-fspec.bottom+1,fspec.right,fspec.bottom),0,0,Qt::Horizontal);
    }

    // left
    if ( (fspec.capsuleH == -1) || (fspec.capsuleH == 2) ) {
      renderElement(painter,element+"-left",QRect(x0,y0+top,fspec.left,h-top-bottom),0,0,Qt::Horizontal);

      // topleft corner
      if (fspec.capsuleV == -1)
        renderElement(painter,element+"-topleft",QRect(x0,y0,fspec.left,fspec.top),0,0,Qt::Horizontal);

      // bottomleft corner
        if (fspec.capsuleV == 1)
          renderElement(painter,element+"-bottomleft",QRect(x0,y1-fspec.bottom+1,fspec.left,fspec.bottom),0,0,Qt::Horizontal);
    }

    // right
    if ( (fspec.capsuleH == 1) || (fspec.capsuleH == 2) ) {
      renderElement(painter,element+"-right",QRect(x1-fspec.right+1,y0+top,fspec.right,h-top-bottom),0,0,Qt::Horizontal);

      // topright corner
      if (fspec.capsuleV == -1)
        renderElement(painter,element+"-topright",QRect(x1-fspec.right+1,y0,fspec.right,fspec.top),0,0,Qt::Horizontal);

      // bottomright corner
        if (fspec.capsuleV == 1)
          renderElement(painter,element+"-bottomright",QRect(x1-fspec.right+1,y1-fspec.bottom+1,fspec.right,fspec.bottom),0,0,Qt::Horizontal);
    }
  }

  #ifdef __DEBUG__
  painter->save();
  painter->setPen(QPen(Qt::green));
  drawRealRect(painter, bounds);
  painter->restore();
  #endif

  emit(sig_renderFrame_end(element));
  __exit_func();
}

void QSvgStyle::renderInterior(QPainter *painter,
                       /* frame bounds */ const QRect &bounds,
                       /* frame spec */ const frame_spec_t &fspec,
                       /* interior spec */ const interior_spec_t &ispec,
                       /* SVG element */ const QString &element,
                       /* orientation */ Qt::Orientation orientation) const
{
  if (!ispec.hasInterior)
    return;

  __enter_func__();
  emit(sig_renderInterior_begin(element));

  if (!fspec.hasCapsule) {
    renderElement(painter,element,interiorRect(bounds,fspec,ispec),ispec.px,ispec.py,orientation,animationcount%ispec.animationFrames);
  } else {
    // add these to compensate the absence of the frame
    int left = 0, right = 0, top = 0, bottom = 0;
    if (fspec.capsuleH == 0) {
      left = fspec.left;
      right = fspec.right;
    }
    if (fspec.capsuleH == -1) {
      right = fspec.right;
    }
    if (fspec.capsuleH == 1) {
      left = fspec.left;
    }
    if (fspec.capsuleV == 0) {
      top = fspec.top;
      bottom = fspec.bottom;
    }
    if (fspec.capsuleV == -1) {
      bottom = fspec.bottom;
    }
    if (fspec.capsuleV == 1) {
      top = fspec.top;
    }
    QRect r = interiorRect(bounds,fspec,ispec).adjusted(-left,-top,right,bottom);
    if ( r.width() < 0 )
      r.setWidth(0);
    if ( r.height() < 0 )
      r.setHeight(0);
    renderElement(painter,element,
                   r,
                   ispec.px,ispec.py,orientation,animationcount%ispec.animationFrames);
  }

  painter->save();
  painter->setCompositionMode(QPainter::CompositionMode_Overlay);
  painter->fillRect(bounds,QBrush(QColor(255,0,0,50)));
  painter->restore();

  #ifdef __DEBUG__
  painter->save();
  painter->setPen(QPen(Qt::red));
  drawRealRect(painter,interiorRect(bounds,fspec,ispec));
  painter->restore();
  #endif

  emit(sig_renderInterior_end(element));
  __exit_func();
}

void QSvgStyle::renderIndicator(QPainter *painter,
                       /* frame bounds */ const QRect &bounds,
                       /* frame spec */ const frame_spec_t &fspec,
                       /* interior spec */ const interior_spec_t &ispec,
                       /* indicator spec */ const indicator_spec_t &dspec,
                       /* indocator SVG element */ const QString &element,
                       Qt::Alignment alignment) const
{
  __enter_func__();
  emit(sig_renderIndicator_begin(element));

  const QRect interior = squaredRect(interiorRect(bounds,fspec,ispec));
  //const QRect interior = squaredRect(frameRect(bounds,fspec));
  //const QRect interior = interiorRect(bounds,fspec,ispec);
  int s = (interior.width() > dspec.size) ? dspec.size : interior.width();

  renderElement(painter,element,
                alignedRect(QApplication::layoutDirection(),alignment,QSize(s,s),interiorRect(bounds,fspec,ispec)),
                0,0,Qt::Horizontal,animationcount%dspec.animationFrames);

  #ifdef __DEBUG__
  painter->save();
  painter->setPen(QPen(Qt::blue));
  drawRealRect(painter, alignedRect(QApplication::layoutDirection(),alignment,QSize(s,s),interiorRect(bounds,fspec,ispec)));
  painter->restore();
  #endif

  emit(sig_renderIndicator_end(element));
  __exit_func();
}

void QSvgStyle::renderLabel(QPainter* painter, Qt::LayoutDirection direction, const QRect& bounds, const frame_spec_t& fspec, const interior_spec_t& ispec, const label_spec_t& lspec, int talign, const QString& text, bool disabled, const QPixmap& icon, const Qt::ToolButtonStyle tialign) const
{
  __enter_func__();
  emit(sig_renderLabel_begin("text:"+text+"/icon:"+(icon.isNull() ? "yes":"no")));

  // FIXME implement Right-to-Left

  // compute text and icon rect
  QRect r(labelRect(bounds,fspec,ispec,lspec));

  #ifdef __DEBUG__
  painter->save();
  painter->setPen(QPen(QColor(255,255,255)));
  drawRealRect(painter, r);
  painter->restore();
  #endif

  QRect ricon = r;
  QRect rtext = r;

  if (tialign == Qt::ToolButtonTextBesideIcon) {
    ricon = alignedRect(Qt::LeftToRight,Qt::AlignVCenter | Qt::AlignLeft, QSize(icon.width(),icon.height()),r);
    rtext = QRect(r.x()+icon.width()+(icon.isNull() ? 0 : lspec.tispace),r.y(),r.width()-ricon.width()-(icon.isNull() ? 0 : lspec.tispace),r.height());
  } else if (tialign == Qt::ToolButtonTextUnderIcon) {
    ricon = alignedRect(Qt::LeftToRight,Qt::AlignTop | Qt::AlignHCenter, QSize(icon.width(),icon.height()),r);
    rtext = QRect(r.x(),r.y()+icon.height()+(icon.isNull() ? 0 : lspec.tispace),r.width(),r.height()-ricon.height()-(icon.isNull() ? 0 : lspec.tispace));
  } else if (tialign == Qt::ToolButtonIconOnly) {
    ricon = alignedRect(Qt::LeftToRight,Qt::AlignCenter, QSize(icon.width(),icon.height()),r);
  }

  if ( text.isNull() || text.isEmpty() ) {
    // When we have no text, center icon
    ricon = alignedRect(Qt::LeftToRight,Qt::AlignCenter, QSize(icon.width(),icon.height()),r);
  }

  rtext = visualRect(direction,bounds,rtext);
  ricon = visualRect(direction,bounds,ricon);

  if (disabled) {
    // FIXME use palette
    painter->save();
    painter->setPen(QPen(QColor(100,100,100)));
  }

  if (tialign != Qt::ToolButtonIconOnly) {
    if ( !text.isNull() && !text.isEmpty() ) {
      if (lspec.hasShadow) {
        painter->save();

        painter->setPen(QPen(QColor(lspec.r,lspec.g,lspec.b,lspec.a)));
        for (int i=0; i<lspec.depth; i++)
          painter->drawText(rtext.adjusted(lspec.xshift+i,lspec.yshift+i,0,0),talign,text);

        painter->restore();
      }
      painter->drawText(rtext,talign,text);
    }
  }

  if (tialign != Qt::ToolButtonTextOnly) {
    if (!icon.isNull()) {
      painter->drawPixmap(ricon,icon);
#ifdef __DEBUG__
      painter->save();
      painter->setPen(QPen(QColor(255,0,255)));
      drawRealRect(painter, ricon);
      painter->restore();
#endif
    }
  }

#ifdef __DEBUG__
  painter->save();
  painter->setPen(QPen(QColor(0,255,255)));
  drawRealRect(painter, rtext);
  painter->restore();
#endif

  if (disabled)
    painter->restore();

  emit(sig_renderLabel_end("text:"+text+"/icon:"+(icon.isNull() ? "yes":"no")));
  __exit_func();
}

inline frame_spec_t QSvgStyle::getFrameSpec(const QString& group) const
{
  return settings->getFrameSpec(group);
}

inline interior_spec_t QSvgStyle::getInteriorSpec(const QString& group) const
{
  return settings->getInteriorSpec(group);
}

inline indicator_spec_t QSvgStyle::getIndicatorSpec(const QString& group) const
{
  return settings->getIndicatorSpec(group);
}

inline label_spec_t QSvgStyle::getLabelSpec(const QString& group) const
{
  return settings->getLabelSpec(group);
}

inline size_spec_t QSvgStyle::getSizeSpec(const QString& group) const
{
  return settings->getSizeSpec(group);
}

void QSvgStyle::capsulePosition(const QWidget *widget, bool &capsule, int &h, int &v) const
{
  capsule = false;
  h = v = 2;
  if (widget && widget->parent()) {
    // get parent widget
    const QWidget *parent = qobject_cast<const QWidget *>(widget->parent());
    if (parent) {
      // get its layout
      QLayout *l = parent->layout();
      if (l) {
        capsule = true;
        // ensure that all widgets in the layout have the same type
        for (int i = 0; i < l->count(); ++i) {
          if (l->itemAt(i)->widget()) {
            //printf("classname %s\n",l->itemAt(i)->widget()->metaObject()->className());
            if (l->itemAt(i)->widget()->metaObject()->className() != widget->metaObject()->className()) {
              capsule = false;
              break;
            }
          }
        }

        if (capsule) {
          int index = -1;
          // get index of this widget
          for (int i = 0; i < l->count(); ++i) {
            if (l->itemAt(i)->widget() == widget) {
              index = i;
              break;
            }
          }

          const QHBoxLayout *hbox = qobject_cast<const QHBoxLayout *>(l);
          if (hbox) {
            // layout is a horizontal box
			if ( hbox->spacing() != 0 ) {
			  capsule = false;
			  return;
			}
            if ( (index == 0) && (index == hbox->count()-1) )
              h = 2;
            else if (index == hbox->count()-1)
              h = 1;
            else if (index == 0)
              h = -1;
            else
              h = 0;

            v = 2;
          }

          const QVBoxLayout *vbox = qobject_cast<const QVBoxLayout *>(l);
          if (vbox) {
            // layout is a horizontal box
			if ( vbox->spacing() != 0 ) {
			  capsule = false;
			  return;
			}
            if ( (index == 0) && (index == vbox->count()-1) )
              v = 2;
            else if (index == vbox->count()-1)
              v = 1;
            else if (index == 0)
              v = -1;
            else
              v = 0;

            h = 2;
          }

          const QGridLayout *gbox = qobject_cast<const QGridLayout *>(l);
          if (gbox) {
            // layout is a grid
			if ( (gbox->horizontalSpacing() != 0) || (gbox->verticalSpacing() != 0) ) {
			  capsule = false;
			  return;
			}

            const int rows = gbox->rowCount();
            const int cols = gbox->columnCount();

			qDebug() << "rows,cols" << rows << cols;

            if (rows == 1)
              v = 2;
            else if (index < cols)
              v = -1;
            else if (index > cols*(rows-1)-1)
              v = 1;
            else
              v = 0;

            if (cols == 1)
              h = 2;
            else if (index%cols == 0)
              h = -1;
            else if (index%cols == cols-1)
              h = 1;
            else
              h = 0;
          }
        }
      }
    }
  }
}

void QSvgStyle::drawRealRect(QPainter* p, const QRect& r) const
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
QString QSvgStyle::state_str(State st, const QWidget* w) const
{
  QString status;

  if ( !isContainerWidget(w) ) {
    status = (st & State_Enabled) ?
      (st & State_Sunken) ? "pressed" :
      (st & State_On) ? "toggled" :
      (st & State_Selected) ? "toggled" :
      (st & State_MouseOver) ? "hovered" : "normal"
    : (st & State_On) ? "disabled-toggled" : "disabled";
  } else {
    // container widgets will have only normal, selected and disabled status
    status = (st & State_Enabled ) ?
      (st & State_Selected) ? "toggled" : "normal"
    : "disabled";
  }

  return status;
}

QIcon::Mode QSvgStyle::state_iconmode(State st) const
{
  return
    (st & State_Enabled) ?
      (st & State_Sunken) ? QIcon::Active :
      (st & State_Selected ) ? QIcon::Selected :
      (st & State_MouseOver) ? QIcon::Active : QIcon::Normal
    : QIcon::Disabled;
}

QIcon::State QSvgStyle::state_iconstate(State st) const
{
  return (st & State_On) ? QIcon::On : QIcon::Off;
}

/* Auto generated */
QString QSvgStyle::PE_str(PrimitiveElement element) const
{
  switch (element) {
    case PE_Q3CheckListController : return "PE_Q3CheckListController";
    case PE_Q3CheckListExclusiveIndicator : return "PE_Q3CheckListExclusiveIndicator";
    case PE_Q3CheckListIndicator : return "PE_Q3CheckListIndicator";
    case PE_Q3DockWindowSeparator : return "PE_Q3DockWindowSeparator";
    case PE_Q3Separator : return "PE_Q3Separator";
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
    case PE_IndicatorTabTear : return "PE_IndicatorTabTear";
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

QString QSvgStyle::CE_str(QStyle::ControlElement element) const
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
    case CE_Q3DockWindowEmptyArea : return "CE_Q3DockWindowEmptyArea";
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

QString QSvgStyle::SE_str(QStyle::SubElement element) const
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
    case SE_Q3DockWindowHandleRect : return "SE_Q3DockWindowHandleRect";
    case SE_ProgressBarGroove : return "SE_ProgressBarGroove";
    case SE_ProgressBarContents : return "SE_ProgressBarContents";
    case SE_ProgressBarLabel : return "SE_ProgressBarLabel";
    case SE_DialogButtonAccept : return "SE_DialogButtonAccept";
    case SE_DialogButtonReject : return "SE_DialogButtonReject";
    case SE_DialogButtonApply : return "SE_DialogButtonApply";
    case SE_DialogButtonHelp : return "SE_DialogButtonHelp";
    case SE_DialogButtonAll : return "SE_DialogButtonAll";
    case SE_DialogButtonAbort : return "SE_DialogButtonAbort";
    case SE_DialogButtonIgnore : return "SE_DialogButtonIgnore";
    case SE_DialogButtonRetry : return "SE_DialogButtonRetry";
    case SE_DialogButtonCustom : return "SE_DialogButtonCustom";
    case SE_ToolBoxTabContents : return "SE_ToolBoxTabContents";
    case SE_HeaderLabel : return "SE_HeaderLabel";
    case SE_HeaderArrow : return "SE_HeaderArrow";
    case SE_TabWidgetTabBar : return "SE_TabWidgetTabBar";
    case SE_TabWidgetTabPane : return "SE_TabWidgetTabPane";
    case SE_TabWidgetTabContents : return "SE_TabWidgetTabContents";
    case SE_TabWidgetLeftCorner : return "SE_TabWidgetLeftCorner";
    case SE_TabWidgetRightCorner : return "SE_TabWidgetRightCorner";
    case SE_ItemViewItemCheckIndicator : return "SE_ItemViewItemCheckIndicator (= SE_ViewItemCheckIndicator)";
    case SE_TabBarTearIndicator : return "SE_TabBarTearIndicator";
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

QString QSvgStyle::CC_str(QStyle::ComplexControl element) const
{
  switch (element) {
    case CC_SpinBox : return "CC_SpinBox";
    case CC_ComboBox : return "CC_ComboBox";
    case CC_ScrollBar : return "CC_ScrollBar";
    case CC_Slider : return "CC_Slider";
    case CC_ToolButton : return "CC_ToolButton";
    case CC_TitleBar : return "CC_TitleBar";
    case CC_Q3ListView : return "CC_Q3ListView";
    case CC_Dial : return "CC_Dial";
    case CC_GroupBox : return "CC_GroupBox";
    case CC_MdiControls : return "CC_MdiControls";
    case CC_CustomBase : return "CC_CustomBase";
  }

  return "CC_Unknown";
}

QString QSvgStyle::SC_str(QStyle::ComplexControl control, QStyle::SubControl subControl) const
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
    case CC_Q3ListView : switch (subControl) {
      case SC_Q3ListView : return "SC_Q3ListView";
      case SC_Q3ListViewBranch : return "SC_Q3ListViewBranch";
      case SC_Q3ListViewExpand : return "SC_Q3ListViewExpand";
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
    default: return "SC_Unknown";
  }

  return "SC_Unknown";
}

QString QSvgStyle::CT_str(QStyle::ContentsType type) const
{
  switch (type) {
    case CT_PushButton : return "CT_PushButton";
    case CT_CheckBox : return "CT_CheckBox";
    case CT_RadioButton : return "CT_RadioButton";
    case CT_ToolButton : return "CT_ToolButton";
    case CT_ComboBox : return "CT_ComboBox";
    case CT_Splitter : return "CT_Splitter";
    case CT_Q3DockWindow : return "CT_Q3DockWindow";
    case CT_ProgressBar : return "CT_ProgressBar";
    case CT_MenuItem : return "CT_MenuItem";
    case CT_MenuBarItem : return "CT_MenuBarItem";
    case CT_MenuBar : return "CT_MenuBar";
    case CT_Menu : return "CT_Menu";
    case CT_TabBarTab : return "CT_TabBarTab";
    case CT_Slider : return "CT_Slider";
    case CT_ScrollBar : return "CT_ScrollBar";
    case CT_Q3Header : return "CT_Q3Header";
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
    default: return "CT_Unknown";
  }

  return "CT_Unknown";
}

/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */

QString QSvgStyle::PE_group(PrimitiveElement element) const
{
  switch (element) {
    // Frames
    case PE_Frame : return "Frame";
    case PE_FrameDefaultButton : return "PushButton";
    case PE_FrameDockWidget : return "DockWidget";
    case PE_FrameFocusRect : return "Focus";
    case PE_FrameGroupBox : return "GroupBox";
    case PE_FrameLineEdit : return "LineEdit";
    case PE_FrameMenu : return "Menu";
    case PE_FrameStatusBarItem : return "StatusBar";
    case PE_FrameTabWidget : return "TabWidget";
    case PE_FrameWindow : return "Window";
    case PE_FrameButtonBevel : return "PushButton";
    case PE_FrameButtonTool : return "ToolButton";
    case PE_FrameTabBarBase : return "TabWidget";
    // Panels (interiors)
    case PE_PanelButtonCommand : return "PushButton";
    case PE_PanelButtonBevel : return "PushButton";
    case PE_PanelButtonTool : return "ToolButton";
    case PE_PanelMenuBar : return "MenuBar";
    case PE_PanelToolBar : return "ToolBar";
    case PE_PanelLineEdit : return "LineEdit";
    case PE_PanelTipLabel : return "Tip";
    case PE_PanelScrollAreaCorner : return "PE_PanelScrollAreaCorner";
    case PE_PanelItemViewItem : return "ViewItem";
    case PE_PanelItemViewRow : return "ViewItem";
    case PE_PanelStatusBar : return "StatusBar";
    case PE_PanelMenu : return "Menu";
    // Indicators
    case PE_IndicatorArrowDown : return "Indicator";
    case PE_IndicatorArrowLeft : return "Indicator";
    case PE_IndicatorArrowRight : return "Indicator";
    case PE_IndicatorArrowUp : return "Indicator";
    case PE_IndicatorBranch : return "Indicator";
    case PE_IndicatorButtonDropDown : return "Indicator";
    case PE_IndicatorItemViewItemCheck : return "CheckBox";
    case PE_IndicatorCheckBox : return "CheckBox";
    case PE_IndicatorDockWidgetResizeHandle : return "DockWidget";
    case PE_IndicatorHeaderArrow : return "Indicator";
    case PE_IndicatorMenuCheckMark : return "CheckBox";
    case PE_IndicatorProgressChunk : return "ProgressbarContents";
    case PE_IndicatorRadioButton : return "RadioButton";
    case PE_IndicatorSpinDown : return "Indicator";
    case PE_IndicatorSpinMinus : return "Indocator";
    case PE_IndicatorSpinPlus : return "Indicator";
    case PE_IndicatorSpinUp : return "Indicator";
    case PE_IndicatorToolBarHandle : return "ToolBar";
    case PE_IndicatorToolBarSeparator : return "ToolBar";
    case PE_IndicatorTabTear : return "TabWidget";
    case PE_IndicatorColumnViewArrow : return "Indicator";
    case PE_IndicatorItemViewItemDrop : return "Indicator";
    case PE_IndicatorTabClose : return "TabWidget";

    default : return QString();
  }

  return QString();
}

QString QSvgStyle::CE_group(ControlElement element) const
{
  switch(element) {
    case CE_PushButton : return "PushButton";
    case CE_PushButtonBevel : return "PushButton";
    case CE_PushButtonLabel : return "PushButton";
    case CE_CheckBox : return "CheckBox";
    case CE_CheckBoxLabel : return "CheckBox";
    case CE_RadioButton : return "RadioButton";
    case CE_RadioButtonLabel : return "RadioButton";
    case CE_TabBarTab : return "TabWidget";
    case CE_TabBarTabShape : return "TabWidget";
    case CE_TabBarTabLabel : return "TabWidget";
    case CE_ProgressBar : return "ProgressBar";
    case CE_ProgressBarGroove : return "ProgressBar";
    case CE_ProgressBarContents : return "ProgressBar";
    case CE_ProgressBarLabel : return "ProgressBar";
    case CE_MenuItem : return "MenuItem";
    case CE_MenuScroller : return "MenuItem";
    case CE_MenuTearoff : return "MenuItem";
    case CE_MenuEmptyArea : return "MenuItem";
    case CE_MenuBarItem : return "MenuBarItem";
    case CE_MenuBarEmptyArea : return "MenuBarItem";
    case CE_ToolButtonLabel : return "ToolButton";
    case CE_Header : return "Header";
    case CE_HeaderSection : return "Header";
    case CE_HeaderLabel : return "Header";
    case CE_ToolBoxTab : return "ToolBox";
    case CE_SizeGrip : return "SizeGrip";
    case CE_Splitter : return "Splitter";
    case CE_RubberBand : return "RubberBand";
    case CE_DockWidgetTitle : return "DockWidget";
    case CE_ScrollBarAddLine : return "ScrollBar";
    case CE_ScrollBarSubLine : return "ScrollBar";
    case CE_ScrollBarAddPage : return "ScrollBar";
    case CE_ScrollBarSubPage : return "ScrollBar";
    case CE_ScrollBarSlider : return "ScrollBar";
    case CE_ScrollBarFirst : return "ScrollBar";
    case CE_ScrollBarLast : return "ScrollBar";
    case CE_FocusFrame : return "Focus";
    case CE_ComboBoxLabel : return "ComboBox";
    case CE_ToolBar : return "ToolBar";
    case CE_ToolBoxTabShape : return "ToolBox";
    case CE_ToolBoxTabLabel : return "ToolBox";
    case CE_HeaderEmptyArea : return "Header";
    case CE_ItemViewItem : return "ItemView";
    case CE_ShapedFrame : return "GenericFrame";
    default : return QString();
  }

  return QString();
}

QString QSvgStyle::CT_group(QStyle::ContentsType type) const
{
  switch (type) {
    case CT_PushButton : return "PushButton";
    case CT_CheckBox : return "CheckBox";
    case CT_RadioButton : return "RadioButton";
    case CT_ToolButton : return "ToolButton";
    case CT_ComboBox : return "PushButton";
    case CT_Splitter : return "Splitter";
    case CT_ProgressBar : return "ProgressBar";
    case CT_MenuItem : return "MenuItem";
    case CT_MenuBarItem : return "MenuBarItem";
    case CT_MenuBar : return "MenuBar";
    case CT_Menu : return "Menu";
    case CT_TabBarTab : return "TabWidget";
    case CT_Slider : return "Slider";
    case CT_ScrollBar : return "ScrollBar";
    case CT_LineEdit : return "LineEdit";
    case CT_SpinBox : return "SpinBox";
    case CT_TabWidget : return "TabWidget";
    case CT_HeaderSection : return "HeaderSection";
    case CT_GroupBox : return "GroupBox";
    default: return QString();
  }

  return QString();
}

QString QSvgStyle::SE_group(SubElement element) const
{
  switch(element) {
    case SE_PushButtonContents : return "PushButton";
    case SE_PushButtonFocusRect : return "PushButton";
    case SE_CheckBoxIndicator : return "CheckBox";
    case SE_CheckBoxContents : return "CheckBox";
    case SE_CheckBoxFocusRect : return "CheckBox";
    case SE_CheckBoxClickRect : return "CheckBox";
    case SE_RadioButtonIndicator : return "RadioButton";
    case SE_RadioButtonContents : return "RadioButton";
    case SE_RadioButtonFocusRect : return "RadioButton";
    case SE_RadioButtonClickRect : return "RadioButton";
    case SE_ComboBoxFocusRect : return "ComboBox";
    case SE_SliderFocusRect : return "Slider";
    case SE_ProgressBarGroove : return "ProgressBar";
    case SE_ProgressBarContents : return "ProgressBar";
    case SE_ProgressBarLabel : return "ProgressBar";
    case SE_ToolBoxTabContents : return "ToolBox";
    case SE_HeaderLabel : return "Header";
    case SE_HeaderArrow : return "Header";
    case SE_TabWidgetTabBar : return "TabWidget";
    case SE_TabWidgetTabPane : return "TabWidget";
    case SE_TabWidgetTabContents : return "TabWidget";
    case SE_TabWidgetLeftCorner : return "TabWidget";
    case SE_TabWidgetRightCorner : return "TabWidget";
    case SE_ItemViewItemCheckIndicator : return "ItemView";
    case SE_TabBarTearIndicator : return "TabWidget";
    case SE_LineEditContents : return "LineEdit";
    case SE_FrameContents : return "Frame";
    case SE_DockWidgetCloseButton : return "DockWidget";
    case SE_DockWidgetFloatButton : return "DockWidget";
    case SE_DockWidgetTitleBarText : return "DockWidget";
    case SE_DockWidgetIcon : return "DockWidget";
    case SE_ItemViewItemDecoration : return "ItemView";
    case SE_ItemViewItemText : return "ItemView";
    case SE_ItemViewItemFocusRect : return "ItemView";
    case SE_TabBarTabLeftButton : return "TabWidget";
    case SE_TabBarTabRightButton : return "TabWidget";
    case SE_TabBarTabText : return "TabWidget";
    case SE_ShapedFrameContents : return "Frame";
    case SE_ToolBarHandle : return "ToolBar";
    default : return QString();
  }

  return QString();
}

QString QSvgStyle::CC_group(QStyle::ComplexControl element) const
{
  switch (element) {
    case CC_SpinBox : return "SpinBox";
    case CC_ComboBox : return "ComboBox";
    case CC_ScrollBar : return "ScrollBar";
    case CC_Slider : return "Slider";
    case CC_ToolButton : return "ToolButton";
    case CC_TitleBar : return "TitleBar";
    case CC_Dial : return "Dial";
    case CC_GroupBox : return "GroupBox";
    default : return QString();
  }

  return QString();
}
