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

#include <QStyle>

#include "groups.h"

QString PE_group(QStyle::PrimitiveElement element)
{
  switch (element) {
    // Frames
    case QStyle::PE_Frame : return "Frame";
    case QStyle::PE_FrameDefaultButton : return "PushButton";
    case QStyle::PE_FrameDockWidget : return "DockWidget";
    case QStyle::PE_FrameFocusRect : return "Frame";
    case QStyle::PE_FrameGroupBox : return "GroupBox";
    case QStyle::PE_FrameLineEdit : return "LineEdit";
    case QStyle::PE_FrameMenu : return "MenuItem";
    case QStyle::PE_FrameStatusBarItem : return "StatusBar";
    case QStyle::PE_FrameTabWidget : return "Frame";
    case QStyle::PE_FrameWindow : return "Window";
    case QStyle::PE_FrameButtonBevel : return "PushButton";
    case QStyle::PE_FrameButtonTool : return "ToolButton";
    case QStyle::PE_FrameTabBarBase : return "TabWidget";
    // Panels (interiors)
    case QStyle::PE_PanelButtonCommand : return "PushButton";
    case QStyle::PE_PanelButtonBevel : return "PushButton";
    case QStyle::PE_PanelButtonTool : return "ToolButton";
    case QStyle::PE_PanelMenuBar : return "MenuBarItem";
    case QStyle::PE_PanelToolBar : return "ToolBar";
    case QStyle::PE_PanelLineEdit : return "LineEdit";
    case QStyle::PE_PanelTipLabel : return "Tooltip";
    case QStyle::PE_PanelScrollAreaCorner : return "PE_PanelScrollAreaCorner";
    case QStyle::PE_PanelItemViewItem : return "ViewItem";
    case QStyle::PE_PanelItemViewRow : return "ViewItem";
    case QStyle::PE_PanelStatusBar : return "StatusBar";
    case QStyle::PE_PanelMenu : return "Menu";
    // Indicators
    case QStyle::PE_IndicatorArrowDown : return "Indicator";
    case QStyle::PE_IndicatorArrowLeft : return "Indicator";
    case QStyle::PE_IndicatorArrowRight : return "Indicator";
    case QStyle::PE_IndicatorArrowUp : return "Indicator";
    case QStyle::PE_IndicatorBranch : return "Indicator";
    case QStyle::PE_IndicatorButtonDropDown : return "Indicator";
    case QStyle::PE_IndicatorItemViewItemCheck : return "CheckBox";
    case QStyle::PE_IndicatorCheckBox : return "CheckBox";
    case QStyle::PE_IndicatorDockWidgetResizeHandle : return "DockWidget";
    case QStyle::PE_IndicatorHeaderArrow : return "Indicator";
    case QStyle::PE_IndicatorMenuCheckMark : return "CheckBox";
    case QStyle::PE_IndicatorProgressChunk : return "ProgressbarContents";
    case QStyle::PE_IndicatorRadioButton : return "RadioButton";
    case QStyle::PE_IndicatorSpinDown : return "Indicator";
    case QStyle::PE_IndicatorSpinMinus : return "Indicator";
    case QStyle::PE_IndicatorSpinPlus : return "Indicator";
    case QStyle::PE_IndicatorSpinUp : return "Indicator";
    case QStyle::PE_IndicatorToolBarHandle : return "ToolBar";
    case QStyle::PE_IndicatorToolBarSeparator : return "ToolBar";
    case QStyle::PE_IndicatorTabTear : return "TabWidget";
    case QStyle::PE_IndicatorColumnViewArrow : return "Indicator";
    case QStyle::PE_IndicatorItemViewItemDrop : return "Indicator";
    case QStyle::PE_IndicatorTabClose : return "TabWidget";

    default : return QString();
  }

  return QString();
}

QString CE_group(QStyle::ControlElement element)
{
  switch(element) {
    case QStyle::CE_PushButton : return "PushButton";
    case QStyle::CE_PushButtonBevel : return "PushButton";
    case QStyle::CE_PushButtonLabel : return "PushButton";
    case QStyle::CE_CheckBox : return "CheckBox";
    case QStyle::CE_CheckBoxLabel : return "CheckBox";
    case QStyle::CE_RadioButton : return "RadioButton";
    case QStyle::CE_RadioButtonLabel : return "RadioButton";
    case QStyle::CE_TabBarTab : return "TabBar";
    case QStyle::CE_TabBarTabShape : return "TabBar";
    case QStyle::CE_TabBarTabLabel : return "TabBar";
    case QStyle::CE_ProgressBar : return "ProgressBar";
    case QStyle::CE_ProgressBarGroove : return "ProgressBar";
    case QStyle::CE_ProgressBarContents : return "ProgressBar";
    case QStyle::CE_ProgressBarLabel : return "ProgressBar";
    case QStyle::CE_MenuItem : return "MenuItem";
    case QStyle::CE_MenuScroller : return "MenuItem";
    case QStyle::CE_MenuTearoff : return "MenuItem";
    case QStyle::CE_MenuEmptyArea : return "MenuItem";
    case QStyle::CE_MenuBarItem : return "MenuBarItem";
    case QStyle::CE_MenuBarEmptyArea : return "MenuBarItem";
    case QStyle::CE_ToolButtonLabel : return "ToolButton";
    case QStyle::CE_Header : return "Header";
    case QStyle::CE_HeaderSection : return "Header";
    case QStyle::CE_HeaderLabel : return "Header";
    case QStyle::CE_ToolBoxTab : return "ToolBox";
    case QStyle::CE_SizeGrip : return "Indicator";
    case QStyle::CE_Splitter : return "Splitter";
    case QStyle::CE_RubberBand : return "RubberBand";
    case QStyle::CE_DockWidgetTitle : return "DockWidget";
    case QStyle::CE_ScrollBarAddLine : return "ScrollBar";
    case QStyle::CE_ScrollBarSubLine : return "ScrollBar";
    case QStyle::CE_ScrollBarAddPage : return "ScrollBar";
    case QStyle::CE_ScrollBarSubPage : return "ScrollBar";
    case QStyle::CE_ScrollBarSlider : return "ScrollBar";
    case QStyle::CE_ScrollBarFirst : return "ScrollBar";
    case QStyle::CE_ScrollBarLast : return "ScrollBar";
    case QStyle::CE_FocusFrame : return "Frame";
    case QStyle::CE_ComboBoxLabel : return "ComboBox";
    case QStyle::CE_ToolBar : return "ToolBar";
    case QStyle::CE_ToolBoxTabShape : return "ToolBox";
    case QStyle::CE_ToolBoxTabLabel : return "ToolBox";
    case QStyle::CE_HeaderEmptyArea : return "Header";
    case QStyle::CE_ItemViewItem : return "ItemView";
    case QStyle::CE_ShapedFrame : return "Frame";
    default : return QString();
  }

  return QString();
}

QString CT_group(QStyle::ContentsType type)
{
  switch (type) {
    case QStyle::CT_PushButton : return "PushButton";
    case QStyle::CT_CheckBox : return "CheckBox";
    case QStyle::CT_RadioButton : return "RadioButton";
    case QStyle::CT_ToolButton : return "ToolButton";
    case QStyle::CT_ComboBox : return "PushButton";
    case QStyle::CT_Splitter : return "Splitter";
    case QStyle::CT_ProgressBar : return "ProgressBar";
    case QStyle::CT_MenuItem : return "MenuItem";
    case QStyle::CT_MenuBarItem : return "MenuBarItem";
    case QStyle::CT_MenuBar : return "MenuBarItem";
    case QStyle::CT_TabBarTab : return "TabBar";
    case QStyle::CT_Slider : return "Slider";
    case QStyle::CT_ScrollBar : return "ScrollBar";
    case QStyle::CT_LineEdit : return "LineEdit";
    case QStyle::CT_SpinBox : return "SpinBox";
    case QStyle::CT_TabWidget : return "TabWidget";
    case QStyle::CT_HeaderSection : return "Header";
    case QStyle::CT_GroupBox : return "GroupBox";
    default: return QString();
  }

  return QString();
}

QString SE_group(QStyle::SubElement element)
{
  switch(element) {
    case QStyle::SE_PushButtonContents : return "PushButton";
    case QStyle::SE_PushButtonFocusRect : return "Frame";
    case QStyle::SE_CheckBoxIndicator : return "CheckBox";
    case QStyle::SE_CheckBoxContents : return "CheckBox";
    case QStyle::SE_CheckBoxFocusRect : return "Frame";
    case QStyle::SE_CheckBoxClickRect : return "CheckBox";
    case QStyle::SE_RadioButtonIndicator : return "RadioButton";
    case QStyle::SE_RadioButtonContents : return "RadioButton";
    case QStyle::SE_RadioButtonFocusRect : return "Frame";
    case QStyle::SE_RadioButtonClickRect : return "RadioButton";
    case QStyle::SE_ComboBoxFocusRect : return "Frame";
    case QStyle::SE_SliderFocusRect : return "Frame";
    case QStyle::SE_ProgressBarGroove : return "ProgressBar";
    case QStyle::SE_ProgressBarContents : return "ProgressBar";
    case QStyle::SE_ProgressBarLabel : return "ProgressBar";
    case QStyle::SE_ToolBoxTabContents : return "ToolBox";
    case QStyle::SE_HeaderLabel : return "Header";
    case QStyle::SE_HeaderArrow : return "Header";
    case QStyle::SE_TabWidgetTabBar : return "TabBar";
    case QStyle::SE_TabWidgetTabPane : return "TabWidget";
    case QStyle::SE_TabWidgetTabContents : return "TabWidget";
    case QStyle::SE_TabWidgetLeftCorner : return "TabWidget";
    case QStyle::SE_TabWidgetRightCorner : return "TabWidget";
    case QStyle::SE_ItemViewItemCheckIndicator : return "ItemView";
    case QStyle::SE_TabBarTearIndicator : return "TabBar";
    case QStyle::SE_LineEditContents : return "LineEdit";
    case QStyle::SE_FrameContents : return "Frame";
    case QStyle::SE_DockWidgetCloseButton : return "DockWidget";
    case QStyle::SE_DockWidgetFloatButton : return "DockWidget";
    case QStyle::SE_DockWidgetTitleBarText : return "DockWidget";
    case QStyle::SE_DockWidgetIcon : return "DockWidget";
    case QStyle::SE_ItemViewItemDecoration : return "ItemView";
    case QStyle::SE_ItemViewItemText : return "ItemView";
    case QStyle::SE_ItemViewItemFocusRect : return "Frame";
    case QStyle::SE_TabBarTabLeftButton : return "TabBar";
    case QStyle::SE_TabBarTabRightButton : return "TabBar";
    case QStyle::SE_TabBarTabText : return "TabBar";
    case QStyle::SE_ShapedFrameContents : return "Frame";
    case QStyle::SE_ToolBarHandle : return "ToolBar";
    default : return QString();
  }

  return QString();
}

QString CC_group(QStyle::ComplexControl element)
{
  switch (element) {
    case QStyle::CC_SpinBox : return "SpinBox";
    case QStyle::CC_ComboBox : return "ComboBox";
    case QStyle::CC_ScrollBar : return "ScrollBar";
    case QStyle::CC_Slider : return "Slider";
    case QStyle::CC_ToolButton : return "ToolButton";
    case QStyle::CC_TitleBar : return "TitleBar";
    case QStyle::CC_Dial : return "Dial";
    case QStyle::CC_GroupBox : return "GroupBox";
    default : return QString();
  }

  return QString();
}
