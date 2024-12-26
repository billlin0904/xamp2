//=====================================================================================================================
// Copyright (c) 2018-2025 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QDialog>
#include <QToolButton>

#include <thememanager.h>

namespace QWK {
	class WidgetWindowAgent;
}

class XAMP_WIDGET_SHARED_EXPORT XDialog : public QDialog {
    Q_OBJECT
public:
    static constexpr auto kMaxTitleHeight = 30;
    static constexpr auto kMaxTitleIcon = 20;
    static constexpr auto kTitleFontSize = 10;

    explicit XDialog(QWidget* parent = nullptr, bool modal = true);

	~XDialog() override;

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

    void installWindowAgent();

    QWK::WidgetWindowAgent* window_agent_{ nullptr };
    QWidget* content_{ nullptr };
};

