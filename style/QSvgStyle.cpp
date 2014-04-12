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

void QSvgStyle::drawPrimitive(PrimitiveElement element, const QStyleOption * option, QPainter * painter, const QWidget * widget) const
{
  __enter_func__();

  emit(sig_drawPrimitive_begin(PE_str(element)));

  int x,y,h,w;

  option->rect.getRect(&x,&y,&w,&h);

  QString status;

  if ( !isContainerWidget(widget) ) {
    status = (option->state & State_Enabled) ?
               (option->state & State_On) ? "toggled" :
               (option->state & State_Sunken) ? "pressed" :
               (option->state & State_Selected) ? "toggled" :
               ((option->state & State_MouseOver) || (option->state & State_HasFocus)) ? "focused" : "normal"
             : "disabled";
  } else {
    // container widget will have only normal, selected and disabled status
    status = (option->state & State_Enabled ) ?
               (option->state & State_Selected) ? "toggled" : "normal"
             : "disabled";
  }

  switch(element) {
    case PE_Widget : {
      // nothing
      break;
    }

    case PE_PanelButtonCommand : {
      const QString group = "PanelButtonCommand";

      frame_spec_t fspec = getFrameSpec(group);
      const interior_spec_t ispec = getInteriorSpec(group);

      __print_group();

      //if (fspec.hasCapsule)
        capsulePosition(widget,fspec.hasCapsule,fspec.capsuleH,fspec.capsuleV);


      renderInterior(painter,option->rect,fspec,ispec,ispec.element+"-"+status);
      //renderFrame(painter,option->rect,fspec,fspec.element+"-"+status);

      break;
    }

    case PE_FrameDefaultButton :
    case PE_FrameButtonBevel : {
      const QString group = "PanelButtonCommand";

      frame_spec_t fspec = getFrameSpec(group);
      const interior_spec_t ispec = getInteriorSpec(group);

      __print_group();

      //if (fspec.hasCapsule)
      capsulePosition(widget,fspec.hasCapsule,fspec.capsuleH,fspec.capsuleV);

      renderFrame(painter,option->rect,fspec,fspec.element+"-"+status);
    }

    case PE_PanelButtonTool : {
      const QString group = "PanelButtonTool";

      frame_spec_t fspec = getFrameSpec(group);
      const interior_spec_t ispec = getInteriorSpec(group);
      const indicator_spec_t dspec = getIndicatorSpec(group);
      const label_spec_t lspec = getLabelSpec(group);

      const QToolButton *w = qobject_cast<const QToolButton *>(widget);
      const QStyleOptionToolButton *opt = qstyleoption_cast<const QStyleOptionToolButton *>(option);

      __print_group();

      if (fspec.hasCapsule)
        capsulePosition(widget,fspec.hasCapsule,fspec.capsuleH,fspec.capsuleV);

      QRect r = option->rect;

      if (w) {
        if ( w->popupMode() == QToolButton::MenuButtonPopup ) {
          // merge with drop down button
          if (!fspec.hasCapsule)
            fspec.capsuleV = 2;
          fspec.hasCapsule = true;
          fspec.capsuleH = -1;
        } else if (
          (w->popupMode() == QToolButton::InstantPopup) ||
          ((w->popupMode() == QToolButton::DelayedPopup) && (opt->features & QStyleOptionToolButton::HasMenu))
        ) {
          // enlarge to put drop down arrow
          r.adjust(0,0,lspec.tispace+dspec.size,0);
        }
        if ( w->autoRaise() && ( (status == "normal") || (status == "disabled") ) ) {
          ;
        } else {
          //renderFrame(painter,r,fspec,fspec.element+"-"+status);
          renderInterior(painter,r,fspec,ispec,ispec.element+"-"+status);
        }
      } else {
        //renderFrame(painter,r,fspec,fspec.element+"-"+status);
        renderInterior(painter,r,fspec,ispec,ispec.element+"-"+status);
      }

      break;
    }

    case PE_FrameButtonTool : {
      const QString group = "PanelButtonTool";

      frame_spec_t fspec = getFrameSpec(group);
      const interior_spec_t ispec = getInteriorSpec(group);
      const indicator_spec_t dspec = getIndicatorSpec(group);
      const label_spec_t lspec = getLabelSpec(group);

      const QToolButton *w = qobject_cast<const QToolButton *>(widget);
      const QStyleOptionToolButton *opt = qstyleoption_cast<const QStyleOptionToolButton *>(option);

      __print_group();

      if (fspec.hasCapsule)
        capsulePosition(widget,fspec.hasCapsule,fspec.capsuleH,fspec.capsuleV);

      QRect r = option->rect;

      if (w) {
        if ( w->popupMode() == QToolButton::MenuButtonPopup ) {
          // merge with drop down button
          if (!fspec.hasCapsule)
            fspec.capsuleV = 2;
          fspec.hasCapsule = true;
          fspec.capsuleH = -1;
        } else if (
          (w->popupMode() == QToolButton::InstantPopup) ||
          ((w->popupMode() == QToolButton::DelayedPopup) && (opt->features & QStyleOptionToolButton::HasMenu))
        ) {
          // enlarge to put drop down arrow
          r.adjust(0,0,lspec.tispace+dspec.size,0);
        }
        if ( w->autoRaise() && ( (status == "normal") || (status == "disabled") ) ) {
          ;
        } else {
          renderFrame(painter,r,fspec,fspec.element+"-"+status);
        }
      } else {
        renderFrame(painter,r,fspec,fspec.element+"-"+status);
      }

      break;
    }

    case PE_IndicatorRadioButton : {
      const QString group = "RadioButton";

      const frame_spec_t fspec = getFrameSpec(group);
      const interior_spec_t ispec = getInteriorSpec(group);

      __print_group();

      if (option->state & State_Enabled)
        if (option->state & State_MouseOver)
          if (option->state & State_On) {
            renderFrame(painter,option->rect,fspec,fspec.element+"-checked-focused");
            renderInterior(painter,option->rect,fspec,ispec,ispec.element+"-checked-focused");
          } else {
            renderFrame(painter,option->rect,fspec,fspec.element+"-focused");
            renderInterior(painter,option->rect,fspec,ispec,ispec.element+"-focused");
          }
        else
          if (option->state & State_On) {
            renderFrame(painter,option->rect,fspec,fspec.element+"-checked-normal");
            renderInterior(painter,option->rect,fspec,ispec,ispec.element+"-checked-normal");
          } else {
            renderFrame(painter,option->rect,fspec,fspec.element+"-normal");
            renderInterior(painter,option->rect,fspec,ispec,ispec.element+"-normal");
          }
      else
        if (option->state & State_On) {
          renderFrame(painter,option->rect,fspec,fspec.element+"-checked-disabled");
          renderInterior(painter,option->rect,fspec,ispec,ispec.element+"-checked-disabled");
        } else {
          renderFrame(painter,option->rect,fspec,fspec.element+"-disabled");
          renderInterior(painter,option->rect,fspec,ispec,ispec.element+"-disabled");
        }

      break;
    }

    case PE_IndicatorViewItemCheck :
    case PE_IndicatorCheckBox : {
      const QString group = "CheckBox";

      const frame_spec_t fspec = getFrameSpec(group);
      const interior_spec_t ispec = getInteriorSpec(group);

      __print_group();

      if (option->state & State_Enabled)
        if (option->state & State_MouseOver)
          if (option->state & State_On) {
            renderFrame(painter,option->rect,fspec,fspec.element+"-checked-focused");
            renderInterior(painter,option->rect,fspec,ispec,ispec.element+"-checked-focused");
          } else if (option->state & State_NoChange) {
            renderFrame(painter,option->rect,fspec,fspec.element+"-tristate-focused");
            renderInterior(painter,option->rect,fspec,ispec,ispec.element+"-tristate-focused");
          } else {
            renderFrame(painter,option->rect,fspec,fspec.element+"-focused");
            renderInterior(painter,option->rect,fspec,ispec,ispec.element+"-focused");
          }
        else
          if (option->state & State_On) {
            renderFrame(painter,option->rect,fspec,fspec.element+"-checked-normal");
            renderInterior(painter,option->rect,fspec,ispec,ispec.element+"-checked-normal");
          } else if (option->state & State_NoChange) {
            renderFrame(painter,option->rect,fspec,fspec.element+"-tristate-normal");
            renderInterior(painter,option->rect,fspec,ispec,ispec.element+"-tristate-normal");
          } else {
            renderFrame(painter,option->rect,fspec,fspec.element+"-normal");
            renderInterior(painter,option->rect,fspec,ispec,ispec.element+"-normal");
          }
      else
        if (option->state & State_On) {
          renderFrame(painter,option->rect,fspec,fspec.element+"-checked-disabled");
          renderInterior(painter,option->rect,fspec,ispec,ispec.element+"-checked-disabled");
        } else  if (option->state & State_NoChange) {
          renderFrame(painter,option->rect,fspec,fspec.element+"-tristate-disabled");
          renderInterior(painter,option->rect,fspec,ispec,ispec.element+"-tristate-disabled");
        } else {
          renderFrame(painter,option->rect,fspec,fspec.element+"-disabled");
          renderInterior(painter,option->rect,fspec,ispec,ispec.element+"-disabled");
        }

      break;
    }

    case PE_IndicatorMenuCheckMark : {
      const QStyleOptionMenuItem *opt =
      qstyleoption_cast<const QStyleOptionMenuItem *>(option);

      if (opt) {
        QStyleOptionMenuItem o(*opt);

        if (opt->checkType == QStyleOptionMenuItem::Exclusive) {
          if (opt->checked)
            o.state |= State_On;
          drawPrimitive(PE_IndicatorRadioButton,&o,painter,widget);
        }

        if (opt->checkType == QStyleOptionMenuItem::NonExclusive) {
          if (opt->checked)
            o.state |= State_On;
          drawPrimitive(PE_IndicatorCheckBox,&o,painter,widget);
        }
      }

      break;
    }

    case PE_FrameFocusRect : {
      const QString group = "Focus";

      const frame_spec_t fspec = getFrameSpec(group);
      const interior_spec_t ispec = getInteriorSpec(group);

      __print_group();

      renderFrame(painter,option->rect,fspec,fspec.element);
      renderInterior(painter,option->rect,fspec,ispec,ispec.element);

      break;
    }

    case PE_IndicatorBranch : {
      QString group = "TreeExpander";

      frame_spec_t fspec = getFrameSpec(group);
      interior_spec_t ispec = getInteriorSpec(group);
      indicator_spec_t dspec = getIndicatorSpec(group);

      __print_group();

      renderFrame(painter,option->rect,fspec,fspec.element+"-"+status);
      renderInterior(painter,option->rect,fspec,ispec,ispec.element+"-"+status);

      if (option->state & State_Children) {
        if (option->state & State_Open)
          renderIndicator(painter,option->rect,fspec,ispec,dspec,dspec.element+"-minus-"+status);
        else
          renderIndicator(painter,option->rect,fspec,ispec,dspec,dspec.element+"-plus-"+status);
      }

      break;
    }

  case PE_FrameMenu :  {
      const QString group = "Menu";

      const frame_spec_t fspec = getFrameSpec(group);

      __print_group();

      renderFrame(painter,option->rect,fspec,fspec.element+"-normal");

      break;
    }

    case PE_FrameWindow : {
      const QString group = "GenericFrame";

      const frame_spec_t fspec = getFrameSpec(group);

      __print_group();

      renderFrame(painter,option->rect,fspec,fspec.element+"-"+status);

      break;
    }

    case PE_FrameTabBarBase :
      break;

    case PE_Frame : {
      const QString group = "GenericFrame";

      const frame_spec_t fspec = getFrameSpec(group);
      const interior_spec_t ispec = getInteriorSpec(group);

      __print_group();

      // ignore hovered, pressed and toggled statuses
      const QString status2 =
        (option->state & State_Enabled) ? "normal" : "disabled";

        renderFrame(painter,option->rect,fspec,fspec.element+"-"+status2);
        // FIXME should frames have an interior ?
        renderInterior(painter,option->rect,fspec,ispec,ispec.element+"-"+status2);

      break;
    }

    case PE_FrameDockWidget :
    case PE_FrameStatusBar :
     {
      const QString group = "GenericFrame";

      const frame_spec_t fspec = getFrameSpec(group);
      const interior_spec_t ispec = getInteriorSpec(group);

      __print_group();

      renderFrame(painter,option->rect,fspec,fspec.element+"-"+status);
      renderInterior(painter,option->rect,fspec,ispec,ispec.element+"-"+status);

      break;
    }

    case PE_FrameGroupBox : {
      const QString group = "GroupBox";

      const frame_spec_t fspec = getFrameSpec(group);
      const interior_spec_t ispec = getInteriorSpec(group);

      __print_group();

      renderFrame(painter,option->rect,fspec,fspec.element+"-"+status);
      renderInterior(painter,option->rect,fspec,ispec,ispec.element+"-"+status);

      break;
    }

    case PE_FrameTabWidget : {
      const QString group = "TabFrame";

      frame_spec_t fspec = getFrameSpec(group);
      const interior_spec_t ispec = getInteriorSpec(group);

      __print_group();

#if 0
      const QStyleOptionTabWidgetFrame *opt =
        qstyleoption_cast<const QStyleOptionTabWidgetFrame *>(option);

      if (opt) {
        QStyleOptionTabWidgetFrameV2 optV2(*opt);

        QRect r = optV2.tabBarRect;

        if ( (optV2.shape == QTabBar::RoundedNorth) ||
             (optV2.shape == QTabBar::TriangularNorth)
        ) {
          fspec.y0c0 = r.x();
          fspec.y0c1 = r.x()+r.width()-1;
        }

        if ( (optV2.shape == QTabBar::RoundedSouth) ||
             (optV2.shape == QTabBar::TriangularSouth)
        ) {
          fspec.y1c0 = r.x();
          fspec.y1c1 = r.x()+r.width()-1;
        }

        if ( (optV2.shape == QTabBar::RoundedWest) ||
             (optV2.shape == QTabBar::TriangularWest)
        ) {
          fspec.x0c0 = r.y();
          fspec.x0c1 = r.y()+r.height()-1;
        }

        if ( (optV2.shape == QTabBar::RoundedEast) ||
             (optV2.shape == QTabBar::TriangularEast)
        ) {
          fspec.x1c0 = r.y();
          fspec.x1c1 = r.y()+r.height()-1;
        }
      }
#endif

      renderFrame(painter,option->rect,fspec,fspec.element+"-"+status);
      renderInterior(painter,option->rect,fspec,ispec,ispec.element+"-"+status);

      break;
    }

    case PE_FrameLineEdit : {
      const QString group = "LineEdit";

      frame_spec_t fspec = getFrameSpec(group);
      if ( qstyleoption_cast<const QStyleOptionSpinBox *>(option) || qstyleoption_cast<const QStyleOptionComboBox *>(option) ) {
        // spin box and combo boxes have attached arrows, so merge with them
        fspec.hasCapsule = true;
        fspec.capsuleH = -1;
        fspec.capsuleV = 2;
      }
      const interior_spec_t ispec = getInteriorSpec(group);

      __print_group();

      renderFrame(painter,option->rect,fspec,fspec.element+"-"+status);

      break;
    }

    case PE_PanelLineEdit : {
      const QString group = "LineEdit";

      frame_spec_t fspec = getFrameSpec(group);
      if ( widget && qobject_cast<const QSpinBox *>(widget) ) {
        qDebug() <<"*********************";
        fspec.hasCapsule = true;
        fspec.capsuleH = -1;
        fspec.capsuleV = 2;
      }
      const interior_spec_t ispec = getInteriorSpec(group);

      __print_group();

      const QLineEdit *w = qobject_cast< const QLineEdit* >(widget);
      if (w && w->hasFrame()) {
        const QString status2 = /* ignore "pressed", gives bad results */
          (option->state & State_Enabled) ?
            (option->state & State_On) ? "toggled" :
            (option->state & State_Selected) ? "toggled" :
            ((option->state & State_MouseOver) || (option->state & State_HasFocus)) ? "focused" : "normal"
          : "disabled";
        renderFrame(painter,option->rect,fspec,fspec.element+"-"+status2);
      }
      renderInterior(painter,option->rect,fspec,ispec,ispec.element+"-"+status);

      break;
    }

    case PE_PanelToolBar : {
      const QString group = "Toolbar";

      const frame_spec_t fspec = getFrameSpec(group);
      const interior_spec_t ispec = getInteriorSpec(group);

      __print_group();

      renderFrame(painter,option->rect,fspec,fspec.element+"-"+status);
      renderInterior(painter,option->rect,fspec,ispec,ispec.element+"-"+status);

      break;
    }

    case PE_IndicatorToolBarHandle : {
      const QString group = "Toolbar";

      const frame_spec_t fspec = getFrameSpec(group);
      const interior_spec_t ispec = getInteriorSpec(group);
      const indicator_spec_t dspec = getIndicatorSpec(group);

      __print_group();

      renderElement(painter,dspec.element+"-handle-"+status,option->rect);

      break;
    }

    case PE_IndicatorToolBarSeparator : {
      const QString group = "Toolbar";

      const frame_spec_t fspec = getFrameSpec(group);
      const interior_spec_t ispec = getInteriorSpec(group);
      const indicator_spec_t dspec = getIndicatorSpec(group);

      __print_group();

      renderElement(painter,dspec.element+"-separator-"+status,option->rect);

      break;
    }

    case PE_IndicatorSpinPlus : {
      const QString group = "IndicatorSpinBox";

      frame_spec_t fspec = getFrameSpec(group);
      fspec.hasCapsule = true;
      fspec.capsuleH = 1;
      fspec.capsuleV = 2;
      const interior_spec_t ispec = getInteriorSpec(group);
      const indicator_spec_t dspec = getIndicatorSpec(group);

      __print_group();

      renderFrame(painter,option->rect,fspec,fspec.element+"-"+status);
      renderInterior(painter,option->rect,fspec,ispec,ispec.element+"-"+status);
      renderIndicator(painter,option->rect,fspec,ispec,dspec,dspec.element+"-plus-"+status);

      break;
    }

    case PE_IndicatorSpinMinus : {
      const QString group = "IndicatorSpinBox";

      frame_spec_t fspec = getFrameSpec(group);
      fspec.hasCapsule = true;
      fspec.capsuleH = 0;
      fspec.capsuleV = 2;
      const interior_spec_t ispec = getInteriorSpec(group);
      const indicator_spec_t dspec = getIndicatorSpec(group);

      __print_group();

      renderFrame(painter,option->rect,fspec,fspec.element+"-"+status);
      renderInterior(painter,option->rect,fspec,ispec,ispec.element+"-"+status);
      renderIndicator(painter,option->rect,fspec,ispec,dspec,dspec.element+"-minus-"+status);

      break;
    }

    case PE_IndicatorSpinUp : {
      const QString group = "IndicatorSpinBox";

      frame_spec_t fspec = getFrameSpec(group);
      fspec.hasCapsule = true;
      fspec.capsuleH = 1;
      fspec.capsuleV = 2;
      const interior_spec_t ispec = getInteriorSpec(group);
      const indicator_spec_t dspec = getIndicatorSpec(group);

      __print_group();

      renderFrame(painter,option->rect,fspec,fspec.element+"-"+status);
      renderInterior(painter,option->rect,fspec,ispec,ispec.element+"-"+status);
      renderIndicator(painter,option->rect,fspec,ispec,dspec,dspec.element+"-up-"+status);

      break;
    }

    case PE_IndicatorSpinDown : {
      const QString group = "IndicatorSpinBox";

      frame_spec_t fspec = getFrameSpec(group);
      fspec.hasCapsule = true;
      fspec.capsuleH = 0;
      fspec.capsuleV = 2;
      const interior_spec_t ispec = getInteriorSpec(group);
      const indicator_spec_t dspec = getIndicatorSpec(group);

      __print_group();

      renderFrame(painter,option->rect,fspec,fspec.element+"-"+status);
      renderInterior(painter,option->rect,fspec,ispec,ispec.element+"-"+status);
      renderIndicator(painter,option->rect,fspec,ispec,dspec,dspec.element+"-down-"+status);

      break;
    }

    case PE_IndicatorHeaderArrow : {
      const QString group = "IndicatorHeaderArrow";

      const frame_spec_t fspec = getFrameSpec(group);
      const interior_spec_t ispec = getInteriorSpec(group);
      const indicator_spec_t dspec = getIndicatorSpec(group);

      const QStyleOptionHeader *opt =
        qstyleoption_cast<const QStyleOptionHeader *>(option);

      __print_group();

      renderFrame(painter,option->rect,fspec,fspec.element+"-"+status);
      renderInterior(painter,option->rect,fspec,ispec,ispec.element+"-"+status);
      if (opt) {
        if (opt->sortIndicator == QStyleOptionHeader::SortDown)
          renderIndicator(painter,option->rect,fspec,ispec,dspec,dspec.element+"-down-"+status);
        else if (opt->sortIndicator == QStyleOptionHeader::SortUp)
          renderIndicator(painter,option->rect,fspec,ispec,dspec,dspec.element+"-up-"+status);
      }

      break;
    }

    case PE_IndicatorButtonDropDown : {
      const QString group = "DropDownButton";

      frame_spec_t fspec = getFrameSpec(group);
      fspec.hasCapsule = true;
      fspec.capsuleH = 1;
      fspec.capsuleV = 2;
      const interior_spec_t ispec = getInteriorSpec(group);
      const indicator_spec_t dspec = getIndicatorSpec(group);

      const QToolButton *w = qobject_cast<const QToolButton *>(widget);

      __print_group();

      if (w) {
        if ( w->autoRaise() && ( (status == "normal") || (status == "disabled") ) ) {
          ;
        } else {
          renderInterior(painter,option->rect,fspec,ispec,ispec.element+"-"+status);
          renderFrame(painter,option->rect,fspec,fspec.element+"-"+status);
        }
      } else {
        renderInterior(painter,option->rect,fspec,ispec,ispec.element+"-"+status);
        renderFrame(painter,option->rect,fspec,fspec.element+"-"+status);
      }

      renderIndicator(painter,option->rect,fspec,ispec,dspec,dspec.element+"-"+status);

      break;
    }

    case PE_PanelMenuBar : {
      break;
    }

    case PE_IndicatorTabTear : {
      const QString group = "Tab";

      const frame_spec_t fspec = getFrameSpec(group);
      const interior_spec_t ispec = getInteriorSpec(group);
      const indicator_spec_t dspec = getIndicatorSpec(group);

      __print_group();

      renderInterior(painter,option->rect,fspec,ispec,dspec.element+"-tear-"+status);

      break;
    }

    case PE_IndicatorArrowUp : {
      const QString group = "IndicatorArrow";

      const frame_spec_t fspec = getFrameSpec(group);
      const interior_spec_t ispec = getInteriorSpec(group);
      const indicator_spec_t dspec = getIndicatorSpec(group);

      __print_group();

      //renderFrame(painter,option->rect,fspec,fspec.element+"-"+status);
      //renderInterior(painter,option->rect,fspec,ispec,ispec.element+"-"+status);
      renderIndicator(painter,option->rect,fspec,ispec,dspec,dspec.element+"-up-"+status);

      break;
    }

    case PE_IndicatorArrowDown : {
      const QString group = "IndicatorArrow";

      const frame_spec_t fspec = getFrameSpec(group);
      const interior_spec_t ispec = getInteriorSpec(group);
      const indicator_spec_t dspec = getIndicatorSpec(group);

      __print_group();

      //renderFrame(painter,option->rect,fspec,fspec.element+"-"+status);
      //renderInterior(painter,option->rect,fspec,ispec,ispec.element+"-"+status);
      renderIndicator(painter,option->rect,fspec,ispec,dspec,dspec.element+"-down-"+status);

      break;
    }

    case PE_IndicatorArrowLeft : {
      const QString group = "IndicatorArrow";

      const frame_spec_t fspec = getFrameSpec(group);
      const interior_spec_t ispec = getInteriorSpec(group);
      const indicator_spec_t dspec = getIndicatorSpec(group);

      __print_group();

      //renderFrame(painter,option->rect,fspec,fspec.element+"-"+status);
      //renderInterior(painter,option->rect,fspec,ispec,ispec.element+"-"+status);
      renderIndicator(painter,option->rect,fspec,ispec,dspec,dspec.element+"-left-"+status);

      break;
    }

    case PE_IndicatorColumnViewArrow :
    case PE_IndicatorArrowRight : {
      const QString group = "IndicatorArrow";

      const frame_spec_t fspec = getFrameSpec(group);
      const interior_spec_t ispec = getInteriorSpec(group);
      const indicator_spec_t dspec = getIndicatorSpec(group);

      __print_group();

      //renderFrame(painter,option->rect,fspec,fspec.element+"-"+status);
      //renderInterior(painter,option->rect,fspec,ispec,ispec.element+"-"+status);
      renderIndicator(painter,option->rect,fspec,ispec,dspec,dspec.element+"-right-"+status);

      break;
    }

    case PE_IndicatorProgressChunk : {
      const QString group = "ProgressbarContents";

      const frame_spec_t fspec = getFrameSpec(group);
      const interior_spec_t ispec = getInteriorSpec(group);

      __print_group();

      renderInterior(painter,option->rect,fspec,ispec,ispec.element+"-"+status);

      break;
    }

    case PE_IndicatorDockWidgetResizeHandle : {
      drawControl(CE_Splitter,option,painter,widget);

      break;
    }

    case PE_PanelItemViewItem :
    case PE_PanelItemViewRow : {
      const QString group = "ViewItem";

      const frame_spec_t fspec = getFrameSpec(group);
      const interior_spec_t ispec = getInteriorSpec(group);

      __print_group();

      if ( status == "normal" ) {
        const QStyleOptionViewItemV2 *opt = qstyleoption_cast<const QStyleOptionViewItemV2 *>(option);

        if ( opt ) {
          if ( opt->features & QStyleOptionViewItemV2::Alternate )
            renderInterior(painter,option->rect,fspec,ispec,ispec.element+"-"+status+"alt");
          else
            renderInterior(painter,option->rect,fspec,ispec,ispec.element+"-"+status);
        } else {
          renderInterior(painter,option->rect,fspec,ispec,ispec.element+"-"+status);
        }
      } else {
        renderInterior(painter,option->rect,fspec,ispec,ispec.element+"-"+status);
      }

      break;
    }

    case PE_PanelStatusBar : {
      break;
    }

    default :
      //qDebug() << "[QSvgStyle]" << __func__ << ": Unhandled primitive " << element;
      QCommonStyle::drawPrimitive(element,option,painter,widget);
      break;
  }

  emit(sig_drawPrimitive_end(PE_str(element)));
  __exit_func();
}

void QSvgStyle::drawControl(ControlElement element, const QStyleOption * option, QPainter * painter, const QWidget * widget) const
{
  __enter_func__();
  emit(sig_drawControl_begin(CE_str(element)));

  int x,y,h,w;
  option->rect.getRect(&x,&y,&w,&h);

  QString status;

  if ( !isContainerWidget(widget) ) {
    status =
        (option->state & State_Enabled) ?
          (option->state & State_On) ? "toggled" :
          (option->state & State_Sunken) ? "pressed" :
          (option->state & State_Selected) ? "toggled" :
          ((option->state & State_MouseOver) || (option->state & State_HasFocus)) ? "focused" : "normal"
        : "disabled";
  } else {
    status = (option->state & State_Enabled) ? "normal" : "disabled";
  }

  const QIcon::Mode iconmode =
        (option->state & State_Enabled) ?
        (option->state & State_Sunken) ? QIcon::Active :
        (option->state & State_MouseOver) ? QIcon::Active : QIcon::Normal
      : QIcon::Disabled;

  const QIcon::State iconstate =
      (option->state & State_On) ? QIcon::On : QIcon::Off;

  switch (element) {
    case CE_PushButtonBevel : {
      drawPrimitive(PE_FrameButtonBevel,option,painter,widget);
      drawPrimitive(PE_PanelButtonCommand,option,painter,widget);

      break;
    }
    case CE_MenuTearoff : {
      const QStyleOptionMenuItem *opt =
          qstyleoption_cast<const QStyleOptionMenuItem *>(option);

      if (opt) {
        const QString group = "MenuItem";

        const indicator_spec_t dspec = getIndicatorSpec(group);

        __print_group();

        renderElement(painter,dspec.element+"-tearoff",option->rect,10,0);
      }

      break;
    }
    case CE_MenuItem : {
      const QStyleOptionMenuItem *opt =
          qstyleoption_cast<const QStyleOptionMenuItem *>(option);

      if (opt) {
        const QString group = "MenuItem";

        frame_spec_t fspec = getFrameSpec(group);
        const interior_spec_t ispec = getInteriorSpec(group);
        const indicator_spec_t dspec = getIndicatorSpec(group);
        const label_spec_t lspec = getLabelSpec(group);

        __print_group();

        if (opt->menuItemType == QStyleOptionMenuItem::Separator)
          renderElement(painter,dspec.element+"-separator",option->rect,20,0);
        else if (opt->menuItemType == QStyleOptionMenuItem::TearOff)
          renderElement(painter,dspec.element+"-tearoff",option->rect,20,0);
        else {
          renderFrame(painter,option->rect,fspec,fspec.element+"-"+status);
          renderInterior(painter,option->rect,fspec,ispec,ispec.element+"-"+status);

          const QStringList l = opt->text.split('\t');

          if (l.size() > 0) {
            // menu label
            if (opt->icon.isNull())
              renderLabel(painter,option->rect.adjusted(opt->maxIconWidth+lspec.tispace,0,0,0),fspec,ispec,lspec,Qt::AlignLeft|Qt::AlignVCenter | Qt::TextShowMnemonic |Qt::TextSingleLine,l[0], !(option->state & State_Enabled));
            else
              renderLabel(painter,option->rect,fspec,ispec,lspec,Qt::AlignLeft|Qt::AlignVCenter | Qt::TextShowMnemonic |Qt::TextSingleLine,l[0],!(option->state & State_Enabled),opt->icon.pixmap(opt->maxIconWidth,iconmode,iconstate));
          }
          if (l.size() > 1)
            // shortcut
            renderLabel(painter,option->rect.adjusted(opt->maxIconWidth,0,-15,0),fspec,ispec,lspec,Qt::AlignRight|Qt::AlignVCenter | Qt::TextShowMnemonic| Qt::TextSingleLine,l[1],!(option->state & State_Enabled));

          QStyleOptionMenuItem o(*opt);
          o.rect = alignedRect(QApplication::layoutDirection(),Qt::AlignRight | Qt::AlignVCenter,QSize(10,10),labelRect(o.rect,fspec,ispec,lspec));
          if (opt->menuItemType == QStyleOptionMenuItem::SubMenu) {
            drawPrimitive(PE_IndicatorArrowRight,&o,painter);
          }

          if (opt->checkType == QStyleOptionMenuItem::Exclusive) {
            if (opt->checked)
              o.state |= State_On;
            drawPrimitive(PE_IndicatorRadioButton,&o,painter,widget);
          }

          if (opt->checkType == QStyleOptionMenuItem::NonExclusive) {
            if (opt->checked)
              o.state |= State_On;
            drawPrimitive(PE_IndicatorCheckBox,&o,painter,widget);
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
        frame_spec_t fspec = getFrameSpec(group);
        fspec.hasCapsule = true;
        fspec.capsuleH = 0;
        fspec.capsuleV = 2;
        group = "MenuBarItem";
        const interior_spec_t ispec = getInteriorSpec(group);
        const label_spec_t lspec = getLabelSpec(group);

        __print_group();

        renderFrame(painter,option->rect,fspec,fspec.element+"-"+status);
        renderInterior(painter,option->rect,fspec,ispec,ispec.element+"-"+status);

        renderLabel(painter,option->rect,fspec,ispec,lspec,Qt::AlignLeft | Qt::AlignVCenter | Qt::TextShowMnemonic | Qt::TextSingleLine,opt->text,!(option->state & State_Enabled));
      }

      break;
    }

    case CE_MenuBarEmptyArea : {
        const QString group = "MenuBar";

        const frame_spec_t fspec = getFrameSpec(group);
        const interior_spec_t ispec = getInteriorSpec(group);

        __print_group();

        // NOTE this does not use the status (otherwise always disabled)
        renderFrame(painter,option->rect,fspec,fspec.element+"-normal");
        renderInterior(painter,option->rect,fspec,ispec,ispec.element+"-normal");

      break;
    }

    case CE_MenuScroller : {
      drawPrimitive(PE_PanelButtonTool,option,painter,widget);
      if (option->state & State_DownArrow)
        drawPrimitive(PE_IndicatorArrowDown,option,painter,widget);
      else
        drawPrimitive(PE_IndicatorArrowUp,option,painter,widget);

      break;
    }

    case CE_MenuHMargin:
    case CE_MenuVMargin:
      break;

    case CE_RadioButtonLabel : {
      const QStyleOptionButton *opt =
          qstyleoption_cast<const QStyleOptionButton *>(option);

      if (opt) {
        QStyleOptionButton o(*opt);

        const QString group = "RadioButton";
        const frame_spec_t fspec = getFrameSpec(group);
        const interior_spec_t ispec = getInteriorSpec(group);
        const label_spec_t lspec = getLabelSpec(group);

        __print_group();

        renderLabel(painter,option->rect,fspec,ispec,lspec,Qt::AlignLeft | Qt::AlignVCenter | Qt::TextShowMnemonic,o.text,!(option->state & State_Enabled),opt->icon.pixmap(opt->iconSize,iconmode,iconstate));
      }

      break;
    }

    case CE_CheckBoxLabel : {
      const QStyleOptionButton *opt =
          qstyleoption_cast<const QStyleOptionButton *>(option);

      if (opt) {
        QStyleOptionButton o(*opt);

        const QString group = "CheckBox";
        const frame_spec_t fspec = getFrameSpec(group);
        const interior_spec_t ispec = getInteriorSpec(group);
        const label_spec_t lspec = getLabelSpec(group);

        __print_group();

        renderLabel(painter,option->rect,fspec,ispec,lspec,Qt::AlignLeft | Qt::AlignVCenter | Qt::TextShowMnemonic,o.text,!(option->state & State_Enabled),opt->icon.pixmap(opt->iconSize,iconmode,iconstate));
      }

      break;
    }

    case CE_ComboBoxLabel : { // non editable
      const QStyleOptionComboBox *opt =
          qstyleoption_cast<const QStyleOptionComboBox *>(option);

      if (opt && !opt->editable) {
        const QString group = "ComboBox";
        const frame_spec_t fspec = getFrameSpec(group);
        const interior_spec_t ispec = getInteriorSpec(group);
        const label_spec_t lspec = getLabelSpec(group);

        __print_group();

        const QRect r = subControlRect(CC_ComboBox,opt,SC_ComboBoxEditField,widget);
        renderLabel(painter,r,fspec,ispec,lspec,Qt::AlignLeft | Qt::AlignVCenter | Qt::TextShowMnemonic,opt->currentText,!(option->state & State_Enabled),opt->currentIcon.pixmap(opt->iconSize,iconmode,iconstate));
      }

      break;
    }

    case CE_TabBarTabShape : {
      const QString group = "Tab";

      frame_spec_t fspec = getFrameSpec(group);
      const interior_spec_t ispec = getInteriorSpec(group);

      __print_group();

      const QStyleOptionTab *opt =
          qstyleoption_cast<const QStyleOptionTab *>(option);

      if (opt) {
        fspec.hasCapsule = true;
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
          fspec.capsuleH = capsule;
          fspec.capsuleV = -1;
        }

        if ( (opt->shape == QTabBar::RoundedSouth) ||
             (opt->shape == QTabBar::TriangularSouth)
        ) {
          fspec.capsuleH = capsule;
          fspec.capsuleV = 1;
        }

        if ( (opt->shape == QTabBar::RoundedWest) ||
             (opt->shape == QTabBar::TriangularWest)
        ) {
          fspec.capsuleV = capsule;
          fspec.capsuleH = -1;
        }

        if ( (opt->shape == QTabBar::RoundedEast) ||
             (opt->shape == QTabBar::TriangularEast)
        ) {
          fspec.capsuleV = capsule;
          fspec.capsuleH = 1;
        }
      }

      renderInterior(painter,option->rect,fspec,ispec,ispec.element+"-"+status);
      renderFrame(painter,option->rect,fspec,fspec.element+"-"+status);

      break;
    }

    case CE_TabBarTabLabel : {
      const QStyleOptionTab *opt =
        qstyleoption_cast<const QStyleOptionTab *>(option);

      if (opt) {
        const QString group = "Tab";

        const frame_spec_t fspec = getFrameSpec(group);
        const interior_spec_t ispec = getInteriorSpec(group);
        const label_spec_t lspec = getLabelSpec(group);

        __print_group();

        renderLabel(painter,option->rect,fspec,ispec,lspec,Qt::AlignLeft | Qt::AlignVCenter | Qt::TextShowMnemonic,opt->text,!(option->state & State_Enabled),opt->icon.pixmap(pixelMetric(PM_TabBarIconSize),iconmode,iconstate));
      }

      break;
    }

    case CE_ToolBoxTabShape : {
      const QString group = "ToolboxTab";

      frame_spec_t fspec = getFrameSpec(group);
      const interior_spec_t ispec = getInteriorSpec(group);

      __print_group();

      renderFrame(painter,option->rect,fspec,fspec.element+"-"+status);
      renderInterior(painter,option->rect,fspec,ispec,ispec.element+"-"+status);

      break;
    }

    case CE_ToolBoxTabLabel : {
      const QStyleOptionToolBox *opt =
          qstyleoption_cast<const QStyleOptionToolBox *>(option);

      if (opt) {
        const QString group = "ToolboxTab";

        frame_spec_t fspec = getFrameSpec(group);
        const interior_spec_t ispec = getInteriorSpec(group);
        const label_spec_t lspec = getLabelSpec(group);

        __print_group();

        renderLabel(painter,option->rect,fspec,ispec,lspec,Qt::AlignCenter | Qt::TextShowMnemonic,opt->text,!(option->state & State_Enabled),opt->icon.pixmap(pixelMetric(PM_TabBarIconSize),iconmode,iconstate));
      }

      break;
    }

    case CE_ProgressBarGroove : {
      const QString group = "Progressbar";

      frame_spec_t fspec = getFrameSpec(group);
      const interior_spec_t ispec = getInteriorSpec(group);

      __print_group();

      renderFrame(painter,option->rect,fspec,fspec.element+"-"+status);
      renderInterior(painter,option->rect,fspec,ispec,ispec.element+"-"+status);

      break;
    }

    case CE_ProgressBarContents : {
      const QStyleOptionProgressBar *opt =
          qstyleoption_cast<const QStyleOptionProgressBar *>(option);

      if (opt) {
        const QString group = "ProgressbarContents";

        frame_spec_t fspec = getFrameSpec(group);
        const interior_spec_t ispec = getInteriorSpec(group);

        __print_group();

        if ( opt->progress >= 0 ) {
          int empty = sliderPositionFromValue(opt->minimum,opt->maximum,opt->maximum-opt->progress,w,false);

          renderFrame(painter,option->rect.adjusted(0,0,-empty,0),fspec,fspec.element+"-"+status);
          renderInterior(painter,option->rect.adjusted(0,0,-empty,0),fspec,ispec,ispec.element+"-"+status);
        } else { // busy progressbar
          QWidget *w = (QWidget *)widget;

          int animcount = progressbars[w];
          int pm = pixelMetric(PM_ProgressBarChunkWidth);
          QRect r = option->rect.adjusted(animcount,0,0,0);
          r.setX(option->rect.x()+(animcount%option->rect.width()));
          r.setWidth(pm);
          if ( r.x()+r.width()-1 > option->rect.x()+option->rect.width()-1 ) {
            // wrap busy indicator
            fspec.hasCapsule = true;
            fspec.capsuleH = 1;
            fspec.capsuleV = 2;
            r.setWidth(option->rect.x()+option->rect.width()-r.x());
            renderFrame(painter,r,fspec,fspec.element+"-"+status);
            renderInterior(painter,r,fspec,ispec,ispec.element+"-"+status);

            fspec.capsuleH = -1;
            r = QRect(option->rect.x(),option->rect.y(),pm-r.width(),option->rect.height());
            renderFrame(painter,r,fspec,fspec.element+"-"+status);
            renderInterior(painter,r,fspec,ispec,ispec.element+"-"+status);
          } else {
            renderFrame(painter,r,fspec,fspec.element+"-"+status);
            renderInterior(painter,r,fspec,ispec,ispec.element+"-"+status);
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

        const frame_spec_t fspec = getFrameSpec(group);
        const interior_spec_t ispec = getInteriorSpec(group);
        const label_spec_t lspec = getLabelSpec(group);

        __print_group();

        renderLabel(painter,option->rect,fspec,ispec,lspec,opt->textAlignment,opt->text,!(option->state & State_Enabled));
      }

      break;
    }

    case CE_Splitter : {
      const QString group = "Splitter";

      const frame_spec_t fspec = getFrameSpec(group);
      const interior_spec_t ispec = getInteriorSpec(group);

      __print_group();

      renderFrame(painter,option->rect,fspec,fspec.element+"-"+status);
      renderInterior(painter,option->rect,fspec,ispec,ispec.element+"-"+status, (h > w) ? Qt::Vertical : Qt::Horizontal);

      break;
    }

    case CE_ScrollBarAddLine : {
      const QString group = "Scrollbar";

      const frame_spec_t fspec = getFrameSpec(group);
      const interior_spec_t ispec = getInteriorSpec(group);
      const indicator_spec_t dspec = getIndicatorSpec(group);

      __print_group();

      renderFrame(painter,option->rect,fspec,fspec.element+"-"+status);
      renderInterior(painter,option->rect,fspec,ispec,ispec.element+"-"+status);
      if (option->state & State_Horizontal)
        renderIndicator(painter,option->rect,fspec,ispec,dspec,dspec.element+"-right-"+status);
      else
        renderIndicator(painter,option->rect,fspec,ispec,dspec,dspec.element+"-down-"+status);

      break;
    }

    case CE_ScrollBarSubLine : {
      const QString group = "Scrollbar";

      const frame_spec_t fspec = getFrameSpec(group);
      const interior_spec_t ispec = getInteriorSpec(group);
      const indicator_spec_t dspec = getIndicatorSpec(group);

      __print_group();

      renderFrame(painter,option->rect,fspec,fspec.element+"-"+status);
      renderInterior(painter,option->rect,fspec,ispec,ispec.element+"-"+status);
      if (option->state & State_Horizontal)
        renderIndicator(painter,option->rect,fspec,ispec,dspec,dspec.element+"-left-"+status);
      else
        renderIndicator(painter,option->rect,fspec,ispec,dspec,dspec.element+"-up-"+status);

      break;
    }

    case CE_ScrollBarSlider : {
      const QString group = "ScrollbarSlider";

      const frame_spec_t fspec = getFrameSpec(group);
      const interior_spec_t ispec = getInteriorSpec(group);

      __print_group();

      renderFrame(painter,option->rect,fspec,fspec.element+"-"+status);
      renderInterior(painter,option->rect,fspec,ispec,ispec.element+"-"+status);

      break;
    }

    case CE_HeaderSection : {
      const QString group = "HeaderSection";

      const frame_spec_t fspec = getFrameSpec(group);
      const interior_spec_t ispec = getInteriorSpec(group);

      __print_group();

      renderFrame(painter,option->rect,fspec,fspec.element+"-"+status);
      renderInterior(painter,option->rect,fspec,ispec,ispec.element+"-"+status);

      break;
    }

    case CE_HeaderLabel : {
      const QStyleOptionHeader *opt =
        qstyleoption_cast<const QStyleOptionHeader *>(option);

      if (opt) {
        const QString group = "HeaderSection";

        const frame_spec_t fspec = getFrameSpec(group);
        const interior_spec_t ispec = getInteriorSpec(group);
        const label_spec_t lspec = getLabelSpec(group);

        __print_group();

        renderLabel(painter,option->rect,fspec,ispec,lspec,opt->textAlignment,opt->text,!(option->state & State_Enabled),opt->icon.pixmap(pixelMetric(PM_SmallIconSize),iconmode,iconstate));
      }

      break;
    }

    case CE_ToolBar : {
      const QString group = "Toolbar";

      const frame_spec_t fspec = getFrameSpec(group);
      const interior_spec_t ispec = getInteriorSpec(group);
      const indicator_spec_t dspec = getIndicatorSpec(group);

      __print_group();

      renderFrame(painter,option->rect,fspec,fspec.element+"-"+status);
      renderInterior(painter,option->rect,fspec,ispec,ispec.element+"-"+status, (option->state & State_Horizontal) ? Qt::Horizontal : Qt::Vertical);

      break;
    }

    case CE_SizeGrip : {
      const QString group = "SizeGrip";

      const frame_spec_t fspec = getFrameSpec(group);
      const interior_spec_t ispec = getInteriorSpec(group);
      const indicator_spec_t dspec = getIndicatorSpec(group);

      __print_group();

      renderFrame(painter,option->rect,fspec,fspec.element+"-"+status);
      renderInterior(painter,option->rect,fspec,ispec,ispec.element+"-"+status);
      renderIndicator(painter,option->rect,fspec,ispec,dspec,dspec.element+"-"+status);

      break;
    }

    case CE_PushButton : {
      drawControl(CE_PushButtonBevel,option,painter,widget);
      drawControl(CE_PushButtonLabel,option,painter,widget);

      break;
    }

    case CE_PushButtonLabel : {
      const QStyleOptionButton *opt =
          qstyleoption_cast<const QStyleOptionButton *>(option);

      if (opt) {
        const QString group = "PanelButtonCommand";
        const frame_spec_t fspec = getFrameSpec(group);
        const interior_spec_t ispec = getInteriorSpec(group);
	const indicator_spec_t dspec = getIndicatorSpec(group);
        const label_spec_t lspec = getLabelSpec(group);

        __print_group();

        const QPushButton *w = qobject_cast<const QPushButton *>(widget);
        if (w && w->isDefault()) {
          QFont f(w->font());
          f.setBold(true);
          painter->setFont(f);
        }

        if ( opt->features & QStyleOptionButton::HasMenu ) {
	  QStyleOptionButton o(*opt);
	  renderLabel(painter,option->rect.adjusted(0,0,-dspec.size-lspec.tispace,0),fspec,ispec,lspec,Qt::AlignCenter | Qt::AlignVCenter | Qt::TextShowMnemonic,opt->text,!(option->state & State_Enabled),opt->icon.pixmap(opt->iconSize,iconmode,iconstate));
	  o.rect = QRect(x+option->rect.width()-lspec.tispace-dspec.size-fspec.right,y,dspec.size,h);
          drawPrimitive(PE_IndicatorArrowDown,&o,painter,widget);
	} else {
	  renderLabel(painter,option->rect,fspec,ispec,lspec,Qt::AlignCenter | Qt::AlignVCenter | Qt::TextShowMnemonic,opt->text,!(option->state & State_Enabled),opt->icon.pixmap(opt->iconSize,iconmode,iconstate));
	}
      }

      break;
    }

    case CE_ToolButtonLabel : {
      const QStyleOptionToolButton *opt =
          qstyleoption_cast<const QStyleOptionToolButton *>(option);

      if (opt) {
        const QString group = "PanelButtonTool";
        const frame_spec_t fspec = getFrameSpec(group);
        const interior_spec_t ispec = getInteriorSpec(group);
        const indicator_spec_t dspec = getIndicatorSpec(group);
        const label_spec_t lspec = getLabelSpec(group);

        __print_group();

        if (opt->arrowType == Qt::NoArrow)
            renderLabel(painter,option->rect,fspec,ispec,lspec,Qt::AlignCenter | Qt::TextShowMnemonic,opt->text,!(option->state & State_Enabled),opt->icon.pixmap(opt->iconSize,iconmode,iconstate),opt->toolButtonStyle);
        else
          renderLabel(painter,option->rect.adjusted(dspec.size+lspec.tispace,0,0,0),fspec,ispec,lspec,Qt::AlignCenter | Qt::TextShowMnemonic,opt->text,!(option->state & State_Enabled),opt->icon.pixmap(opt->iconSize,iconmode,iconstate),opt->toolButtonStyle);

	// alignement of arrow
	Qt::AlignmentFlag hAlign = Qt::AlignLeft;
	if ( opt->text.isEmpty() && opt->icon.isNull() )
	  hAlign = Qt::AlignCenter;

        switch (opt->arrowType) {
          case Qt::NoArrow :
            break;
          case Qt::UpArrow :
            renderIndicator(painter,option->rect,fspec,ispec,dspec,dspec.element+"-up-"+status,hAlign | Qt::AlignVCenter);
            break;
          case Qt::DownArrow :
            renderIndicator(painter,option->rect,fspec,ispec,dspec,dspec.element+"-down-"+status,hAlign | Qt::AlignVCenter);
            break;
          case Qt::LeftArrow :
            renderIndicator(painter,option->rect,fspec,ispec,dspec,dspec.element+"-left-"+status,hAlign | Qt::AlignVCenter);
            break;
          case Qt::RightArrow :
            renderIndicator(painter,option->rect,fspec,ispec,dspec,dspec.element+"-right-"+status,hAlign | Qt::AlignVCenter);
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

        const frame_spec_t fspec = getFrameSpec(group);
        const interior_spec_t ispec = getInteriorSpec(group);
        const label_spec_t lspec = getLabelSpec(group);

        const QDockWidget *w = qobject_cast<const QDockWidget *>(widget);
        __print_group();

        renderFrame(painter,option->rect,fspec,fspec.element+"-"+status);
        if ( w ) {
          renderInterior(painter,option->rect,fspec,ispec,ispec.element+"-"+status, (w->features() & QDockWidget::DockWidgetVerticalTitleBar) ? Qt::Vertical : Qt::Horizontal);
        } else {
          renderInterior(painter,option->rect,fspec,ispec,ispec.element+"-"+status);
        }
        renderLabel(painter,option->rect,fspec,ispec,lspec,Qt::AlignLeft | Qt::AlignVCenter | Qt::TextShowMnemonic,opt->title,!(option->state & State_Enabled));
      }

      break;
    }

    case CE_ShapedFrame : {
      const QString group = "GenericFrame";

      const frame_spec_t fspec = getFrameSpec(group);
      const interior_spec_t ispec = getInteriorSpec(group);

      __print_group();

      const QStyleOptionFrameV3 *opt =
        qstyleoption_cast<const QStyleOptionFrameV3 *>(option);

      if ( opt && (opt->frameShape == QFrame::HLine) ) {
        renderElement(painter,
                      fspec.element+"-"+"hsep",
                      option->rect,
                      0,0,Qt::Horizontal,animationcount%fspec.animationFrames);
      } else if (opt && (opt->frameShape == QFrame::VLine) ) {
        renderElement(painter,
                      fspec.element+"-"+"vsep",
                      option->rect,
                      0,0,Qt::Horizontal,animationcount%fspec.animationFrames);
      } else if (opt && (opt->frameShape != QFrame::NoFrame) ) {
        drawPrimitive(PE_Frame,opt,painter,widget);
      }

      break;
    }

    default :
      //qDebug() << "[QSvgStyle] " << __func__ << ": Unhandled control " << element;
      QCommonStyle::drawControl(element,option,painter,widget);
  }

  emit(sig_drawControl_end(CE_str(element)));
  __exit_func();
}

void QSvgStyle::drawComplexControl(ComplexControl control, const QStyleOptionComplex * option, QPainter * painter, const QWidget * widget) const
{
  __enter_func__();
  emit(sig_drawComplexControl_begin(CC_str(control)));

  int x,y,h,w;
  option->rect.getRect(&x,&y,&w,&h);

  QString status;

  if ( !isContainerWidget(widget) ) {
    status =
    (option->state & State_Enabled) ?
    (option->state & State_On) ? "toggled" :
    (option->state & State_Sunken) ? "pressed" :
    (option->state & State_Selected) ? "toggled" :
    ((option->state & State_MouseOver) || (option->state & State_HasFocus)) ? "focused" : "normal"
    : "disabled";
  } else {
    status = (option->state & State_Enabled) ? "normal" : "disabled";
  }

  switch (control) {
    case CC_ToolButton : {
      const QStyleOptionToolButton *opt =
        qstyleoption_cast<const QStyleOptionToolButton *>(option);

      if (opt) {
        QStyleOptionToolButton o(*opt);

        o.rect = subControlRect(CC_ToolButton,opt,SC_ToolButton,widget);
        drawPrimitive(PE_FrameButtonTool,&o,painter,widget);
        drawPrimitive(PE_PanelButtonTool,&o,painter,widget);
        drawControl(CE_ToolButtonLabel,&o,painter,widget);

        if (widget) {
          const QToolButton * w = qobject_cast<const QToolButton *>(widget);
          if (w) {
            o.rect = subControlRect(CC_ToolButton,opt,SC_ToolButtonMenu,widget);
            if (w->popupMode() == QToolButton::MenuButtonPopup)
              drawPrimitive(PE_IndicatorButtonDropDown,&o,painter,widget);
            else if (w->popupMode() == QToolButton::InstantPopup)
              drawPrimitive(PE_IndicatorArrowDown,&o,painter,widget);
            else if ((w->popupMode() == QToolButton::DelayedPopup) && (opt->features & QStyleOptionToolButton::HasMenu)) {
              // Double down arrow for delayed popups
              drawPrimitive(PE_IndicatorArrowDown,&o,painter,widget);
              o.rect.adjust(0,-10,0,0);
              drawPrimitive(PE_IndicatorArrowDown,&o,painter,widget);
            }
          }
        }
      }

      break;
    }

    case CC_SpinBox : {
      const QStyleOptionSpinBox *opt =
        qstyleoption_cast<const QStyleOptionSpinBox *>(option);

      if (opt) {
        QStyleOptionSpinBox o(*opt);

        o.rect = subControlRect(CC_SpinBox,opt,SC_SpinBoxEditField,widget);
        drawPrimitive(PE_FrameLineEdit,&o,painter,widget);
        /* The QSpinBox widget will itself invoke the drawing of a PanelLineEdit,
         * so this is not needed */
        //drawPrimitive(PE_PanelLineEdit,&o,painter,widget);

        if (opt->buttonSymbols == QAbstractSpinBox::UpDownArrows) {
          o.rect = subControlRect(CC_SpinBox,opt,SC_SpinBoxUp,widget);
          drawPrimitive(PE_IndicatorSpinUp,&o,painter,widget);
          o.rect = subControlRect(CC_SpinBox,opt,SC_SpinBoxDown,widget);
          drawPrimitive(PE_IndicatorSpinDown,&o,painter,widget);
        }
        if (opt->buttonSymbols == QAbstractSpinBox::PlusMinus) {
          o.rect = subControlRect(CC_SpinBox,opt,SC_SpinBoxUp,widget);
          drawPrimitive(PE_IndicatorSpinPlus,&o,painter,widget);
          o.rect = subControlRect(CC_SpinBox,opt,SC_SpinBoxDown,widget);
          drawPrimitive(PE_IndicatorSpinMinus,&o,painter,widget);
        }
      }

      break;
    }

    case CC_ComboBox : {
      const QStyleOptionComboBox *opt =
          qstyleoption_cast<const QStyleOptionComboBox *>(option);

      if (opt) {
        QStyleOptionComboBox o(*opt);

        QString group;

        if (!opt->editable)
          group = "ComboBox";
        else
          group = "LineEdit";

        frame_spec_t fspec = getFrameSpec(group);
        fspec.hasCapsule = true;
        fspec.capsuleH = -1;
        fspec.capsuleV = 2;
        const interior_spec_t ispec = getInteriorSpec(group);
        const label_spec_t lspec = getLabelSpec(group);

        o.rect = subControlRect(CC_ComboBox,opt,SC_ComboBoxEditField,widget);

        renderFrame(painter,o.rect,fspec,fspec.element+"-"+status);
        renderInterior(painter,o.rect,fspec,ispec,ispec.element+"-"+status);

        o.rect = subControlRect(CC_ComboBox,opt,SC_ComboBoxArrow,widget);
        drawPrimitive(PE_IndicatorButtonDropDown,&o,painter,widget);

        // FIXME when the combo box is editable, the draw of edit field is called
        // (by who ?) but the icon is missing (edit fields do not have icons)
        // so draw the icon here
        if (opt->editable) {
          const QIcon::Mode iconmode =
            (option->state & State_Enabled) ?
            (option->state & State_Sunken) ? QIcon::Active :
            (option->state & State_MouseOver) ? QIcon::Active : QIcon::Normal
            : QIcon::Disabled;

          const QIcon::State iconstate =
            (option->state & State_On) ? QIcon::On : QIcon::Off;

          const QRect r = subControlRect(CC_ComboBox,opt,SC_ComboBoxEditField,widget);
          renderLabel(painter,r,fspec,ispec,lspec,Qt::AlignLeft | Qt::AlignVCenter | Qt::TextShowMnemonic," ",!(option->state & State_Enabled),opt->currentIcon.pixmap(opt->iconSize,iconmode,iconstate));
        }
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
        renderFrame(painter,o.rect,fspec,fspec.element+"-"+status);
        renderInterior(painter,o.rect,fspec,ispec,ispec.element+"-"+status);

        o.rect = subControlRect(CC_ScrollBar,opt,SC_ScrollBarAddLine,widget);
        drawControl(CE_ScrollBarAddLine,&o,painter,widget);

        o.rect = subControlRect(CC_ScrollBar,opt,SC_ScrollBarSubLine,widget);
        drawControl(CE_ScrollBarSubLine,&o,painter,widget);

        o.rect = subControlRect(CC_ScrollBar,opt,SC_ScrollBarSlider,widget);
        drawControl(CE_ScrollBarSlider,&o,painter,widget);
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
            renderFrame(painter,empty,fspec,fspec.element+"-focused");
            renderInterior(painter,empty,fspec,ispec,ispec.element+"-focused",(option->state & State_Horizontal) ? Qt::Vertical : Qt::Horizontal);
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
            renderFrame(painter,full,fspec,fspec.element+"-toggled");
            renderInterior(painter,full,fspec,ispec,ispec.element+"-toggled",(option->state & State_Horizontal) ? Qt::Vertical : Qt::Horizontal);
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
            renderFrame(painter,empty,fspec,fspec.element+"-normal");
            renderInterior(painter,empty,fspec,ispec,ispec.element+"-normal",(option->state & State_Horizontal) ? Qt::Vertical : Qt::Horizontal);
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
            renderFrame(painter,full,fspec,fspec.element+"-toggled");
            renderInterior(painter,full,fspec,ispec,ispec.element+"-toggled",(option->state & State_Horizontal) ? Qt::Vertical : Qt::Horizontal);
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
          renderFrame(painter,empty,fspec,fspec.element+"-disabled");
          renderInterior(painter,empty,fspec,ispec,ispec.element+"-disabled",(option->state & State_Horizontal) ? Qt::Vertical : Qt::Horizontal);
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
          renderFrame(painter,full,fspec,fspec.element+"-disabled");
          renderInterior(painter,full,fspec,ispec,ispec.element+"-disabled",(option->state & State_Horizontal) ? Qt::Vertical : Qt::Horizontal);
        }

        group = "SliderCursor";

        fspec = getFrameSpec(group);
        ispec = getInteriorSpec(group);
        indicator_spec_t dspec = getIndicatorSpec(group);

        o.rect = subControlRect(CC_Slider,opt,SC_SliderHandle,widget);

        renderFrame(painter,o.rect,fspec,fspec.element+"-"+status);
        renderInterior(painter,o.rect,fspec,ispec,ispec.element+"-"+status);
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

        renderElement(painter,"dial-empty",empty);
        painter->save();
        painter->setClipRect(full);
        renderElement(painter,"dial-full",empty);
        painter->restore();
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

        renderFrame(painter,o.rect,fspec,fspec.element+"-"+status);
        renderInterior(painter,o.rect,fspec,ispec,ispec.element+"-"+status);

        o.rect = subControlRect(CC_TitleBar,opt,SC_TitleBarLabel,widget);

        renderLabel(painter,o.rect,fspec,ispec,lspec,Qt::AlignLeft | Qt::AlignVCenter, o.text, !(option->state & State_Enabled),o.icon.pixmap(pixelMetric(PM_TitleBarHeight)));

        drawComplexControl(CC_MdiControls,opt,painter,widget);
      }

      break;
    }

    case CC_GroupBox : {
      const QStyleOptionGroupBox *opt =
	qstyleoption_cast<const QStyleOptionGroupBox *>(option);

      const QGroupBox *w = qobject_cast<const QGroupBox *>(widget);

      if (opt) {
        QStyleOptionGroupBox o(*opt);

        QString group = "GroupBox";

        frame_spec_t fspec = getFrameSpec(group);
        interior_spec_t ispec = getInteriorSpec(group);
        label_spec_t lspec = getLabelSpec(group);

        QRect r1,r2;

        r1 = subControlRect(CC_GroupBox,&o,SC_GroupBoxFrame,widget);
        r2 = subControlRect(CC_GroupBox,&o,SC_GroupBoxLabel,widget);

        // Draw frame and interior around contents
        renderFrame(painter,r1,fspec,fspec.element+"-"+status);
        renderInterior(painter,r1,fspec,ispec,ispec.element+"-"+status);

        // Draw frame and interior around title
        fspec.hasCapsule = true;
        fspec.capsuleH = 2;
        fspec.capsuleV = -1; // FIXME bottom titles
        renderFrame(painter,r2,fspec,fspec.element+"-"+status);
        renderInterior(painter,r2,fspec,ispec,ispec.element+"-"+status);

        // Draw title
        fspec.hasCapsule = false;
        r2 = subControlRect(CC_GroupBox,&o,SC_GroupBoxLabel,widget);
        if ( w && w->isCheckable() ) {
          renderLabel(painter,r2.adjusted(pixelMetric(PM_IndicatorWidth)+pixelMetric(PM_CheckBoxLabelSpacing),0,0,0),fspec,ispec,lspec,opt->textAlignment | Qt::TextShowMnemonic,opt->text,!opt->state & State_Enabled);
          QStyleOption oo;
          oo.initFrom(w);
          if ( w->isChecked() )
            oo.state |= State_On;
          oo.rect = subControlRect(CC_GroupBox,&o,SC_GroupBoxCheckBox,widget);
          drawPrimitive(PE_IndicatorCheckBox,&oo,painter,NULL);
        } else
          renderLabel(painter,r2,fspec,ispec,lspec,opt->textAlignment | Qt::TextShowMnemonic,opt->text,!opt->state & State_Enabled);
      }
      break;
    }

    // case CC_MdiControls : {

    //   break;
    // }

    default :
      //qDebug() << "[QSvgStyle] " << __func__ << ": Unhandled complex control " << control;
      QCommonStyle::drawComplexControl(control,option,painter,widget);
  }

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

    case PM_ScrollBarExtent : return 6;
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

QSize QSvgStyle::sizeFromContents ( ContentsType type, const QStyleOption * option, const QSize & contentsSize, const QWidget * widget) const
{
  if (!option)
    return contentsSize;

  emit(sig_sizeFromContents_begin(CT_str(type)));

  int x,y,w,h;
  option->rect.getRect(&x,&y,&w,&h);

  int dw = contentsSize.width();
  int dh = contentsSize.height();

  QSize s;

  switch (type) {
    case CT_LineEdit : {
      QFont f = QApplication::font();
        if (widget)
          f = widget->font();

      const QString group = "LineEdit";

      const frame_spec_t fspec = getFrameSpec(group);
      const interior_spec_t ispec = getInteriorSpec(group);
      const label_spec_t lspec = getLabelSpec(group);
      const size_spec_t sspec = getSizeSpec(group);

      s = sizeFromContents(f,fspec,ispec,lspec,sspec,"W",QPixmap())+QSize(8,0); // +8: seems required by LineEdit widget itself
      s = QSize(s.width() < dw ? dw : s.width(),s.height());
      break;
    }

    case CT_SpinBox : {
      const QStyleOptionSpinBox *opt =
        qstyleoption_cast<const QStyleOptionSpinBox *>(option);

      if (opt) {
        QFont f = QApplication::font();
        if (widget)
          f = widget->font();

        const QString group = "LineEdit";

        const frame_spec_t fspec = getFrameSpec(group);
        const interior_spec_t ispec = getInteriorSpec(group);
        const label_spec_t lspec = getLabelSpec(group);
        const size_spec_t sspec = getSizeSpec(group);

        if (widget) {
          const QSpinBox *w = qobject_cast<const QSpinBox *>(widget);
          if (w)
            s = sizeFromContents(f,fspec,ispec,lspec,sspec,w->prefix()+QString("%1").arg(w->maximum())+w->suffix(),QPixmap())+QSize(40,0)+QSize(8,0);
        } else
          s = QCommonStyle::sizeFromContents(CT_SpinBox,opt,contentsSize,widget)+QSize(40,0);
      }

      break;
    }

    case CT_ComboBox : {
      const QStyleOptionComboBox *opt =
          qstyleoption_cast<const QStyleOptionComboBox *>(option);

      if (opt) {
        QFont f = QApplication::font();
        if (widget)
          f = widget->font();

        const QString group = "ComboBox";

        const frame_spec_t fspec = getFrameSpec(group);
        const interior_spec_t ispec = getInteriorSpec(group);
        const label_spec_t lspec = getLabelSpec(group);
        const size_spec_t sspec = getSizeSpec(group);

        QString text = opt->currentText; // longest text

        s = sizeFromContents(f,fspec,ispec,lspec,sspec,"W",QPixmap());

        if (widget && !opt->editable) {
          const QComboBox *w = qobject_cast<const QComboBox *>(widget);

          if (w) {
            for (int i=0; i < w->count(); i++) {
              QSize _s = sizeFromContents(f,fspec,ispec,lspec,sspec,w->itemText(i),w->itemIcon(i).pixmap(w->iconSize()));
              s = QSize(qMax(s.width(),_s.width()),qMax(s.height(),_s.height()));
            }
          }
        }

        if (s.width() < contentsSize.width())
          s.setWidth(contentsSize.width());
        if (s.height() < contentsSize.height())
          s.setHeight(contentsSize.height());

        s += QSize(20,0); // drop down button
      }

      break;
    }

    case CT_PushButton : {
      const QStyleOptionButton *opt =
        qstyleoption_cast<const QStyleOptionButton *>(option);

      if (opt) {
        QFont f = QApplication::font();

        // set bold for default button
        const QPushButton *w = qobject_cast<const QPushButton *>(widget);
        if (w/* && w->isDefault()*/) {
          // some buttons become bold when pressed, so ...
          f = w->font();
          f.setBold(true);
        }

        const QString group = "PanelButtonCommand";

        const frame_spec_t fspec = getFrameSpec(group);
        const interior_spec_t ispec = getInteriorSpec(group);
        const label_spec_t lspec = getLabelSpec(group);
	const indicator_spec_t dspec = getIndicatorSpec(group);
        const size_spec_t sspec = getSizeSpec(group);

        __print_group();

        s = sizeFromContents(f,fspec,ispec,lspec,sspec,(opt->text.isEmpty() && opt->icon.isNull()) ? "W" : opt->text,opt->icon.pixmap(opt->iconSize));

	if ( opt->features & QStyleOptionButton::HasMenu ) {
	  s.rwidth() += lspec.tispace+dspec.size;
	}
      }

      break;
    }

    case CT_RadioButton : {
      const QStyleOptionButton *opt =
        qstyleoption_cast<const QStyleOptionButton *>(option);

      if (opt) {
        QFont f = QApplication::font();
        if (widget)
          f = widget->font();

        const QString group = "RadioButton";

        const frame_spec_t fspec = getFrameSpec(group);
        const interior_spec_t ispec = getInteriorSpec(group);
        const label_spec_t lspec = getLabelSpec(group);
        const size_spec_t sspec = getSizeSpec(group);

        s = sizeFromContents(f,fspec,ispec,lspec,sspec,opt->text,opt->icon.pixmap(opt->iconSize))+QSize(pixelMetric(PM_CheckBoxLabelSpacing)+pixelMetric(PM_IndicatorWidth),0);
	s = s.expandedTo(QSize(pixelMetric(PM_IndicatorWidth),pixelMetric(PM_IndicatorWidth))); // minimal checkbox size is size of indicator
      }

      break;
    }

    case CT_CheckBox : {
      const QStyleOptionButton *opt =
        qstyleoption_cast<const QStyleOptionButton *>(option);

      if (opt) {
        QFont f = QApplication::font();
        if (widget)
          f = widget->font();

        const QString group = "CheckBox";

        const frame_spec_t fspec = getFrameSpec(group);
        const interior_spec_t ispec = getInteriorSpec(group);
        const label_spec_t lspec = getLabelSpec(group);
        const size_spec_t sspec = getSizeSpec(group);

        s = sizeFromContents(f,fspec,ispec,lspec,sspec,opt->text,opt->icon.pixmap(opt->iconSize))+QSize(pixelMetric(PM_CheckBoxLabelSpacing)+pixelMetric(PM_IndicatorWidth),0);
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

        const frame_spec_t fspec = getFrameSpec(group);
        const interior_spec_t ispec = getInteriorSpec(group);
        const label_spec_t lspec = getLabelSpec(group);
        const size_spec_t sspec = getSizeSpec(group);

        if (opt->menuItemType == QStyleOptionMenuItem::Separator)
          s = QSize(dw,2); /* FIXME there is no PM_MenuSeparatorHeight pixel metric */
        else {
          s = sizeFromContents(f,fspec,ispec,lspec,sspec,opt->text,opt->icon.pixmap(opt->maxIconWidth));
        }

        if (opt->icon.pixmap(opt->maxIconWidth).isNull())
          s.rwidth() += lspec.tispace+opt->maxIconWidth;

        if ( (opt->menuItemType == QStyleOptionMenuItem::SubMenu) ||
             (opt->checkType == QStyleOptionMenuItem::Exclusive) ||
             (opt->checkType == QStyleOptionMenuItem::NonExclusive)
            ) {
          s.rwidth() += 15+lspec.tispace;
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
        frame_spec_t fspec = getFrameSpec(group);
        group = "MenuBarItem";
        const interior_spec_t ispec = getInteriorSpec(group);
        const label_spec_t lspec = getLabelSpec(group);
        const size_spec_t sspec = getSizeSpec(group);

        s = sizeFromContents(f,fspec,ispec,lspec,sspec,opt->text,opt->icon.pixmap(opt->maxIconWidth));
      }

      break;
    }

    case CT_ProgressBar : {
      const QStyleOptionProgressBar *opt =
        qstyleoption_cast<const QStyleOptionProgressBar *>(option);

      if (opt) {
        QFont f = QApplication::font();
        if (widget)
          f = widget->font();

        const QString group = "Progressbar";
        const frame_spec_t fspec = getFrameSpec(group);
        const interior_spec_t ispec = getInteriorSpec(group);
        const label_spec_t lspec = getLabelSpec(group);
        const size_spec_t sspec = getSizeSpec(group);

        s = sizeFromContents(f,fspec,ispec,lspec,sspec,opt->text,QPixmap());
      }

      break;
    }

    case CT_ToolButton : {
      const QStyleOptionToolButton *opt =
        qstyleoption_cast<const QStyleOptionToolButton *>(option);

      if (opt) {
        QFont f = QApplication::font();
        if (widget)
          f = widget->font();

        const QString group = "PanelButtonTool";

        const frame_spec_t fspec = getFrameSpec(group);
        const interior_spec_t ispec = getInteriorSpec(group);
        const label_spec_t lspec = getLabelSpec(group);
        const indicator_spec_t dspec = getIndicatorSpec(group);
        const size_spec_t sspec = getSizeSpec(group);

	// minimum size
	QSize ms;
	if ( opt->text.isEmpty() && (opt->toolButtonStyle == Qt::ToolButtonTextOnly) )
	  ms = sizeFromContents(f,fspec,ispec,lspec,sspec,"W",opt->icon.pixmap(opt->iconSize),Qt::ToolButtonTextOnly);
	if ( opt->icon.isNull() && (opt->toolButtonStyle == Qt::ToolButtonIconOnly) )
	  ms = sizeFromContents(f,fspec,ispec,lspec,sspec,"W",opt->icon.pixmap(opt->iconSize),Qt::ToolButtonTextOnly);
	if ( opt->text.isEmpty() && opt->icon.isNull() )
	  ms = sizeFromContents(f,fspec,ispec,lspec,sspec,"W",opt->icon.pixmap(opt->iconSize),Qt::ToolButtonTextOnly);

	s = sizeFromContents(f,fspec,ispec,lspec,sspec,opt->text,opt->icon.pixmap(opt->iconSize),opt->toolButtonStyle);

        if (opt->arrowType != Qt::NoArrow) {
          s = sizeFromContents(f,fspec,ispec,lspec,sspec,opt->text,opt->icon.pixmap(opt->iconSize),opt->toolButtonStyle)+QSize(dspec.size,0);
	  if ( (opt->toolButtonStyle != Qt::ToolButtonIconOnly) && !opt->text.isEmpty() )
	    s = s + QSize(lspec.tispace,0);
	  else if ( (opt->toolButtonStyle != Qt::ToolButtonTextOnly) && !opt->icon.isNull() )
	    s = s + QSize(lspec.tispace,0);

	  ms = ms+QSize(lspec.tispace+dspec.size,0);
	}

        if (widget) {
          const QToolButton * w = qobject_cast<const QToolButton *>(widget);
          if (w) {
            if (w->popupMode() == QToolButton::MenuButtonPopup)
              s.rwidth() += 20;
            else if (
              (w->popupMode() == QToolButton::InstantPopup) ||
              ((w->popupMode() == QToolButton::DelayedPopup) && (opt->features & QStyleOptionToolButton::HasMenu))
                 )
              s.rwidth() += lspec.tispace+dspec.size;
          }
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

        const frame_spec_t fspec = getFrameSpec(group);
        const interior_spec_t ispec = getInteriorSpec(group);
        const label_spec_t lspec = getLabelSpec(group);
        const size_spec_t sspec = getSizeSpec(group);

        s = sizeFromContents(f,fspec,ispec,lspec,sspec,opt->text,opt->icon.pixmap(pixelMetric(PM_ToolBarIconSize)));

        if (widget) {
          const QTabBar *w = qobject_cast<const QTabBar*>(widget);
          if (w && w->tabsClosable())
            s.rwidth() += pixelMetric(PM_TabCloseIndicatorWidth,option,widget)+lspec.tispace;
        }
      }

      break;
    }

    case CT_HeaderSection : {
      const QStyleOptionHeader *opt =
        qstyleoption_cast<const QStyleOptionHeader *>(option);

      if (opt) {
        QFont f = QApplication::font();
        if (widget)
          f = widget->font();

        const QString group = "HeaderSection";

        const frame_spec_t fspec = getFrameSpec(group);
        const interior_spec_t ispec = getInteriorSpec(group);
        const label_spec_t lspec = getLabelSpec(group);
        const size_spec_t sspec = getSizeSpec(group);

        s = sizeFromContents(f,fspec,ispec,lspec,sspec,opt->text,opt->icon.pixmap(pixelMetric(PM_SmallIconSize)));
      }

      break;
    }

    case CT_Slider : {
      if (option->state & State_Horizontal)
        s = QSize(dw,pixelMetric(PM_SliderControlThickness,option,widget)+2); // +2 for frame
      else
        s = QSize(pixelMetric(PM_SliderControlThickness,option,widget),dh+2);

      break;
    }

    case CT_GroupBox : {
      const QStyleOptionGroupBox *opt =
        qstyleoption_cast<const QStyleOptionGroupBox *>(option);

      if (opt) {

        QFont f = QApplication::font();
        if (widget)
          f = widget->font();

        const QString group = "GroupBox";

        const frame_spec_t fspec = getFrameSpec(group);
        const interior_spec_t ispec = getInteriorSpec(group);
        label_spec_t lspec = getLabelSpec(group);
	const indicator_spec_t dspec = getIndicatorSpec(group);
        const size_spec_t sspec = getSizeSpec(group);

        __print_group();

	// title size
	const QGroupBox * w = qobject_cast<const QGroupBox *>(widget);
	QSize st;

	if ( w && w->isCheckable() )
          st = sizeFromContents(f,fspec,ispec,lspec,sspec,opt->text,QPixmap())+QSize(pixelMetric(PM_CheckBoxLabelSpacing)+pixelMetric(PM_IndicatorWidth),0);
        else
	  st = sizeFromContents(f,fspec,ispec,lspec,sspec,opt->text,QPixmap());

        // add contents to st, 30 is title shift (left and right)
	s = QSize(qMax(st.width()+30+30,contentsSize.width()+fspec.left+fspec.right),contentsSize.height()+st.height()+fspec.top+fspec.bottom);
      }

      break;
    }

    default : s = QCommonStyle::sizeFromContents(type,option,contentsSize,widget);
  }

#ifdef __DEBUG__
//   if (widget){
//     QWidget *w = (QWidget *)widget;
//     w->setToolTip(QString("%1\n<b>sizeFromContents()</b>:%2,%3\n").arg(w->toolTip()).arg(s.width()).arg(s.height()));
//   }
#endif

  emit(sig_sizeFromContents_end(CT_str(type)));
  return s;
}

QSize QSvgStyle::sizeFromContents(const QFont &font,
                     /* frame spec */ const frame_spec_t &fspec,
                     /* interior spec */ const interior_spec_t &ispec,
                     /* label spec */ const label_spec_t &lspec,
                     /* size spec */ const size_spec_t &sspec,
                     /* text */ const QString &text,
                     /* icon */ const QPixmap &icon,
                     /* text-icon alignment */ const Qt::ToolButtonStyle tialign) const
{
  Q_UNUSED(ispec);

  QSize s;
  s.setWidth(fspec.left+fspec.right+lspec.left+lspec.right);
  s.setHeight(fspec.top+fspec.bottom+lspec.top+lspec.bottom);
  if (lspec.hasShadow) {
    s.rwidth() += lspec.xshift+lspec.depth;
    s.rheight() += lspec.yshift+lspec.depth;
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

    th = QFontMetrics(font).height()*(l.size());
    for (int i=0; i<l.size(); i++) {
      tw = qMax(tw,QFontMetrics(font).width(l[i]));
    }
  }

  if (tialign == Qt::ToolButtonIconOnly) {
    s.rwidth() += icon.width();
    s.rheight() += icon.height();
  } else if (tialign == Qt::ToolButtonTextOnly) {
    s.rwidth() += tw;
    s.rheight() += th;
  } else if (tialign == Qt::ToolButtonTextBesideIcon) {
    s.rwidth() += (icon.isNull() ? 0 : icon.width()) + (icon.isNull() ? 0 : (text.isEmpty() ? 0 : lspec.tispace)) + tw;
    s.rheight() += qMax(icon.height(),th);
  } else if (tialign == Qt::ToolButtonTextUnderIcon) {
    s.rwidth() += qMax(icon.width(),tw);
    s.rheight() += icon.height() + (icon.isNull() ? 0 : lspec.tispace) + th;
  }

  if ( (sspec.minH > 0) && (s.height() < sspec.minH) )
    s.setHeight(sspec.minH);

  if ( (sspec.minW > 0) && (s.width() < sspec.minW) )
    s.setWidth(sspec.minW);

  if (sspec.fixedH > 0)
    s.setHeight(sspec.fixedH);

  if (sspec.fixedW > 0)
    s.setWidth(sspec.fixedW);

  return s;
}

QRect QSvgStyle::subElementRect(SubElement element, const QStyleOption * option, const QWidget * widget) const
{
  switch (element) {
    case SE_CheckBoxFocusRect :
    case SE_RadioButtonFocusRect :
    case SE_ProgressBarGroove :
    case SE_HeaderLabel :
    case SE_ProgressBarLabel : return option->rect;
    case SE_ProgressBarContents : {
      const QString group = "Progressbar";

      const frame_spec_t fspec = getFrameSpec(group);
      const interior_spec_t ispec = getInteriorSpec(group);

      return interiorRect(option->rect, fspec,ispec);
    }
    case SE_LineEditContents : {
      const QString group = "LineEdit";

      const frame_spec_t fspec = getFrameSpec(group);
      const interior_spec_t ispec = getInteriorSpec(group);

      return interiorRect(option->rect, fspec,ispec);
    }
    case SE_PushButtonContents : {
      const QString group = "PanelButtonCommand";

      const frame_spec_t fspec = getFrameSpec(group);
      const interior_spec_t ispec = getInteriorSpec(group);

      return interiorRect(option->rect,fspec,ispec);
    }
    case SE_ItemViewItemFocusRect : return QRect();

    default : return QCommonStyle::subElementRect(element,option,widget);
  }
}

QRect QSvgStyle::subControlRect(ComplexControl control, const QStyleOptionComplex * option, SubControl subControl, const QWidget * widget) const
{
  int x,y,h,w;
  option->rect.getRect(&x,&y,&w,&h);

  switch (control) {
    case CC_TitleBar :
      switch (subControl) {
        case SC_TitleBarLabel : return option->rect;

        default : return QCommonStyle::subControlRect(control,option,subControl,widget);
      }
      break;

    case CC_SpinBox :
      switch (subControl) {
        case SC_SpinBoxFrame : return QRect();
        case SC_SpinBoxEditField : return QRect(x,y,w-40,h);
        case SC_SpinBoxUp : return QRect(x+w-20,y,20,h);
        case SC_SpinBoxDown : return QRect(x+w-40,y,20,h);

        default : return QCommonStyle::subControlRect(control,option,subControl,widget);
      }
      break;

    case CC_ComboBox :
      switch (subControl) {
        case SC_ComboBoxFrame : return QRect();
        case SC_ComboBoxEditField : return QRect(x,y,w-20,h);
        case SC_ComboBoxArrow : return QRect(x+w-20,y,20,h);
        case SC_ComboBoxListBoxPopup : {
          const QString group = "ComboBox";

          const frame_spec_t fspec = getFrameSpec(group);
          const interior_spec_t ispec = getInteriorSpec(group);
          const label_spec_t lspec = getLabelSpec(group);
          const size_spec_t sspec = getSizeSpec(group);

          int maxW = 0;

          if (widget) {
            const QComboBox * w = qobject_cast<const QComboBox *>(widget);
            QSize s;
            for (int i=0; i<w->count(); i++) {
              s = sizeFromContents(w->font(),fspec,ispec,lspec,sspec, w->itemText(i),w->itemIcon(i).pixmap(w->iconSize()));
              if (s.width() > maxW)
                maxW = s.width();
            }

            if (w->count() > w->maxVisibleItems()) {
              /* add scrollbar width */
              maxW = maxW+pixelMetric(PM_ScrollBarExtent);
            }
          }

          if (maxW < w)
            maxW = w; /* popup width not shorter than combo itself */

          // FIXME limit the width of the combo box
          return QRect(0,h,maxW,0 /* height seems not to be used */);
        }

        default : return QCommonStyle::subControlRect(control,option,subControl,widget);
      }
      break;

//     case CC_MdiControls :
//       switch (subControl) {
//         case SC_MdiCloseButton : return QRect(0,0,30,30);
//
//         default : return QCommonStyle::subControlRect(control,option,subControl,widget);
//       }
//       break;

    case CC_ScrollBar : {
      const int extent = pixelMetric(PM_ScrollBarExtent,option,widget);
      if (option->state & State_Horizontal)
        switch (subControl) {
          case SC_ScrollBarGroove : return QRect(x+extent,y,w-2*extent,h);
          case SC_ScrollBarSubLine : return QRect(x,y,extent,extent);
          case SC_ScrollBarAddLine : return QRect(x+w-extent,y,extent,extent);
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
              return QRect(x+start,y,length,h);
            } else
              return QRect();
          }

          default : return QCommonStyle::subControlRect(control,option,subControl,widget);

          break;
        }
      else
        switch (subControl) {
          case SC_ScrollBarGroove : return QRect(x,y+extent,w,h-2*extent);
          case SC_ScrollBarSubLine : return QRect(x,y,extent,extent);
          case SC_ScrollBarAddLine : return QRect(x,y+h-extent,extent,extent);
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
              return QRect(x,y+start,w,length);
            } else
              return QRect();
          }

          default : return QCommonStyle::subControlRect(control,option,subControl,widget);

          break;
        }

      break;
    }

    case CC_Slider : {
      const int thick = pixelMetric(PM_SliderThickness,option,widget);
      const bool horiz = (option->state & State_Horizontal);
      switch (subControl) {
        case SC_SliderGroove :
          if (horiz)
            return QRect(x,y+(h-thick)/2,w,thick);
          else
            return QRect(x+(w-thick)/2,y,thick,h);

        case SC_SliderHandle : {
          const QStyleOptionSlider *opt =
            qstyleoption_cast<const QStyleOptionSlider *>(option);

          if (opt) {
            subControlRect(CC_Slider,option,SC_SliderGroove,widget).getRect(&x,&y,&w,&h);

            const int len = pixelMetric(PM_SliderLength, option, widget);
            const int thickness = pixelMetric(PM_SliderControlThickness, option, widget);
            const int sliderPos(sliderPositionFromValue(opt->minimum, opt->maximum, opt->sliderPosition, (horiz ? w : h) - len, opt->upsideDown));

            if (horiz)
              return QRect(x+sliderPos,y+(h-thickness)/2,len,thickness);
            else
              return QRect(x+(w-len)/2,y+sliderPos,thickness,len);
          }

        }

        default : return QCommonStyle::subControlRect(control,option,subControl,widget);
      }

      break;
    }

    case CC_Dial : {
      const bool horiz = (option->state & State_Horizontal);
      switch(subControl) {
        case SC_DialGroove : return alignedRect(QApplication::layoutDirection(), Qt::AlignHCenter | Qt::AlignVCenter, QSize(qMin(option->rect.width(),option->rect.height()),qMin(option->rect.width(),option->rect.height())), option->rect);
        case SC_DialHandle : {
          const QStyleOptionSlider *opt =
            qstyleoption_cast<const QStyleOptionSlider *>(option);

          if (opt) {
            subControlRect(CC_Dial,option,SC_DialGroove,widget).getRect(&x,&y,&w,&h);

            const int sliderPos(sliderPositionFromValue(opt->minimum, opt->maximum, opt->sliderPosition, (horiz ? w : h), opt->upsideDown));

            if (horiz)
              return QRect(x,y,w,h-sliderPos);
            else
              return QRect(x,y,w-sliderPos,h);
          }

        }

        default : return QCommonStyle::subControlRect(control,option,subControl,widget);
      }

      break;
    }

    case CC_ToolButton : {
      switch (subControl) {
        case SC_ToolButton : {
          const QStyleOptionToolButton *opt =
            qstyleoption_cast<const QStyleOptionToolButton *>(option);

          const QString group = "PanelButtonTool";

          const indicator_spec_t dspec = getIndicatorSpec(group);
          const label_spec_t lspec = getLabelSpec(group);

          if (opt) {
            if (widget) {
              const QToolButton * w = qobject_cast<const QToolButton *>(widget);
              if (w) {
                if (w->popupMode() == QToolButton::MenuButtonPopup)
                  return option->rect.adjusted(0,0,-20,0);
                else if (
                  (w->popupMode() == QToolButton::InstantPopup) ||
                  ((w->popupMode() == QToolButton::DelayedPopup) && (opt->features & QStyleOptionToolButton::HasMenu))
                )
                  return option->rect.adjusted(0,0,-dspec.size-lspec.tispace,0);
              }
            }
          }

          return option->rect;
        }

        case SC_ToolButtonMenu : {
          const QStyleOptionToolButton *opt =
            qstyleoption_cast<const QStyleOptionToolButton *>(option);

          const QString group = "PanelButtonTool";

          const frame_spec_t fspec = getFrameSpec(group);
          const interior_spec_t ispec = getInteriorSpec(group);
          const indicator_spec_t dspec = getIndicatorSpec(group);
          const label_spec_t lspec = getLabelSpec(group);

          if (opt) {
            if (widget) {
              const QToolButton * ww = qobject_cast<const QToolButton *>(widget);
              if (ww) {
                if (ww->popupMode() == QToolButton::MenuButtonPopup)
                  return QRect(x+w-20,y,20,h);
                else if (
                  (ww->popupMode() == QToolButton::InstantPopup) ||
                  ((ww->popupMode() == QToolButton::DelayedPopup) && (opt->features & QStyleOptionToolButton::HasMenu))
                )
                  return QRect(x+w-lspec.tispace-dspec.size-fspec.right,y+10,dspec.size,h-10);
              }
            }
          }

          return option->rect;
        }

        default : return QCommonStyle::subControlRect(control,option,subControl,widget);
      }

      break;
    }

    case CC_GroupBox : {
      const QString group = "GroupBox";

      frame_spec_t fspec = getFrameSpec(group);
      interior_spec_t ispec = getInteriorSpec(group);
      label_spec_t lspec = getLabelSpec(group);
      size_spec_t sspec = getSizeSpec(group);

      const QStyleOptionGroupBox *opt =
        qstyleoption_cast<const QStyleOptionGroupBox *>(option);

      // title size
      QSize stitle;
      const QGroupBox  * w = qobject_cast<const QGroupBox *>(widget);
      if (opt) {
	if (w) {
	  stitle = sizeFromContents(w->font(),fspec,ispec,lspec,sspec,opt->text,QPixmap());
	  if ( w && w->isCheckable() ) {
	    // ensure checkbox indicator fits within title interior
	    stitle = stitle+QSize(pixelMetric(PM_IndicatorWidth)+pixelMetric(PM_CheckBoxLabelSpacing),0);
	    stitle = stitle.expandedTo(QSize(0,fspec.top+fspec.bottom+pixelMetric(PM_IndicatorHeight)));
	  }
	}
      }

      QRect labelRect = alignedRect(QApplication::layoutDirection(), opt->textAlignment & ~Qt::AlignVertical_Mask,
                                    stitle,
                                    option->rect.adjusted(30,0,-30,0));

      switch (subControl) {
	// FIXME take into account label V alignment
        case SC_GroupBoxCheckBox : {
          // align checkbopx inside label rect
          return alignedRect(QApplication::layoutDirection(),Qt::AlignLeft | Qt::AlignVCenter,
			     QSize(pixelMetric(PM_IndicatorWidth),pixelMetric(PM_IndicatorHeight)),
                             labelRect.adjusted(fspec.left+lspec.left,fspec.top,-fspec.right-lspec.right,-fspec.bottom));
        }
        case SC_GroupBoxLabel : {
            // Shift for checkbox will be done be drawComplexControl()
            return labelRect;
            //return QRect(option->rect.x()+30,option->rect.y(),stitle.width(),stitle.height());
        }
	case SC_GroupBoxFrame : {
	  return QRect(option->rect.x(),option->rect.y()+stitle.height(),option->rect.width(),option->rect.height()-stitle.height());
	}
	case SC_GroupBoxContents : {
          qDebug() << "SC_GroupBoxContents" << x << y << option->rect.width() << h;
          qDebug() << "SC_GroupBoxContents" << stitle.height();
          // here option->rect is not the whole groupbox rect
	  return interiorRect(option->rect.adjusted(0,stitle.height(),0,-fspec.bottom /* ???? */),fspec,ispec);
	}

        default : return QCommonStyle::subControlRect(control,option,subControl,widget);
      }
    }

    default : return QCommonStyle::subControlRect(control,option,subControl,widget);
  }

  return QCommonStyle::subControlRect(control,option,subControl,widget);
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

void QSvgStyle::renderLabel(QPainter *painter,
                     /* frame bounds */ const QRect &bounds,
                     /* frame spec */ const frame_spec_t &fspec,
                     /* interior spec */ const interior_spec_t &ispec,
                     /* label spec */ const label_spec_t &lspec,
                     /* text alignment */ int talign,
                     /* text */ const QString &text,
                     /* text disabled ? */ bool disabled,
                     /* icon */ const QPixmap &icon,
                     /* text-icon alignment */ const Qt::ToolButtonStyle tialign) const
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
    ricon = alignedRect(QApplication::layoutDirection(),Qt::AlignVCenter | Qt::AlignLeft, QSize(icon.width(),icon.height()),r);
    rtext = QRect(r.x()+icon.width()+(icon.isNull() ? 0 : lspec.tispace),r.y(),r.width()-ricon.width()-(icon.isNull() ? 0 : lspec.tispace),r.height());
  } else if (tialign == Qt::ToolButtonTextUnderIcon) {
    ricon = alignedRect(QApplication::layoutDirection(),Qt::AlignTop | Qt::AlignHCenter, QSize(icon.width(),icon.height()),r);
    rtext = QRect(r.x(),r.y()+icon.height()+(icon.isNull() ? 0 : lspec.tispace),r.width(),r.height()-ricon.height()-(icon.isNull() ? 0 : lspec.tispace));
  } else if (tialign == Qt::ToolButtonIconOnly) {
    ricon = alignedRect(QApplication::layoutDirection(),Qt::AlignCenter, QSize(icon.width(),icon.height()),r);
  }

  if ( text.isNull() || text.isEmpty() ) {
    ricon = alignedRect(QApplication::layoutDirection(),Qt::AlignCenter, QSize(icon.width(),icon.height()),r);
  }

  if (disabled) {
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
/* Auto generated */
const QString QSvgStyle::PE_str(PrimitiveElement element) const
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

const QString QSvgStyle::CE_str(QStyle::ControlElement element) const
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

const QString QSvgStyle::SE_str(QStyle::SubElement element) const
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

const QString QSvgStyle::CC_str(QStyle::ComplexControl element) const
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

const QString QSvgStyle::SC_str(QStyle::ComplexControl control, QStyle::SubControl subControl) const
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

const QString QSvgStyle::CT_str(QStyle::ContentsType type) const
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
