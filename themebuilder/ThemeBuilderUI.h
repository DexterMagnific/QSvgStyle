/***************************************************************************
 *   Copyright (C) 2014 by Saïd LANKRI   *
 *   said.lankri@gmail.com   *
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
#ifndef THEMEBUILDERUI_H
#define THEMEBUILDERUI_H

#include <QStringList>
#include <QShowEvent>
#include <QFileSystemWatcher>
#include <QElapsedTimer>
#include <QColor>

#include "ui_ThemeBuilderUIBase.h"
#include "GenSubFramePropUI.h"
#include <specs.h>
#include "SvgGen.h"

class ThemeConfig;
class QTreeWidget;
class QTreeWidgetItem;
class QMenu;
class QSvgThemableStyle;
class QTimer;
class QFileSystemWatcher;
class NewThemeUI;

class ThemeBuilderUI : public QMainWindow, private Ui::ThemeBuilderUIBase {
  Q_OBJECT

  public:
    ThemeBuilderUI(QWidget *parent = 0);
    virtual ~ThemeBuilderUI();

  private slots:
    // Called on main button clicks
    void slot_newTheme();
    void slot_openTheme();
    void slot_saveTheme();
    void slot_optimizeSvg();
    void slot_editSvg();
    void slot_quit();

    // Called when the SVG file has changed
    void slot_svgFileChanged(const QString &filename);

    void slot_reloadSvgFile();

    void slot_openRecentTheme();

    // Called when the current widget in the same QTreeView has changed
    void slot_widgetChanged(QListWidgetItem *current, QListWidgetItem *previous);
    // Called when the current toolbox tab has changed
    void slot_toolboxTabChanged(int index);

    // Called when the user changes the config from the UI. It saves the current
    // group settings and updates the preview
    void slot_uiSettingsChanged();

    // called on properties tab changes
    void slot_authorEditChanged(const QString &text);
    void slot_descrEditChanged(const QString &text);
    void slot_themeNameEditChanged(const QString &text);

    // called on common tab changes
    void slot_inheritCbChanged(int state);
    void slot_inheritComboChanged(int idx);

    void slot_frameCbChanged(int state);
    void slot_frameIdCbChanged(int state);
    void slot_frameIdComboChanged(const QString &text);
    void slot_frameWidthCbChanged(int state);
    void slot_frameWidthSpinChanged(int val);

    void slot_interiorCbChanged(int state);
    void slot_interiorIdCbChanged(int state);
    void slot_interiorIdComboChanged(const QString &text);
    void slot_interiorRepeatCbChanged(int state);
    void slot_interiorRepeatXSpinChanged(int val);
    void slot_interiorRepeatYSpinChanged(int val);

    void slot_indicatorIdCbChanged(int state);
    void slot_indicatorIdComboChanged(const QString &text);
    void slot_indicatorSizeCbChanged(int state);
    void slot_indicatorSizeSpinChanged(int);

    void slot_labelSpacingCbChanged(int state);
    void slot_labelSpacingSpinChanged(int val);
    void slot_labelMarginCbChanged(int state);
    void slot_labelMarginHSpinChanged(int val);
    void slot_labelMarginVSpinChanged(int val);

    // called on preview tab changes
    void slot_repaintBtnClicked(bool checked);
    void slot_rtlBtnClicked(bool checked);
    void slot_drawModeBtnClicked(bool checked);
    void slot_detachBtnClicked(bool checked);
    void slot_enableBtnClicked(bool checked);
    void slot_previewVariantBtnClicked(bool checked);
    void slot_fontSizeChanged(int val);
    void slot_paletteChanged(int val);

    // called on SVG Gen tab changes
    void slot_genFrameBtnClicked(bool checked);
    void slot_genInteriorBtnClicked(bool checked);
    void slot_genShadowBtnClicked(bool checked);
    void slot_genRoundBtnClicked(bool checked);
    void slot_genSplitBtnClicked(bool checked);
    void slot_genFrameWidthChanged(int val);
    void slot_genInteriorRoundnessChanged(qreal val);
    void slot_genSquareBtnClicked(bool checked);
    void slot_genSubFrameWidthChanged(qreal val);
    void slot_genCopyBtnClicked(bool checked);
    void slot_genBasenameChanged(const QString &text);
    void slot_genVariantChanged(const QString &text);
    void slot_genStatusChanged(const QString &text);
    void slot_genSubFrameFillTypeBtnClicked(bool checked);
    void slot_genSubFrameColor1BtnClicked(bool checked);
    void slot_genSubFrameColor2BtnClicked(bool checked);
    void slot_genInteriorFillTypeBtnClicked(bool checked);
    void slot_genInteriorColor1BtnClicked(bool checked);
    void slot_genInteriorColor2BtnClicked(bool checked);
    void slot_genShadowFillTypeBtnClicked(bool checked);
    void slot_genShadowColor1BtnClicked(bool checked);
    void slot_genShadowColor2BtnClicked(bool checked);
    void slot_genShadowWidthChanged(qreal val);

    // Callbacks from QSvgStyle that are triggered when it renders widgets
    void slot_drawPrimitive_begin(const QString &s);
    void slot_drawPrimitive_end(const QString &s);
    void slot_drawControl_begin(const QString &s);
    void slot_drawControl_end(const QString &s);
    void slot_drawComplexControl_begin(const QString &s);
    void slot_drawComplexControl_end(const QString &s);
    void slot_renderFrame_begin(const QString &s);
    void slot_renderFrame_end(const QString &s);
    void slot_renderInterior_begin(const QString &s);
    void slot_renderInterior_end(const QString &s);
    void slot_renderIndicator_begin(const QString &s);
    void slot_renderIndicator_end(const QString &s);
    void slot_renderLabel_begin(const QString &s);
    void slot_renderLabel_end(const QString &s);
    void slot_renderElement_begin(const QString &s);
    void slot_renderElement_end(const QString &s);
    void slot_sizeFromContents_begin(const QString &s);
    void slot_sizeFromContents_end(const QString &s);

    // TESTING
    // END TESTING

  protected:
    virtual bool eventFilter(QObject *o, QEvent *e);
    virtual void closeEvent(QCloseEvent *e);

  private:
    // enum that allow us to attach arbitrary data to QTreeViewItem items
    // using @ref QTreeViewItem::setData()
    enum {
      GroupRole = Qt::UserRole + 10, // the theme configuration group for the
                                     // selected widget
      SettingRole = Qt::UserRole + 11, // the specific setting name
    };

    // opens a theme
    bool openTheme(const QString &filename);

    // Saves list of recent files
    void saveRecentFiles();
    // Loads list of recent files
    void loadRecentFiles();

    // Resets the entire UI as if theme builder has just been started
    // To be called before opening any config file
    void resetUi();

    // If current theme is modified, shows a dialog box that asks to save it
    // returns false if the user chooses to cancel button, true if it presses
    // yes or no
    // It the user presses yes, the theme is saved
    bool ensureSettingsSaved();

    // When called, starts a timer that will trigger
    // settings of current group to be saved and then preview update
    // The flow is as follows:
    // schedulePreviewpdate() -> timer -> slot_uiSettingsChanged() -> saveSettingsFromUi()
    // -> repaint of widget
    void schedulePreviewUpdate(bool modified = true);

    // Block signals from various UI widgets while we are changing them
    void blockUISignals(bool blocked);

    // Setup the UI to reflect the settings for the given item
    void setupUiForWidget(const QListWidgetItem *current);
    // Setup the preview widget to reflect the given item
    void setupPreviewForWidget(const QListWidgetItem *current);
    // Save current UI settings for widget
    void saveSettingsFromUi(const QListWidgetItem *current);
    // wrapper around slot_XXXX_begin
    void noteStyleOperation_begin(const QString &op, const QString &args);
    void noteStyleOperation_end(const QString &op, const QString &args);
    // clear drawStackTree, delete all its items
    void clearDrawStackTree();
    // Sets the given style for the given widget and all its children
    void setStyleForWidgetAndChildren(QStyle* style, QWidget* w);
    // Setup the Quick SVG Tab according to the svgGen object
    void setupSvgGenUI();
    // Setup Quick SVG SubFrame UI according to svgGen object
    void setupSubFramePropsUI(GenSubFramePropUI *w);
    // Setup Quick SVG Interior UI according to svgGen object
    void setupInteriorPropsUI();
    // Setup Quick SVG Shadow UI according to svgGen object
    void setupShadowPropsUI();

    // Optimizes SVG
    void optimizeSvg(const QString& inPath, const QString& outPath);

    QTreeWidget *drawStackTree;

    // Menu for recent files
    QMenu *recentFiles;

    // widgets to be inserted inside the status bar
    QLabel *statusbarLbl1, *statusbarLbl2;

    // current opened theme config
    ThemeConfig *config;
    // current element spec, as read from the config file
    element_spec_t raw_es;
    // element spec of inherited
    element_spec_t inherit_es;

    // an instance of QSvgStyle
    QSvgThemableStyle *style;

    // current widget being previewed
    QWidget *previewWidget;
    // Time to paint preview widget
    QElapsedTimer paintTimer;

    // state save
    int currentToolboxTab;
    // current selected item in toolbox
    QListWidgetItem *currentWidget;
    // last inserted item inside drawStackTree
    QTreeWidgetItem *currentDrawStackItem;
    // preview mode
    int currentDrawMode; // 0=normal, 1=wireframe, 2=overdraw
    // current widget variant
    int currentPreviewVariant;
    // cfg file modified
    bool cfgModified;
    // update preview when UI changes ?
    bool previewUpdateEnabled;
    // current open theme filename
    QString cfgFile;
    // associated SVG file
    QString svgFile;
    // geometry of detached preview
    QRect detachedPeviewGeometry;
    // Timer for repainting the widget when settings change
    QTimer *timer, *timer2;
    // New theme dialog
    NewThemeUI *newThemeDlg;

    // Temporary file, a copy of the current opened theme
    QString tempCfgFile;
    // to watch SVG file for file changes
    QFileSystemWatcher svgWatcher;
    // SVG quick generator
    SvgGen *svgGen;
    QVector<GenSubFramePropUI *> genSubFrameProps;
};

#endif // THEMEBUILDERUI_H
