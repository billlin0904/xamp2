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

	QString getTag() const;

	bool isEnable() const;

	void setEnable(bool enable);

	void enable();
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

	void addTag(const QString &tag, bool uniform_item_sizes = false);

	void clearTag();

	void setListViewFixedHeight(int32_t height);

	void enableTag(const QString& tag);

	void disableAllTag(const QString& skip_tag);	

	void sort();
	
public slots:
	void onCurrentThemeChanged(ThemeColor theme_color);

	void onThemeColorChanged(QColor background_color, QColor color);

signals:
	void tagChanged(const QSet<QString> &tags);

	void tagClear();

private:
	QListWidget* list_;
	QSet<QString> tags_;
};

