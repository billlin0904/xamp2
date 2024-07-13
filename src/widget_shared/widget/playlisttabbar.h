//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QLineEdit>
#include <QTabBar>
#include <widget/widget_shared_global.h>

class XAMP_WIDGET_SHARED_EXPORT PlaylistTabBar final : public QTabBar {
	Q_OBJECT
public:
	static constexpr size_t kSmallTabCount = 3;
	static constexpr size_t kSmallTabWidth = 230;
	static constexpr size_t kMaxButtonWidth = 50;

	explicit PlaylistTabBar(QWidget* parent = nullptr);
	
	void setTabCount(int32_t count);
signals:
	void textChanged(int32_t index, const QString &text);

public slots:
	void onFinishRename();

private:
	void finishRename();

	void mouseDoubleClickEvent(QMouseEvent* event) override;

	void focusOutEvent(QFocusEvent* event) override;

	bool eventFilter(QObject* object, QEvent* event) override;

	QSize tabSizeHint(int index) const override;

	void resizeEvent(QResizeEvent* event) override;

	int32_t edited_index_{0};
	int32_t tab_count_{ 0 };
	QLineEdit* line_edit_{nullptr};
};
