//=====================================================================================================================
// Copyright (c) 2018-2025 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QLineEdit>
#include <QTabBar>
#include <widget/widget_shared_global.h>

enum WidthModes {
	FIXED_WIDTH_MODE,
	DYNAMIC_WIDTH_MODE
};

class XAMP_WIDGET_SHARED_EXPORT PlaylistTabBar final : public QTabBar {
	Q_OBJECT
public:
	static constexpr size_t kButtonWidth = 60;

	explicit PlaylistTabBar(QWidget* parent = nullptr);

	void setWidthMode(WidthModes mode);
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

	WidthModes width_mode_{ FIXED_WIDTH_MODE };
	int32_t edited_index_{0};
	QLineEdit* line_edit_{nullptr};
};
