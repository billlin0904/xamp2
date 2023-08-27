//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QListWidget>
#include <QSet>

#include <widget/themecolor.h>

class QLabel;

class TagWidgetItem : public QListWidgetItem {
public:
	TagWidgetItem(const QString& tag, QColor color, QLabel* label, QListWidget* parent = nullptr);

	~TagWidgetItem() override;

	QString GetTag() const;

	bool IsEnable() const;

	void SetEnable(bool enable);

	void Enable();
private:
	bool enabled_ = true;
	QColor color_;
	QString tag_;
	QLabel* label_;
};

class TagListView : public QFrame {
	Q_OBJECT
public:
	explicit TagListView(QWidget* parent = nullptr);

	void AddTag(const QString &tag, bool uniform_item_sizes = false);

	void ClearTag();

	void SetListViewFixedHeight(int32_t height);

	void EnableTag(const QString& tag);

	void OnCurrentThemeChanged(ThemeColor theme_color);

	void OnThemeColorChanged(QColor backgroundColor, QColor color);
signals:
	void TagChanged(const QSet<QString> &tags);

	void TagClear();

private:
	QListWidget* taglist_;
	QSet<QString> tags_;
};

