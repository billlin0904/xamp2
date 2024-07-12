//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QDialog>
#include <QToolButton>
#include <FramelessHelper/Widgets/framelessdialog.h>
#include <FramelessHelper/Widgets/standardtitlebar.h>

#include <thememanager.h>

FRAMELESSHELPER_USE_NAMESPACE
using namespace wangwenx190::FramelessHelper::Global;

#ifdef Q_OS_WIN
using XDialogBase = FramelessDialog;
#else
using XDialogBase = QDialog;
#endif

class XAMP_WIDGET_SHARED_EXPORT XDialog : public XDialogBase {
    Q_OBJECT
public:
    static constexpr auto kMaxTitleHeight = 30;
    static constexpr auto kMaxTitleIcon = 20;
    static constexpr auto kTitleFontSize = 10;

    explicit XDialog(QWidget* parent = nullptr, bool modal = true);

    void setContentWidget(QWidget* content, bool no_moveable = false, bool disable_resize = true);

    QWidget* contentWidget() const {
        return content_;
    }

    void setTitle(const QString& title);

    void setIcon(const QIcon& icon);

public slots:
    void onThemeChangedFinished(ThemeColor theme_color);

private:
    void closeEvent(QCloseEvent* event) override;

    void setContent(QWidget* content);

#ifdef Q_OS_WIN
    QLabel* title_frame_label_{ nullptr };    
    QToolButton* icon_{ nullptr };
    QToolButton* close_button_{ nullptr };
    QToolButton* max_win_button_{ nullptr };
    QToolButton* min_win_button_{ nullptr };
    QFrame* title_frame_{ nullptr };
#else
    StandardTitleBar* title_bar_;
#endif
    QWidget* content_{ nullptr };
};

