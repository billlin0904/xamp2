//=====================================================================================================================
// Copyright (c) 2018-2023 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QStackedWidget>
#include <QMap>
#include <QFrame>

#include <thememanager.h>

class QVBoxLayout;
class GenreView;
class ClickableLabel;

class GenrePage : public QFrame {
	Q_OBJECT
public:
	explicit GenrePage(QWidget* parent = nullptr);

	void setGenre(const QString& genre);

	GenreView* view() {
		return genre_view_;
	}
signals:
	void goBackPage();

private:
	ClickableLabel* genre_label_;
	GenreView* genre_view_;
};

class GenreViewPage : public QStackedWidget {
public:
	explicit GenreViewPage(QWidget* parent = nullptr);

	void addGenre(const QString& genre);

	void removeGenre(const QString& genre);

	void OnCurrentThemeChanged(ThemeColor theme_color);

	void OnThemeColorChanged(QColor backgroundColor, QColor color);

	void refresh();

	void clear();
private:
	GenrePage* genre_page_;
	QFrame* genre_frame_;
	QVBoxLayout* genre_frame_layout_;
	QMap<QString, QPair<GenrePage*, GenreView*>> genre_view_;
};
